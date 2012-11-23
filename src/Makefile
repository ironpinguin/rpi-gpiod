# uncomment on Rasperry
# EXTRA_CFLAGS = -DNO_SIG_HANDLER
#
INC_DIR   = -I../../wiringPi/wiringPi
LIB_DIR   = -L../../wiringPi

LIBS      = -lwiringPi

MOCK_BIN  = gpiod_mock
MOCK_OBJ  = wiringpimock.o

LD_FLAGS  = $(LIB_DIR) $(LIBS)
CFLAGS    = -g $(INC_DIR)

SRC       = gpiod.c
OBJ       = $(SRC:.c=.o)

gpiod: $(OBJ)
	cc -o $@ gpiod.o $(LD_FLAGS)

gpiod_mock: $(OBJ) $(MOCK_OBJ)
	cc -o $@  gpiod.o $(MOCK_OBJ)

.c.o:
	cc $(CFLAGS) $(EXTRA_CFLAGS) -c $< 

clean:
	rm -f *~ gpiod gpiod.testreport $(MOCK_BIN) $(MOCK_OBJ) $(OBJ)

test: $(MOCK_BIN)
	@./test.sh
