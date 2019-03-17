/* satpi.c

   Copyright (C) 2014 - 2019 Marc Postema (mpostema09 -at- gmail.com)

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
#include <FwDecl.h>
#include <RtspServer.h>
#include <HttpServer.h>
#include <upnp/ssdp/Server.h>
#include <StreamManager.h>
#include <InterfaceAttr.h>
#include <Properties.h>
#include <Log.h>
#include <Utils.h>
#include <StringConverter.h>
#include <base/XMLSaveSupport.h>
#ifdef ADDDVBCA
#include <decrypt/dvbca/DVBCA.h>
#endif

#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DAEMON_NAME "SatPI"
#define LOCK_FILE   "SatPI.lock"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

static std::atomic_bool exitApp;
std::atomic_bool restartApp;
static int retval;
static int otherSig;

class SatPI :
	public base::XMLSaveSupport,
	public base::XMLSupport {
	public:
		// =======================================================================
		//  -- Constructors and destructor ---------------------------------------
		// =======================================================================
		SatPI(bool ssdp,
			const std::string &ifaceName,
			const std::string &appdataPath,
			const std::string &webPath,
			const std::string &dvbPath,
			unsigned int httpPort,
			unsigned int rtspPort) :
			XMLSaveSupport(appdataPath + "/" + "SatPI.xml"),
			_interface(ifaceName),
			_streamManager(),
			_properties(_interface.getUUID(), appdataPath, webPath, httpPort, rtspPort),
			_httpServer(*this, _streamManager, _interface.getIPAddress(), _properties),
			_rtspServer(_streamManager, _interface.getIPAddress()),
			_ssdpServer(_interface.getIPAddress(), _properties) {
			_properties.setFunctionNotifyChanges(std::bind(&XMLSaveSupport::notifyChanges, this));
			_ssdpServer.setFunctionNotifyChanges(std::bind(&XMLSaveSupport::notifyChanges, this));
			//
			_streamManager.enumerateDevices(_interface.getIPAddress(), appdataPath, dvbPath);
			//
			std::string xml;
			if (restoreXML(xml)) {
				fromXML(xml);
			} else {
				saveXML();
			}

			_httpServer.initialize(_properties.getHttpPort(), true);
			_rtspServer.initialize(_properties.getRtspPort(), true);
			if (ssdp) {
				_ssdpServer.startThread();
			}
		}

		virtual ~SatPI() {}

		// =======================================================================
		// -- Other member functions ---------------------------------------------
		// =======================================================================

	public:

		bool exitApplication() const {
			return _properties.exitApplication();
		}

		// =======================================================================
		// -- base::XMLSupport ---------------------------------------------------
		// =======================================================================

	public:

		virtual void addToXML(std::string &xml) const override {
			xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
			ADD_XML_BEGIN_ELEMENT(xml, "data");
				{
					// application data
					ADD_XML_BEGIN_ELEMENT(xml, "appdata");
					ADD_XML_ELEMENT(xml, "uptime", std::time(nullptr) - _properties.getApplicationStartTime());
					ADD_XML_ELEMENT(xml, "appversion", _properties.getSoftwareVersion());
					ADD_XML_ELEMENT(xml, "uuid", _properties.getUUID());
					ADD_XML_END_ELEMENT(xml, "appdata");
				}
				ADD_XML_ELEMENT(xml, "streams", _streamManager.toXML());
				ADD_XML_ELEMENT(xml, "configdata", _properties.toXML());
				ADD_XML_ELEMENT(xml, "ssdp", _ssdpServer.toXML());

			ADD_XML_END_ELEMENT(xml, "data");
		}

		virtual void fromXML(const std::string &xml) override {
			std::string element;
			if (findXMLElement(xml, "streams", element)) {
				_streamManager.fromXML(element);
			}
			if (findXMLElement(xml, "configdata", element)) {
				_properties.fromXML(element);
			}
			if (findXMLElement(xml, "ssdp", element)) {
				_ssdpServer.fromXML(element);
			}
			saveXML();
		}

		// =======================================================================
		// -- base::XMLSaveSupport -----------------------------------------------
		// =======================================================================

		public:

            virtual bool saveXML() const {
                std::string xml;
                addToXML(xml);
                return XMLSaveSupport::saveXML(xml);
            }

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

/*
 *
 */
