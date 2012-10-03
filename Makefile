# Makefile

PACKAGE:=whoosh

CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
OBJCOPY=$(CROSS_COMPILE)objcopy

CFLAGS:=-g -Os -nostartfiles -nostdlib
LDFLAGS:= -g -Ttext 0 -X -Map whoosh.map

SRCS:= script.lds main.c
OBJS:=$(SRCS:.c=.o)

all: $(PACKAGE).bin $(PACKAGE).bin-swapped

$(PACKAGE).bin: $(PACKAGE)
	$(OBJCOPY) -O binary -R .bss -R .comment $< $@

$(PACKAGE).bin-swapped: $(PACKAGE).bin
	cat $< | ./wordswap > $@

$(PACKAGE): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.s: %.c
	$(CC) -S $(CFLAGS) $< -o $@

%_config:
	@cp configs/$(@:_config=).h config.h

.PHONY: clean distclean
clean:
	@-rm -rf *.o $(PACKAGE) $(PACKAGE).bin $(PACKAGE).bin-swapped *.map out

distclean: clean
	@-rm -f config.h
