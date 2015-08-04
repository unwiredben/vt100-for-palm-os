# $Id: Makefile,v 1.15 1998/02/24 20:03:34 swetland Exp $
thisdir := $(shell basename $$PWD)

#### Make macros to substitute the associated macros in .in files ###
# The Significant Us
BRIAN_MAIL=swetland@frotz.net
VASSILII_MAIL=vassilii@tarunz.org
BRIAN_URL=http://www.frotz.net/swetland/
VASSILII_URL=http://www.tarunz.org/~vassilii/
BRIAN_NAME=Brian J. Swetland
VASSILII_NAME=Vassilii Khachaturov
export BRIAN_MAIL BRIAN_URL BRIAN_NAME
export VASSILII_MAIL VASSILII_URL VASSILII_NAME
##########################################################################

### app config

GLUELIB=/usr/share/prc-tools/sdk/lib/m68k-palmos-coff/libPalmOSGlue.a
OBJS = term.o vt100.o keymap.o $(GLUELIB)
APP = vt100
export APP
PRC = $(APP).prc
export PRC
RSC = $(APP).rsc
APPID = vt1x
ICONTEXT = Pilot VT100
export ICONTEXT
DEVICE = /dev/pilot # symlink to wherever pilot is attached
CODEVICE = /dev/copilot # symlink to /dev/ttyqe
PORT=2000
#PALMINCS=/usr/local/m68k-palmos-coff/include/PalmOS2
PALMINCS=/usr/share/prc-tools/sdk/include
RELEASE=1.0
export RELEASE

### tool definitions

CC = m68k-palmos-gcc
PILRC = pilrc
OBJRES = m68k-palmos-obj-res
BUILDPRC = build-prc
CFLAGS = -O2 -Wall -W -I$(PALMINCS) -DAPPID=$(APPID)
GDB=m68k-palmos-gdb
PILOT_XFER=pilot-xfer
COPILOT = cd ..; xcopilot512 -double -serial

### Do useful stuff down here

all: .rsrc $(PRC) web

$(APP): $(OBJS)
	$(CC) $(CFLAGS) -o $(APP) $(OBJS)

.rsrc/code.stamp: .rsrc $(APP)
	@cp $(APP) .rsrc
	cd .rsrc ; $(OBJRES) $(APP)
	@touch .rsrc/code.stamp

$(RSC): $(RSC).in
	sed \
	-e 's/@PRC@/$(PRC)/g' \
	-e 's/@ICONTEXT@/$(ICONTEXT)/g' \
	-e 's/@RELEASE@/$(RELEASE)/g' \
	-e 's/@BRIAN_MAIL@/$(BRIAN_MAIL)/g' \
	-e 's/@BRIAN_NAME@/$(BRIAN_NAME)/g' \
	-e 's/@BRIAN_URL@/$(subst /,\/,$(BRIAN_URL))/g' \
	-e 's/@VASSILII_MAIL@/$(VASSILII_MAIL)/g' \
	-e 's/@VASSILII_NAME@/$(VASSILII_NAME)/g' \
	-e 's/@VASSILII_URL@/$(subst /,\/,$(VASSILII_URL))/g' \
	$< > $@

.rsrc/bin.stamp: .rsrc $(RSC)
	$(PILRC) -q $(RSC) .rsrc
	@touch .rsrc/bin.stamp

$(PRC): .rsrc/code.stamp .rsrc/bin.stamp
	cd .rsrc ; $(BUILDPRC) ../$@ "$(ICONTEXT)" $(APPID) *.grc *.bin

.rsrc: 
	@mkdir .rsrc

### Maintenance
.PHONY: clean debug install codebug coinstall tar gzip

clean: 
	rm -rf *~ *.o $(PRC) $(APP) .rsrc createdb *.pdb font.h fcompile $(RSC)
	cd docs ; $(MAKE) clean

install: $(PRC)
	$(PILOT_XFER) $(DEVICE) -i $(PRC)

debug:
	echo target pilot $(DEVICE) > .gdbinit
	echo "Start $(ICONTEXT) on pilot"
	$(GDB) $(PROG)

coinstall: $(PRC)
	PILOTRATE=38400 PILOTPORT=$(CODEVICE) $(PILOT_XFER) -i $(PRC)

codebug:
	echo target pilot localhost:$(PORT) > .gdbinit
	$(COPILOT) -gdebug :$(PORT) &
	echo "Start $(ICONTEXT) on copilot"
	sleep 15
	$(GDB) $(PROG)

tar:	../$(thisdir).tar

../$(thisdir).tar:	clean
	-rm -f $@
	cd ..; tar cvf $(thisdir).tar $(thisdir)

gzip:	../$(thisdir).tar.gz

../$(thisdir).tar.gz: ../$(thisdir).tar
	-rm -f $@
	gzip --best $<

### Explicit dependencies
.rsrc/bin.stamp: icon.pbm font.pbm rsrc.h
term.o vt100.o: vt100.h
term.o keymap.o: rsrc.h
term.o keymap.o: keymap.h
term.o keymap.o: chcodes.h

web::
	cd docs ; $(MAKE) all

### end
