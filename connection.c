/* connection.c

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
#include <net/if.h>
#include <sys/ioctl.h>

#include "connection.h"
#include "utils.h"
#include "applog.h"

/*
 *
 */
static ssize_t recv_recvfrom_httpc_message(int fd, char **buf, int recv_flags, struct sockaddr_in *si_other, socklen_t *addrlen) {
#define HTTPC_TIMEOUT 3
	size_t read_len = 0;
	size_t msg_size = 2048;
	size_t timeout = HTTPC_TIMEOUT;
	int found_end = 0;
	int done = 0;

	// read until we have '\r\n\r\n' (end of HTTP/RTSP message) then check
	// if the header field 'Content-Length' is present
	*buf = malloc(msg_size + 1);
	do {
		const ssize_t size = recvfrom(fd, *buf + read_len, msg_size - read_len, recv_flags, (struct sockaddr *)si_other, addrlen);
		if (size > 0) {
			read_len += size;
			// terminate buf
			*(*buf + read_len) = 0;
			// reset time out
			timeout = HTTPC_TIMEOUT;
			if (found_end && read_len == msg_size) {
				done = 1;
			} else if (!found_end) {
				if (read_len > 4 && strstr(*buf, "\r\n\r\n") != NULL) {
					found_end = 1;
					const size_t header_size = (strstr(*buf, "\r\n\r\n") - *buf) + 4;
					char *line = get_header_field_from(*buf, "Content-Length");
					// Do we need to read content?
					if (line == NULL) {
						// adjust buffer to what we have received
						*buf = realloc(*buf, read_len + 1);
						done = 1;
					} else {
						char *val;
						strtok_r(line, ":", &val);
						msg_size = header_size + atoi(val);
						FREE_PTR(line);
						// check already finished?
						if (read_len == msg_size) {
							done = 1;
						} else {
							// adjust buffer to real size
							*buf = realloc(*buf, msg_size + 1);
						}
					}
				} else if (msg_size == read_len) {
					// increase size
					msg_size = read_len + 100;
					*buf = realloc(*buf, msg_size + 1);
				}
			}
		} else {
			if (timeout != 0 && size == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				usleep(50000);
				--timeout;
			} else {
				FREE_PTR(*buf);
				return size;
			}
		}
	} while (!done);
	
	// terminate
	*(*buf + read_len) = 0;

	return read_len;
}

/*
 *
 */
ssize_t recv_httpc_message(int fd, char **buf, int recv_flags) {
	return recv_recvfrom_httpc_message(fd, buf, recv_flags, NULL, NULL);
}

/*
 *
 */
ssize_t recvfrom_httpc_message(int fd, char **buf, int recv_flags, struct sockaddr_in *si_other, socklen_t *addrlen) {
	return recv_recvfrom_httpc_message(fd, buf, recv_flags, si_other, addrlen);
}

/*
 *
 */
void clear_connection_properties(TcpConnection_t *connection) {
	size_t i;

	// Clear
	connection->server.fd = -1;
	for (i = 0; i < MAX_TCP_CONN; ++i) {
		connection->client[i].fd = -1;
	}
	for (i = 0; i < MAX_POLL_CNT; ++i) {
		connection->pfd[i].fd = -1;
		connection->pfd[i].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
		connection->pfd[i].revents = 0;
	}
}

/*
 *
 */
void close_connection(TcpConnection_t *connection) {
	size_t i;
	for (i = 0; i < MAX_TCP_CONN; ++i) {
		CLOSE_FD(connection->client[i].fd);
	}
	CLOSE_FD(connection->server.fd);
	clear_connection_properties(connection);
}

/*
 *
 */
int init_udp_socket(SocketAttr_t *server, int port, in_addr_t s_addr) {
	// fill in the socket structure with host information
	memset(server, 0, sizeof(server));
	server->addr.sin_family      = AF_INET;
	server->addr.sin_addr.s_addr = s_addr;
	server->addr.sin_port        = htons(port);

	if ((server->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		PERROR("socket send");
		return -1;
	}
	int val = 1;
	if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("Server setsockopt SO_REUSEADDR");
		return -1;
	}
	return 1;
}

/*
 *
 */
