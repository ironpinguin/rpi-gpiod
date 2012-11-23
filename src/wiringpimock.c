#include "wiringPi.h" 

void (*pinMode)     (int pin, int mode);
int  (*digitalRead) (int pin);
void (*digitalWrite)(int pin, int value);

int digitalReadWMock(int pin) {
    return (pin % 2);
}

void digitalWriteWMock(int pin, int value) {
}

void pinModeWMock(int pin, int mode) {
}

int wiringPiSetupGpio () {
    pinMode      = pinModeWMock;
    digitalRead  = digitalReadWMock;
    digitalWrite = digitalWriteWMock;

    return 1;
}

int wpiPinToGpio (int wpiPin)
{
      return wpiPin & 63;
}


