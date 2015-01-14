/* rtsp.c

   Copyright (C) 2014 Marc Postema

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   Or, point your browser to http://www.gnu.org/copyleft/gpl.html

*/
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <time.h>
#include <poll.h>

#include <linux/dvb/frontend.h>

#include "rtsp.h"
#include "tune.h"
#include "rtp.h"
#include "rtcp.h"
#include "utils.h"
#include "applog.h"

#define RTSP_503_ERROR  "RTSP/1.0 503 Service Unavailable\r\n" \
                        "CSeq: %d\r\n" \
                        "Content-Type: text/parameters\r\n" \
                        "Content-Length: %d\r\n" \
                        "\r\n"

#define RTSP_500_ERROR  "RTSP/1.0 500 Internal Server Error\r\n" \
                        "CSeq: %d\r\n" \
                        "\r\n"

/*
 *
 */
static int get_string_parameter_from(const char *msg, const char *header_field, const char *parameter, char *val, size_t size) {
	char *ptr;
	char *token_id_val;
	char *token_id;
	const char delim[] = "?; &\n";

	char *line = get_header_field_from(msg, header_field);

	if (line) {
		// first find part after delim i.e. '?'
		char *token = strtok_r(line, delim, &ptr);

		while (token != NULL) {
			// find token i.e. src=1 or pids=1,2,3
			token = strtok_r(NULL, delim, &ptr);

			if (token != NULL) {
				// now find token id i.e. src(=1) or pids(=1,2,3)
				token_id = strtok_r(token, "=", &token_id_val);
				if (strcmp(parameter, token_id) == 0) {
					const size_t actual_size = strlen(token_id_val) + 1;
					if (size > actual_size) {
						memcpy(val, token_id_val, actual_size);
						FREE_PTR(line);
						return 1;
					}
				}
			}
		}
		FREE_PTR(line);
	}
	return -1;
}

/*
 *
 */
static int get_int_parameter_from(const char *msg, const char *header_field, const char *parameter) {
	char val[20];
	if (get_string_parameter_from(msg, header_field, parameter, val, sizeof(val)) == 1) {
		return atoi(val);
	}
	return -1;
}

/*
 *
 */
static double get_double_parameter_from(const char *msg, const char *header_field, const char *parameter) {
	char val[20];
	if (get_string_parameter_from(msg, header_field, parameter, val, sizeof(val)) == 1) {
		return atof(val);
	}
	return -1.0;
}

/*
 *
 */
