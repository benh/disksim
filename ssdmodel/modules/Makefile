# DiskSim SSD support
# ©2008 Microsoft Corporation. All Rights Reserved


include .paths

CFLAGS=-g $(DISKSIM_CFLAGS) $(LIBDDBG_CFLAGS) $(LIBPARAM_CFLAGS) $(DISKMODEL_CFLAGS) $(SSDMODEL_CFLAGS) -I..

LIBPARAM=$(LIBPARAM_PREFIX)
PACKAGE=ssdmodel

all : all-redirect

# module parameter proto files
PARAM_PROTO = ssd.modspec

PARAM_CODE = $(PARAM_PROTO:%.modspec=$(PACKAGE)_%_param.c)

PARAM_OBJ = $(PARAM_PROTO:%.modspec=$(PACKAGE)_%_param.o)


PARAM_HEADERS = $(PARAM_PROTO:%.modspec=$(PACKAGE)_%_param.h)

$(PARAM_CODE): $(PACKAGE)_%_param.c: %.modspec 
	$(LIBPARAM)/mod.pl $(PACKAGE) $< 
	indent $@ || true

all-redirect: $(PARAM_OBJ) modules.h

modules.h: $(PARAM_HEADERS)
	$(LIBPARAM)/make_modules_h.pl $(PACKAGE) *.modspec > modules.h

clean:
	rm -f *.h *.c *.tex *~ *.o

distclean: clean
