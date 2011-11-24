#LDFLAGS = -lm
#HP_FAST_OFLAGS = +O4
#NCR_FAST_OFLAGS = -O4 -Hoff=BEHAVED
#DEBUG_OFLAGS = /Yd /DASSERTS /DFDEBUG
DEBUG_OFLAGS = /Zi /DASSERTS
#PROF_OFLAGS = -g -DASSERTS -p
#GPROF_OFLAGS = -g -DASSERTS -G
CFLAGS = $(DEBUG_OFLAGS)
#CC = cc
#CC = gcc -Wall
DISKSIM_OBJ = disksim_intr.obj disksim_cache.obj disksim_pfsim.obj disksim_pfdisp.obj\
	disksim_synthio.obj disksim_iotrace.obj disksim_iosim.obj disksim_logorg.obj\
	disksim_redun.obj disksim_ioqueue.obj disksim_iodriver.obj disksim_bus.obj\
	disksim_controller.obj disksim_ctlrdumb.obj disksim_ctlrsmart.obj\
	disksim_disk.obj disksim_diskctlr.obj disksim_diskcache.obj disksim_diskmap.obj\
	disksim_diskmech.obj disksim_stat.obj disksim_rand48.obj

all : disksim rms hplcomb # syssim

clean :
	rm -f *.obj disksim syssim rms hplcomb core

rms : rms.c
	$(CC) rms.c /Ferms

hplcomb : hplcomb.c
	$(CC) hplcomb.c /Fehplcomb

disksim : disksim.obj $(DISKSIM_OBJ)
	$(CC) $(CFLAGS) /Fedisksim disksim.obj $(DISKSIM_OBJ) $(LDFLAGS)

syssim : syssim_driver.obj disksim_main.obj disksim_interface.obj $(DISKSIM_OBJ)
	$(CC) $(CFLAGS) /Fesyssim syssim_driver.obj disksim_main.obj disksim_interface.obj $(DISKSIM_OBJ) $(LDFLAGS)

disksim_rand48.obj : disksim_rand48.c disksim_rand48.h ieee754.h
	$(CC) /c $(CFLAGS) disksim_rand48.c

disksim_stat.obj : disksim_stat.c disksim_stat.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_stat.c

disksim_diskmech.obj : disksim_diskmech.c disksim_disk.h disksim_stat.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_diskmech.c

disksim_diskmap.obj : disksim_diskmap.c disksim_disk.h disksim_stat.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_diskmap.c

disksim_diskcache.obj : disksim_diskcache.c disksim_disk.h disksim_stat.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_diskcache.c

disksim_diskctlr.obj : disksim_diskctlr.c disksim_disk.h disksim_stat.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_diskctlr.c

disksim_disk.obj : disksim_disk.c disksim_disk.h disksim_stat.h disksim_ioqueue.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_disk.c

disksim_ctlrsmart.obj : disksim_ctlrsmart.c disksim_controller.h disksim_cache.h disksim_ioqueue.h disksim_orgface.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_ctlrsmart.c

disksim_ctlrdumb.obj : disksim_ctlrdumb.c disksim_controller.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_ctlrdumb.c

disksim_controller.obj : disksim_controller.c disksim_controller.h disksim_orgface.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_controller.c

disksim_bus.obj : disksim_bus.c disksim_bus.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_bus.c

disksim_iodriver.obj : disksim_iodriver.c disksim_iodriver.h disksim_ioqueue.h disksim_orgface.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_iodriver.c

disksim_redun.obj : disksim_redun.c disksim_logorg.h disksim_orgface.h disksim_iosim.h disksim_stat.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_redun.c

disksim_logorg.obj : disksim_logorg.c disksim_logorg.h disksim_orgface.h disksim_iosim.h disksim_stat.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_logorg.c

disksim_ioqueue.obj : disksim_ioqueue.c disksim_ioqueue.h disksim_iosim.h disksim_stat.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_ioqueue.c

disksim_iosim.obj : disksim_iosim.c disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_iosim.c

disksim_iotrace.obj : disksim_iotrace.c disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_iotrace.c

disksim_synthio.obj : disksim_synthio.c disksim_pfsim.h disksim_synthio.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_synthio.c

disksim_pfdisp.obj : disksim_pfdisp.c disksim_pfsim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_pfdisp.c

disksim_pfsim.obj : disksim_pfsim.c disksim_ioface.h disksim_pfsim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_pfsim.c

disksim_cache.obj : disksim_cache.c disksim_cache.h disksim_iosim.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_cache.c

disksim_intr.obj : disksim_intr.c disksim_ioface.h disksim_pfface.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim_intr.c

disksim.obj : disksim.c disksim_ioface.h disksim_pfface.h disksim_global.h
	$(CC) /c $(CFLAGS) disksim.c

disksim_main.obj : disksim.c disksim_ioface.h disksim_pfface.h disksim_global.h
	$(CC) /c /Fodisksim_main.obj $(CFLAGS) -DEXTERNAL_MAIN disksim.c

disksim_interface.obj: disksim_interface.c disksim_global.h disksim_ioface.h syssim_driver.h
	$(CC) /c $(CFLAGS) disksim_interface.c

syssim_driver.obj: syssim_driver.c syssim_driver.h
	$(CC) /c $(CFLAGS) syssim_driver.c

