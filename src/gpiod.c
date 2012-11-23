/*
 * gpiod - a GPIO daemon for Raspeberry Pi
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>

#include "gpiod.h"
#include "wiringPi.h"

#define PID_FILE "/var/run/gpiod.pid"
#define BUFFER_SIZE 64

#define CLIENT_READ    "READ"
#define CLIENT_READALL "READALL"
#define CLIENT_WRITE   "WRITE"
#define CLIENT_MODE    "MODE"

#define SERVER_OK    "OK"
#define SERVER_ERROR "ERROR"

char *socket_filename;
int flag_verbose     = 0;
int flag_dont_detach = 0;

void usage() {
    printf("Usage: gpio [ -d ] [ -v ] [ -s socketfile ] [ -h ]\n");
    printf("    -d           don't daemonize\n");
    printf("    -v           verbose\n");
    printf("    -s sockefile use the given file for for socket\n");
    printf("    -h           show help (this meassge)\n");
}

void delete_pid_file() {
    if (unlink(PID_FILE) == -1) {
	perror(PID_FILE);
	exit (EXIT_FAILURE);
    }
}

void delete_socket_file() {
    if (unlink(socket_filename) == -1) {
	perror(socket_filename);
	exit (EXIT_FAILURE);
    }
}

void cleanup_and_exit() {
    if (!flag_dont_detach) {
	delete_pid_file();
    }
    delete_socket_file();
    exit(EXIT_SUCCESS);
}

void write_pid_file() {
    FILE *pid_file;
    if ((pid_file = fopen(PID_FILE, "w+")) == NULL) {
	perror(PID_FILE);
	exit (EXIT_FAILURE);
    }
    fprintf(pid_file, "%d\n", getpid());
    fclose(pid_file);
}

int write_error_msg_to_client(int fd, char *msg) {
    char buf[BUFFER_SIZE];
    size_t len;
    snprintf(buf, BUFFER_SIZE, "%s - %s\n", SERVER_ERROR, msg);
    len = strlen(buf);
    write(fd, buf, len);
}

int write_all_data_to_client(int fd) {
    int pin;
    char msg[BUFFER_SIZE];
    size_t len;

    snprintf(msg, BUFFER_SIZE, "%s\n", SERVER_OK);
    len = strlen(msg);
    write(fd, msg, len);
    for (pin = 0 ; pin < NUM_PINS ; ++pin) {
	snprintf(msg, BUFFER_SIZE, "%d %3d %s %d\n", pin, wpiPinToGpio (pin), pinNames [pin], digitalRead (pin));
	len = strlen(msg);
	write(fd, msg, len);
    }
}


int read_client(int socketfd) {
    struct sockaddr_un address;
    socklen_t address_len = sizeof(address);
    int client_socket_fd;
    int n;
    char buf[BUFFER_SIZE];
    char client_cmd;
    char *p;
    int pin_num; /* 0..7 */
    int value;   /* 0..1 */
    char *dir;   /* IN / OUT */
    char msg[BUFFER_SIZE];
    size_t len;

    if ((client_socket_fd = accept(socketfd,(struct sockaddr *)&address,&address_len)) < 0) {
	perror("accept");
    }
    n=read(client_socket_fd,buf,BUFFER_SIZE);
    if (n == -1) {
	perror("read");
    }
    printf("client send: %s\n", buf);

    if (strncmp(buf, CLIENT_READALL, 7) == 0) {
	printf("EXECUTING %s\n", CLIENT_READALL);
	write_all_data_to_client(client_socket_fd);
    } else if (strncmp(buf, CLIENT_READ, 4) == 0) {
	int value;
	int n;
	p = buf + 5;
	n = sscanf(p, "%d", &pin_num);
	if (n != 1) {
	    write_error_msg_to_client(client_socket_fd, "parameter of type integer expected");
	} else if ((pin_num < 0) || (pin_num > 16)) {
	    write_error_msg_to_client(client_socket_fd, "unknown port number");
	} else {
	    if (flag_verbose) {
		printf("EXECUTING %s PIN %d\n", CLIENT_READ, pin_num);
	    }

	    value = digitalRead(pin_num);
	    if (flag_verbose) {
		printf("VALUE = %d\n", value);
	    }
	    snprintf(msg, BUFFER_SIZE, "%s - %d\n", SERVER_OK, value);
	    len = strlen(msg);
	    write(client_socket_fd, msg, len);
	}
    } else if (strncmp(buf, CLIENT_WRITE, 5) == 0) {
	p = buf + 6;
	sscanf(p, "%d %d", &pin_num, &value);
	printf("EXECUTING %s PIN %d VALUE = %d\n", CLIENT_WRITE, pin_num, value);

	digitalWrite(pin_num, value);
	snprintf(msg, BUFFER_SIZE, "%s\n", SERVER_OK);
	len = strlen(msg);
	write(client_socket_fd, msg, len);
    } else if (strncmp(buf, CLIENT_MODE, 4) == 0) {
	p = buf + 5;
	sscanf(p, "%d %s", &pin_num, dir);
	printf("EXECUTING %s PIN %d DIR = %s\n", CLIENT_MODE, pin_num, dir);

	pinMode(pin_num, strcmp(dir, "IN")==0?0:1);
	snprintf(msg, BUFFER_SIZE, "%s\n", SERVER_OK);
	len = strlen(msg);
	write(client_socket_fd, msg, len);
    } else {
	snprintf(msg, BUFFER_SIZE, "%s - %s\n", SERVER_ERROR, "unkown command");
	len = strlen(msg);
	write(client_socket_fd, msg, len);
    }
    close(client_socket_fd);
}

int main(int argc, char **argv) {
    int ch;
    pid_t pid;
    int socketfd;
    struct sockaddr_un address;
    socklen_t address_len;
#ifndef NO_SIG_HANDLER
    struct sigaction sig_act, sig_oact;
#endif
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

    if (!flag_dont_detach) {
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

	write_pid_file();
    } 

#ifndef NO_SIG_HANDLER
    sig_act.sa_handler = cleanup_and_exit;
    sigemptyset (&sig_act.sa_mask);
    if (sigaction(SIGTERM, &sig_act, &sig_oact) == -1) {
        perror("sigaction");
        exit (EXIT_FAILURE);
    }
    if (sigaction(SIGSEGV, &sig_act, &sig_oact) == -1) {
        perror("sigaction");
        exit (EXIT_FAILURE);
    }
    if (sigaction(SIGINT, &sig_act, &sig_oact) == -1) {
        perror("sigaction");
        exit (EXIT_FAILURE);
    }
#endif

    if ((socketfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0) {
    	perror("creating socket");
        exit (EXIT_FAILURE);
    }

    bzero((char *) &address, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, socket_filename);
    address_len = strlen(address.sun_path) + sizeof(address.sun_family) + 1;

    printf("Binding to socket file: %s\n", socket_filename);
    if (bind(socketfd, (struct sockaddr *)&address, address_len) < 0) {
    	perror("binding socket");
        exit (EXIT_FAILURE);
    }

    if (listen(socketfd, 10) == -1) {
    	perror("listening socket");
        exit (EXIT_FAILURE);
    }

    if (wiringPiSetupGpio() == -1) {
	printf ("Unable to initialise GPIO mode.\n");
        exit (EXIT_FAILURE);
    }
    while(read_client(socketfd)) {
    }
}
