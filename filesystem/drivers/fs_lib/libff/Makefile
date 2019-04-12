
ROOTDIR = $(N64_INST)


INSTALLDIR = $(ROOTDIR)/mips64-elf

INCPATH = -I$(CURDIR)/include -I$(ROOTDIR)/mips64-elf/include -I$(ROOTDIR)/mips64-elf/mips64-elf/include
CFLAGS = -O2 -G 0 -Wall -g -mtune=vr4300 -march=vr4300 -D_GCC_LIMITS_H_ $(INCPATH) -frename-registers
ASFLAGS = -mtune=vr4300 -march=vr4300

N64PREFIX = $(ROOTDIR)/mips64-elf/bin/mips64-elf-
CC = $(N64PREFIX)gcc
AS = $(N64PREFIX)as
LD = $(N64PREFIX)ld
AR = $(N64PREFIX)ar
RANLIB = $(N64PREFIX)ranlib
RM = rm -rf

libff.a_OBJS = ff.o diskio.o option/ccsbcs.o

all: $(libff.a_OBJS) libff.a

%.a: $(libff.a_OBJS)
	$(RM) $@
	$(AR) cru $@ $($@_OBJS)
	$(RANLIB) $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o *.a option/*.o

install: all
	install -D --mode=644 ff.h $(INSTALLDIR)/include
	install -D --mode=644 ffconf.h $(INSTALLDIR)/include
	install -D --mode=644 integer.h $(INSTALLDIR)/include
	install -D --mode=644 diskio.h $(INSTALLDIR)/include
	install -D --mode=644 libff.a $(INSTALLDIR)/lib
