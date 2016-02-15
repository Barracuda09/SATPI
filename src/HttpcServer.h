/* HttpcServer.h

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef HTTPC_SERVER_H_INCLUDE
#define HTTPC_SERVER_H_INCLUDE HTTPC_SERVER_H_INCLUDE

#include <FwDecl.h>
#include <socket/TcpSocket.h>
#include <Utils.h>

FW_DECL_NS0(Stream);
FW_DECL_NS0(StreamManager);
FW_DECL_NS0(InterfaceAttr);

/// HTTP Client Server
class HttpcServer :
	public TcpSocket {
	private:
		static const char *HTML_BODY_WITH_CONTENT;
		static const char *HTML_BODY_NO_CONTENT;

		static const std::string HTML_PROTOCOL_RTSP_VERSION;
		static const std::string HTML_PROTOCOL_HTTP_VERSION;

	public:

		static const std::string HTML_OK;
		static const std::string HTML_NO_RESPONSE;
		static const std::string HTML_NOT_FOUND;
		static const std::string HTML_MOVED_PERMA;
		static const std::string HTML_SERVICE_UNAVAILABLE;

		static const std::string CONTENT_TYPE_XML;
		static const std::string CONTENT_TYPE_HTML;
		static const std::string CONTENT_TYPE_JS;
		static const std::string CONTENT_TYPE_CSS;
		static const std::string CONTENT_TYPE_PNG;
		static const std::string CONTENT_TYPE_ICO;
		static const std::string CONTENT_TYPE_VIDEO;

		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		HttpcServer(int maxClients, const std::string &protocol, int port, bool nonblock,
		            StreamManager &streamManager,
		            const InterfaceAttr &interface);

		virtual ~HttpcServer();

	protected:

		///
		void getHtmlBodyWithContent(std::string &htmlBody, const std::string &html,
			const std::string &location, const std::string &contentType, std::size_t docTypeSize, std::size_t cseq) const;

		///
		void getHtmlBodyNoContent(std::string &htmlBody, const std::string &html,
			const std::string &location, const std::string &contentType, std::size_t cseq) const;

		/// Method for getting the required files
		virtual bool methodGet(SocketClient &UNUSED(client)) {
			return false;
		}

		/// Method for getting the required files
		virtual bool methodPost(const SocketClient &UNUSED(client)) {
			return false;
		}

		///
		virtual bool methodSetup(Stream &UNUSED(stream), int UNUSED(clientID), std::string &UNUSED(htmlBody)) {
			return false;
		}

		///
		virtual bool methodPlay(Stream &UNUSED(stream), int UNUSED(clientID), std::string &UNUSED(htmlBody)) {
			return false;
		}

		///
		virtual bool methodOptions(Stream &UNUSED(stream), int UNUSED(clientID), std::string &UNUSED(htmlBody)) {
			return false;
		}

		///
		virtual bool methodDescribe(Stream &UNUSED(stream), int UNUSED(clientID), std::string &UNUSED(htmlBody)) {
			return false;
		}

		///
		virtual bool methodTeardown(Stream &UNUSED(stream), int UNUSED(clientID), std::string &UNUSED(htmlBody)) {
			return false;
		}

		/// Process the @c SocketClient when there is data received
		virtual bool process(SocketClient &client);

		///
		void processStreamingRequest(SocketClient &client);

		/// Is called when the connection is closed by the client and should
		/// take appropriate action
		virtual bool closeConnection(SocketClient &client);

	private:

		///
		const std::string &getProtocolVersionString() const;

		// =======================================================================
		// Data members
		// =======================================================================
	protected:
		StreamManager &_streamManager;
		const InterfaceAttr &_interface;

}; // class HttpcServer

#endif // HTTPC_SERVER_H_INCLUDE