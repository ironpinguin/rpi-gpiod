all: 
	$(MAKE) --directory src gpiod_mock
test:
	$(MAKE) --directory src test
gpiod:
	$(MAKE) --directory src gpiod
gpiod_mock:
	$(MAKE) --directory src gpiod_mock
gpiod_glmock:
	$(MAKE) --directory src gpiod_glmock
install:
	$(MAKE) --directory src install
uninstall:
	$(MAKE) --directory src uninstall
clean: 
	$(MAKE) --directory src clean
