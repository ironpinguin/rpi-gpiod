/*
 * gpiod - a GPIO daemon for Raspeberry Pi
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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

    while ((ch = getopt(argc, argv, "dhvs:")) != -1) {
	 switch (ch) {
	     case 'd':
		 flag_dont_detach = 1;
	     	 break;
	     case 'h':
		 usage();
	     	 exit(0);
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
}