int init_mutlicast_udp_socket(SocketAttr_t *server, int port, char *ip_addr) {
	// fill in the socket structure with host information
	memset(server, 0, sizeof(*server));
	server->addr.sin_family      = AF_INET;
	server->addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server->addr.sin_port        = htons(port);

	if ((server->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		PERROR("socket listen");
		return -1;
	}

	int val = 1;
	if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("Server setsockopt SO_REUSEADDR");
		return -1;
	}

	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
	mreq.imr_interface.s_addr = inet_addr(ip_addr);
	if (setsockopt(server->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
		PERROR("IP_ADD_MEMBERSHIP");
		return -1;
	}

	// bind the socket to the port number 
	if (bind(server->fd, (struct sockaddr *) &server->addr, sizeof(server->addr)) == -1) {
		PERROR("udp multi bind");
		return -1;
	}
	return 1;
}

/*
 *
 */
int init_server_socket(SocketAttr_t *server, int backlog, int port, int nonblock) {
	/* fill in the socket structure with host information */
	memset(server, 0, sizeof(*server));
	server->addr.sin_family      = AF_INET;
	server->addr.sin_addr.s_addr = INADDR_ANY;
	server->addr.sin_port        = htons(port);

	if ((server->fd = socket(AF_INET, SOCK_STREAM | ((nonblock) ? SOCK_NONBLOCK : 0), 0)) == -1) {
		PERROR("Server socket");
		return -1;
	}

	int val = 1;
	if (setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1) {
		PERROR("Server setsockopt SO_REUSEADDR");
		return -1;
	}

	// bind the socket to the port number 
	if (bind(server->fd, (struct sockaddr *) &server->addr, sizeof(server->addr)) == -1) {
		PERROR("Server bind");
		return -1;
	}

	// start to listen
	if (listen(server->fd, backlog) == -1) {
		PERROR("Server listen");
		return -1;
	}
	return 1;
}

/*
 *
 */
int accept_connection(const SocketAttr_t *server, SocketAttr_t *client, char *ip_addr, int show_log_info) {
	// check if we have a client?
	socklen_t addrlen = sizeof(client->addr);
	const int fd_conn = accept(server->fd, (struct sockaddr *) &client->addr, &addrlen);
	if (fd_conn != -1) {
		// save connected file descriptor
		client->fd = fd_conn;

		// save client ip address
		strcpy(ip_addr, (char *)inet_ntoa(client->addr.sin_addr));

		// Show who is connected
		if (show_log_info) {
			if (ntohs(server->addr.sin_port) == RTSP_PORT) {
				SI_LOG_INFO("RTSP Connection from %s Port %d", ip_addr, ntohs(client->addr.sin_port));
			} else {
				SI_LOG_INFO("Connection from %s Port %d", ip_addr, ntohs(client->addr.sin_port));
			}
		}
		return 1;
	}
	return -1;
}

/*
 *
 */
int get_interface_properties(Interface_Attr_t *interface) {
	struct ifreq ifr;
	struct sockaddr_in addr;
	int fd = -1;
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons(8843);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		PERROR("Server socket");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;

	// get interface name
	if (ioctl(fd, SIOCGIFNAME, &ifr) != 0) {
		PERROR("ioctl SIOCGIFNAME");
		CLOSE_FD(fd);
		return -1;
	}

	// Get hardware address
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) != 0) {
		PERROR("ioctl SIOCGIFHWADDR");
		CLOSE_FD(fd);
		return -1;
	}
	
	const unsigned char* mac=(unsigned char*)ifr.ifr_hwaddr.sa_data;
	snprintf(interface->mac_addr_decorated, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	snprintf(interface->mac_addr, 18, "%02x%02x%02x%02x%02x%02x",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	
	// get interface name
	if (ioctl(fd, SIOCGIFNAME, &ifr) != 0) {
		PERROR("ioctl SIOCGIFNAME");
		CLOSE_FD(fd);
		return -1;
	}
	
	// get PA address
	if (ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
		PERROR("ioctl SIOCGIFHWADDR");
		CLOSE_FD(fd);
		return -1;
	}
	CLOSE_FD(fd);
	memcpy(interface->ip_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), 20);
	memcpy(interface->iface_name, ifr.ifr_name, IFNAMSIZ);
	
	SI_LOG_INFO("%s: %s [%s]", interface->iface_name, interface->ip_addr, interface->mac_addr_decorated);
	
	return 1;
}

