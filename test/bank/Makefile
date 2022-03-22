ROOT = ../..

CC = dpu-upmem-dpurte-clang
NR_TASKLETS = 24

TM := norec
SRCDIR := $(ROOT)/src

LIBDIR := $(ROOT)/lib
INCDIR := $(ROOT)/include
LIBNOREC := $(LIBDIR)/lib$(TM).a
TARGET := bank

CFLAGS += -Wall -Wno-unused-label -Wno-unused-function -std=c11

LDFLAGS += -L$(LIBDIR) -l$(TM)
CPPFLAGS += -I$(SRCDIR)

# DEFINES += -DTX_IN_MRAM
# DEFINES += -DRO_TX

.PHONY:	all clean

all: $(TARGET)

%.o: %.c
	$(CC) -DNR_TASKLETS=$(NR_TASKLETS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

bank.o:

$(TARGET): $(TARGET).o $(LIBNOREC)
	$(CC) -o $@ $< -DNR_TASKLETS=$(NR_TASKLETS) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o *.s