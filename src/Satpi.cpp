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
#include "RtspServer.h"
#include "HttpServer.h"
#include "SsdpServer.h"
#include "Streams.h"
#include "InterfaceAttr.h"
#include "Properties.h"
#include "Log.h"
#include "StringConverter.h"
#ifdef LIBDVBCSA
	#include "DvbapiClient.h"
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

int exitApp;
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
			exitApp = 1;
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
			exitApp = 1;
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
           "\t--help         show this help and exit\r\n" \
           "\t--version      show the version number\r\n" \
           "\t--user xx      run as user\r\n" \
           "\t--no-daemon    do NOT daemonize\r\n" \
           "\t--no-ssdp      do NOT advertise server\r\n", prog_name);
}

/*
 *
 */
int main(int argc, char *argv[]) {
	bool ssdp = true;
	bool daemon = true;
	int i;
	char *user = NULL;
	extern const char *satpi_version;
	exitApp = 0;

	std::string path;
	std::string file;
	StringConverter::splitPath(argv[0], path, file);

	// Check options
	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--no-ssdp") == 0) {
			ssdp = false;
		} else if (strcmp(argv[i], "--user") == 0) {
			user = argv[i+1];
			++i; // because next was the user-name
		} else if (strcmp(argv[i], "--no-daemon") == 0) {
			daemon = false;
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
		daemonize("/tmp/" LOCK_FILE, user);
	}

	// notify we are alive
	SI_LOG_INFO("--- starting SatPI version: %s ---", satpi_version);
	SI_LOG_INFO("Number of processors online: %d", ThreadBase::getNumberOfProcessorsOnline());
	SI_LOG_INFO("Default network buffer size: %d KBytes", InterfaceAttr::getNetworkUDPBufferSize() / 1024);

	InterfaceAttr interface;
	Streams streams;
#ifdef LIBDVBCSA
	Functor1Ret<StreamProperties &, int> getStreamProperties = makeFunctor((Functor1Ret<StreamProperties &, int>*)0, streams, &Streams::getStreamProperties);
	Functor1Ret<bool, int> updateFrontend = makeFunctor((Functor1Ret<bool, int>*)0, streams, &Streams::updateFrontend);
	DvbapiClient dvbapi(getStreamProperties, updateFrontend);
	streams.enumerateFrontends("/dev/dvb", &dvbapi);
	Properties properties(interface.getUUID(), streams.getXMLDeliveryString(), path);
	HttpServer httpserver(interface, streams, properties, &dvbapi);
#else
	streams.enumerateFrontends("/dev/dvb", NULL);
	Properties properties(interface.getUUID(), streams.getXMLDeliveryString(), path);
	HttpServer httpserver(interface, streams, properties, NULL);
#endif

	RtspServer server(streams, interface.getIPAddress());

	SsdpServer ssdpserver(interface, properties);
	if (ssdp) {
		ssdpserver.startThread();
	}

	// Loop
	while (!exitApp) {
		usleep(12000);
	}
	SI_LOG_INFO("--- stopped ---");

	// close logging interface
	closelog();

	close_app_log();

	return EXIT_SUCCESS;
}


