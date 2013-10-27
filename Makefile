all: 
	$(MAKE) --directory src gpiod_mock
test:
	$(MAKE) --directory src test
gpiod:
	$(MAKE) --directory src gpiod
gpiod_mock:
	$(MAKE) --directory src gpiod_mock