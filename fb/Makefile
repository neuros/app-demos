DATA=320x240_duck.rgb565 \
720x480_bars.uyvy \
720x576_test.uyvy
SCRIPTS=fbhelper.sh

TARGET_DATA_DIR=/usr/local/share/test/
TARGET_BIN_DIR=/usr/local/bin/
DATA_DIR=$(ROOTFS)/fs/$(TARGET_DATA_DIR)
BIN_DIR=$(ROOTFS)/fs/$(TARGET_BIN_DIR)

all: fblib.o fb-demo

fblib.o: fblib.c
	arm-linux-gcc -Wall -c $? -I$(KNL_INSTALL_DIR)/include -DDATA_DIR="\"$(TARGET_DATA_DIR)\""

fb-demo: main.c fblib.o
	arm-linux-gcc -Wall -o $@ main.c fblib.o -I$(KNL_INSTALL_DIR)/include -DDATA_DIR="\"$(TARGET_DATA_DIR)\""

install: fb-demo
	if test ! -e $(BIN_DIR); then mkdir -p $(BIN_DIR); fi
	cp $? $(BIN_DIR)
	cp $(SCRIPTS) $(BIN_DIR)
	if test ! -e $(DATA_DIR); then mkdir -p $(DATA_DIR); fi
	cp $(DATA) $(DATA_DIR)
clean:
	rm fb-demo fblib.o