static int parse_stream_string(const char *msg, const char *header_field, const char *delim1, const char *delim2, Client_t *client) {
	char *ptr;
    char *token_id_val;

	int  val_int;
	double val_double;
	char val_str[1024];

	if ((val_double = get_double_parameter_from(msg, header_field, "freq")) != -1) {
		client->fe->channel.freq = (uint32_t)(val_double * 1000.0);
		client->fe->tuned = 0;
	}
	if ((val_int = get_int_parameter_from(msg, header_field, "sr")) != -1) {
		client->fe->channel.srate = val_int * 1000UL;
	}
	if (get_string_parameter_from(msg, header_field, "pol", val_str, sizeof(val_str)) == 1) {
		if (strcmp(val_str, "h") == 0) {
			client->fe->diseqc.pol = 0;
		} else if (strcmp(val_str, "v") == 0) {
			client->fe->diseqc.pol = 1;
		}
	}

	char *line = get_header_field_from(msg, header_field);

	if (line) {
		// first find part after delim1 i.e. '?'
		char *token = strtok_r(line, delim1, &ptr);

		while (token != NULL) {
			// find token i.e. src=1 or pids=1,2,3
			token = strtok_r(NULL, delim2, &ptr);

			if (token != NULL) {
				// now find token id i.e. src(=1) or pids(=1,2,3)
				char *token_id = strtok_r(token, "=", &token_id_val);

				// check the token_id
				if (strcmp(token_id, "src") == 0) {
					const int src = atoi(token_id_val);
					client->fe->diseqc.src = src;
					if (src < MAX_LNB) {
						client->fe->diseqc.LNB = &client->fe->lnb_array[src];
					} else {
						SI_LOG_ERROR("src to big: %d", src);
					}
				} else if (strcmp(token_id, "ro") == 0) {
					// "0.35", "0.25", "0.20"
					if (strcmp(token_id_val, "0.35") == 0) {
						client->fe->channel.rolloff = ROLLOFF_35;
					} else if (strcmp(token_id_val, "0.25") == 0) {
						client->fe->channel.rolloff = ROLLOFF_25;
					} else if (strcmp(token_id_val, "0.20") == 0) {
						client->fe->channel.rolloff = ROLLOFF_20;
					} else if (strcmp(token_id_val, "auto") == 0) {
						client->fe->channel.rolloff = ROLLOFF_AUTO;
					} else {
						SI_LOG_ERROR("Unknown Rolloff [%s]", token_id_val);
					}
				} else if (strcmp(token_id, "fec") == 0) {
					const int fec = atoi(token_id_val);
					// "12", "23", "34", "56", "78", "89", "35", "45", "910"
					if (fec == 12) {
						client->fe->channel.fec = FEC_1_2;
					} else if (fec == 23) {
						client->fe->channel.fec = FEC_2_3;
					} else if (fec == 34) {
						client->fe->channel.fec = FEC_3_4;
					} else if (fec == 35) {
						client->fe->channel.fec = FEC_3_5;
					} else if (fec == 45) {
						client->fe->channel.fec = FEC_4_5;
					} else if (fec == 56) {
						client->fe->channel.fec = FEC_5_6;
					} else if (fec == 67) {
						client->fe->channel.fec = FEC_6_7;
					} else if (fec == 78) {
						client->fe->channel.fec = FEC_7_8;
					} else if (fec == 89) {
						client->fe->channel.fec = FEC_8_9;
					} else if (fec == 910) {
						client->fe->channel.fec = FEC_9_10;
					} else if (fec == 999) {
						client->fe->channel.fec = FEC_AUTO;
					} else {
						client->fe->channel.fec = FEC_NONE;
					}
				} else if (strcmp(token_id, "msys") == 0) {
					if (strcmp(token_id_val, "dvbs2") == 0) {
						client->fe->channel.delsys = SYS_DVBS2;
					} else if (strcmp(token_id_val, "dvbs") == 0) {
						client->fe->channel.delsys = SYS_DVBS;
					}
				} else if (strcmp(token_id, "mtype") == 0) {
					if (strcmp(token_id_val, "8psk") == 0) {
						client->fe->channel.modtype = PSK_8;
					} else if (strcmp(token_id_val, "qpsk") == 0) {
						client->fe->channel.modtype = QPSK;
					}
				} else if (strcmp(token_id, "pids") == 0 ||
						   strcmp(token_id, "addpids") == 0 ||
						   strcmp(token_id, "delpids") == 0) {
					char *val, *val_ptr;
					const int pid = (strcmp(token_id, "delpids") == 0) ? 0 : 1;
					val = strtok_r(token_id_val, ",", &val_ptr);
					while (val != NULL) {
						client->fe->pid.data[atoi(val)].used = pid;
						val = strtok_r(NULL, ",", &val_ptr);
					}
					client->fe->pid.changed = 1;
				} else if (strcmp(token_id, "client_port") == 0) {
					const int port = atoi(token_id_val);
					// client RTP
					client->rtp.client.addr.sin_family = AF_INET;
					client->rtp.client.addr.sin_addr.s_addr = inet_addr(client->ip_addr);
					client->rtp.client.addr.sin_port = htons(port);

					// client RTCP
					client->rtcp.client.addr.sin_family = AF_INET;
					client->rtcp.client.addr.sin_addr.s_addr = inet_addr(client->ip_addr);
					client->rtcp.client.addr.sin_port = htons(port+1);

					// server RTP
					client->rtp.server.addr.sin_family = AF_INET;
					client->rtp.server.addr.sin_port = htons(port+2);

					// server RTCP
					client->rtcp.server.addr.sin_family = AF_INET;
					client->rtcp.server.addr.sin_port = htons(port+3);
				}
			}
		}
	}
	FREE_PTR(line);
	return 1;
}

/*
 *
 */
