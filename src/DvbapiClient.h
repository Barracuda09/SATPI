/* DvbapiClient.h

   Copyright (C) 2015 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef DVB_API_CLIENT_H_INCLUDE
#define DVB_API_CLIENT_H_INCLUDE

#include "ThreadBase.h"
#include "SocketClient.h"
#include "Mutex.h"

// forward declaration
class StreamProperties;

/// DVBAPI Client
class DvbapiClient :
		public ThreadBase {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		DvbapiClient();
		virtual ~DvbapiClient();

		bool decrypt(StreamProperties &properties, unsigned char *data, int len);

	protected:
		/// Thread function
		virtual void threadEntry();

	private:

		///
		bool initClientSocket(SocketClient &client, int port, in_addr_t s_addr);

		///
		void sendClientInfo();

		///
		void collectPAT(StreamProperties &properties, unsigned char *data, int len);
		
		///
		void collectPMT(StreamProperties &properties, unsigned char *data, int len);

		// =======================================================================
		// Data members
		// =======================================================================
		Mutex _mutex;
		SocketClient _client;
		std::string _server_ip_addr;

}; // class DvbapiClient

#endif // DVB_API_CLIENT_H_INCLUDE