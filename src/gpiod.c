/*
 * gpiod - a GPIO daemon for Raspeberry Pi
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#define PID_FILE "/var/run/gpiod.pid"

void usage() {
    printf("Usage: gpio [ -d ] [ -v ] [ -s socketfile ] [ -h ]\n");
    printf("    -d           don't daemonize\n");
    printf("    -v           verbose\n");
    printf("    -s sockefile use the given file for for socket\n");
    printf("    -h           show help (this meassge)\n");
}

int main(int argc, char **argv) {
    int flag_dont_detach = 0;
    int flag_verbose     = 0;
    char *socket_filename;
    int ch;
    pid_t pid;
    FILE *pid_file;

    while ((ch = getopt(argc, argv, "dhvs:")) != -1) {
	 switch (ch) {
	     case 'd':
		 flag_dont_detach = 1;
	     	 break;
	     case 'h':
		 usage();
	     	 exit(EXIT_SUCCESS);
		 break;
	     case 'v':
		 flag_verbose = 1;
	     	 break;
	     case 's':
		 socket_filename = optarg;
	     	 break;
	     default:
		 usage();
	     	 exit(1);
	 }
    }

    pid = fork();
    if (pid < 0) {
	exit(EXIT_FAILURE);
    }
    // terminate parent
    if (pid > 0) {
	exit(EXIT_SUCCESS);
    }
    if (setsid() < 0) {
	exit (EXIT_FAILURE);
    }

    // write pid file
    if ((pid_file = fopen(PID_FILE, "w+")) == NULL) {
	perror(PID_FILE);
	exit (EXIT_FAILURE);
    }
    fprintf(pid_file, "%d\n", getpid());
    fclose(pid_file);

    while(1) {
    }
}
