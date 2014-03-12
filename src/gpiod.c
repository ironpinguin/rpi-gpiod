/**
 * gpiod - a GPIO daemon for Raspeberry Pi
 *
 */ 

#include "gpiod.h"

/** 
 * \brief Pid file for deamon mode.
 * 
 *  The location of the pid file for deamon modus.
 */
#define PID_FILE "/var/run/gpiod.pid"

static char *pinNames [] = {
    "GPIO 0",
    "GPIO 1",
    "GPIO 2",
    "GPIO 3",
    "GPIO 4",
    "GPIO 5",
    "GPIO 6",
    "GPIO 7",
    "SDA   ",
    "SCL   ",
    "CE0   ",
    "CE1   ",
    "MOSI  ",
    "MISO  ",
    "SCLK  ",
    "TxD   ",
    "RxD   ",
    "GPIO 8",
    "GPIO 9",
    "GPIO10",
    "GPIO11",
};


char *socket_filename;    /**< Socket file name */
int flag_verbose     = 0; /**< variable to set verbose output */
int flag_dont_detach = 0; /**< variable to not run as daemon */
int client_socket_fd = 0;

/**
 * \brief Set verbose flag
 *
 * Set the verbose flag
 *
 * @param flag
 */
void set_flag_verbose(int flag) {
	flag_verbose = flag;
}

/**
 * \brief Get verbose flag
 *
 * Get the verbose flag
 */
int get_flag_verbose() {
	return flag_verbose;
}

/**
 * \brief set don't detach flag
 *
 * Set the don't detach flag
 *
 * @param flag
 */
void set_flag_dont_detach(int flag) {
	flag_dont_detach = flag;
}

/**
 * \brief set socket filename
 *
 * @param name
 */
void set_socket_filename(char* name) {
	socket_filename = name;
}

/**
 * \brief get socket filename
 */
char* get_socket_filename() {
	return socket_filename;
}

/**
 * \brief get client socket file descriptor.
 */
int get_client_socket_fd() {
	return client_socket_fd;
}

/**
 * \brief Usage of the program.
 * 
 * Print the usage to stdout.
 */
void usage() {
  printf("Usage: gpiod [ -d ] [ -v ] [ -s socketfile ] [ -a diport ] [ -l ledport ] [ -c spics ] [ -i configfile ] [ -h ]\n");
  printf("    -d            don't daemonize\n");
  printf("    -v            verbose\n");
  printf("    -s sockefile  use the given file for for socket\n");
  printf("    -a diport     set di pin of the lcd display (default: %d)\n", DI);
  printf("    -l ledport    set backlight pwm port of the lcd display (default: %d)\n", LED);
  printf("    -c spics      set the spi chipselect fo the lcd display (default: %d)\n", SPICS);
  printf("    -i configfile use the given config file to configure gpiod\n");
  printf("    -h            show help (this message)\n");
}


void do_write_info(int client_socket_fd) {
    write_msg_to_client(client_socket_fd, "Commands:");
    write_msg_to_client(client_socket_fd, "READ pin => Read input pin.");
    write_msg_to_client(client_socket_fd, "WRITE pin value => Write value output pin.");
    write_msg_to_client(client_socket_fd, "READALL => Read all pins.");
    write_msg_to_client(client_socket_fd, "MODE pin mode => Set mode of pin. possible modes: (IN|OUT).");
}
/**
 * \brief strrpos implementation.
 * 
 * Self impelemtation of strrpos
 * 
 * @param string The string to search into.
 * @param niddle The char where will be searched.
 * 
 * @return char position or -1
 */
int strrpos(char *string, char niddle) {
    char *pos_info;
    
    pos_info = strrchr(string, niddle);
    if (pos_info == NULL) {
        return -1;
    } else {
        return pos_info - string;
    }
}

/**
 * Delete the pid file for cleanup.
 */
void delete_pid_file() {
  if (unlink(PID_FILE) == -1) {
    perror(PID_FILE);
    exit (EXIT_FAILURE);
  }
}

