gpiod: gpiod.o wiringpimock.o
	cc -o gpiod gpiod.o wiringpimock.o

gpiod.o: gpiod.c
	cc -c -I../../wiringPi/wiringPi gpiod.c

wiringpimock.o: wiringpimock.c
	cc -c -I../../wiringPi/wiringPi wiringpimock.c

clean:
	rm *~ gpiod gpiod.o
