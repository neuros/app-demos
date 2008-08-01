all:ir-blaster-demo

ir-blaster-demo:
	arm-linux-gcc main.c blaster.c learning.c -I$(KNL_INSTALL_DIR)/include -o ir-blaster-demo
clean:
	rm -f *.o ir-blaster-demo