/**
 * Delete the socket file for cleanup.
 */
void delete_socket_file() {
  if (unlink(socket_filename) == -1) {
    perror(socket_filename);
    exit (EXIT_FAILURE);
  }
}

/**
 * Cleanup on exit.
 */
void cleanup_and_exit() {
  if (!flag_dont_detach) {
    delete_pid_file();
  }
  delete_socket_file();
  exit(EXIT_SUCCESS);
}

/**
 * Write pid file if started as daemon.
 */
void write_pid_file() {
  FILE *pid_file;
  if ((pid_file = fopen(PID_FILE, "w+")) == NULL) {
    perror(PID_FILE);
    exit (EXIT_FAILURE);
  }
  fprintf(pid_file, "%d\n", getpid());
  fclose(pid_file);
}

/**
 * Write error message to socket client.
 * 
 * @param fd  Socket file descriptor.
 * @param msg Message to write.
 */
void write_error_msg_to_client(int fd, char *msg) {
  char buf[BUFFER_SIZE];
  size_t len;
  snprintf(buf, BUFFER_SIZE, "%s - %s\n", SERVER_ERROR, msg);
  len = strlen(buf);
  write(fd, buf, len);
}

/**
 * Write a integer value to socket.
 * 
 * @param fd    File descriptor of the socket.
 * @param value Integer value.
 */
void write_int_value_to_client(int fd, int value) {
  char buf[BUFFER_SIZE];
  size_t len;
  snprintf(buf, BUFFER_SIZE, "%s - %d\n", SERVER_OK, value);
  len = strlen(buf);
  write(fd, buf, len);
}

/**
 * Write a message to socket.
 * 
 * @param fd  File descriptor of the socket.
 * @param msg Message to write.
 */
void write_msg_to_client(int fd, char *msg) {
  char buf[BUFFER_SIZE];
  size_t len;
  snprintf(buf, BUFFER_SIZE, "%s - %s\n", SERVER_OK, msg);
  len = strlen(buf);
  write(fd, buf, len);
}

/**
 * Write all pin data to socket.
 * 
 * @param fd File descriptor of the socket.
 */
void write_all_data_to_client(int fd) {
  int pin;
  char msg[BUFFER_SIZE];
  size_t len;

  if (flag_verbose) {
    printf("EXECUTING %s\n", CLIENT_READALL);
  }  
  snprintf(msg, BUFFER_SIZE, "%s\n", SERVER_OK);
  len = strlen(msg);
  write(fd, msg, len);
  for (pin = 0 ; pin < NUM_PINS ; ++pin) {
    snprintf(msg, BUFFER_SIZE, "%d %3d %s %d\n", pin, wpiPinToGpio(pin), pinNames[pin], digitalRead(pin));
    len = strlen(msg);
    write(fd, msg, len);
  }
}

/**
 * check if pin number is valid.
 * 
 * @param pin_num The pin number to check.
 * 
 * @return 0 or 1
 */
int is_valid_pin_num(int pin_num) {
  return (pin_num >= 0) && (pin_num < 16);
}

/**
 * Check if pin value is valid.
 * 
 * @param value The value to set the pin.
 * 
 * @return 0 or 1
 */
int is_valid_pin_value(int value) {
  return (value == 0) || (value == 1);
}

/**
 * Check if PIN Mode is valid actual only "IN" and "OUT" are allowed.
 * 
 * @param mode_str The mode string to check
 * 
 * @return 0 or 1 
 */
int is_valid_pin_mode(char *mode_str) {
  return ((strncmp(mode_str, "IN", 2) == 0) || (strncmp(mode_str, "OUT", 3) == 0));
}

/**
 * \brief Read from pin
 * 
 * Get Pinnumber, read status of pin and write it to the socket.
 * 
 * @param client_socket_fd The socket file descriptor.
 * @param buf              Input Command.
 */
