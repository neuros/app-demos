ifndef PRJROOT
    $(error You must first source the BSP environment: "source neuros-env")
endif

INSTDIR=${INSTALL_MOD_PATH}/usr/local/bin/test

.PHONY: install clean

all:
install:
	@mkdir -p ${INSTDIR}
	@cp -a test_network.bat $(INSTDIR)/
	@cp -a t1 $(INSTDIR)/
	@cp -a v1 $(INSTDIR)/
	@cp -a v2 $(INSTDIR)/
	@cp -a a1 $(INSTDIR)/
	@cp -a a2 $(INSTDIR)/
	@cp -a a3 $(INSTDIR)/
clean:
