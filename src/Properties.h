/* Properties.h

   Copyright (C) 2015 Marc Postema (m.a.postema -at- alice.nl)

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
#define PROPERTIES_H_INCLUDE

#include <string>

/// The class @c Properties carries all the available/open Properties
class Properties  {
	public:
		// =======================================================================
		// Constructors and destructor
		// =======================================================================
		Properties(const std::string &uuid, const std::string &delsysString, const std::string &startPath);
		virtual ~Properties();

		///
		const std::string &getSoftwareVersion() const { return _versionString; }

		///
		const std::string &getUUID() const { return _uuid; }

		///
		const std::string &getDeliverySystemString() const { return _delsysString; }

		///
		const std::string &getStartPath() const { return _startPath; }

		/// Get and Set BootID
		void         setBootID(unsigned int bootID) { _bootID = bootID; }
		unsigned int getBootID() const              { return _bootID; }

		/// Get and Set DeviceID
		void         setDeviceID(unsigned int deviceID) { _deviceID = deviceID; }
		unsigned int getDeviceID() const                { return _deviceID; }

		/// Get and Set SSDP Announce Time
		void         setSsdpAnnounceTimeSec(unsigned int sec) { _ssdpAnnounceTimeSec = sec; }
		unsigned int getSsdpAnnounceTimeSec() const           { return _ssdpAnnounceTimeSec; }
		
		/// Get application start time
		time_t       getApplicationStartTime() const { return _appStartTime; }
	protected:

	private:
		// =======================================================================
		// Data members
		// =======================================================================
		std::string   _delsysString;
		std::string   _uuid;
		std::string   _versionString;
		std::string   _startPath;
		unsigned int  _bootID;
		unsigned int  _deviceID;
		unsigned int  _ssdpAnnounceTimeSec;
		time_t        _appStartTime;         // the application start time (EPOCH)
}; // class Properties

#endif // PROPERTIES_H_INCLUDE