static void child_handler(int signum) {
	switch(signum) {
		case SIGCHLD:
		// fall-through
		case SIGINT:
		// fall-through
		case SIGALRM:
			retval = EXIT_FAILURE;
			exitApp = true;
			break;
		case SIGKILL:
			SI_LOG_INFO("stopping (KILL)" );
		// fall-through
		case SIGUSR1:
		// fall-through
		case SIGHUP:
		// fall-through
		case SIGTERM:
			retval = EXIT_SUCCESS;
			exitApp = true;
			break;
		default:
			otherSig = 1;
			break;
	}
}

/*
 *
 */
static void daemonize(const std::string &lockfile, const char *user) {
	pid_t pid, sid;
	int lfp = -1;
	char str[10];

	// already a daemon
	if ( getppid() == 1 ) {
		return;
	}

	// create the lock file as the current user
	// give group 'rw' permission and other users 'r' permissions
	if (!lockfile.empty()) {
		lfp = open(lockfile.c_str(), O_RDWR | O_CREAT, 0664);
		if (lfp < 0) {
			printf("unable to create lock file %s, %s (code=%d)\r\n", lockfile.c_str(), strerror(errno), errno);
			exit(EXIT_FAILURE);
		}
	}
	// drop current user, and try to run as 'user'
	if (user) {
		printf("try running as user: %s\r\n", user);
		struct passwd *pw = getpwnam(user);
		if (pw) {
			if (setuid(pw->pw_uid) != 0) {
				printf("unable to set uid: %d (%s)\r\n", pw->pw_uid, strerror(errno));
				CLOSE_FD(lfp);
				exit(EXIT_FAILURE);
			}
		} else {
			printf("unable to find user: %s\r\n", user);
			CLOSE_FD(lfp);
			exit(EXIT_FAILURE);
		}
	}

	// trap signals that we expect to receive
	signal(SIGCHLD, child_handler);
	signal(SIGUSR1, child_handler);
	signal(SIGALRM, child_handler);
	signal(SIGTERM, child_handler);
	signal(SIGHUP,  child_handler);
	signal(SIGKILL, child_handler);
	signal(SIGINT,  child_handler);

	// fork off the parent process
	pid = fork();
	if (pid < 0) {
		printf("unable to fork daemon, %s (code=%d)\r\n", strerror(errno), errno);
		CLOSE_FD(lfp);
		exit(EXIT_FAILURE);
	}

	// if we got a good PID, then we can exit the parent process
	if (pid > 0) {
		// Wait for confirmation from the child via SIGUSR1, or
		// for two seconds to elapse (SIGALRM)
		CLOSE_FD(lfp);
		alarm(2);
		pause();
		exit(EXIT_FAILURE);
	}

	// Change the file mode mask
	umask(0);

	// at this point we are executing as the child process
	struct passwd *pw = getpwuid(geteuid());
	printf("running as user: %s with pid %d (%s)\r\n", pw->pw_name, getpid(), lockfile.c_str());

	// now record our pid to lockfile
	sprintf(str, "%d\n", getpid());
	if (write(lfp, str, strlen(str)) == -1) {
		printf("pid file write, %s (code=%d)\r\n", strerror(errno), errno);
	}
	CLOSE_FD(lfp);

	// cancel certain signals
	signal(SIGCHLD, SIG_DFL); // A child process dies
	signal(SIGTSTP, SIG_IGN); // Various TTY signals
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);

	// create a new SID for the child process
	sid = setsid();
	if (sid < 0) {
		printf("unable to create a new session, %s (code %d)", strerror(errno), errno);
		exit(EXIT_FAILURE);
	}

	// redirect standard files to /dev/null
	if (!freopen("/dev/null", "r", stdin) ||
	    !freopen("/dev/null", "w", stdout) ||
	    !freopen("/dev/null", "w", stderr)) {
		printf("freopen failed, %s (code %d)", strerror(errno), errno);
		exit(EXIT_FAILURE);
	}

	// tell the parent process that we are running
	kill(getppid(), SIGUSR1);

}

/*
 *
 */
