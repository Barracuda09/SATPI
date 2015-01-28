/* http.c

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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#include "http.h"
#include "rtp.h"
#include "utils.h"
#include "connection.h"
#include "applog.h"

#define UTIME_DEL 200000

#define HTML_BODY_CONT	"HTTP/1.1 %s\r\n" \
						"Server: SatIP WebServer v0.1\r\n" \
						"Location: %s\r\n" \
						"Content-Type: %s\r\n" \
						"Content-Length: %d\r\n" \
						"\r\n"

#define HTML_OK            "200 OK"
#define HTML_NO_RESPONSE   "204 No Response"
#define HTML_NOT_FOUND     "404 Not Found"
#define HTML_MOVED_PERMA   "301 Moved Permanently"


#define CONTENT_TYPE_XML   "text/xml; charset=UTF-8"
#define CONTENT_TYPE_HTML  "text/html; charset=UTF-8"
#define CONTENT_TYPE_JS    "text/javascript; charset=UTF-8"
#define CONTENT_TYPE_CSS   "text/css; charset=UTF-8"
#define CONTENT_TYPE_PNG   "image/png"
#define CONTENT_TYPE_ICO   "image/x-icon"

static TcpConnection_t tcp_conn;
static pthread_t threadID;

/*
 *
 */
static char *make_frontend_xml(const RtpSession_t *rtpsession) {
	char *ptr = NULL;
	// make data xml
	addString(&ptr, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
	addString(&ptr, "<data>\r\n");

	pthread_mutex_lock(&((RtpSession_t *)rtpsession)->mutex);
	
	// application data
	addString(&ptr, "<appdata>");
	addString(&ptr, "<uptime>%d</uptime>", time(NULL) - rtpsession->appStartTime);
	addString(&ptr, "<rtppayload>%.3f</rtppayload>", (rtpsession->rtp_payload / (1024.0 * 1024.0)));
	addString(&ptr, "</appdata>");

	pthread_mutex_unlock(&((RtpSession_t *)rtpsession)->mutex);
	
	addString(&ptr, "<frontend>");
	size_t i;
	for (i = 0; i < rtpsession->fe.max_fe; ++i) {
		const Frontend_t *fe = rtpsession->fe.array[i];
		addString(&ptr, "<frontenddata>");

		// frontend info
		pthread_mutex_lock(&((Frontend_t *)fe)->mutex);
		addString(&ptr, "<frontendindex>%d</frontendindex>", fe->index);
		addString(&ptr, "<frontendname>%s</frontendname>", fe->fe_info.name);
		addString(&ptr, "<pathname>%s</pathname>", fe->path_to_fe);
		addString(&ptr, "<attached>%d</attached>", fe->attached);
		addString(&ptr, "<freq>%d Hz to %d Hz</freq>", fe->fe_info.frequency_min, fe->fe_info.frequency_max);
		addString(&ptr, "<symbol>%d symbols/s to %d symbols/s</symbol>", fe->fe_info.symbol_rate_min, fe->fe_info.symbol_rate_max);
		pthread_mutex_unlock(&((Frontend_t *)fe)->mutex);

		// channel
		addString(&ptr, "<delsys>%s</delsys>", delsys_to_string(fe->channel.delsys));
		addString(&ptr, "<tunefreq>%d</tunefreq>", fe->channel.freq);
		addString(&ptr, "<modulation>%s</modulation>", modtype_to_sting(fe->channel.modtype));
		addString(&ptr, "<fec>%s</fec>", fec_to_string(fe->channel.fec));

		addString(&ptr, "<tunesymbol>%d</tunesymbol>", fe->channel.srate);
		addString(&ptr, "<rolloff>%s</rolloff>", rolloff_to_sting(fe->channel.rolloff));
		addString(&ptr, "<src>%d</src>", fe->diseqc.src);
		addString(&ptr, "<pol>%c</pol>", fe->diseqc.pol ? 'V' : 'H');

		// monitor
		pthread_mutex_lock(&((Frontend_t *)fe)->monitor.mutex);
		addString(&ptr, "<status>%d</status>", fe->monitor.status);
		addString(&ptr, "<signal>%d</signal>", fe->monitor.strength);
		addString(&ptr, "<snr>%d</snr>", fe->monitor.snr);
		addString(&ptr, "<ber>%d</ber>", fe->monitor.ber);
		addString(&ptr, "<unc>%d</unc>", fe->monitor.ublocks);
		pthread_mutex_unlock(&((Frontend_t *)fe)->monitor.mutex);

		addString(&ptr, "</frontenddata>");
	}
	addString(&ptr, "</frontend>");
	addString(&ptr, "</data>\r\n");

	return ptr;
}

/*
 *
 */
static char *make_data_xml(const RtpSession_t *rtpsession) {
	char *ptr = NULL;
	size_t i = 0;
	size_t j = 0;

	// make data xml
	addString(&ptr, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n");
	addString(&ptr, "<data>\r\n");
	
	pthread_mutex_lock(&((RtpSession_t *)rtpsession)->mutex);

	// application data
	addString(&ptr, "<appdata>");
	addString(&ptr, "<uptime>%d</uptime>", time(NULL) - rtpsession->appStartTime);
	addString(&ptr, "<rtppayload>%d</rtppayload>", rtpsession->rtp_payload);
	addString(&ptr, "</appdata>");
	
	pthread_mutex_unlock(&((RtpSession_t *)rtpsession)->mutex);

	// pid data
	addString(&ptr, "<pids>");
	for (i = 0; i < rtpsession->fe.max_fe; ++i) {
		const Frontend_t *fe = rtpsession->fe.array[i];
		addString(&ptr, "<pidlist>");

		pthread_mutex_lock(&((Frontend_t *)fe)->mutex);
		if (!fe->pid.all) {
			for (j = 0; j < MAX_PIDS; ++j) {
				if (fe->pid.data[j].used) {
					addString(&ptr, "<pid>%d</pid><cc>%d</cc><count>%d</count>", j, fe->pid.data[j].cc_error, fe->pid.data[j].count);
				}
			}
		}
		pthread_mutex_unlock(&((Frontend_t *)fe)->mutex);

		addString(&ptr, "</pidlist>\r\n");
	}
	addString(&ptr, "</pids>");

	// rtp
	addString(&ptr, "<rtp>");
	for (i = 0; i < MAX_CLIENTS; ++i) {
		const Client_t *cl = &rtpsession->client[i];
		pthread_mutex_lock(&((Client_t *)cl)->mutex);
		addString(&ptr, "<rtpdata>");
		addString(&ptr, "<ip>%s</ip>", inet_ntoa(cl->rtp.client.addr.sin_addr));
		addString(&ptr, "<rtpport>%d</rtpport>", ntohs(cl->rtp.client.addr.sin_port));
		addString(&ptr, "<rtcpport>%d</rtcpport>", ntohs(cl->rtcp.client.addr.sin_port));
		addString(&ptr, "<spc>%d</spc>", cl->spc);
		addString(&ptr, "<soc>%d</soc>", cl->soc);
		addString(&ptr, "</rtpdata>");
		pthread_mutex_unlock(&((Client_t *)cl)->mutex);
	}
	addString(&ptr, "</rtp>");

	addString(&ptr, "</data>\r\n");

	return ptr;
}

/*
 *
 */
static char * read_file(const char *file, int *file_size) {
	int fd;
	if ((fd = open(file, O_RDONLY | O_NONBLOCK)) < 0) {
		SI_LOG_INFO("GET %s", file);
		PERROR("File not found");
		return NULL;
	}
	const off_t size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	char *buf = malloc(size);
	if (buf) {
		*file_size = read(fd, buf, size);
	}
	CLOSE_FD(fd);
	SI_LOG_DEBUG("GET %s (size %d)", file, *file_size);
	return buf;
}

/*
 *
 */
static int post_http(int fd, const char *msg, RtpSession_t *session) {
	char htmlBody[500];
	extern int syslog_on;
	
//	SI_LOG_DEBUG("%s", msg);
	
	char *cont = get_content_type_from(msg);
	if (cont) {
		char *val;
		char *id = strtok_r(cont, "=", &val);
		if (strncmp(id, "ssdp_interval", 13) == 0) {
			session->ssdp_announce_time_sec = atoi(val);
			SI_LOG_INFO("Setting SSDP annouce interval to: %d", session->ssdp_announce_time_sec);
		} else if (strncmp(id, "syslog_on", 9) == 0) {
			if (strncmp(val, "true", 4) == 0) {
				syslog_on = 1;
				SI_LOG_INFO("Logging to syslog: %s", val);
			} else {
				SI_LOG_INFO("Logging to syslog: %s", val);
				syslog_on = 0;
			}
		}
		FREE_PTR(cont);
	}
	// setup reply
	snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_NO_RESPONSE, "", CONTENT_TYPE_HTML, 0);

	// send 'htmlBody' to client
	if (send(fd, htmlBody, strlen(htmlBody), MSG_NOSIGNAL) == -1) {
		PERROR("send htmlBody");
		return -1;
	}
	return 1;
}

/*
 *
 */
static int get_http(int fd, const char *msg, const RtpSession_t *rtpsession) {
	char htmlBody[500];
	char *docType = NULL;
	int docTypeSize;
	char file[30];
	char path[100];

	// special ugly
	if (strstr(msg, "STOP") != NULL) {
		SI_LOG_INFO("Stop requested..");
		extern int exitApp;
		exitApp = 1;
		return 0;
	}

	// Parse what to get
	if (strstr(msg, "/ ") != NULL) {
#define HTML_MOVED	"<html>\r\n" \
					"<head>\r\n" \
					"<title>Page is moved</title>\r\n" \
					"</head>\r\n" \
					"<body>\r\n" \
					"<h1>Moved</h1>\r\n" \
					"<p>This page is moved:\r\n" \
					"<a href=\"%s:%d%s\">link</a>.</p>\r\n" \
					"</body>\r\n" \
					"</html>"
		snprintf(file, sizeof(file), "/index.html");
		addString(&docType, HTML_MOVED, rtpsession->interface.ip_addr, HTTP_PORT, file);
		docTypeSize = strlen(docType);
		snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_MOVED_PERMA, file, CONTENT_TYPE_HTML, docTypeSize);
	} else {
		char *ptr = NULL;
		char *line = get_header_field_from(msg, "GET");
		// first token is GET second is requested file
	    strtok_r(line, " ", &ptr);
	    const char *token = strtok_r(ptr, " ", &ptr);
		snprintf(file, sizeof(file), "%s", token);
		snprintf(path, sizeof(path), "web%s", file);
		FREE_PTR(line);
	}

	if (docType == NULL) {
		// load the requested file
		if (strstr(file, "data.xml") != NULL) {
			docType = make_data_xml(rtpsession);
			docTypeSize = strlen(docType);

			snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize);
		} else if (strstr(file, "frontend.xml") != NULL) {
			docType = make_frontend_xml(rtpsession);
			docTypeSize = strlen(docType);

			snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize);
		} else if (strstr(file, "log.xml") != NULL) {
			docType = make_log_xml();
			docTypeSize = strlen(docType);
			
			snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize);
		} else if ((docType = read_file(path, &docTypeSize))) {
			if (strstr(file, ".xml") != NULL) {
				// check if the request is the SAT>IP description xml then fill in the UUID and tuner string
				if (strstr(docType, "urn:ses-com:device") != NULL) {
					docTypeSize -= 4; // minus 2x %s
					docTypeSize += strlen(rtpsession->fe.del_sys_str);
					docTypeSize += strlen(rtpsession->uuid);
					char *doc_desc_xml = malloc(docTypeSize+1);
					snprintf(doc_desc_xml, docTypeSize+1, docType, rtpsession->uuid, rtpsession->fe.del_sys_str);
					FREE_PTR(docType);
					docType = doc_desc_xml;
				}
				snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_XML, docTypeSize);
			} else if (strstr(file, ".html") != NULL) {
				snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_HTML, docTypeSize);
			} else if (strstr(file, ".js") != NULL) {
				snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_JS, docTypeSize);
			} else if (strstr(file, ".css") != NULL) {
				snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_CSS, docTypeSize);
			} else if ((strstr(file, ".png") != NULL) ||
					   (strstr(file, ".ico") != NULL)) {
				snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_PNG, docTypeSize);
			} else {
				snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_OK, file, CONTENT_TYPE_HTML, docTypeSize);
			}
		} else if ((docType = read_file("web/404.html", &docTypeSize))) {
			snprintf(htmlBody, sizeof(htmlBody), HTML_BODY_CONT, HTML_NOT_FOUND, file, CONTENT_TYPE_HTML, docTypeSize);
		}
	}

	// send something?
	if (docType) {
//		SI_LOG_INFO("%s", htmlBody);
		// send 'htmlBody' to client
		if (send(fd, htmlBody, strlen(htmlBody), 0) == -1) {
			PERROR("send htmlBody");
			FREE_PTR(docType);
			return -1;
		}
		// send 'docType' to client
		if (send(fd, docType, docTypeSize, 0) == -1) {
			PERROR("send docType");
			FREE_PTR(docType);
			return -1;
		}
		FREE_PTR(docType);
		return 1;
	}
	return 0;
}

