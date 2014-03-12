/*
 * lcd.h
 *
 *  Created on: 10.03.2014
 *      Author: michele
 */

#ifndef LCD_H_
#define LCD_H_

#include "dog128.h"
#include "gpiod.h"

#define BUFFER_SIZE 128

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

void do_lcd_commands(int client_socket_fd, char *buf);
void set_lcd_di(int di);
int get_lcd_di();
void set_lcd_led(int led);
int get_lcd_led();
void set_lcd_spics(int spics);
int get_lcd_spics();
void do_write_lcd_info(int client_socket_fd);
void do_write_lcd_font_info(int client_socket_fd);

#endif /* LCD_H_ */