static int setup_rtsp(const char *msg, Client_t *client, FrontendArray_t *fe, const char *server_ip_addr) {
#define RTSP_SETUP_OK	"RTSP/1.0 200 OK\r\n" \
						"CSeq: %d\r\n" \
						"Session: %08X;timeout=%d\r\n" \
						"Transport: RTP/AVP;unicast;client_port=%d-%d;source=%s;server_port=%d-%d\r\n" \
						"com.ses.streamID: %d\r\n" \
						"\r\n"
	char rtsp[500];
	int ret = 1;

	// lock - client data - frontend pointer
	pthread_mutex_lock(&client->mutex);
	pthread_mutex_lock(&client->fe_ptr_mutex);

	SI_LOG_INFO("Setup Message");
	
	// Do we have an frontend attached, check for requested one or find a free one
	if (client->fe == NULL) {
		const int fe_nr = get_int_parameter_from(msg, "SETUP", "fe");
		if (fe_nr != -1 && (fe_nr - 1) < fe->max_fe) {
			size_t timeout = 0;
			do {
				if (!fe->array[fe_nr-1]->attached) {
					client->fe = fe->array[fe_nr-1];
					client->fe->attached = 1;
					SI_LOG_INFO("Frontend: %d, Attaching to client %s as requested", client->fe->index, client->ip_addr);
					break;
				} else {
					// if frontend is busy try again...
					SI_LOG_INFO("Frontend: %d, Requested but busy.. not connected.. Retry..", fe_nr - 1);
					// unlock - client data - frontend pointer BEFORE SLEEP
					pthread_mutex_unlock(&client->mutex);
					pthread_mutex_unlock(&client->fe_ptr_mutex);
					
					usleep(150000);
					
					// lock - client data - frontend pointer AFTER SLEEP
					pthread_mutex_lock(&client->mutex);
					pthread_mutex_lock(&client->fe_ptr_mutex);
					++timeout;
				}
			} while (timeout < 3);
		} else {
			// dynamic alloc a frontend 
			size_t i;
			for (i = 0; i < fe->max_fe; ++i) {
				if (!fe->array[i]->attached) {
					client->fe = fe->array[i];
					client->fe->attached = 1;
					SI_LOG_INFO("Frontend: %d, Attaching to client %s", client->fe->index, client->ip_addr);
					break;
				}
			}
		}
	}
	// now we should have an frontend if not give 503 error
	if (client->fe != NULL) {
		// init variables for 'new' stream ?
		if (strstr(msg, "/stream=") == NULL) {
			client->rtsp.cseq = 1;
			client->rtsp.streamID += 1;
			client->rtsp.sessionID += 2;
		} else {
			SI_LOG_INFO("Modify Transport Parameters");
			++client->rtsp.cseq;
		}

		parse_stream_string(msg, "SETUP", "?", "& ", client);
		parse_stream_string(msg, "Transport", ":", ";\r", client);

		if (setup_frontend_and_tune(client->fe) == 1) {
			if (update_pid_filters(client->fe) == 1) {
				// set bufPtr to begin of RTP data (after Header)
				client->rtp.bufPtr = client->rtp.buffer + RTP_HEADER_LEN;
				// setup reply
				snprintf(rtsp, sizeof(rtsp), RTSP_SETUP_OK, client->rtsp.cseq, client->rtsp.sessionID, SESSION_TIMEOUT,
					ntohs(client->rtp.client.addr.sin_port), ntohs(client->rtcp.client.addr.sin_port),
					server_ip_addr,
					ntohs(client->rtp.server.addr.sin_port), ntohs(client->rtcp.server.addr.sin_port),
					client->rtsp.streamID);
				ret = 1;
			} else {
				SI_LOG_ERROR("Frontend: %d, Error setting pid filters", client->fe->index);
				// setup reply
				snprintf(rtsp, sizeof(rtsp), RTSP_500_ERROR, client->rtsp.cseq);
				ret = -1;
			}
		} else {
			SI_LOG_ERROR("Frontend: %d, Tuning failed... Try again.", client->fe->index);
			// setup reply
			snprintf(rtsp, sizeof(rtsp), RTSP_500_ERROR, client->rtsp.cseq);
			ret = -1;
		}
	} else {
		// setup reply
		snprintf(rtsp, sizeof(rtsp), RTSP_503_ERROR, client->rtsp.cseq, 0);
		ret = -1;
	}

	// unlock - client data - frontend pointer
	pthread_mutex_unlock(&client->mutex);
	pthread_mutex_unlock(&client->fe_ptr_mutex);

//	SI_LOG_INFO("%s", rtsp);

	// send reply to client
	if (send(client->rtsp.socket.fd, rtsp, strlen(rtsp), MSG_NOSIGNAL) == -1) {
		PERROR("send");
		ret = -1;
	}
	return ret;
}

/*
 *
 */
static int play_rtsp(const char *msg, Client_t *client, const char *server_ip_addr) {
#define RTSP_PLAY_OK	"RTSP/1.0 200 OK\r\n" \
						"RTP-Info: url=rtsp://%s/stream=%d\r\n" \
						"CSeq: %d\r\n" \
						"Session: %08X\r\n" \
						"\r\n"
	char rtsp[100];
	int ret = 1;

	// lock - client data - frontend pointer
	pthread_mutex_lock(&client->mutex);
	pthread_mutex_lock(&client->fe_ptr_mutex);
	
	SI_LOG_INFO("Play Message");
	
	if (client->fe) {
		// lock - frontend data
		pthread_mutex_lock(&client->fe->mutex);

		parse_stream_string(msg, "PLAY", "?", "& ", client);

		++client->rtsp.cseq;
		if (setup_frontend_and_tune(client->fe) == 1) {
			if (update_pid_filters(client->fe) == 1) {
				// set bufPtr to begin of RTP data (after Header)
				client->rtp.bufPtr = client->rtp.buffer + RTP_HEADER_LEN;
				// setup watchdog
				client->rtsp.watchdog = time(NULL) + SESSION_TIMEOUT + 5;
				snprintf(rtsp, sizeof(rtsp), RTSP_PLAY_OK, server_ip_addr, client->rtsp.streamID, client->rtsp.cseq, client->rtsp.sessionID);
				ret = 1;
			} else {
				SI_LOG_ERROR("Frontend: %d, Error setting pid filters", client->fe->index);
				// setup reply
				snprintf(rtsp, sizeof(rtsp), RTSP_500_ERROR, client->rtsp.cseq);
				ret = -1;
			}
		} else {
			SI_LOG_ERROR("Frontend: %d, Tuning failed... Try again.", client->fe->index);
			// setup reply
			snprintf(rtsp, sizeof(rtsp), RTSP_500_ERROR, client->rtsp.cseq);
			ret = -1;
		}
		// unlock - frontend data
		pthread_mutex_unlock(&client->fe->mutex);

	} else {
		SI_LOG_ERROR("No Frontend for client %s (%d - %d) ??", client->ip_addr,
						ntohs(client->rtp.client.addr.sin_port), ntohs(client->rtcp.client.addr.sin_port));
		// setup reply
		snprintf(rtsp, sizeof(rtsp), RTSP_503_ERROR, client->rtsp.cseq, 0);
		ret = -1;
	}

	// unlock - client data - frontend pointer
	pthread_mutex_unlock(&client->fe_ptr_mutex);
	pthread_mutex_unlock(&client->mutex);

//	SI_LOG_INFO("%s", rtsp);

	// send reply to client
	if (send(client->rtsp.socket.fd, rtsp, strlen(rtsp), MSG_NOSIGNAL) == -1) {
		PERROR("send");
		ret = -1;
	}

	return ret;
}

/*
 *
 */
static int options_rtsp(Client_t *client) {
#define RTSP_OPTIONS_OK	"RTSP/1.0 200 OK\r\n" \
						"CSeq: %d\r\n" \
						"Public: OPTIONS, SETUP, PLAY, TEARDOWN\r\n" \
						"Session: %08X\r\n" \
						"\r\n"
	char rtspOk[100];

	++client->rtsp.cseq;
	snprintf(rtspOk, sizeof(rtspOk), RTSP_OPTIONS_OK, client->rtsp.cseq, client->rtsp.sessionID);
//	SI_LOG_INFO("%s", rtspOk);

	// send reply to client
	if (send(client->rtsp.socket.fd, rtspOk, strlen(rtspOk), MSG_NOSIGNAL) == -1) {
		PERROR("error sending: RTSP_TEARDOWN_OK");
		return -1;
	}
	return 1;
}

/*
 *
 */
