/* satpi.c

   Copyright (C) 2014 Marc Postema (mpostema09 -at- gmail.com)

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
#include "RtspServer.h"
#include "HttpServer.h"
#include <upnp/ssdp/Server.h>
#include "Streams.h"
#include "InterfaceAttr.h"
#include "Properties.h"
#include "Log.h"
#include "StringConverter.h"
#ifdef LIBDVBCSA
	#include <decrypt/dvbapi/Client.h>
#endif

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

FW_DECL_NS0(StreamInterfaceDecrypt);


bool exitApp;
static int retval;
static int otherSig;

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
static void daemonize(const char *lockfile, const char *user) {
	pid_t pid, sid;
	int lfp = -1;
	char str[10];

	// already a daemon
	if ( getppid() == 1 ) {
		return;
	}

	// create the lock file as the current user
	// give group 'rw' permission and other users 'r' permissions
	if (lockfile && lockfile[0]) {
		lfp = open(lockfile, O_RDWR | O_CREAT, 0664);
		if (lfp < 0) {
			printf("unable to create lock file %s, %s (code=%d)\r\n", lockfile, strerror(errno), errno);
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
	printf("running as user: %s with pid %d (%s)\r\n", pw->pw_name, getpid(), lockfile);

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
	       "\t--app-data-path  set path for application state data, like xml etc\r\n" \
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

	unsigned int httpPort = 8875;
	unsigned int rtspPort = 554;
	std::string currentPath;
	std::string appdataPath;
	std::string webPath;
	std::string file;
	StringConverter::splitPath(argv[0], currentPath, file);
	appdataPath = currentPath;
	webPath = currentPath + "/" + "web";

	// Check options
	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--no-ssdp") == 0) {
			ssdp = false;
		} else if (strcmp(argv[i], "--user") == 0) {
			user = argv[i+1];
			++i; // because next was the user-name
		} else if (strcmp(argv[i], "--no-daemon") == 0) {
			daemon = false;
		} else if (strcmp(argv[i], "--http-path") == 0) {
			webPath = argv[i+1];
			++i;
		} else if (strcmp(argv[i], "--app-data-path") == 0) {
			appdataPath = argv[i+1];
			++i;
		} else if (strcmp(argv[i], "--http-port") == 0) {
			httpPort = atoi(argv[i+1]);
			++i;
			if (httpPort < 1024 || httpPort > 65535) {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--rtsp-port") == 0) {
			rtspPort = atoi(argv[i+1]);
			++i;
			if (rtspPort <  554 || rtspPort > 65535) {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--version") == 0) {
			printf("SatPI version: %s\r\n", satpi_version);
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
	open_app_log();

	// Daemonize
	if (daemon) {
		daemonize("/var/lock/" LOCK_FILE, user);
	}

	// notify we are alive
	SI_LOG_INFO("--- starting SatPI version: %s ---", satpi_version);
	SI_LOG_INFO("Number of processors online: %d", base::ThreadBase::getNumberOfProcessorsOnline());
	SI_LOG_INFO("Default network buffer size: %d KBytes", InterfaceAttr::getNetworkUDPBufferSize() / 1024);

	{
		InterfaceAttr interface;
		Streams streams;
#ifdef LIBDVBCSA
		base::Functor1Ret<StreamInterfaceDecrypt *, int> getStreamInterfaceDecrypt = makeFunctor((base::Functor1Ret<StreamInterfaceDecrypt *, int>*) 0, streams, &Streams::getStreamInterfaceDecrypt);
		decrypt::dvbapi::Client dvbapi(appdataPath + "/" + "dvbapi.xml", getStreamInterfaceDecrypt);
		streams.enumerateDevices(&dvbapi);
		Properties properties(appdataPath + "/" + "config.xml", interface.getUUID(),
		                      streams.getXMLDeliveryString(), appdataPath, webPath, httpPort, rtspPort);
		HttpServer httpserver(streams, interface, properties, &dvbapi);
#else
		streams.enumerateDevices(nullptr);
		Properties properties(appdataPath + "/" + "config.xml", interface.getUUID(),
		                      streams.getXMLDeliveryString(), appdataPath, webPath, httpPort, rtspPort);
		HttpServer httpserver(streams, interface, properties, nullptr);
#endif
		RtspServer server(streams, properties, interface);
		upnp::ssdp::Server ssdpserver(interface, properties);
		if (ssdp) {
			ssdpserver.startThread();
		}

		// Loop
		while (!exitApp && !properties.exitApplication()) {
			usleep(12000);
		}
	}
	SI_LOG_INFO("--- stopped ---");

	// close logging interface
	closelog();

	close_app_log();

	return EXIT_SUCCESS;
}
