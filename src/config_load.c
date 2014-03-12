/*
 * config.c
 *
 *  Created on: 10.03.2014
 *      Author: michele
 */

#include "stdlib.h"
#include "gpiod.h"
#include <libconfig.h>
#include "wiringpi.h"
#include "config_load.h"
#include "interrupt.h"

void load_params(int argc, char **argv) {
  config_t cfg;
  config_setting_t *setting, *interrupt_setting;
  char const *config_socket, *inter_name, *inter_type_string, *inter_pud;
  char *config_file_name;
  int ch, inter_pin, inter_type, inter_wait, r, pud;
  int lcd_di, lcd_led, lcd_spics, read_config = 0;
  InterruptInfo *interrupt_info;

  while ((ch = getopt(argc, argv, "dhvs:a:l:c:i:")) != -1) {
    switch (ch) {
      case 'd':
        set_flag_dont_detach(1);
        break;
      case 'h':
        usage();
        exit(EXIT_SUCCESS);
        break;
     case 'v':
       set_flag_verbose(1);
       break;
     case 's':
       set_socket_filename(optarg);
       break;
     case 'a':
       if (is_valid_pin_num(atoi(optarg))) {
         set_lcd_di(atoi(optarg));
       } else {
         printf("Only valid Pinnumber between 1 and 16 allowed!\n");
         usage();
         exit(EXIT_FAILURE);
       }
       break;
     case 'l':
       if (is_valid_pin_num(atoi(optarg))) {
         set_lcd_led(atoi(optarg));
       } else {
         printf("Only valid Pinnumber between 1 and 16 allowed!\n");
         usage();
         exit(EXIT_FAILURE);
       }
       break;
     case 'c':
       if (is_valid_pin_num(atoi(optarg))) {
         set_lcd_spics(atoi(optarg));
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
      printf("\n%s:%d - %s\n", config_file_name, config_error_line(&cfg), config_error_text(&cfg));
      config_destroy(&cfg);
      exit(1);
    }

    if (config_lookup_string(&cfg, "socket", &config_socket)) {
      set_socket_filename(strndup(config_socket, strlen(config_socket)));
      printf("Socket file configured from config file as: %s\n", get_socket_filename());
    }

    setting = config_lookup(&cfg, "lcd");

    if (setting != NULL) {
      if (!config_setting_lookup_int(setting, "di_pin", &lcd_di)) {
    	  set_lcd_di(lcd_di);
    	  if (get_flag_verbose()) {
    		  printf("Set DI Pin of the LCD Display to %i with config file\n", lcd_di);
    	  }
      }
      if (!config_setting_lookup_int(setting, "led_pin", &lcd_led)) {
    	  set_lcd_led(lcd_led);
    	  if (get_flag_verbose()) {
    		  printf("Set PWM LED Pin of the LCD Display to %i with config file\n", lcd_led);
    	  }
      }
      if (!config_setting_lookup_int(setting, "spi_cs", &lcd_spics)) {
    	  set_lcd_spics(lcd_spics);
    	  if (get_flag_verbose()) {
    		  printf("Set SPI CS Pin of the LCD Display to %i with config file\n", lcd_spics);
    	  }
      }
    }

    setting = config_lookup(&cfg, "interrupt");

    if (setting != NULL)
    {
      if (config_setting_type(setting) == CONFIG_TYPE_LIST) {
        set_interrupts_count(config_setting_length(setting));
        // Max interrupts are 10 if more configured only the first 10 are used!
        if (get_interrupts_count() > 10) {
          set_interrupts_count(10);
        }
        for (r=0; r < get_interrupts_count(); r++) {
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
          interrupt_info = malloc(sizeof(InterruptInfo));

          if (r <= 10) {
            interrupt_info->pin    = inter_pin;
            interrupt_info->wait   = inter_wait;
            interrupt_info->type   = inter_type;
            interrupt_info->name   = strndup(inter_name, strlen(inter_name));
            interrupt_info->occure = 0;
            interrupt_info->pud    = pud;
            set_interrupt_info(r, *interrupt_info);
          }
        }
      }
    }

    config_destroy(&cfg);
  }
}
