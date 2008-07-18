DATA=320x240_duck.rgb565 \
720x480_bars.uyvy \
720x576_test.uyvy

TARGET_DATA_DIR=/usr/local/share/test/
TARGET_BIN_DIR=/usr/local/bin/test/
DATA_DIR=$(ROOTFS)/fs/$(TARGET_DATA_DIR)
BIN_DIR=$(ROOTFS)/fs/$(TARGET_BIN_DIR)
fb-demo: main.c
	arm-linux-gcc -Wall -o $@ main.c -I$(KNL_INSTALL_DIR)/include -DDATA_DIR="\"$(TARGET_DATA_DIR)\""

install: fb-demo
	if test ! -e $(BIN_DIR); then mkdir -p $(BIN_DIR); fi
	cp $? $(BIN_DIR)
	if test ! -e $(DATA_DIR); then mkdir -p $(DATA_DIR); fi
	cp $(DATA) $(DATA_DIR)
clean:
	rm fb-demo
