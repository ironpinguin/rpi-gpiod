/*
 * interrupt.h
 *
 *  Created on: 12.03.2014
 *      Author: michele
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include <stdlib.h>
#include "wiringPi.h"
#include "gpiod.h"

typedef struct InterruptInfo {
  int pin; //> Gpio pin where the interrupt is occur.
  int type; //> Interrupt type INT_EDGE_FALLING, INT_EDGE_RISING, INT_EDGE_BOTH
  char *name; //> Interrupt name to write on socket.
  int wait;  //> Wait until next interrupt will be used.
  unsigned long occure; //> Last occur of interrupt in unix timestamp.
  int pud; //> Pull resistior mode.
} InterruptInfo;

void registerInterrupts();
void set_interrupt_info(int pos, InterruptInfo info);
void set_interrupts_count(int count);
int get_interrupts_count();

#endif /* INTERRUPT_H_ */
