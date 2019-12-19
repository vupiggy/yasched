### Makefile ---

## Author: Luke Huang <lukehuang.ca@gmail.com>

SUBDIRS = cli lkm

include ./Makefile.in

doc:
	@echo "MK	documentation"
	@make -C documentation

clean:
	@for dir in $(SUBDIRS);			\
	 do					\
		echo "CL	$$dir";		\
		$(MAKE) -C $$dir clean;		\
		echo "";			\
	 done

### Makefile ends here
