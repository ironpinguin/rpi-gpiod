/**
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
#include <sys/time.h>
#include <libconfig.h>

#include "gpiod.h"
#include "wiringPi.h"
#include "dog128.h"

/** 
 * \brief Pid file for deamon mode.
 * 
 *  The location of the pid file for deamon modus.
 */
#define PID_FILE "/var/run/gpiod.pid"

/**
 * \brief The Buffer size for socket input reading
 * 
 * The buffer size of chars to read from socket every time.
 */
#define BUFFER_SIZE 128

/**
 * \brief read client command.
 * 
 * Read the gpio pin.
 */
#define CLIENT_READ    "READ"

/**
 * \brief Readall client command.
 * 
 * Read all gpio pin mode and status
 */
#define CLIENT_READALL "READALL"

/**
 * \brief write client command.
 * 
 * Write to gpio pin.
 */
#define CLIENT_WRITE   "WRITE"

/**
 * \brief mode client command.
 * 
 * Set gpio pin mode.
 */
#define CLIENT_MODE    "MODE"

/**
 * \brief lcd client command.
 * 
 * Prefix for all lcd commands.
 */
#define CLIENT_LCD     "LCD"

/**
 * \brief lcd inof client commad.
 * 
 * Give list off all lcd commands
 */
#define LCD_INFO       "INFO"
#define LCD_FONT_INFO  "FONTINFO"
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

typedef struct InterruptInfo {
  int pin; //> Gpio pin where the interrupt is occure.
  int type; //> Interrupt type INT_EDGE_FALLING, INT_EDGE_RISING, INT_EDGE_BOTH
  char *name; //> Interrupt name to write on socket.
  int wait;  //> Wait until next interrupt will be used.
  unsigned long occure; //> Last occure of interrupt in unix timestamp.
  int pud; //> Pull resistior mode.
} InterruptInfo;

int interrupts_count = 0;
InterruptInfo interrupt_infos[10];
char *socket_filename;    /**< Socket file name */
int flag_verbose     = 0; /**< variable to set verbose output */
int flag_dont_detach = 0; /**< variable to not run as deamon */
int lcd_is_init      = 0; /**< variable if lcd display is initialized */
int lcd_di           = DI; /**< variable with gpio pi of di lcd signal */
int lcd_led          = LED; /**< variable with gpio pwm pin for lcd backlight */
int lcd_spics        = SPICS; /**< variable with spi cs */
int client_socket_fd = 0;

/**
 * \brief Usage of the program.
 * 
 * Print the usage to stdout.
 */
void usage() {
  printf("Usage: gpiod [ -d ] [ -v ] [ -s socketfile ] [ -a diport ] [ -l ledport ] [ -c spics ] [ -i configfile ] [ -h ]\n");
  printf("    -d            don't daemonize\n");
  printf("    -v            verbose\n");
  printf("    -s sockefile  use the given file for for socket\n");
  printf("    -a diport     set di pin of the lcd display (default: %d)\n", DI);
  printf("    -l ledport    set backlight pwm port of the lcd display (default: %d)\n", LED);
  printf("    -c spics      set the spi chipselect fo the lcd display (default: %d)\n", SPICS);
  printf("    -i configfile use the given config file to configure gpiod\n");
  printf("    -h            show help (this meassge)\n");
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
 * Write pid file if started as deamon.
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
    snprintf(msg, BUFFER_SIZE, "%d %3d %s %d\n", pin, wpiPinToGpio (pin), pinNames [pin], digitalRead (pin));
    len = strlen(msg);
    write(fd, msg, len);
  }
}

/**
 * Init the lcd display only once.
 */
void init_lcd() {
  if (!lcd_is_init) {
    init(lcd_di, lcd_led, lcd_spics);
    initFonts();
    lcd_is_init = 1;
  }
}

/**
 * check if pin nummber is valid.
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
 * \brief work on lcd commands.
 * 
 * Identify and execute the given LCD command.
 * 
 * @param client_socket_fd The unix socket file descriptor.
 * @param buf              The input puffer with the command.
 */
void do_lcd_commands(int client_socket_fd, char *buf) {
  char command[BUFFER_SIZE], *text;
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
    text = malloc(strlen(buf) + 1);
    int n = sscanf(buf, "%s %d %d %d %[^\t\n]", command, &fontId, &x1, &y1, text);
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
    write_msg_to_client(client_socket_fd, "LCD BACKLIGHT value => change backlight between 0 and 100%.");
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
  } else if (strncmp(command, LCD_FONT_INFO, strlen(LCD_FONT_INFO)) == 0) {
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 4x6 ID 0");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 5x12 ID 2");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 5x8 ID 4");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 6x10 ID 6");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 6x8 ID 8");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 7x12 ID 10");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 7x12 ID 11");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 7x12 ID 12");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 7x12 ID 13");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 8x12 ID 14");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 8x12 ID 15");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 8x14 ID 16");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 8x14 ID 17");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 8x8 ID 18");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 8x8 ID 19");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 10x16 ID 20");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 10x16 ID 21");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 16x26 ID 26");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 16x26 ID 27");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 24x40 ID 30");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 24x40 ID 31");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 32x53 ID 32");
    write_msg_to_client(client_socket_fd, "LCD FONT SIZE 32x53 ID 33");
  } else {
    write_error_msg_to_client(client_socket_fd, "unknown lcd command");
  }
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

