/* rtp.h

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

#ifndef _RTP_H
#define _RTP_H

#include <pthread.h>

#include "tune.h"
#include "connection.h"

#define RTP_HEADER_LEN 12

#define MTU             1500
#define TS_PACKET_SIZE  188

enum Rtsp_State {
	NotConnected,
	Connected,
	Error
};

typedef enum {
	Starting,
	Started,
	Stopping,
	Stopped,
	Not_Initialized
} Session_t;

// RTSP connection properties
typedef struct {
	SocketAttr_t    socket;         //
	time_t          watchdog;       // watchdog
	unsigned int    check_watchdog; // check watchdog
	int             sessionID;      // session ID
	int             streamID;       // stream ID
	int             cseq;           // sequence number
	int             shall_close;    // If the connection shall be closed by client
	enum Rtsp_State state;          //
} RTSP_t;

// RTP connection properties
typedef struct {
	pthread_t     threadID;       // 
	Session_t     state;          // show the RTP session state
	long          timestamp;      //
	long          send_interval;  // RTP interval time (100ms)
	uint16_t      cseq;           // RTP sequence number
	unsigned char buffer[MTU];    // RTP/UDP buffer
	unsigned char *bufPtr;        // RTP/UDP buffer pointer
	SocketAttr_t  server;         // rtp socket addr info (IP and Port = even number)
	SocketAttr_t  client;         // rtp socket addr info (IP and Port = even number)
} RTP_t;

// RTCP connection properties
typedef struct {
	pthread_t     threadID;       //
	Session_t     state;          // show the RTCP session state
	SocketAttr_t  server;         // rtcp socket addr info (IP and Port = rtpPort + 1)
	SocketAttr_t  client;         // rtcp socket addr info (IP and Port = rtpPort + 1)
} RTCP_t;

// Client properties
typedef struct {
	pthread_mutex_t mutex;        // client mutex
	char            ip_addr[20];  // ip address of client
	pthread_mutex_t fe_ptr_mutex; // frontend pointer mutex
	Frontend_t      *fe;          // pointer to used frontend
	RTSP_t          rtsp;         //
	RTP_t           rtp;          //
	RTCP_t          rtcp;         //
	uint32_t        ssrc;         // synchronisation source identifier of sender
	uint32_t        spc;          // sender RTP packet count  (used in SR packet)
	uint32_t        soc;          // sender RTP payload count (used in SR packet)
	double          rtp_payload;  //
	
	int (*teardown_session) (void *client);
	int             teardown_graceful;
} Client_t;

//
typedef struct {
#define MAX_CLIENTS   8
	FrontendArray_t  fe;                      //
	Client_t         client[MAX_CLIENTS];     //
	SocketAttr_t     rtsp_server;             //
	pthread_t        rtsp_threadID;           //

	Interface_Attr_t interface;               //
	char             uuid[40];                //

	pthread_mutex_t  mutex;                   // global mutex
	time_t           appStartTime;            // the application start time (EPOCH)
	double           rtp_payload;             // Total RTP payload count in MBytes
	unsigned int     ssdp_announce_time_sec;  // SSDP announce interval
} RtpSession_t;

// Initialize RTP/RTCP connection data
void init_rtp(RtpSession_t *rtpsession);

// Start RTP
void start_rtp(RtpSession_t *rtpsession);

// Stop and close RTP/RTCP connection
int stop_rtp(RtpSession_t *rtpsession);

#endif
