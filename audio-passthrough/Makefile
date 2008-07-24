##############################################################################
# Description: Makefile for audio_test program
##############################################################################
ifndef PRJROOT
    $(error You must first source the BSP environment: "source neuros-env")
endif

EXECUTABLE=audio_test
OBJECTS=audio_test.o
INSTDIR=${INSTALL_MOD_PATH}/usr/local/bin/test

CROSSCOMPILE=arm-linux-
CC=$(CROSSCOMPILE)gcc
CFLAGS=-I$(KNL_INSTALL_DIR)/usr/include -Wall

.PHONY: install clean

all:$(EXECUTABLE)

install:all
	@mkdir -p ${INSTDIR}
	@cp -a $(EXECUTABLE) $(INSTDIR)/
$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)
clean:
	@rm -f $(OBJECTS) $(EXECUTABLE)
