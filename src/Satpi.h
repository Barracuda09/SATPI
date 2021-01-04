/* Satpi.h

   Copyright (C) 2014 - 2021 Marc Postema (mpostema09 -at- gmail.com)

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
#ifndef SATPI_H_INCLUDE
#define SATPI_H_INCLUDE SATPI_H_INCLUDE

#include <base/XMLSaveSupport.h>
#include <base/XMLSupport.h>
#include <RtspServer.h>
#include <HttpServer.h>
#include <upnp/ssdp/Server.h>
#include <StreamManager.h>
#include <InterfaceAttr.h>
#include <Properties.h>

#include <string>

class SatPI :
	public base::XMLSaveSupport,
	public base::XMLSupport {
		// =====================================================================
		//  -- Constructors and destructor -------------------------------------
		// =====================================================================
	public:

		SatPI(bool ssdp,
			const std::string &ifaceName,
			const std::string &currentPath,
			const std::string &appdataPath,
			const std::string &webPath,
			const std::string &dvbPath,
			unsigned int httpPort,
			unsigned int rtspPort,
			const bool enableChildPIPE);

		virtual ~SatPI();

		// =====================================================================
		// -- base::XMLSupport -------------------------------------------------
		// =====================================================================
	public:

		virtual void doAddToXML(std::string &xml) const final;

		virtual void doFromXML(const std::string &xml) final;

		// =======================================================================
		// -- base::XMLSaveSupport -----------------------------------------------
		// =======================================================================
	public:

		virtual bool saveXML() const;

		// =====================================================================
		// -- Other member functions -------------------------------------------
		// =====================================================================
	public:

		bool exitApplication() const;

		// =======================================================================
		// -- Data members -------------------------------------------------------
		// =======================================================================
	private:

		InterfaceAttr _interface;
		StreamManager _streamManager;
		Properties _properties;
		HttpServer _httpServer;
		RtspServer _rtspServer;
		upnp::ssdp::Server _ssdpServer;
};

#endif // SATPI_H_INCLUDE
