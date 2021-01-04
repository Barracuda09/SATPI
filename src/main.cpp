/* main.cpp

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

#include <Satpi.h>
#include <StringConverter.h>
#include <Utils.h>
#include <base/ChildPIPEReader.h>

#include <atomic>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <execinfo.h>

#define LOCK_FILE   "SatPI.lock"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

static std::atomic_bool exitApp;
static std::atomic_bool restartApp;
static int retval;
static int otherSig;
static std::string appName;

static void createBackTrace(const char *app) {
	// DO NOT alloc memory!!
	void *array[25];
	const size_t size = backtrace(array, 25);

	// Log all the frames to Backtrace File
	char file[256];
	snprintf(file, sizeof(file), "/tmp/%s.bt", app);
	int backtraceFile = open(file, O_CREAT|O_WRONLY|O_TRUNC, 0664);
	if (backtraceFile > 0) {
		backtrace_symbols_fd(array, size, backtraceFile);
	} else {
		backtrace_symbols_fd(array, size, STDOUT_FILENO);
	}
	exit(1);
}

static void annotateBackTrace(const char *app, const char *file) {
	std::ifstream bt(file, std::ios::in);
	int i = 0;
	for (std::string line; std::getline(bt, line); ++i) {
		const auto begin = line.find('[');
		const auto end = line.find(']');
		if (begin != std::string::npos && end != std::string::npos) {
			const std::string addr = line.substr(begin + 1, end - begin - 1);

			char file[256];
			snprintf(file, sizeof(file),"addr2line %s -e %s", addr.c_str(), app);

			base::ChildPIPEReader exec;
			exec.open(file);
			std::string code;
			char buffer[256];
			std::size_t s;
			while ((s = exec.read(reinterpret_cast<unsigned char *>(buffer), 255)) > 0) {
				code.append(buffer, s);
				code.erase(std::find(code.begin(), code.end(), '\n'));
			}

			std::cout << "[bt] #" << i << " " << code << " -- " << line << std::endl;
		}
	}
}

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
			SI_LOG_INFO("stopping (KILL)");
		// fall-through
		case SIGUSR1:
		// fall-through
		case SIGHUP:
		// fall-through
		case SIGTERM:
			retval = EXIT_SUCCESS;
			exitApp = true;
			break;
		case SIGSEGV:
			createBackTrace(appName.c_str());
			break;
		default:
			SI_LOG_ERROR("Handle 'Other' signal");
			otherSig = 1;
			break;
	}
}

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
	signal(SIGSEGV, child_handler);

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

static void printUsage(const char *prog_name) {
	printf("Usage %s [OPTION]\r\n\r\nOptions:\r\n" \
	       "\t--help              show this help and exit\r\n" \
	       "\t--version           show the version number\r\n" \
	       "\t--user xx           run as user\r\n" \
	       "\t--dvb-path          set path were to find dvb devices default /dev/dvb\r\n" \
	       "\t--app-data-path     set path for application state data eg. xml files etc\r\n" \
	       "\t--iface-name        set the network interface to bind to (eg. eth0)\r\n" \
	       "\t--http-path         set root path of web/http pages\r\n" \
	       "\t--http-port         set http port default 8875 (1024 - 65535)\r\n" \
	       "\t--rtsp-port         set rtsp port default 554  ( 554 - 65535)\r\n" \
	       "\t--backtrace <file>  backtrace 'file'\r\n" \
	       "\t--childpipe         enabled Frontend 'Child PIPE - TS Reader'\r\n" \
	       "\t--no-daemon         do NOT daemonize\r\n" \
	       "\t--no-ssdp           do NOT advertise server\r\n", prog_name);
}

int main(int argc, char *argv[]) {
	bool ssdp = true;
	bool daemon = true;
	bool enableChildPIPE = false;
	int i;
	char *user = nullptr;
	extern const char *satpi_version;
	exitApp = false;

	// Defaults in Properties
	unsigned int httpPort = 0;
	unsigned int rtspPort = 0;

	std::string ifaceName;
	std::string currentPath;
	std::string appdataPath;
	std::string webPath;
	std::string dvbPath;

	//
	StringConverter::splitPath(argv[0], currentPath, appName);
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
		} else if (strcmp(argv[i], "--childpipe") == 0) {
			enableChildPIPE = true;
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
				httpPort = std::stoi(argv[i]);
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
				rtspPort = std::stoi(argv[i]);
				if (rtspPort <  554 || rtspPort > 65535) {
					printUsage(argv[0]);
					return EXIT_FAILURE;
				}
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--backtrace") == 0) {
			if (i + 1 < argc) {
				++i;
				annotateBackTrace(appName.c_str(), argv[i]);
				return EXIT_SUCCESS;
			}
			printUsage(argv[0]);
			return EXIT_FAILURE;
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

	// Open logging
	Log::openAppLog("SatPI");

	// Daemonize
	if (daemon) {
		daemonize("/var/lock/" LOCK_FILE, user);
	}

	// trap signals that we expect to receive
	signal(SIGSEGV, child_handler);
	signal(SIGPIPE, SIG_IGN);

	// notify we are alive
	SI_LOG_INFO("--- Starting SatPI version: %s ---", satpi_version);
	SI_LOG_INFO("Number of processors online: %d", base::ThreadBase::getNumberOfProcessorsOnline());
	SI_LOG_INFO("Default network buffer size: %d KBytes", InterfaceAttr::getNetworkUDPBufferSize() / 1024);
	do {
		try {
			restartApp = false;

#ifdef ADDDVBCA
			DVBCA dvbca;
			dvbca.startThread();
#endif
			SatPI satpi(ssdp, ifaceName, currentPath, appdataPath,
					webPath, dvbPath, httpPort, rtspPort, enableChildPIPE);

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

	Log::closeAppLog();

	return EXIT_SUCCESS;
}
