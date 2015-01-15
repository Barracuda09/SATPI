/* connection.h

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


#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <net/if.h>
#include <poll.h>

#define HTTP_PORT 8875
#define RTSP_PORT 554
#define SSDP_PORT 1900

// Socket attributes
typedef struct {
	struct sockaddr_in addr;        //
	int fd;                         // file descriptor
} SocketAttr_t;

// Interface attributes
typedef struct {
	char ip_addr[20];               // ip address of the used interface
	char mac_addr_decorated[20];    // mac address of the used interface
	char mac_addr[20];              // mac address of the used interface
	char iface_name[IFNAMSIZ];      // used interface name i.e. eth0
} Interface_Attr_t;

// TCP connection data
typedef struct {
#define SERVER_POLL     0                 // server poll pos
#define CLIENT_POLL_OFF 1                 // poll offset for clients
#define MAX_TCP_CONN    5                 // max connections
#define MAX_POLL_CNT    MAX_TCP_CONN + 1  // max poll (client + server)
	SocketAttr_t server;                  //
	SocketAttr_t client[MAX_TCP_CONN];    //
	struct pollfd pfd[MAX_POLL_CNT];      // 0 = used for server rest for clients
} TcpConnection_t;

//
ssize_t recv_httpc_message(int fd, char **buf, int recv_flags);

//
void clear_connection_properties(TcpConnection_t *connection);

//
void close_connection(TcpConnection_t *connection);

//
int init_server_socket(SocketAttr_t *server, int backlog, int port, int nonblock);

// Accept an connection save client IP address
int accept_connection(const SocketAttr_t *server, SocketAttr_t *client, char *ip_addr, int show_log_info);

// Get interface properties
int get_interface_properties(Interface_Attr_t *interface);

#endif