static int teardown_session(void *arg) {
	int ret = 1;
	Client_t *client = (Client_t *)arg;
	
#define RTSP_TEARDOWN_OK	"RTSP/1.0 200 OK\r\n" \
							"CSeq: %d\r\n" \
							"Session: %08X\r\n" \
							"\r\n"

	// lock - client data - frontend pointer
	pthread_mutex_lock(&client->mutex);
	pthread_mutex_lock(&client->fe_ptr_mutex);

	// stop: RTP - RTCP
	if (client->rtp.state == Stopped && client->rtcp.state == Stopped) {
		// clear rtsp state
		client->rtsp.watchdog  = 0;
		client->rtsp.state     = NotConnected;
		
		if (client->fe) {
			// clear frontend
			client->fe->tuned      = 0;
			client->fe->attached   = 0;

			clear_pid_filters(client->fe);

			// Detaching Frontend
			SI_LOG_INFO("Frontend: %d, Detached from client %s", client->fe->index, client->ip_addr);
			client->fe = NULL;
		} else {
			SI_LOG_ERROR("Frontend: x, Not attached anymore to client %s", client->ip_addr);
		}

		// Need to send reply
		if (client->teardown_graceful) {
			char rtspOk[100];
			++client->rtsp.cseq;
			snprintf(rtspOk, sizeof(rtspOk), RTSP_TEARDOWN_OK, client->rtsp.cseq, client->rtsp.sessionID);
//			SI_LOG_INFO("%s", rtspOk);

			// send reply to client
			if (send(client->rtsp.socket.fd, rtspOk, strlen(rtspOk), MSG_NOSIGNAL) == -1) {
				PERROR("error sending: RTSP_TEARDOWN_OK");
				ret = -1;
			}
		}
		// clear rtsp session state
		client->rtsp.streamID  = 0;
		client->rtsp.sessionID = 0;
		// clear callback
		client->teardown_session = NULL;
		
		CLOSE_FD(client->rtsp.socket.fd);
	} else {
		ret = 0;
	}

	// unlock - client data - frontend pointer
	pthread_mutex_unlock(&client->fe_ptr_mutex);
	pthread_mutex_unlock(&client->mutex);
	return ret;
}

/*
 *
 */
static void setup_teardown_message(Client_t *client, int graceful) {
	pthread_mutex_lock(&client->mutex);
	
	if (client->teardown_session == NULL) {
		SI_LOG_INFO("Teardown Message, gracefull: %d", graceful);
		
		client->teardown_graceful = graceful;
		client->teardown_session = &teardown_session;
		// all ready stopped ?
		if (client->rtp.state == Stopped && client->rtcp.state == Stopped) {
			client->teardown_session(client);
		} else {
			// stop: RTP - RTCP
			if (client->rtp.state != Stopped) {
				client->rtp.state = Stopping;
			}
			if (client->rtcp.state != Stopped) {
				client->rtcp.state = Stopping;
			}
		}
	} else {
		SI_LOG_INFO("Teardown Message underway");
	}
	pthread_mutex_unlock(&client->mutex);
}

/*
 *
 */
