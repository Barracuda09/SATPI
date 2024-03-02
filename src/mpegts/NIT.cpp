/* NIT.cpp

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
#include <mpegts/NIT.h>
#include <Log.h>

namespace mpegts {

namespace {

	std::string polTostring(int fec) {
		switch (fec) {
			case 0:
				return "h";
			case 1:
				return "v";
			case 3:
				return "l";
			case 4:
				return "r";
			default:
				return "unknown";
		};
	}

	std::string fecInnerTostring(int fec) {
		switch (fec) {
			case 0:
				return "Not def";
			case 1:
				return "12";
			case 2:
				return "23";
			case 3:
				return "34";
			case 4:
				return "56";
			case 5:
				return "78";
			case 6:
				return "89";
			case 7:
				return "35";
			case 8:
				return "45";
			case 9:
				return "910";
			case 15:
				return "none";
			default:
				return StringConverter::stringFormat("FEC Inner res. @#1", DIGIT(fec, 2));
		}
	}

	std::string fecOuterTostring(int fec) {
		switch (fec) {
			case 0:
				return "not defined";
			case 1:
				return "none";
			case 2:
				return "RS(204/188)";
			default:
				return StringConverter::stringFormat("FEC Outer res. @#1", DIGIT(fec, 2));
		}
	}

	std::string msysTostring(int msys) {
		switch (msys) {
			case 0:
				return "DVB-S";
			case 1:
				return "DVB-S2";
			default:
				return "Unknown msys";
		};
	}

	std::string mtypeSatTostring(int mtype) {
		switch (mtype) {
			case 0:
				return "auto";
			case 1:
				return "qpsk";
			case 2:
				return "8psk";
			case 3:
				return "16qam";
			default:
				return "Unknown mtype";
		};
	}

	std::string mtypeCableTostring(int mtype) {
		switch (mtype) {
			case 0:
				return "auto";
			case 1:
				return "16qam";
			case 2:
				return "32qam";
			case 3:
				return "64qam";
			case 4:
				return "128qam";
			case 5:
				return "256qam";
			default:
				return "Unknown mtype";
		};
	}

	std::string rolloffTostring(int rolloff) {
		switch (rolloff) {
			case 0:
				return "0.35";
			case 1:
				return "0.25";
			case 2:
				return "0.20";
			case 3:
				return "reserved";
			default:
				return "Unknown Rolloff";
		};
	}
}

// =============================================================================
//  -- mpegts::TableData -------------------------------------------------------
// =============================================================================

void NIT::clear() noexcept {
	TableData::clear();
	_table.clear();
	_nid = 0;
}

// =============================================================================
//  -- base::XMLSupport --------------------------------------------------------
// =============================================================================

void NIT::doAddToXML(std::string &xml) const {
	ADD_XML_ELEMENT(xml, "networkID", DIGIT(_nid, 4));
	ADD_XML_ELEMENT(xml, "networkName", _networkName);
	for (const auto& entry : _table) {
		std::string name = "transportStreamID_" + DIGIT(entry.transportStreamID, 0);
		ADD_XML_BEGIN_ELEMENT(xml, name);
			ADD_XML_ELEMENT(xml, "msys", entry.msys);
			ADD_XML_ELEMENT(xml, "freq", entry.freq);
			ADD_XML_ELEMENT(xml, "srate", entry.srate);
			ADD_XML_ELEMENT(xml, "mtype", entry.mtype);
			ADD_XML_ELEMENT(xml, "fec", entry.fec);
			ADD_XML_ELEMENT(xml, "fecOut", entry.fecOut);
		ADD_XML_END_ELEMENT(xml, name);
	}
}

void NIT::doFromXML(const std::string &UNUSED(xml)) {}

// =============================================================================
//  -- Other member functions --------------------------------------------------
// =============================================================================

void NIT::parse(const FeID id) {
	for (std::size_t secNr = 0; secNr < _numberOfSections; ++secNr) {
		TableData::Data tableData;
		if (getDataForSectionNumber(secNr, tableData)) {
			const unsigned char* data = tableData.data.data();
			size_t index = 8;
			_nid =  getWord(index, data);

//			SI_LOG_BIN_DEBUG(data, tableData.data.size(), "Frontend: @#1, NIT data", id);

			SI_LOG_INFO("Frontend: @#1, NIT - Section Length: @#2  NID: @#3  Version: @#4  secNr: @#5 lastSecNr: @#6  CRC: @#7",
				id, DIGIT(tableData.sectionLength, 4), DIGIT(_nid, 4), tableData.version, tableData.secNr, tableData.lastSecNr, HEX(tableData.crc, 4));
			
			// Network Descriptors
			index = 13;
			const size_t netDescLenEnd = (getWord(index, data) & 0xFFF) + index;
			while (index < netDescLenEnd) {
				const uint8_t tag = getByte(index, data);
				const size_t descLenEnd = getByte(index, data) + index;
				switch (tag) {
					case 0x40: { // networkNameDescriptor
						while (index < descLenEnd) {
							if (const char c = static_cast<char>(getByte(index, data)); c > 0) {
								_networkName += c;
							}
						}
						SB_LOG_INFO(MPEGTS_TABLES, "Frontend: @#1, NIT - Network Name Descriptor: @#2", id, _networkName);
						break;
					}
					case 0x4A: // linkageDescriptor
					case 0x5F: // privateDataSpecifierDescriptor
					default:
						index = descLenEnd;
						break;
				};
			}
			// Transport Stream Descriptors
			unsigned int streamDescLenEnd = (getWord(index, data) & 0xFFF) + index;
			while (index < streamDescLenEnd) {
				Data entry;
				entry.transportStreamID = getWord(index, data);
				entry.originalNetworkID = getWord(index, data);
				const size_t transportDescLenEnd  = (getWord(index, data) & 0xFFF) + index;
				while (index < transportDescLenEnd) {
					const uint8_t tag = getByte(index, data);
					const size_t descLenEnd = getByte(index, data) + index;
					switch (tag) {
						case 0x43: { // satelliteDeliverySystemDescriptor
							const uint32_t freq = getDWord(index, data);
							const uint16_t orbit = getWord(index, data);
							const uint8_t d1 = getByte(index, data);
							const uint32_t d2 = getDWord(index, data);
							const int westEastFlag = (d1 >> 7);
							const int pol = (d1 >> 5) & 0x3;
							const int msys = ((d1 >> 2) & 0x1);
							const int mtype = (d1 & 0x3);
							const int rolloff = ((d1 >> 3) & 0x3);
							const int sb = d2 >> 4;
							const int fec = d2 & 0x3;
							entry.msys = "dvbs2";
							SB_LOG_INFO(MPEGTS_TABLES, "Frontend: @#1, NIT - TID: @#2  ID: @#3  Orbit: @#4  WEFlag: @#5  Freq: @#6  SymbolRate: @#7  msys: @#8  mtype: @#9  fec: @#10  pol: @#11  ro: @#12", id,
								entry.transportStreamID,
								HEX(entry.originalNetworkID, 4),
								StringConverter::toStringFrom4BitBCD(orbit, 4),
								westEastFlag,
								StringConverter::toStringFrom4BitBCD(freq, 8),
								StringConverter::toStringFrom4BitBCD(sb, 7),
								msysTostring(msys),
								mtypeSatTostring(mtype),
								fecInnerTostring(fec),
								polTostring(pol),
								rolloffTostring(rolloff));
							break;
						}
						case 0x44: { // cableDeliverySystemDescriptor
							const uint32_t freq = getDWord(index, data);
							const uint32_t d1 = get24Bits(index, data);
							const uint32_t d2 = getDWord(index, data);
							const int fecOuter = ((d1 >> 8) & 0xF);
							const int mtype = (d1 & 0xFF);
							const int sb = d2 >> 4;
							const int fecInner = d2 & 0xF;
							entry.msys = "dvbc";
							entry.freq = StringConverter::toStringFrom4BitBCD(freq, 8);
							entry.srate = StringConverter::toStringFrom4BitBCD(sb, 7);
							entry.mtype = mtypeCableTostring(mtype);
							entry.fec = fecInnerTostring(fecInner);
							entry.fecOut = fecOuterTostring(fecOuter);
							_table.emplace_back(entry);
							SB_LOG_INFO(MPEGTS_TABLES, "Frontend: @#1, NIT - TID: @#2  ID: @#3  Freq: @#4  SymbolRate: @#5  mtype: @#6  fec-inner: @#7  fec-outer: @#8", id,
								entry.transportStreamID,
								HEX(entry.originalNetworkID, 4),
								StringConverter::toStringFrom4BitBCD(freq, 8),
								StringConverter::toStringFrom4BitBCD(sb, 7),
								mtypeCableTostring(mtype),
								fecInnerTostring(fecInner),
								fecOuterTostring(fecOuter));
							break;
						}
						case 0x41: // serviceListDescriptor
						default:
							index = descLenEnd;
							break;
					};
				}
			}
		}
	}
}

mpegts::TSData NIT::generateFrom(
		FeID id, const base::M3UParser::TransformationMap &info) {
	mpegts::TSData data;

	int prognr = 0;
	for (const auto& [freq, element] : info) {
		SI_LOG_DEBUG("Frontend: @#1, Generating NIT: Prog NR: @#2 - @#3  PMT PID: @#4",
			id, HEX(prognr, 4), DIGIT(prognr, 5), element.freq);
	}
	return data;
}

}