static void printUsage(const char *prog_name) {
	printf("Usage %s [OPTION]\r\n\r\nOptions:\r\n" \
	       "\t--help           show this help and exit\r\n" \
	       "\t--version        show the version number\r\n" \
	       "\t--user xx        run as user\r\n" \
	       "\t--dvb-path       set path were to find dvb devices default /dev/dvb\r\n" \
	       "\t--app-data-path  set path for application state data eg. xml files etc\r\n" \
	       "\t--iface-name     set the network interface to bind to (eg. eth0)\r\n" \
	       "\t--http-path      set root path of web/http pages\r\n" \
	       "\t--http-port      set http port default 8875 (1024 - 65535)\r\n" \
	       "\t--rtsp-port      set rtsp port default 554  ( 554 - 65535)\r\n" \
	       "\t--no-daemon      do NOT daemonize\r\n" \
	       "\t--no-ssdp        do NOT advertise server\r\n", prog_name);
}

/*
 *
 */
int main(int argc, char *argv[]) {
	bool ssdp = true;
	bool daemon = true;
	int i;
	char *user = nullptr;
	extern const char *satpi_version;
	exitApp = false;
	restartApp = false;

	// Defaults in Properties
	unsigned int httpPort = 0;
	unsigned int rtspPort = 0;

	std::string ifaceName;
	std::string currentPath;
	std::string appdataPath;
	std::string webPath;
	std::string dvbPath;
	std::string file;
	//
	StringConverter::splitPath(argv[0], currentPath, file);
	appdataPath = currentPath;
	webPath = currentPath + "/" + "web";
	dvbPath = "/dev/dvb";

	// Check options
	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--no-ssdp") == 0) {
			ssdp = false;
		} else if (strcmp(argv[i], "--user") == 0) {
			if (i + 1 < argc) {
				++i;
				user = argv[i];
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--no-daemon") == 0) {
			daemon = false;
		} else if (strcmp(argv[i], "--dvb-path") == 0) {
			if (i + 1 < argc) {
				++i;
				dvbPath = argv[i];
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--app-data-path") == 0) {
			if (i + 1 < argc) {
				++i;
				appdataPath = argv[i];
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--iface-name") == 0) {
			if (i + 1 < argc) {
				++i;
				ifaceName = argv[i];
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--http-path") == 0) {
			if (i + 1 < argc) {
				++i;
				webPath = argv[i];
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--http-port") == 0) {
			if (i + 1 < argc) {
				++i;
				httpPort = atoi(argv[i]);
				if (httpPort < 1024 || httpPort > 65535) {
					printUsage(argv[0]);
					return EXIT_FAILURE;
				}
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--rtsp-port") == 0) {
			if (i + 1 < argc) {
				++i;
				rtspPort = atoi(argv[i]);
				if (rtspPort <  554 || rtspPort > 65535) {
					printUsage(argv[0]);
					return EXIT_FAILURE;
				}
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--version") == 0) {
			std::cout << "SatPI version: " << satpi_version << "\r\n";
			return EXIT_SUCCESS;
		} else if (strcmp(argv[i], "--help") == 0) {
			printUsage(argv[0]);
			return EXIT_SUCCESS;
		} else {
			printUsage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	// initialize the logging interface
	openlog(DAEMON_NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	// open our logging
	Log::open_app_log();

	// Daemonize
	if (daemon) {
		daemonize("/var/lock/" LOCK_FILE, user);
	}

	signal(SIGPIPE, SIG_IGN);

	// notify we are alive
	SI_LOG_INFO("--- Starting SatPI version: %s ---", satpi_version);
	SI_LOG_INFO("Number of processors online: %d", base::ThreadBase::getNumberOfProcessorsOnline());
	SI_LOG_INFO("Default network buffer size: %d KBytes", InterfaceAttr::getNetworkUDPBufferSize() / 1024);
	do {
		try {
#ifdef ADDDVBCA
			DVBCA dvbca;
			dvbca.startThread();
#endif
			SatPI satpi(ssdp, ifaceName, appdataPath, webPath, dvbPath, httpPort, rtspPort);

			// Loop
			while (!exitApp && !satpi.exitApplication() && !restartApp) {
				std::this_thread::sleep_for(std::chrono::milliseconds(120));
			}
		} catch (...) {
			SI_LOG_ERROR("Caught an Exception");
		}
		if (restartApp) {
			SI_LOG_INFO("--- Restarting SatPI version: %s ---", satpi_version);
		}
	} while (restartApp);
	SI_LOG_INFO("--- stopped ---");

	// close logging interface
	closelog();

	Log::close_app_log();

	return EXIT_SUCCESS;
}
