// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#include <libparam/libparam.h>
//vp - to remove compilation warnings
//#include <src/modules/disksim_ioqueue_param.h>

#ifndef _DISKSIM_SSD_PARAM_H
#define _DISKSIM_SSD_PARAM_H

#ifdef __cplusplus
extern"C"{
#endif

#define DISKSIM_SSD_MAX       10


/* prototype for disksim_ssd param loader function */
   struct ssd *disksim_ssd_loadparams(struct lp_block *b);

typedef enum {
   DISKSIM_SSD_SCHEDULER,
   DISKSIM_SSD_MAX_QUEUE_LENGTH,
   DISKSIM_SSD_BLOCK_COUNT,
   DISKSIM_SSD_BUS_TRANSACTION_LATENCY,
   DISKSIM_SSD_BULK_SECTOR_TRANSFER_TIME,
   DISKSIM_SSD_NEVER_DISCONNECT,
   DISKSIM_SSD_PRINT_STATS,
   DISKSIM_SSD_COMMAND_OVERHEAD,
   DISKSIM_SSD_MODEL_TYPE,
   DISKSIM_SSD_NELEMENTS,
   DISKSIM_SSD_FLASH_PAGE_SIZE,
   DISKSIM_SSD_PAGES_PER_BLOCK,
   DISKSIM_SSD_BLOCKS_PER_ELEMENT,
   DISKSIM_SSD_ELEMENT_STRIDE,
   DISKSIM_SSD_CHIP_XFER_LATENCY,
   DISKSIM_SSD_PAGE_READ_LATENCY,
   DISKSIM_SSD_PAGE_WRITE_LATENCY,
   DISKSIM_SSD_BLOCK_ERASE_LATENCY,

   //vp
   DISKSIM_SSD_WRITE_POLICY,
   DISKSIM_SSD_RESERVE_BLOCKS,
   DISKSIM_SSD_MIN_FREEBLKS_PERCENTAGE,
   DISKSIM_SSD_CLEANING_POLICY,
   DISKSIM_SSD_PLANES_PER_PKG,
   DISKSIM_SSD_BLOCKS_PER_PLANE,
   DISKSIM_SSD_PLANE_BLOCK_MAPPING,
   DISKSIM_SSD_COPY_BACK,
   DISKSIM_SSD_NUM_PARALLEL_UNITS,
   DISKSIM_SSD_ELEMENTS_PER_GANG,
   DISKSIM_SSD_CLEANING_IN_BACKGROUND,
   DISKSIM_SSD_GANG_SHARE,
   DISKSIM_SSD_ALLOC_POOL_LOGIC
} disksim_ssd_param_t;


#define DISKSIM_SSD_MAX_PARAM       (DISKSIM_SSD_ALLOC_POOL_LOGIC + 1)
extern void * DISKSIM_SSD_loaders[];
extern lp_paramdep_t DISKSIM_SSD_deps[];


static struct lp_varspec disksim_ssd_params [] = {
   {"Scheduler", BLOCK, 1 },
   {"Max queue length", I, 1 },
   {"Block count", I, 0 },
   {"Bus transaction latency", D, 0 },
   {"Bulk sector transfer time", D, 1 },
   {"Never disconnect", I, 1 },
   {"Print stats", I, 1 },
   {"Command overhead", D, 1 },
   {"Timing model", I, 1 },
   {"Flash chip elements", I, 1 },
   {"Page size", I, 1 },
   {"Pages per block", I, 1 },
   {"Blocks per element", I, 1 },
   {"Element stride pages", I, 1 },
   {"Chip xfer latency", D, 1 },
   {"Page read latency", D, 1 },
   {"Page write latency", D, 1 },
   {"Block erase latency", D, 1 },

   //vp - new parameters
   {"Write policy", I, 1},
   {"Reserve pages percentage", I, 1},
   {"Minimum free blocks percentage", I, 1},
   {"Cleaning policy", I, 1},
   {"Planes per package", I, 1},
   {"Blocks per plane", I, 1},
   {"Plane block mapping", I, 1},
   {"Copy back", I, 1},
   {"Number of parallel units", I, 1},
   {"Elements per gang", I, 1},
   {"Cleaning in background", I, 1},
   {"Gang share", I, 1},
   {"Allocation pool logic", I, 1},
   {0,0,0}
};
static struct lp_mod disksim_ssd_mod = { "disksim_ssd", disksim_ssd_params, DISKSIM_SSD_MAX_PARAM, (lp_modloader_t)disksim_ssd_loadparams,  0, 0, DISKSIM_SSD_loaders, DISKSIM_SSD_deps };


#ifdef __cplusplus
}
#endif
#endif // _DISKSIM_SSD_PARAM_H