static void *thread_work_rtsp(void *arg) {
	RtpSession_t *rtpsession = (RtpSession_t *)arg;
	struct pollfd pfd[MAX_CLIENTS+1]; // plus 1 for server
	size_t i;

	SI_LOG_INFO("Setting up RTSP server");

	init_server_socket(&rtpsession->rtsp_server, MAX_CLIENTS, RTSP_PORT, 1);
	pfd[SERVER_POLL].fd = rtpsession->rtsp_server.fd;
	pfd[SERVER_POLL].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
	pfd[SERVER_POLL].revents = 0;
	for (i = 0; i < MAX_CLIENTS; ++i) {
		pfd[i+1].fd = -1;
		pfd[i+1].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
		pfd[i+1].revents = 0;
	}

	for (;;) {
		// call poll with a timeout of 500 ms
		const int pollRet = poll(pfd, MAX_CLIENTS+1, 500);
		if (pollRet > 0) {
			// Check who is sending data, so iterate over pfd
			for (i = 0; i < MAX_CLIENTS+1; ++i) {
				if (pfd[i].revents != 0) {
					if (i == SERVER_POLL) {
						// Server -> connection from client?
						size_t j;
						for (j = 0; j < MAX_CLIENTS; ++j) {
							if (rtpsession->client[j].rtsp.socket.fd == -1) {
								if (accept_connection(&rtpsession->rtsp_server, &rtpsession->client[j].rtsp.socket, rtpsession->client[j].ip_addr, 1) == 1) {
									// setup polling
									pfd[j+1].fd = rtpsession->client[j].rtsp.socket.fd;
									pfd[j+1].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
									pfd[j+1].revents = 0;

									// clear watchdog
									rtpsession->client[j].rtsp.watchdog = 0;
									rtpsession->client[j].rtsp.state = Connected;
								}
								break;
							}
						}
					} else {
						// client sends data to us, get client
						Client_t *client = &rtpsession->client[i-1];
						switch (client->rtsp.state) {
							case NotConnected:
								// Do nothing here
								break;
							case Connected:
								{
									char *msg = NULL;
									// receive rtsp message
									const int dataSize = recv_httpc_message(client->rtsp.socket.fd, &msg, MSG_DONTWAIT);
									if (dataSize > 0) {
										SI_LOG_DEBUG("%s", msg);

										if (strstr(msg, "SETUP") != NULL) {
											if (setup_rtsp(msg, client, &rtpsession->fe, rtpsession->interface.ip_addr) == -1) {
												SI_LOG_ERROR("Setup RTSP message failed");
												client->rtsp.state = Error;
											}
										} else if (strstr(msg, "PLAY") != NULL) {
											if (play_rtsp(msg, client, rtpsession->interface.ip_addr) == 1) {
												pthread_mutex_lock(&client->mutex);
												if (client->rtp.state != Started) {
													client->rtp.state = Starting;
												}
												if (client->rtcp.state != Started) {
													client->rtcp.state = Starting;
												}
												pthread_mutex_unlock(&client->mutex);
											}
										} else if (strstr(msg, "TEARDOWN") != NULL) {
											setup_teardown_message(client, 1);
											pfd[i].fd = -1;
										} else if (strstr(msg, "OPTIONS") != NULL) {
											options_rtsp(client);
											// reset watchdog and give some extra timeout
											client->rtsp.watchdog = time(NULL) + SESSION_TIMEOUT + 5;
										} else {
											SI_LOG_ERROR("Unknown RTSP message (%s)", msg);
											client->rtsp.state = Error;
										}
										// Close connection or keep-alive
										if (strstr(msg, "close") != NULL) {
											SI_LOG_INFO("RTSP Requested Connection closed by Client: %s", client->ip_addr);
										}

									} else if (dataSize == 0) {
										SI_LOG_ERROR("RTSP Connection closed by Client: %s ??", client->ip_addr);
										client->rtsp.state = Error;
									}
									FREE_PTR(msg);
								}
								break;
							default:
								SI_LOG_ERROR("Wrong RTSP State");
								// Fall-through
							case Error:
								CLOSE_FD(rtpsession->rtsp_server.fd);
								init_server_socket(&rtpsession->rtsp_server, MAX_CLIENTS, RTSP_PORT, 1);
								
								setup_teardown_message(client, 0);
								pfd[i].fd = -1;
								break;
						}
					}
				}
			}
		} else {
			// Check clients have time-out?
			size_t i;
			for (i = 0; i < MAX_CLIENTS; ++i) {
				Client_t *client = &rtpsession->client[i];
				// check watchdog
				if (client->rtsp.watchdog != 0 && client->rtsp.watchdog < time(NULL)) {
					SI_LOG_INFO("RTSP Connection Watchdog kicked-in, for client %s", client->ip_addr);
					setup_teardown_message(client, 0);
					pfd[i+1].fd = -1;
				}
			}
		}
//		usleep(10000);
	}
	return NULL;
}

/*
 *
 */
void start_rtsp(RtpSession_t *rtpsession) {
	if (pthread_create(&(rtpsession->rtsp_threadID), NULL, &thread_work_rtsp, rtpsession) != 0) {
		PERROR("thread_work_rtsp");
	}
	pthread_setname_np(rtpsession->rtsp_threadID, "thread_rtsp");
}

/*
 *
 */
int stop_rtsp(RtpSession_t *rtpsession) {
	pthread_cancel(rtpsession->rtsp_threadID);
	pthread_join(rtpsession->rtsp_threadID, NULL);

	return 1;
}