void do_read_from_pin(int client_socket_fd, char *buf) {
  int pin_num;
  int n = sscanf(buf, "%d", &pin_num);
  if (n != 1) {
    write_error_msg_to_client(client_socket_fd, "parameter of type integer expected");
  } else if (!is_valid_pin_num(pin_num)) {
    write_error_msg_to_client(client_socket_fd, "unknown port number");
  } else {
    int value;
    if (flag_verbose) {
      printf("EXECUTING %s PIN %d\n", CLIENT_READ, pin_num);
    }

    value = digitalRead(pin_num);
    if (flag_verbose) {
      printf("VALUE = %d\n", value);
    }
    write_int_value_to_client(client_socket_fd, value);
  }
}

/**
 * \brief Write to pin.
 * 
 * Get Pinnumber and value. Write the value to the gpio pin.
 * 
 * @param client_socket_fd The socket file descriptor.
 * @param buf              The command.
 */
void do_write_to_pin(int client_socket_fd, char *buf) {
  int pin_num, value;
  int n = sscanf(buf, "%d %d", &pin_num, &value);
  if (n != 2) {
    write_error_msg_to_client(client_socket_fd, "expected WRITE <#pin> <0|1>");
  } else if (!is_valid_pin_num(pin_num)) {
    write_error_msg_to_client(client_socket_fd, "unknown port number");
  } else if (!is_valid_pin_value(value)) {
    write_error_msg_to_client(client_socket_fd, "value must be 0 or 1");
  } else {
    if (flag_verbose) {
      printf("EXECUTING %s PIN %d VALUE = %d\n", CLIENT_WRITE, pin_num, value);
    }
    digitalWrite(pin_num, value);
    write_msg_to_client(client_socket_fd, "operation performed");
  }
}

/**
 * \brief Set the pin mode.
 * 
 * Get pinnumber and mode. The set the given mode for the pin.
 * 
 * @param client_socket_fd The socket file descriptor.
 * @param buf              The command.
 */
void do_set_pin_mode(int client_socket_fd, char *buf) {
  char mode_str[BUFFER_SIZE];
  int pin_num;
  int n = sscanf(buf, "%d %s", &pin_num, mode_str);
  if (n != 2) {
    write_error_msg_to_client(client_socket_fd, "expected MODE <#pin> <IN|OUT>");
  } else if (!is_valid_pin_num(pin_num)) {
    write_error_msg_to_client(client_socket_fd, "unknown port number");
  } else if (!is_valid_pin_mode(mode_str)) {
    write_error_msg_to_client(client_socket_fd, "mode must be IN or OUT");
  } else {
    int mode = strcmp(mode_str, "IN")==0?0:1;
    if (flag_verbose) {
      printf("EXECUTING %s PIN %d DIR = %s (%d)\n", CLIENT_MODE, pin_num, mode_str, mode);
    }
    pinMode(pin_num, mode);
    write_msg_to_client(client_socket_fd, "operation performed");
  }
}

/**
 * Read command and select the right subroutine.
 * 
 * @param command          Command puffer from input.
 * @param client_socket_fd The socket file descriptor.
 */
void read_command(char *command, int client_socket_fd) {
    if (strncmp(command, CLIENT_READALL, 7) == 0) {
      write_all_data_to_client(client_socket_fd);
    } else if (strncmp(command, CLIENT_READ, strlen(CLIENT_READ)) == 0) {
      do_read_from_pin(client_socket_fd, command + strlen(CLIENT_READ) + 1);
    } else if (strncmp(command, CLIENT_WRITE, strlen(CLIENT_WRITE)) == 0) {
      do_write_to_pin(client_socket_fd, command + strlen(CLIENT_WRITE) + 1);
    } else if (strncmp(command, CLIENT_MODE, 4) == 0) {
      do_set_pin_mode(client_socket_fd, command + strlen(CLIENT_MODE) + 1);
    } else if (strncmp(command, CLIENT_LCD, strlen(CLIENT_LCD)) == 0) {
      do_lcd_commands(client_socket_fd, command + strlen(CLIENT_LCD) + 1);
    } else if (strncmp(command, CLIENT_INFO, strlen(CLIENT_INFO)) == 0) {
    	do_write_info(client_socket_fd);
    	do_write_lcd_info(client_socket_fd);
    } else {
      write_error_msg_to_client(client_socket_fd,  "unkown command");
    }
}

