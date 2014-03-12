/**
 * gpiod - a GPIO daemon for Raspeberry Pi
 *
 */ 
#include "lcd.h"

int lcd_is_init      = 0; /**< variable if lcd display is initialized */
int lcd_di           = DI; /**< variable with gpio pi of di lcd signal */
int lcd_led          = LED; /**< variable with gpio pwm pin for lcd backlight */
int lcd_spics        = SPICS; /**< variable with spi cs */

/**
 * \brief set lcd di
 *
 * @param di
 */
void set_lcd_di(int di) {
	lcd_di = di;
}

/**
 * \brief get lcd di pin
 *
 * @return int
 */
int get_lcd_di() {
	return lcd_di;
}

/**
 * \brief set lcd led pin
 *
 * @param led
 */
void set_lcd_led(int led) {
	lcd_led = led;
}

/**
 * \brief get lcd led pin
 *
 * @return int
 */
int get_lcd_led() {
	return lcd_led;
}

/**
 * \brief set lcd spi chip select id
 *
 * @param spics
 */
void set_lcd_spics(int spics) {
	lcd_spics = spics;
}

/**
 * \brief get lcd spi chip select id
 *
 * @return int
 */
int get_lcd_spics() {
	return lcd_spics;
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
	  do_write_lcd_info(client_socket_fd);
  } else if (strncmp(command, LCD_FONT_INFO, strlen(LCD_FONT_INFO)) == 0) {
	  do_write_lcd_font_info(client_socket_fd);
  } else {
    write_error_msg_to_client(client_socket_fd, "unknown lcd command");
  }
}

void do_write_lcd_info(int client_socket_fd) {
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
    write_msg_to_client(client_socket_fd, "LCD FONTINFO => get a list of all fonts.");
    write_msg_to_client(client_socket_fd, "LCD INFO => get this info.");
}

void do_write_lcd_font_info(int client_socket_fd) {
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
}
