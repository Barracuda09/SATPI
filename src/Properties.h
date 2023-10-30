/* Properties.h

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
#ifndef PROPERTIES_H_INCLUDE
#define PROPERTIES_H_INCLUDE PROPERTIES_H_INCLUDE

#include <base/Mutex.h>
#include <base/XMLSupport.h>

#include <string>
#include <ctime>

/// The class @c Properties carries all the available/open Properties
class Properties :
	public base::XMLSupport  {
		// =====================================================================
		// -- Static const data ------------------------------------------------
		// =====================================================================
	public:

		static constexpr unsigned int TCP_PORT_MAX = 65535;
		static constexpr unsigned int HTTP_PORT_MIN = 1024;
		static constexpr unsigned int RTSP_PORT_MIN = 554;

		// =====================================================================
		// -- Constructors and destructor --------------------------------------
		// =====================================================================
	public:

		Properties(
			const std::string &uuid,
			const std::string &currentPathOpt,
			const std::string &appdataPathOpt,
			const std::string &webPathOpt,
			const std::string &ipAddress,
			unsigned int httpPortOpt,
			unsigned int rtspPortOpt);

		virtual ~Properties() = default;

		// =====================================================================
		// -- base::XMLSupport -------------------------------------------------
		// =====================================================================
	private:

		/// @see XMLSupport
		virtual void doAddToXML(std::string &xml) const final;

		/// @see XMLSupport
		virtual void doFromXML(const std::string &xml) final;

		// =====================================================================
		// -- Other member functions -------------------------------------------
		// =====================================================================
	public:

		///
		std::string getSoftwareVersion() const;

		///
		std::string getUPnPVersion() const;

		///
		std::string getUUID() const;

		///
		std::string getAppDataPath() const;

		///
		std::string getWebPath() const;

		///
		std::string getXSatipM3U() const;

		///
		std::string getXMLDeviceDescriptionFile() const;

		/// Set HttpPort
		void setHttpPort(unsigned int httpPort);

		/// Get HttpPort
		unsigned int getHttpPort() const;

		/// Set RtspPort
		void setRtspPort(unsigned int rtspPort);

		/// Get RtspPort
		unsigned int getRtspPort() const;

		/// Get IP Adress
		std::string getIpAddress() const;

		/// Get application start time
		std::time_t getApplicationStartTime() const;

		/// Check do we need to exit the application
		bool exitApplication() const;

		/// Sets the exit application flag
		void setExitApplication();

	protected:

		// =====================================================================
		// -- Data members -----------------------------------------------------
		// =====================================================================
	private:

		base::Mutex _mutex;
		std::string _uuid;
		std::string _versionString;
		std::string _xSatipM3U;
		std::string _xmlDeviceDescriptionFile;
		std::string _webPath;
		std::string _appdataPath;
		std::string _ipAddress;
		unsigned int _httpPort;
		unsigned int _rtspPort;
		std::string _webPathOpt;
		std::string _appdataPathOpt;
		unsigned int _httpPortOpt;
		unsigned int _rtspPortOpt;
		std::time_t _appStartTime;     // the application start time (EPOCH)
		bool _exitApplication;
};

#endif // PROPERTIES_H_INCLUDE
