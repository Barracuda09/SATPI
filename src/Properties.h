/* Properties.h

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
#ifndef PROPERTIES_H_INCLUDE
#define PROPERTIES_H_INCLUDE PROPERTIES_H_INCLUDE

#include "Mutex.h"
#include "XMLSupport.h"

#include <string>

/// The class @c Properties carries all the available/open Properties
class Properties : public XMLSupport  {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Properties(const std::string &uuid, const std::string &delsysString, const std::string &startPath);

		virtual ~Properties();

		/// Add data to an XML for storing or web interface
		virtual void addToXML(std::string &xml) const;

		/// Get data from an XML for restoring or web interface
		virtual void fromXML(const std::string &xml);

		///
		std::string getSoftwareVersion() const;

		///
		std::string getUUID() const;

		///
		std::string getDeliverySystemString() const;

		///
		std::string getStartPath() const;

		///
		std::string getXSatipM3U() const;

		///
		std::string getXMLDeviceDescriptionFile() const;

		/// Set BootID
		void setBootID(unsigned int bootID);

		/// Get BootID
		unsigned int getBootID() const;

		/// Set DeviceID
		void setDeviceID(unsigned int deviceID);

		/// Get DeviceID
		unsigned int getDeviceID() const;

		/// Set SSDP Announce Time
		void setSsdpAnnounceTimeSec(unsigned int sec);

		/// Get SSDP Announce Time
		unsigned int getSsdpAnnounceTimeSec() const;

		/// Get application start time
		time_t getApplicationStartTime() const;

	protected:

	private:
		// =======================================================================
		// Data members
		// =======================================================================
		Mutex         _mutex;
		std::string   _delsysString;
		std::string   _uuid;
		std::string   _versionString;
		std::string   _startPath;
		std::string   _xSatipM3U;
		std::string   _xmlDeviceDescriptionFile;
		unsigned int  _bootID;
		unsigned int  _deviceID;
		unsigned int  _ssdpAnnounceTimeSec;
		time_t        _appStartTime;         // the application start time (EPOCH)
}; // class Properties

#endif // PROPERTIES_H_INCLUDE