# Path to root directory
ROOT ?= .

CC = dpu-upmem-dpurte-clang
LD = $(CC)

CFLAGS := -Wall -Wextra -Wno-unused-label -Wno-unused-function
# CFLAGS  += -g -w -O3
# CFLAGS  += -m32

TM := norec
SRCDIR := $(ROOT)/src
LIBDIR = $(ROOT)/lib
LIBNOREC := $(LIBDIR)/lib$(TM).a

BANK := bank

.PHONY:	all clean

all: $(LIBNOREC)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Additional dependencies
$(SRCDIR)/norec.o: $(SRCDIR)/norec.h $(SRCDIR)/thread_def.h $(SRCDIR)/utils.h


$(LIBNOREC): $(SRCDIR)/$(TM).o
	$(AR) crus $@ $^

test: $(LIBNOREC)
	$(MAKE) -C test

clean:
	rm -f $(LIBNOREC) $(SRCDIR)/*.o
	TARGET=clean $(MAKE) -C test