/*
 *
 */
static void * thread_work_http(void *arg) {
	RtpSession_t *rtpsession = (RtpSession_t *)arg;

	// Clear
	clear_connection_properties(&tcp_conn);

	// init
	init_server_socket(&tcp_conn.server, MAX_TCP_CONN, HTTP_PORT, 1);

	tcp_conn.pfd[SERVER_POLL].fd = tcp_conn.server.fd;
	tcp_conn.pfd[SERVER_POLL].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
	tcp_conn.pfd[SERVER_POLL].revents = 0;


	for (;;) {
		// call poll with a timeout of 500 ms
		const int pollRet = poll(tcp_conn.pfd, MAX_POLL_CNT, 500);
		if (pollRet > 0) {
			// Check who is sending data, so iterate over pfd
			size_t i;
			for (i = 0; i < MAX_POLL_CNT; ++i) {
				if (tcp_conn.pfd[i].revents != 0) {
					if (i == SERVER_POLL) {
						// Server -> connection from client?
						size_t j;
						for (j = 0; j < MAX_TCP_CONN; ++j) {
							if (tcp_conn.client[j].fd == -1) {
								char ip_addr[20];
								if (accept_connection(&tcp_conn.server, &tcp_conn.client[j], ip_addr, 0) == 1) {
									tcp_conn.pfd[j + CLIENT_POLL_OFF].fd = tcp_conn.client[j].fd;
								}
								break;
							}
						}
					} else {
						char *msg = NULL;
						// try to read data
						const int dataSize = recv_httpc_message(tcp_conn.pfd[i].fd, &msg, MSG_DONTWAIT);
						if (dataSize > 0) {
//							SI_LOG_INFO("%s", msg);

							// parse HTML
							if (strstr(msg, "GET /") != NULL) {
								get_http(tcp_conn.pfd[i].fd, msg, rtpsession);
							} else if (strstr(msg, "POST /") != NULL) {
								post_http(tcp_conn.pfd[i].fd, msg, rtpsession);
							} else {
								SI_LOG_ERROR("Unknown HTML message connection: %s", msg);
							}
							// Close connection or keep-alive
							if (strstr(msg, "keep-alive") == NULL) {
								SI_LOG_DEBUG("Requested Connection closed by Client %d", i);
								CLOSE_FD(tcp_conn.pfd[i].fd);
								tcp_conn.client[i-1].fd = -1;
							}
						} else if (dataSize == 0) {
							SI_LOG_DEBUG("Connection closed by Client %d", i);
							CLOSE_FD(tcp_conn.pfd[i].fd);
							tcp_conn.client[i-1].fd = -1;
						}
						FREE_PTR(msg);
					}
				}
			}
		} else if (pollRet == 0) {
			usleep(5000);
		} else if (pollRet == -1) {
			PERROR("HTML Poll Error");
		}
	}
	return NULL;
}

/*
 *
 */
void start_http(RtpSession_t *rtpsession) {
	SI_LOG_INFO("Setting up HTTP server");

	if (pthread_create(&threadID, NULL, &thread_work_http, rtpsession) != 0) {
		SI_LOG_ERROR("thread_work_http");
	}
	pthread_setname_np(threadID, "thread_http");
}

/*
 *
 */
int stop_http() {
	close_connection(&tcp_conn);
	
	pthread_cancel(threadID);
	pthread_join(threadID, NULL);
	return 1;
}
