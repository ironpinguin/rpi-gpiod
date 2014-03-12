#ifndef GPIOD_H_
#define GPIOD_H_

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

#include "wiringPi.h"
#include "lcd.h"
#include "config_load.h"
#include "interrupt.h"

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

#define CLIENT_INFO    "INFO"

#define SERVER_OK    "OK"
#define SERVER_ERROR "ERROR"

void write_all_data_to_client(int fd);
void do_read_from_pin(int client_socket_fd, char *buf);
void do_write_to_pin(int client_socket_fd, char *buf);
void do_set_pin_mode(int client_socket_fd, char *buf);
int is_valid_pin_num(int pin_num);
int is_valid_pin_value(int value);
void write_msg_to_client(int fd, char *msg);
void write_int_value_to_client(int fd, int value);
void write_error_msg_to_client(int fd, char *msg);
void usage();
void set_flag_verbose(int flag);
void set_flag_dont_detach(int flag);
void set_socket_filename(char* name);
char* get_socket_filename();
int get_flag_verbose();
int get_client_socket_fd();

#endif /* GPIOD_H_ */
