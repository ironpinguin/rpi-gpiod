GPIO Deamon for Raspberry Pi
============================

##What does it?

This small Server open a unix socket to contol the gpio pins without need of root rights to other programms.

##External Requirements
* wiringPi  **git submodule included**
* rpi-dog128 **git submodule included**
* libconfig [http://www.hyperrealm.com/libconfig/]
* pthread

##TODO
- Implement multiple client.
- Add simple Security.

##COMMANDS
- READ *pin*
    - Read the given output pin.
- WRITE *pin*
    - Write the given input pin.
- READALL
  - Read the status of all pins.
- MODE *pin* *mode*
  - Set the mode of the pin. Modus: *IN*|*OUT*
- LCD INFO
  - Print all LCD Commands
- LCD FONTINFO
  - Print all exising Fonts.
- LCD CLEAR
  - Clear LCD Framebuffer.
- LCD SHOW
  - Send framebuffer to LCD.
- LCD LINE *x1* *y1* *x2* *y2*
  - Write line from x1 y1 to x2 y2 in framebuffer.
- LCD RECT *x1* *y1* *x2* *y2* *fill*
  - Write rectangle from left top x1 y1 to right bottom x2 y2 with fill=1 or not fill=0 in framebuffer.
- LCD CIRCLE *cx* *cy* *radius* *fill*
  - Write circle cx cy radius with fill=1 or not fill=0 in framebuffer.
- LCD ELLIPSE *cx* *cy* *radius1* *radius2* *fill*
  - Write ellipse cx cy and radius1 + radius2 with fill=1 or not fill=0 in framebuffer.
- LCD BACKLIGHT *percent*
  - Set LED Backlight in percent.
- LCD CONTRAST *cont*
  - Set contrast between 6 and 24.
- LCD TEXT *fontId* *x* *y* *text*
  - Write the text with frontId at position x y in framebuffer.
- LCD DOT *x* *y*
  - Write a dot at x y in framebuffer.
- LCD COLOR *color*
  - Set color to 1 write or 0 delete for every write command.
- LCD INVERT
  - Invert the display.
- LCD DSPNORMAL *n*
  - Set display normal to 0 or 1.

##Configfile
Example Config File
```
# Example gpiod configuration

# Unix Socket file
socket = "/var/lib/gpiod/socket";

# Setup the pins for the spi lcd interface (dog128)
lcd = {
  di_pin  = 6; /* Pin where the DI is attached */
  led_pin = 1; /* PWM pin to change backlight led brigness */
  spi_cs  = 0; /* SPI chipselect id */
};


# Interrupts 
interrupt = ( { pin  = 4;         /* WiringPi gpio pin number */
                type = "falling"; /* Interrupt edge ["falling", "rising", "both"] */
                name = "Goal1";   /* Name to write on interrupt to the socket */
                wait = 500;       /* Wait in milliseconds until next interrupt will be processed */
                pud  = "none"; }, /* Init pin with "none", "up", "down" resistor */
              { pin  = 5;
                type = "falling";
                name = "Goal2";
                wait = 500; 
                pud  = "none"; },
              { pin  = 16;
                type = "rising";
                name = "Bottom";
                wait = 500; 
                pud  = "up"; },
              { pin  = 7;
                type = "rising";
                name = "Left";
                wait = 500; 
                pud  = "up"; },
              { pin  = 2;
                type = "rising";
                name = "Top";
                wait = 500; 
                pud  = "up"; },
              { pin  = 0;
                type = "rising";
                name = "Middle";
                wait = 500; 
                pud  = "up"; },
              { pin  = 3;
                type = "rising";
                name = "Right";
                wait = 500; 
                pud  = "up"; } 
            );
```

