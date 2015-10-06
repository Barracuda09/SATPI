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
#include "DvbapiClientProperties.h"
#include "XMLSupport.h"
#include "Functor3.h"

#include <string>

/// The class @c DvbapiClient is for decrypting streams
class DvbapiClient : public ThreadBase,
                     public XMLSupport {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		DvbapiClient(const Functor3<int, int, bool> &setECMPIDCallback);
		virtual ~DvbapiClient();

		///
		bool decrypt(int streamID, unsigned char *data, uint32_t &len);

		///
		bool stopDecrypt(int streamID);

		///
		bool clearDecrypt(int streamID);

		/// Add data to an XML for storing or web interface
		virtual void addToXML(std::string &xml) const;

		/// Get data from an XML for restoring or web interface
		virtual void fromXML(const std::string &xml);
	protected:
		/// Thread function
		virtual void threadEntry();

	private:

		///
		bool initClientSocket(SocketClient &client, int port, in_addr_t s_addr);

		///
		void sendClientInfo();

		///
		void collectPAT(int streamID, const unsigned char *data, int len);

		///
		void collectPMT(int streamID, const unsigned char *data, int len);

		///
		void cleanPacketPMT(int streamID, unsigned char *data);

		///
		void collectECM(int streamID, const unsigned char *data);

		// =======================================================================
		// Data members
		// =======================================================================
		Mutex _mutex;
		bool  _connected;
		bool _enabled;
		SocketClient _client;
		std::string _serverIpAddr;
		int _serverPort;

		/// @todo alloc this dynamic
		DvbapiClientProperties _properties[5];

		Functor3<int, int, bool> _setECMPIDCallback;

}; // class DvbapiClient

#endif // DVB_API_CLIENT_H_INCLUDE