EE_BIN  = libxmptest.elf
EE_OBJS = libxmptest.o
EE_LIBS = -laudsrv -L../lib -lxmp-lite -ldebug
EE_CFLAGS = -std=gnu99 -I../include/libxmp-lite -fno-builtin-printf

all: $(EE_BIN) audsrv.irx

audsrv.irx:
	cp $(PS2SDK)/iop/irx/audsrv.irx $@

run: $(EE_BIN)
	ps2client reset
	sleep 5
	ps2client execee host:$(EE_BIN).elf
frun: $(EE_BIN)
	ps2client execee host:$(EE_BIN).elf

reset:
	ps2client reset

clean:
	rm -f $(EE_OBJS) $(EE_BIN) audsrv.irx

install: all

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
