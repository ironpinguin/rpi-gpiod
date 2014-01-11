INC_DIR   = -I../../wiringPi/wiringPi -I../lib/wiringPi/wiringPi -I../lib/rpi-dog128/src
LIB_DIR   = -L/usrl/local/lib -L../lib/rpi-dog128/src

LIBS      = -lwiringPi -lpthread -ldog128 -lconfig

SHLIB_EXT = so

INSTALL_DIR = /usr/local/bin

MOCK_BIN  = gpiod_mock
MOCK_OBJ  = wiringpimock.o
MOCK_LIB = libwiringPi_mock.$(SHLIB_EXT)

LD_FLAGS  = $(LIB_DIR) $(LIBS)
CFLAGS    = -Wall -g $(INC_DIR) -fPIC

SRC       = gpiod.c
OBJ       = $(SRC:.c=.o)


gpiod: $(OBJ)
	$(CC) -o $@ gpiod.o $(LD_FLAGS) 

mock_lib: ../lib/rpi-dog128/src/libwiringPi_mock.a
	$(MAKE) --directory ../lib/rpi-dog128 mock

glmock_lib: ../lib/rpi-dog128/src/libwiringPi_glmock.so
	$(MAKE) --directory ../lib/rpi-dog128 glmock

$(MOCK_LIB): $(MOCK_OBJ)
	$(CC) -shared -o $@ $(MOCK_OBJ)

gpiod_mock: mock_lib $(OBJ) 
	$(CC) -o $@  gpiod.o  ../lib/rpi-dog128/src/libwiringPi_mock.a -L. -lpthread -lconfig

gpiod_glmock: glmock_lib $(OBJ)
	$(CC) -o $@  gpiod.o  -lwiringPi_glmock $(CFLAGS) $(LIB_DIR) -lpthread -ldog128 -lconfig

.c.o:
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $<

clean:
	rm -f *~ gpiod gpiod.testreport $(MOCK_BIN) $(MOCK_OBJ) $(OBJ) $(MOCK_LIB)

install: install-bin

install-bin:
	@echo "[Install Binary]"
	@install -m 0755 gpiod $(INSTALL_DIR)/

uninstall:
	@echo "[UnInstall]"
	@rm -f $(INSTALL_DIR)/gpiod

test test-mock-lib: gpiod $(MOCK_LIB)
	@./test.sh --with-mock-lib


test-mock-bin: $(MOCK_BIN)
	@./test.sh --with-mock-bin

test-real-pi: $(MOCK_LIB)
	@./test.sh
