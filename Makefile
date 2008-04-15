CC ?= gcc

.PHONY: clean all

all: Makefile.local
	$(MAKE) -C utils
	$(MAKE) -C polyselect
	(cd postsieve/tifa; scons CC=$(CC))
	$(MAKE) -C sieve
	$(MAKE) -C postsieve/checknorms
	$(MAKE) -C linalg
	$(MAKE) -C sqrt/naive
	$(MAKE) -C gf2x
	$(MAKE) -C gf2x/cantor
	$(MAKE) -C linalg/bw
	$(MAKE) -C linalg/bl	FAKE=1

clean:
	$(MAKE) -C utils		clean
	$(MAKE) -C polyselect		clean
	(cd postsieve/tifa; scons -c)
	$(MAKE) -C sieve		clean
	$(MAKE) -C postsieve/checknorms	clean
	$(MAKE) -C linalg		clean
	$(MAKE) -C sqrt/naive		clean
	$(MAKE) -C gf2x			clean
	$(MAKE) -C gf2x/cantor		clean
	$(MAKE) -C linalg/bw		clean
	$(MAKE) -C linalg/bl		clean

Makefile.local:
	@echo "Makefile.local does not exist. Let's create an empty one."
	touch Makefile.local

