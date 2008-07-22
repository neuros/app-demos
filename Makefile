all:
	@$(MAKE) -C audio-passthrough
	@$(MAKE) -C fb
	@$(MAKE) -C video
	@$(MAKE) -C ir-blaster-test
	@$(MAKE) -C test-scripts

install:all
	@$(MAKE) -C audio-passthrough install
	@$(MAKE) -C fb install
	@$(MAKE) -C video install
	@$(MAKE) -C ir-blaster-test install
	@$(MAKE) -C test-scripts install

clean:
	@$(MAKE) -C audio-passthrough clean
	@$(MAKE) -C fb clean
	@$(MAKE) -C video clean
	@$(MAKE) -C ir-blaster-test clean
	@$(MAKE) -C test-scripts clean
