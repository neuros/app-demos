ifndef PRJROOT
    $(error You must first source the BSP environment: "source neuros-env")
endif

INSTDIR=${INSTALL_MOD_PATH}/usr/local/bin/test

all:blaster-test

blaster-test:
	arm-linux-gcc main.c -o blaster-test
install:all
	@mkdir -p $(INSTDIR)
	@cp -a blaster-test $(INSTDIR)/
clean:
	rm -f *.o blaster-test
