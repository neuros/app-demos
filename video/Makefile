TARGET_BIN_DIR=/usr/local/bin/
BIN_DIR=$(ROOTFS)/fs/$(TARGET_BIN_DIR)

video-demo: main.c $(shell pwd)/../fb/fblib.o
	$(CC) -Wall -o $@ main.c $(shell pwd)/../fb/fblib.o -I$(KNL_INSTALL_DIR)/include -I$(shell pwd)/../fb/

install: video-demo
	if test ! -e $(BIN_DIR); then mkdir -p $(BIN_DIR); fi
	cp $? $(BIN_DIR)
clean:
	@rm -f video-demo
