CFLAGS = -g
# uncomment on host
MOCK_FILE = wiringpimock.o

# uncomment on Rasperry
# EXTRA_CFLAGS = -DNO_SIG_HANDLER
# LD_FLAGS     = -L../../wiringPi -lwiringPi

gpiod: gpiod.o $(MOCK_FILE)
	cc -o gpiod gpiod.o $(LD_FLAGS) $(MOCK_FILE)

gpiod.o: gpiod.c
	cc $(CFLAGS) -c -I../../wiringPi/wiringPi $(EXTRA_CFLAGS)  gpiod.c

wiringpimock.o: wiringpimock.c
	cc $(CFLAGS) -c -I../../wiringPi/wiringPi wiringpimock.c

clean:
	rm -f *~ gpiod.o wiringpimock.o gpiod gpiod.testreport

test: gpiod
	@./test.sh