/**
 * Read client input function for endless loop.
 * 
 * @param socketfd Read client input.
 * @return Allways 1
 */
int read_client(int socketfd) {
  struct sockaddr_un address;
  socklen_t address_len = sizeof(address);
  int n, parted = 0, len;
  char new_line = '\n';
  char *command, *rest = NULL, *part = NULL, *step, buf[BUFFER_SIZE];
  
  step = malloc(BUFFER_SIZE*2);
  bzero(buf, BUFFER_SIZE);

  if ((client_socket_fd = accept(socketfd, (struct sockaddr *) &address, &address_len)) < 0) {
    perror("accept");
  }
  

  while ( (n = read(client_socket_fd, buf, BUFFER_SIZE-1)) > 0 ) {
    if (n == -1) {
      perror("read");
    }
    if (flag_verbose) {
        printf("client send: %s\n", buf);
    }
    
    len = strlen(buf);
    
    if (parted) {
        part = strndup(rest, strlen(rest));
    } else {
        if (part != NULL) {
            free(part);
        }
        part = NULL;
    }
    
    if (strrpos(buf, new_line) < len - 1) {
        parted = 1;
    } else {
        parted = 0;
    }
    
    command = strtok(buf, "\n");

    if (command == NULL) {
        perror("can't read command");
    } else {
        if (rest != NULL) {
            free(rest);
        }
        if (part != NULL) {
            bzero(step, BUFFER_SIZE * 2);
            strncpy(step, part, strlen(part));
            strncat(step, command, strlen(command));
            rest = strndup(step, strlen(step));
        } else {
            rest = strndup(command, strlen(command));
        }
    }
    
    if (flag_verbose) {
        printf("client command to process: %s\n", command);
    }
    
    while(command != NULL) {
        command = strtok(NULL, "\n");
        if (command != NULL) {
            read_command(rest, client_socket_fd);
            if (rest != NULL) {
                free(rest);
            }
            rest = NULL;
            rest = strndup(command, strlen(command));
        }
    }
    if (parted == 0) {
        read_command(rest, client_socket_fd);
    }
    bzero(buf, BUFFER_SIZE);
  }
  close(client_socket_fd);

  return 1;
}

/**
 * Main function to analyse command line parameters.
 * 
 * @param argc
 * @param argv
 * @return 
 */
int main(int argc, char **argv) {
  pid_t pid;
  int socketfd;
  struct sockaddr_un address;
  socklen_t address_len;
#ifndef NO_SIG_HANDLER
  struct sigaction sig_act, sig_oact;
#endif
  
  load_params(argc, argv);

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
  strncpy(address.sun_path, socket_filename, strlen(socket_filename));
  address_len = strlen(address.sun_path) + sizeof(address.sun_family) + 1;

  if (flag_verbose) {
    printf("Binding to socket file: %s\n", socket_filename);
  }
  if (bind(socketfd, (struct sockaddr *)&address, address_len) < 0) {
    perror("binding socket");
    exit (EXIT_FAILURE);
  }

  if (chmod(socket_filename, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
    perror("socket_file");
    exit (EXIT_FAILURE);
  }

  if (listen(socketfd, 10) == -1) {
    perror("listening socket");
    exit (EXIT_FAILURE);
  }

  if (wiringPiSetup() == -1) {
    printf ("Unable to initialise GPIO mode.\n");
    exit (EXIT_FAILURE);
  }
  
  registerInterrupts();

  while(read_client(socketfd)) {
    // Loop to accept client connection
  }

  return 0;
}
