INC_DIR   = -I../../wiringPi/wiringPi -I../lib/wiringPi/wiringPi -I../lib/rpi-dog128/src
LIB_DIR   = -L/usrl/local/lib -L../lib/rpi-dog128/src

LIBS      = -lwiringPi -lpthread -lrpi-dog128

SHLIB_EXT = so

MOCK_BIN  = gpiod_mock
MOCK_OBJ  = wiringpimock.o
MOCK_LIB = libwiringPi_mock.$(SHLIB_EXT)

LD_FLAGS  = $(LIB_DIR) $(LIBS)
CFLAGS    = -Wall -g $(INC_DIR) -fPIC

SRC       = gpiod.c
OBJ       = $(SRC:.c=.o)

gpiod: $(OBJ)
	$(CC) -o $@ gpiod.o $(LD_FLAGS) 

mock_lib: $(MOCK_LIB)

$(MOCK_LIB): $(MOCK_OBJ)
	$(CC) -shared -o $@ $(MOCK_OBJ)

gpiod_mock: $(OBJ) $(MOCK_LIB)
	$(CC) -o $@  gpiod.o $(MOCK_OBJ) -lwiringPi_mock -ldog128 -L.

.c.o:
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $<

clean:
	rm -f *~ gpiod gpiod.testreport $(MOCK_BIN) $(MOCK_OBJ) $(OBJ) $(MOCK_LIB)


test test-mock-lib: gpiod $(MOCK_LIB)
	@./test.sh --with-mock-lib


test-mock-bin: $(MOCK_BIN)
	@./test.sh --with-mock-bin

test-real-pi: $(MOCK_LIB)
	@./test.sh
