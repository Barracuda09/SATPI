/* DVBCA.h

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
#ifndef DVBCA_H_INCLUDE
#define DVBCA_H_INCLUDE DVBCA_H_INCLUDE

#include <FwDecl.h>
#include <base/ThreadBase.h>
#include <base/XMLSupport.h>

#include <cstddef>
#include <string>
#include <ctime>

FW_DECL_NS1(mpegts, PMT);

#define MAX_SESSIONS 15

// Session Status see Table 7 Page 20
#define SESSION_RESOURCE_OPEN          0x00
#define SESSION_RESOURCE_NOT_FOUND     0xF0
#define SESSION_RESOURCE_UNAVAILABLE   0xF1
#define SESSION_RESOURCE_VERSION_LOWER 0xF2
#define SESSION_RESOURCE_RESOURCE_BUSY 0xF3

// Transport Tags Values see Table A.16 Page 70
//                                           host<-->module
#define TPDU_SB                        0x80 // P <----
#define TPDU_RCV                       0x81 // P ---->
#define TPDU_CREATE_T_C                0x82 // P ---->
#define TPDU_CREATE_T_C_REPLY          0x83 // P <----
#define TPDU_DELETE_T_C                0x84 // P <---->
#define TPDU_DELETE_T_C_REPLY          0x85 // P <---->
#define TPDU_REQUEST_T_C               0x86 // P <----
#define TPDU_NEW_T_C                   0x87 // P ---->
#define TPDU_T_C_ERROR                 0x88 // P ---->
#define TPDU_DATA_LAST                 0xA0 // C <---->
#define TPDU_DATA_MORE                 0xA1 // C <---->

// Session Tag Values see Table 14 Page 23
//                                             host<-->module
#define SPDU_SESSION_NUMBER            0x90  //    <--->
#define SPDU_OPEN_SESSION_REQUEST      0x91  //    <---
#define SPDU_OPEN_SESSION_RESPONSE     0x92  //    --->
#define SPDU_CREATE_SESSION            0x93  //    --->
#define SPDU_CREATE_SESSION_RESPONSE   0x94  //    <---
#define SPDU_CLOSE_SESSION_REQUEST     0x95  //    <--->
#define SPDU_CLOSE_SESSION_RESPONSE    0x96  //    <--->

// Application object tag value see Table 58 Page 56
//                                               host<-->app
#define APDU_BEGIN_PRIMITIVE_TAG        0x9F
#define APDU_PROFILE_ENQUIRY            0x009F8010 // <--->
#define APDU_PROFILE                    0x009F8011 // <--->
#define APDU_PROFILE_CHANGE             0x009F8012 // <--->
#define APDU_APPLICATION_INFO_ENQUIRY   0x009F8020 //  --->
#define APDU_APPLICATION_INFO           0x009F8021 // <---
#define APDU_ENTER_MENU                 0x009F8022 //  --->
#define APDU_CA_INFO_ENQUIRY            0x009F8030 //  --->
#define APDU_CA_INFO                    0x009F8031 // <---
#define APDU_CA_PMT                     0x009F8032 //  --->
#define APDU_CA_REPLY                   0x009F8033 // <---
#define APDU_CA_TUNE                    0x009F8400 // <---
#define APDU_CA_REPLACE                 0x009F8401 // <---
#define APDU_CA_CLEAR_REPLACE           0x009F8402 // <---
#define APDU_CA_ASK_RELEASE             0x009F8403 //  --->
#define APDU_DATE_TIME_ENQUIRY          0x009F8440 // <---
#define APDU_DATE_TIME                  0x009F8441 //  --->
#define APDU_MMI_CLOSE_MMI              0x009F8800 //  --->
#define APDU_MMI_DISPLAY_CONTROL        0x009F8801 // <---
#define APDU_MMI_DISPLAY_REPLY          0x009F8802 //  --->
#define APDU_MMI_TEXT_LAST              0x009F8803 // <---
#define APDU_MMI_TEXT_MORE              0x009F8804 // <---
#define APDU_MMI_KEYPAD_CONTROL         0x009F8805 // <---
#define APDU_MMI_KEYPRESS               0x009F8806 //  --->
#define APDU_MMI_ENQUIRY                0x009F8807 // <---
#define APDU_MMI_ANSWER                 0x009F8808 // --->
#define APDU_MMI_MENU_LAST              0x009F8809 // <---
#define APDU_MMI_MENU_MORE              0x009F880A // <---
#define APDU_MMI_MENU_ANSWER            0x009F880B // --->
#define APDU_MMI_LIST_LAST              0x009F880C // <---
#define APDU_MMI_LIST_MORE              0x009F880D // <---
#define APDU_MMI_SUBTITLE_SEGMENT_LAST  0x009F880E // <---
#define APDU_MMI_SUBTITLE_SEGMENT_MORE  0x009F880F // --->
#define APDU_MMI_DISPLAY_MESSAGE        0x009F8810 // <---
#define APDU_MMI_SCENE_END_MARK         0x009F8811 // <---
#define APDU_MMI_SCENE_DONE             0x009F8812 // <---
#define APDU_MMI_SCENE_CONTROL          0x009F8813 // --->
#define APDU_MMI_SUBTITLE_DOWNLOAD_LAST 0x009F8814 // <---
#define APDU_MMI_SUBTITLE_DOWNLOAD_MORE 0x009F8815 // --->
#define APDU_MMI_FLUSH_DOWNLOAD         0x009F8816 // <---
#define APDU_MMI_DOWNLOAD_REPLY         0x009F8817 // <---
/*
T comms_cmd 9F 8C 00 low-speed comms. <---
T connection_descriptor 9F 8C 01 low-speed comms. <---
T comms_reply 9F 8C 02 low-speed comms. --->
T comms_send_last 9F 8C 03 low-speed comms. <---
T comms_send_more 9F 8C 04 low-speed comms. <---
T comms_rcv_last 9F 8C 05 low-speed comms. --->
T comms_rcv_more 9F 8C 06 low-speed comms. --->
*/

