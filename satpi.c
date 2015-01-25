#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/mman.h>

#include <linux/types.h>

#include "tune.h"
#include "ssdp.h"
#include "rtsp.h"
#include "rtp.h"
#include "rtcp.h"
#include "http.h"
#include "utils.h"
#include "applog.h"

#define DAEMON_NAME "SatIP Server"
#define LOCK_FILE   "SatIP.lock"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int exitApp;
static int retval;
static int otherSig;

static RtpSession_t rtpsession;

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
           "\t--user xx      run as user\r\n" \
           "\t--no-daemon    do not daemonize\r\n" \
           "\t--no-rtcp      do NOT send RTCP packets\r\n"
           "\t--no-ssdp      do NOT advertise server\r\n", prog_name);
}

/*
 *
 */
int main(int argc, char *argv[]) {
	int ssdp = 1;
	int rtcp = 1;
	int daemon = 1;
	int i;
	char *user = NULL;

	exitApp = 0;

	// Check options
	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--no-rtcp") == 0) {
			rtcp = 0;
		} else if (strcmp(argv[i], "--no-ssdp") == 0) {
			ssdp = 0;
		} else if (strcmp(argv[i], "--user") == 0) {
			user = argv[i+1];
			++i; // because next was the user-name
		} else if (strcmp(argv[i], "--no-daemon") == 0) {
			daemon = 0;
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
	open_satip_log();

	// Daemonize
	if (daemon) {
		daemonize("/tmp/" LOCK_FILE, user);
	}

	// notify we are alive
	SI_LOG_INFO("--- starting ---");

	// detect the attached frontends and get frontend properties
	if (detect_attached_frontends("/dev/dvb", &rtpsession.fe) == 0) {
		printf("Error: No frontend found!!\n");
//		return EXIT_FAILURE;
	}

	// initialize all variables
	init_rtp(&rtpsession);

	// get interface IP and MAC addresses
	get_interface_properties(&rtpsession.interface);

	// Make UUID with MAC address
	snprintf(rtpsession.uuid, sizeof(rtpsession.uuid), "50c958a8-e839-4b96-b7ae-%s", rtpsession.interface.mac_addr);

	start_rtsp(&rtpsession);
	start_http(&rtpsession);
	start_rtp(&rtpsession);

	if (rtcp) {
		start_rtcp(&rtpsession);
	}

	// as last start server advertisements if requested
	if (ssdp) {
		start_ssdp(&rtpsession);
	}

	// Loop
	while (!exitApp) {
		usleep(150000);
	}

	stop_rtsp(&rtpsession);

	// cleanup threads
	stop_http();
	if (ssdp) {
		stop_ssdp();
	}
	stop_rtp(&rtpsession);

	SI_LOG_INFO("--- stopped ---");

	// close our logging
	close_satip_log();

	// close logging interface
	closelog();
	return retval;
}