void interrupt(int id) {
  unsigned long time;
  struct timeval tv;
  char msg[50];
  gettimeofday(&tv, NULL);
  time = (unsigned long) (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
  if (interrupt_infos[id].occure + interrupt_infos[id].wait <= time) {
    interrupt_infos[id].occure = time;
    sprintf(msg, "%s", interrupt_infos[id].name);
    write_msg_to_client(client_socket_fd, msg);
  }
}

void interrupt0(void) {
  interrupt(0);
}

void interrupt1(void) {
  interrupt(1);
}

void interrupt2(void) {
  interrupt(2);
}

void interrupt3(void) {
  interrupt(3);
}

void interrupt4(void) {
  interrupt(4);
}

void interrupt5(void) {
  interrupt(5);
}

void interrupt6(void) {
  interrupt(6);
}

void interrupt7(void) {
  interrupt(7);
}

void interrupt8(void) {
  interrupt(8);
}

void interrupt9(void) {
  interrupt(9);
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
  config_t cfg;
  config_setting_t *setting, *interrupt_setting;
  char const *config_socket, *inter_name, *inter_type_string, *inter_pud;
  char *config_file_name;
  int ch, inter_pin, inter_type, inter_wait, r, pud;
  pid_t pid;
  int socketfd, read_config = 0;
  struct sockaddr_un address;
  socklen_t address_len;
#ifndef NO_SIG_HANDLER
  struct sigaction sig_act, sig_oact;
#endif
  while ((ch = getopt(argc, argv, "dhvs:a:l:c:i:")) != -1) {
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
     case 'i':
       read_config = 1;
       config_file_name = optarg;
       break;
     default:
       usage();
       exit(1);
    }
  }

  if (read_config) {
    config_init(&cfg);
    /* Read the config file and about on error */
    if (!config_read_file(&cfg, config_file_name)) {
      printf("\n%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
      config_destroy(&cfg);
      exit(1);
    }

    if (config_lookup_string(&cfg, "socket", &config_socket)) {
      socket_filename = strndup(config_socket, strlen(config_socket));
      printf("Socket file configured from config file as: %s\n", socket_filename);
    }

    setting = config_lookup(&cfg, "lcd");
    
    if (setting != NULL) {
      if (!config_setting_lookup_int(setting, "di_pin", &lcd_di) && flag_verbose) {
        printf("Set DI Pin of the LCD Display to %i with config file\n", lcd_di);
      }
      if (!config_setting_lookup_int(setting, "led_pin", &lcd_led) && flag_verbose) {
        printf("Set PWM LED Pin of the LCD Display to %i with config file\n", lcd_led);
      }
      if (!config_setting_lookup_int(setting, "spi_cs", &lcd_spics) && flag_verbose) {
        printf("Set SPI CS Pin of the LCD Display to %i with config file\n", lcd_spics);
      }
    }

    setting = config_lookup(&cfg, "interrupt");

    if (setting != NULL)
    {
      if (config_setting_type(setting) == CONFIG_TYPE_LIST) {
        interrupts_count = config_setting_length(setting);
        // Max interrupts are 10 if more configured only the first 10 are used!
        if (interrupts_count > 10) {
          interrupts_count = 10;
        }
        for (r=0; r < interrupts_count; r++) {
          interrupt_setting = config_setting_get_elem(setting, r);
          if (!(config_setting_lookup_int(interrupt_setting, "pin", &inter_pin)
              && config_setting_lookup_string(interrupt_setting, "type", &inter_type_string)
              && config_setting_lookup_string(interrupt_setting, "name", &inter_name)
              && config_setting_lookup_int(interrupt_setting, "wait", &inter_wait)
              && config_setting_lookup_string(interrupt_setting, "pud", &inter_pud))) {
            // TODO: Error message if configuration is not valid
            continue;
          }
          
          if(strncmp(inter_pud, "none", strlen("none")) == 0) {
            pud = PUD_OFF;
          } else if (strncmp(inter_pud, "up", strlen("up")) == 0) {
            pud = PUD_UP;
          } else if (strncmp(inter_pud, "down", strlen("down")) == 0) {
            pud = PUD_DOWN;
          } else {
            // TODO: Error message if configuration is not valid
            continue;
          }
          
          if(strncmp(inter_type_string, "falling", strlen("falling")) == 0) {
            inter_type = INT_EDGE_FALLING;
          } else if (strncmp(inter_type_string, "rising", strlen("rising")) == 0) {
            inter_type = INT_EDGE_RISING;
          } else if (strncmp(inter_type_string, "both", strlen("both")) == 0) {
            inter_type = INT_EDGE_BOTH;
          } else {
            // TODO: Error message if configuration is not valid
            continue;
          }
          
          if (r <= 10) {
            interrupt_infos[r].pin    = inter_pin;
            interrupt_infos[r].wait   = inter_wait;
            interrupt_infos[r].type   = inter_type;
            interrupt_infos[r].name   = strndup(inter_name, strlen(inter_name));
            interrupt_infos[r].occure = 0;
            interrupt_infos[r].pud    = pud;
          }
        }
      }
    }

    config_destroy(&cfg);
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
  
  
  // Setup pin and interrupts callback from the configuration.
  for (r=0; r < interrupts_count; r++) {
    pinMode(interrupt_infos[r].pin, INPUT);
    pullUpDnControl(interrupt_infos[r].pin, interrupt_infos[r].pud);
    switch (r) {
      case (0):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt0);
        break;
      case (1):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt1);
        break;
      case (2):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt2);
        break;
      case (3):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt3);
        break;
      case (4):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt4);
        break;
      case (5):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt5);
        break;
      case (6):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt6);
        break;
      case (7):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt7);
        break;
      case (8):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt8);
        break;
      case (9):
        wiringPiISR(interrupt_infos[r].pin, interrupt_infos[r].type, &interrupt9);
        break;
    }
  }

  while(read_client(socketfd)) {
    // Loop to accept client connection
  }

  return 0;
}