// MMI Display Control cmd see Table 34 Page 37
#define MMI_SET_MMI_MODE                             0x01
#define MMI_GET_DISPLAY_CHAR_TABLE_LIST              0x02
#define MMI_GET_INPUT_CHAR_TABLE_LIST                0x03
#define MMI_GET_OVERLAY_GRAPHICS_CHARACTERISTICS     0x04
#define MMI_GET_FULL_SCREEN_GRAPHICS_CHARACTERISTICS 0x05

// MMI Mode see Table 34 Page 37
#define MMI_HIGH_LEVEL                     0x01
#define MMI_LOW_LEVEL_OVERLAY_GRAPHICS     0x02
#define MMI_LOW_LEVEL_FULL_SCREEN_GRAPHICS 0x03

// Recources see Table 57 Page 54
#define RESOURCE_MANAGER               0x00010041
#define APPLICATION_MANAGER            0x00020041
#define CA_MANAGER                     0x00030041
#define AUTHENTICATION                 0x00100041
#define HOST_CONTROLER                 0x00200041
#define DATE_TIME                      0x00240041
#define MMI_MANAGER                    0x00400041

///
class DVBCA :
	public base::XMLSupport,
	public base::ThreadBase {
	public:

		using CAData = std::basic_string<unsigned char>;

		// =======================================================================
		//  -- Constructors and destructor ---------------------------------------
		// =======================================================================
	public:

		DVBCA();

		virtual ~DVBCA();

		// =======================================================================
		// -- base::XMLSupport ---------------------------------------------------
		// =======================================================================
	public:

		virtual void addToXML(std::string &xml) const final;

		virtual void fromXML(const std::string &xml) final;

		// =======================================================================
		//  -- base::ThreadBase --------------------------------------------------
		// =======================================================================
	protected:

		/// Thread function
		virtual void threadEntry();

		// =======================================================================
		//  -- Other member functions --------------------------------------------
		// =======================================================================
	public:

		bool open(const std::size_t id);

		void close();

		bool reset();

		bool sendCAPMT(std::size_t sessionNB, const mpegts::PMT &pmt);

	protected:

		void openSessionRequest(const CAData &spduTag);

		void closeSessionRequest(const CAData &spduTag);

		void sessionNumber(const CAData &spduTag);

		void createSessionFor(std::size_t resource, std::size_t &sessionStatus, std::size_t &sessionNB);

		std::size_t findSessionNumberForRecource(std::size_t resource) const;

		void clearSessionFor(std::size_t sessionNB);

		std::size_t read(CAData &data) const;

		bool sendPDU(const CAData &data);

		bool sendAPDU(std::size_t sessionNB, const CAData &data);

		void sendOpenSessionResponse(std::size_t resource, std::size_t sessionStatus, std::size_t sessionNB);

		void createAndSendAPDUTag(std::size_t sessionNB, std::size_t apduTag);

		void createAndSendAPDUTag(std::size_t sessionNB, std::size_t apduTag, const CAData &apduData);

		CAData addResourceID(std::size_t resource);

		void parseAPDUTag(std::size_t sessionNB, const CAData &apduData);

		void parseMMIMenu(std::size_t sessionNB, const CAData &apduData);

		void parseCAInfo(std::size_t sessionNB, const CAData &apduData);

		void parseApplicationInfo(std::size_t sessionNB, const CAData &apduData);

		// ================================================================
		//  -- Data members -----------------------------------------------
		// ================================================================
	private:

		using Session_t = struct Session {
			std::size_t _resource;
			std::size_t _sessionNB;
			unsigned char _sessionStatus;
		};

		using  Handle = int;
		Handle _fd;
		Handle _fdFifo;
		std::size_t _id;
		std::size_t _timeoutCnt;
		std::time_t _repeatTime;
		std::size_t _timeDateInterval;
		bool _connected;
		bool _waiting;
		Session_t _sessions[MAX_SESSIONS];

		CAData apduTimeData;
};

#endif // DVBCA_H_INCLUDE
