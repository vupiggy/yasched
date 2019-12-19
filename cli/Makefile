### Makefile --- 

## Author: <lukehuang.ca@gmail.com>

# Add your targets here 
TARGETS = yasub yasadmin
CFLAGS += -I ../include -g

default: $(TARGETS)

yasub : yasub.o
	$(call cmd,link)

yasadmin : LDFLAGS += -I/usr/local/include -L/usr/local/lib -ledit
yasadmin : yasadmin.o ioctl.o opts.o common.o yash.o procfs.o
	$(call cmd,link)

include ../Makefile.in

clean:
	$(call cmd,sclean)

.PHONY: default

### Makefile ends here
