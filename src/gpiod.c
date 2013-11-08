/*
 * gpiod - a GPIO daemon for Raspeberry Pi
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>

#include "gpiod.h"
#include "wiringPi.h"
#include "dog128.h"

#define PID_FILE "/var/run/gpiod.pid"
#define BUFFER_SIZE 128

#define CLIENT_READ    "READ"
#define CLIENT_READALL "READALL"
#define CLIENT_WRITE   "WRITE"
#define CLIENT_MODE    "MODE"
#define CLIENT_LCD     "LCD"

#define LCD_INFO       "INFO"
#define LCD_CLEAR      "CLEAR"
#define LCD_SHOW       "SHOW"
#define LCD_LINE       "LINE"
#define LCD_RECT       "RECT"
#define LCD_CIRCLE     "CIRCLE"
#define LCD_ELLIPSE    "ELLIPSE"
#define LCD_BACKLIGHT  "BACKLIGHT"
#define LCD_CONTRAST   "CONTRAST"
#define LCD_DSPNORMAL  "DSPNORMAL"
#define LCD_INVERT     "INVERT"
#define LCD_TEXT       "TEXT"
#define LCD_DOT        "DOT"
#define LCD_COLOR      "COLOR"

#define SERVER_OK    "OK"
#define SERVER_ERROR "ERROR"

char *socket_filename;
int flag_verbose     = 0;
int flag_dont_detach = 0;
int lcd_is_init      = 0;
int lcd_di           = DI;
int lcd_led          = LED;
int lcd_spics        = SPICS;

void usage() {
  printf("Usage: gpio [ -d ] [ -v ] [ -s socketfile ] [ -h ]\n");
  printf("    -d           don't daemonize\n");
  printf("    -v           verbose\n");
  printf("    -s sockefile use the given file for for socket\n");
  printf("    -a diport    set di pin of the lcd display (default: %d)\n", DI);
  printf("    -l ledport   set backlight pwm port of the lcd display (default: %d)\n", LED);
  printf("    -c spics     set the spi chipselect fo the lcd display (default: %d)\n", SPICS);
  printf("    -h           show help (this meassge)\n");
}

int strrpos(char *string, char niddle) {
    char *pos_info;
    
    pos_info = strrchr(string, niddle);
    if (pos_info == NULL) {
        return -1;
    } else {
        return pos_info - string;
    }
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

void write_error_msg_to_client(int fd, char *msg) {
  char buf[BUFFER_SIZE];
  size_t len;
  snprintf(buf, BUFFER_SIZE, "%s - %s\n", SERVER_ERROR, msg);
  len = strlen(buf);
  write(fd, buf, len);
}

void write_int_value_to_client(int fd, int value) {
  char buf[BUFFER_SIZE];
  size_t len;
  snprintf(buf, BUFFER_SIZE, "%s - %d\n", SERVER_OK, value);
  len = strlen(buf);
  write(fd, buf, len);
}

void write_msg_to_client(int fd, char *msg) {
  char buf[BUFFER_SIZE];
  size_t len;
  snprintf(buf, BUFFER_SIZE, "%s - %s\n", SERVER_OK, msg);
  len = strlen(buf);
  write(fd, buf, len);
}

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
    snprintf(msg, BUFFER_SIZE, "%d %3d %s %d\n", pin, wpiPinToGpio (pin), pinNames [pin], digitalRead (pin));
    len = strlen(msg);
    write(fd, msg, len);
  }
}

void init_lcd() {
  if (!lcd_is_init) {
    // TODO: set di, led and spics per commandline parameter.
    init(lcd_di, lcd_led, lcd_spics);
    initFonts();
    lcd_is_init = 1;
  }
}

int is_valid_pin_num(int pin_num) {
  return (pin_num >= 0) && (pin_num < 16);
}

int is_valid_pin_value(int value) {
  return (value == 0) || (value == 1);
}

int is_valid_pin_mode(char *mode_str) {
  return ((strncmp(mode_str, "IN", 2) == 0) || (strncmp(mode_str, "OUT", 3) == 0));
}

void do_lcd_commands(int client_socket_fd, char *buf) {
  char command[BUFFER_SIZE], text[BUFFER_SIZE];
  int x1, x2, y1, y2, r1, r2, fill, fontId;
  int n = sscanf(buf, "%s", command);
  if (n != 1) {
    write_error_msg_to_client(client_socket_fd, "parameter of type string expected");
  } else if (strncmp(command, LCD_LINE, strlen(LCD_LINE)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d %d %d %d", command, &x1, &y1, &x2, &y2);
    if (n != 5) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for draw line");
    } else {
      line(x1, y1, x2, y2);
    }
  } else if (strncmp(command, LCD_SHOW, strlen(LCD_SHOW)) == 0) {
    init_lcd();
    show();
  } else if (strncmp(command, LCD_CLEAR, strlen(LCD_CLEAR)) == 0) {
    init_lcd();
    clear();
  } else if (strncmp(command, LCD_INVERT, strlen(LCD_INVERT)) == 0) {
    init_lcd();
    invert();
  } else if (strncmp(command, LCD_RECT, strlen(LCD_RECT)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d %d %d %d %d", command, &x1, &y1, &x2, &y2, &fill);
    if (n != 6) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for draw rect");
    } else {
      rect(x1, y1, x2, y2, fill);
    }
  } else if (strncmp(command, LCD_CIRCLE, strlen(LCD_CIRCLE)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d %d %d %d", command, &x1, &y1, &r1, &fill);
    if (n != 5) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for draw circle");
    } else {
      circle(x1, y1, r1, fill);
    }
  } else if (strncmp(command, LCD_ELLIPSE, strlen(LCD_ELLIPSE)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d %d %d %d %d", command, &x1, &y1, &r1, &r2, &fill);
    if (n != 6) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for draw ellipse");
    } else {
      ellipse(x1, y1, r1, r2, fill);
    }
  } else if (strncmp(command, LCD_DOT, strlen(LCD_DOT)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d %d", command, &x1, &y1);
    if (n != 3) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for draw dot");
    } else {
      dot(x1, y1);
    }
  } else if (strncmp(command, LCD_COLOR, strlen(LCD_COLOR)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d", command, &x1);
    if (n != 2) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for set pen color");
    } else if (x1 < 0 || x1 > 1) {
      write_error_msg_to_client(client_socket_fd, "parameters for set pen color can be only 0 or 1");
    } else {
      setPenColor(x1);
    }
  } else if (strncmp(command, LCD_BACKLIGHT, strlen(LCD_BACKLIGHT)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d", command, &x1);
    if (n != 2) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for set backlight");
    } else if (x1 < 0 || x1 > 100) {
      write_error_msg_to_client(client_socket_fd, "parameters for set backlight can be only 0 or 100");
    } else {
      backlight(x1);
    }
  } else if (strncmp(command, LCD_CONTRAST, strlen(LCD_CONTRAST)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d", command, &x1);
    if (n != 2) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for set contrast");
    } else if (x1 < 5 || x1 > 25) {
      write_error_msg_to_client(client_socket_fd, "parameters for set contrast can be only 0 or 1024");
    } else {
      contrast(x1);
    }
  } else if (strncmp(command, LCD_DSPNORMAL, strlen(LCD_DSPNORMAL)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d", command, &x1);
    if (n != 2) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters for set display normal");
    } else if (x1 < 0 || x1 > 1) {
      write_error_msg_to_client(client_socket_fd, "parameters for set display normal can be only 0 or 1");
    } else {
      displaynormal(x1);
    }
  } else if (strncmp(command, LCD_TEXT, strlen(LCD_TEXT)) == 0) {
    init_lcd();
    int n = sscanf(buf, "%s %d %d %d %s", command, &fontId, &x1, &y1, text);
    if (n != 5) {
      write_error_msg_to_client(client_socket_fd, "unexpected parameters to write text");
    } else if (fontId < 0 || fontId > 33) {
      write_error_msg_to_client(client_socket_fd, "fontId to write Text must be between 0 and 33");
    } else {
      selectFont(fontId);
      writeText(text, x1, y1);
    }
  } else if (strncmp(command, LCD_INFO, strlen(LCD_INFO)) == 0) {
    write_msg_to_client(client_socket_fd, "LCD Commands:");
    write_msg_to_client(client_socket_fd, "LCD CLEAR => clear screen buffer.");
    write_msg_to_client(client_socket_fd, "LCD SHOW => wirte screen buffer to lcd.");
    write_msg_to_client(client_socket_fd, "LCD BACKLIGHT value => change backlight between 0 and 1024.");
    write_msg_to_client(client_socket_fd, "LCD CONTRAST value => change contrast between 5 and 25.");
    write_msg_to_client(client_socket_fd, "LCD DSPNORMAL value => change display 0 => normal, 1 => reverse.");
    write_msg_to_client(client_socket_fd, "LCD INVERT => invert display.");
    write_msg_to_client(client_socket_fd, "LCD DOT x1 y1 => write a dot to the screen buffer.");
    write_msg_to_client(client_socket_fd, "LCD COLOR value => set color of drawing 0 => delete pixel, 1 => write pixel.");
    write_msg_to_client(client_socket_fd, "LCD LINE x1 y1 x2 y2 => write line to screen buffer.");
    write_msg_to_client(client_socket_fd, "LCD RECT x1 y1 x2 y2 => write rectangle to screen buffer.");
    write_msg_to_client(client_socket_fd, "LCD CIRCLE x1 y1 r1 fill => write circle to screen buffer.");
    write_msg_to_client(client_socket_fd, "LCD ELLIPSE x1 y1 r1 r2 fill => write ellipse to screen buffer.");
    write_msg_to_client(client_socket_fd, "LCD TEXT fontId x1 y1 \"TEXT\" => write text to screen buffer.");
  } else {
    write_error_msg_to_client(client_socket_fd, "unknown lcd command");
  }
}

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
    } else {
      write_error_msg_to_client(client_socket_fd,  "unkown command");
    }
}

int read_client(int socketfd) {
  struct sockaddr_un address;
  socklen_t address_len = sizeof(address);
  int client_socket_fd, n, parted = 0, len;
  char new_line = '\n';
  char *command, *rest, *part, buf[BUFFER_SIZE];
  
  bzero(buf, BUFFER_SIZE);

  if ((client_socket_fd = accept(socketfd, (struct sockaddr *) &address, &address_len)) < 0) {
    perror("accept");
  }

  while ( (n = read(client_socket_fd, buf, BUFFER_SIZE)) > 0 ) {
    if (n == -1) {
      perror("read");
    }
    if (flag_verbose) {
        printf("client send: %s\n", buf);
    }
    
    len = strlen(buf);
    
    if (parted) {
        part = strdup(rest);
    } else {
        free(part);
        part = NULL;
    }
    
    if (strrpos(buf, new_line) < len - 1) {
        parted = 1;
    } else {
        parted = 0;
    }
    
    if (parted)
    command = strtok(buf, "\n");
    
    if (command == NULL) {
        perror("can't read command");
    } else {
        if (part != NULL) {
            rest = strcat(part, command);
        } else {
            rest = strdup(command);
        }
        read_command(rest, client_socket_fd);
        
    }
    
    while(command != NULL) {
        command = strtok(NULL, "\n");
        if (command != NULL || (command == NULL && parted == 0)) {
            read_command(command, client_socket_fd);
        }
        if (command != NULL) {
            free(rest);
            rest = 0;
            rest = strdup(command);
        }
    }
  }
  close(client_socket_fd);

  return 1;
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
  while ((ch = getopt(argc, argv, "dhvs:a:l:c:")) != -1) {
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
     case 'a':
       if (is_valid_pin_num(atoi(optarg))) {
         lcd_di = atoi(optarg);
       } else {
         printf("Only valid Pinnumber between 1 and 16 allowed!\n");
         usage();
         exit(EXIT_FAILURE);
       }
       break;
     case 'l':
       if (is_valid_pin_num(atoi(optarg))) {
         lcd_led = atoi(optarg);
       } else {
         printf("Only valid Pinnumber between 1 and 16 allowed!\n");
         usage();
         exit(EXIT_FAILURE);
       }
       break;
     case 'c':
       if (is_valid_pin_num(atoi(optarg))) {
         lcd_spics = atoi(optarg);
       } else {
         printf("Only 0 or 1 allowed!\n");
         usage();
         exit(EXIT_FAILURE);
       }
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

  while(read_client(socketfd)) {
    // Loop to accept client connection
  }

  return 0;
}
