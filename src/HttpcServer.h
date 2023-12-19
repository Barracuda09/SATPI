/* HttpcServer.h

   Copyright (C) 2014 - 2023 Marc Postema (mpostema09 -at- gmail.com)

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

#include <Defs.h>
#include <FwDecl.h>
#include <socket/TcpSocket.h>
#include <Unused.h>

FW_DECL_NS0(Stream);
FW_DECL_NS0(StreamManager);

FW_DECL_SP_NS1(output, StreamClient);

/// HTTP Client Server
class HttpcServer :
	public TcpSocket {
	private:
		static const char* HTML_BODY_WITH_CONTENT;
		static const char* HTML_BODY_NO_CONTENT;

		static const std::string HTML_PROTOCOL_RTSP_VERSION;
		static const std::string HTML_PROTOCOL_HTTP_VERSION;

	public:

		static const std::string HTML_OK;
		static const std::string HTML_NO_RESPONSE;
		static const std::string HTML_NOT_FOUND;
		static const std::string HTML_MOVED_PERMA;
		static const std::string HTML_REQUEST_TIMEOUT;
		static const std::string HTML_SERVICE_UNAVAILABLE;

		static const std::string CONTENT_TYPE_CSS;
		static const std::string CONTENT_TYPE_HTML;
		static const std::string CONTENT_TYPE_ICO;
		static const std::string CONTENT_TYPE_JS;
		static const std::string CONTENT_TYPE_JSON;
		static const std::string CONTENT_TYPE_PNG;
		static const std::string CONTENT_TYPE_XML;
		static const std::string CONTENT_TYPE_TEXT;
		static const std::string CONTENT_TYPE_VIDEO;

		// =======================================================================
		// Constructors and destructor
		// =======================================================================
	public:

		HttpcServer(int maxClients, const std::string &protocol,
			StreamManager &streamManager,
			const std::string &bindIPAddress);

		virtual ~HttpcServer() = default;

		/// Call this to initialize, setup and start this server
		virtual void initialize(
			int port,
			bool nonblock);

	protected:

		///
		void getHtmlBodyWithContent(std::string &htmlBody, const std::string &html,
			const std::string &location, const std::string &contentType,
			std::size_t docTypeSize, std::size_t cseq, unsigned int rtspPort = 0) const;

		///
		void getHtmlBodyNoContent(std::string &htmlBody, const std::string &html,
			const std::string &location, const std::string &contentType, std::size_t cseq) const;

		/// HTTP Method for getting the required files or stream
		virtual bool methodGet(SocketClient &UNUSED(client), bool UNUSED(headOnly)) {
			return false;
		}

		/// HTTP Method for received 'posted' data
		virtual bool methodPost(SocketClient &UNUSED(client)) {
			return false;
		}

		/// RTSP Method
		virtual void methodDescribe(const std::string &UNUSED(sessionID), int UNUSED(cseq), FeIndex UNUSED(index), std::string &UNUSED(htmlBody)) {}

		/// Process the data received from @c SocketClient
		virtual bool process(SocketClient &client) final;

		/// Process the the Method HTTP/RTSP
		void processStreamingRequest(SocketClient &client);

	private:

		///
		const std::string &getProtocolVersionString() const;

		// =======================================================================
		// Data members
		// =======================================================================
	protected:

		StreamManager &_streamManager;
		std::string _bindIPAddress;

};

#endif // HTTPC_SERVER_H_INCLUDE

