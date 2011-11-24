# DiskSim SSD support
# ©2008 Microsoft Corporation. All Rights Reserved

include .paths

LDFLAGS = -lm -L. -ldisksim $(DISKMODEL_LDFLAGS) $(MEMSMODEL_LDFLAGS) \
                            $(LIBPARAM_LDFLAGS) $(LIBDDBG_LDFLAGS) 

HP_FAST_OFLAGS = +O4
NCR_FAST_OFLAGS = -O4 -Hoff=BEHAVED 
FREEBLOCKS_OFLAGS =
DEBUG_OFLAGS = -g -DASSERTS # -DDEBUG=1
PROF_OFLAGS = -g -DASSERTS -p
GPROF_OFLAGS = -g -DASSERTS -pg
CFLAGS = $(DEBUG_OFLAGS) $(DISKSIM_CFLAGS) $(DISKMODEL_CFLAGS) -I../ -I$(DISKSIM_PREFIX)/src

#CC = cc
CC = gcc -Wall -Wno-unused -MD
# because purify spits out warnings on stdout...
CC-DEP = gcc $(LIBPARAM_CFLAGS) $(DISKMODEL_CFLAGS)

# The following lines create a dependency target based on the state of 
# the modules files. If the .c and .h files are not created, the dependency
# target is modules which invokes "make -C modules". If the files are already
# created, the target are the files themselves. 
# This expression is to avoid remaaking of the libdisksim.a with ar and ranlib
# even if no files have changed. 
MODULEDEPS = $(wildcard modules/*.h modules/*.c)
ifeq ($(MODULEDEPS),)
MODULEDEPS = modules
endif

all: libssdmodel.a

clean:
	rm -f TAGS *.o libssdmodel.a
	$(MAKE) -C modules clean

realclean: clean
	rm -f *.d .depend
	$(MAKE) -C modules clean

distclean: realclean
	rm -f *~ *.a
	rm -rf ../lib ../include
	$(MAKE) -C modules distclean

.PHONY: modules

#modules: modules/* ;

modules: 
	$(MAKE) -C modules
	mkdir -p include/ssdmodel/modules
	cp -pR modules/*.h include/ssdmodel/modules
	cp ssd.h include/ssdmodel

modules/*.h modules/*.c:
	$(MAKE) -C modules

-include *.d

DISKSIM_SSD_SRC = ssd.c ssd_timing.c ssd_clean.c \
			    ssd_gang.c ssd_init.c ssd_utils.c 

DISKSIM_SSD_OBJ = $(DISKSIM_SSD_SRC:.c=.o) 

$(DISKSIM_SSD_OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@


# production rule for making disksim with freeblocks support
# FREEBLOCKS_OFLAGS = -DFREEBLOCKS
#fbdisksim :
#	$(MAKE) FREEBLOCKS_OFLAGS=-DFREEBLOCKS all

#rms : rms.c
#	$(CC) $< -lm -o $@

#hplcomb : hplcomb.c
#	$(CC) $< -o $@

libssdmodel.a: $(MODULEDEPS) $(DISKSIM_SSD_OBJ) modules/ssdmodel_ssd_param.o
	ar cru $@ $(DISKSIM_SSD_OBJ) modules/ssdmodel_ssd_param.o
	ranlib $@
	mkdir -p lib
	cp libssdmodel.a lib

########################################################################

# rule to automatically generate dependencies from source files
#%.d: %.c
#	set -e; $(CC-DEP) -M $(CPPFLAGS) $<  \
#		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
#		[ -s $@ ] 2>/dev/null >/dev/null || rm -f $@


# this is a little less aggressive and annoying than the above
depend: .depend 

.depend: *.c *.h
	$(MAKE) -C modules
	rm -f .depend
	$(foreach file, $(DISKSIM_SSD_SRC), \
		$(CC-DEP) $(CFLAGS) -M $(file) >> .depend; )

