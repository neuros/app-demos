ifndef PRJROOT
    $(error You must first source the BSP environment: "source neuros-env")
endif

INSTDIR=${INSTALL_MOD_PATH}/usr/local/bin/test

.PHONY: install clean

all:
install:
	@mkdir -p ${INSTDIR}
	@cp -a test_network.bat $(INSTDIR)/
	@cp -a test_osd.sh $(INSTDIR)/
clean:
