/*
 * interrupt.c
 *
 *  Created on: 12.03.2014
 *      Author: michele
 */

#include "interrupt.h"

int interrupts_count = 0;
InterruptInfo interrupt_infos[10];

void set_interrupt_info(int pos, InterruptInfo info) {
	interrupt_infos[pos] = info;
}
void set_interrupts_count(int count) {
	interrupts_count = count;
}

int get_interrupts_count() {
	return interrupts_count;
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
    write_msg_to_client(get_client_socket_fd(), msg);
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

void registerInterrupts() {
	int r;
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
}
