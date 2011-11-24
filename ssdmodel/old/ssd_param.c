// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#include "ssd_param.h"
#include <libparam/bitvector.h>
#include "../ssdmodel/ssd.h"
#include "../ssdmodel/ssd_timing.h"

static int DISKSIM_SSD_SCHEDULER_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_SCHEDULER_loader(
                struct ssd * result, struct lp_block *blk) {
   int i;
   result->queue =
                  (struct ioq *)disksim_ioqueue_loadparams(
                        blk, device_printqueuestats, device_printcritstats,
                        device_printidlestats, device_printintarrstats,
                        device_printsizestats);
   assert(result->queue != NULL);

   /*
   // queue for the gangs
   //printf("Total gang = %d\n", SSD_NUM_GANG(result));
   for (i = 0; i < SSD_MAX_ELEMENTS; i ++) {
         result->gang_meta[i].queue =
                    (struct ioq *)disksim_ioqueue_loadparams(
                          blk, device_printqueuestats, device_printcritstats,
                          device_printidlestats, device_printintarrstats,
                          device_printsizestats);
         assert(result->gang_meta[i].queue != NULL);
   }

   for (i=0;i<SSD_MAX_ELEMENTS;i++) {

        result->elements[i].queue =
                  (struct ioq *)disksim_ioqueue_loadparams(
                        blk, device_printqueuestats, device_printcritstats,
                        device_printidlestats, device_printintarrstats,
                        device_printsizestats);
        assert(result->elements[i].queue != NULL);
    }
   */
}

static int DISKSIM_SSD_MAX_QUEUE_LENGTH_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_MAX_QUEUE_LENGTH_loader(struct ssd * result, int i) {
if (! (i >= 0)) { // foo
 }
 result->maxqlen = i;

}

static int DISKSIM_SSD_BLOCK_COUNT_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_BLOCK_COUNT_loader(struct ssd * result, int i) {
if (! (i >= 1)) { // foo
  }
  result->numblocks = i;
}

static int DISKSIM_SSD_BUS_TRANSACTION_LATENCY_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_BUS_TRANSACTION_LATENCY_loader(struct ssd * result, double d) {
if (! (d >= 0.0)) { // foo
 }
 result->bus_transaction_latency = d;

}

static int DISKSIM_SSD_BULK_SECTOR_TRANSFER_TIME_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_BULK_SECTOR_TRANSFER_TIME_loader(struct ssd * result, double d) {
if (! (d >= 0.0)) { // foo
 }
 result->blktranstime = d;

}

static int DISKSIM_SSD_NEVER_DISCONNECT_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_NEVER_DISCONNECT_loader(struct ssd * result, int i) {
if (! (RANGE(i,0,1))) { // foo
 }
 result->neverdisconnect = i;

}

static int DISKSIM_SSD_PRINT_STATS_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_PRINT_STATS_loader(struct ssd * result, int i) {
if (! (RANGE(i,0,1))) { // foo
 }
 result->printstats = i;

}

static int DISKSIM_SSD_COMMAND_OVERHEAD_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_COMMAND_OVERHEAD_loader(struct ssd * result, double d) {
if (! (d >= 0.0)) { // foo
 }
 result->overhead = d;
}

static int DISKSIM_SSD_MODEL_TYPE_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_MODEL_TYPE_loader(struct ssd * result, int i) {
if (! (RANGE(i,SSD_SIMPLE_MODEL,SSD_LOGDISK_MODEL))) { // foo
 }
 result->params.ssd_model = i;
}

static int DISKSIM_SSD_FLASH_CHIP_ELEMENTS_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_FLASH_CHIP_ELEMENTS_loader(struct ssd * result, int i) {
if (! (RANGE(i,1,SSD_MAX_ELEMENTS))) { // foo
 }
 result->params.nelements = i;
}
static int DISKSIM_SSD_FLASH_PAGE_SIZE_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_FLASH_PAGE_SIZE_loader(struct ssd * result, int i) {
if (! (i>=8)) { // foo
 }
 result->params.page_size = i;
}
static int DISKSIM_SSD_PAGES_PER_BLOCK_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_PAGES_PER_BLOCK_loader(struct ssd * result, int i) {
if (! (i>0)) { // foo
 }
 result->params.pages_per_block = i;
}

static int DISKSIM_SSD_BLOCKS_PER_ELEMENT_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_BLOCKS_PER_ELEMENT_loader(struct ssd * result, int i) {
if (! (i>0)) { // foo
 }
 result->params.blocks_per_element = i;
}

static int DISKSIM_SSD_ELEMENT_STRIDE_PAGES_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_ELEMENT_STRIDE_PAGES_loader(struct ssd * result, int i) {
if (! (i>=8)) { // foo
 }
 result->params.element_stride_pages = i;
}
static int DISKSIM_SSD_CHIP_XFER_LATENCY_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_CHIP_XFER_LATENCY_loader(struct ssd * result, double d) {
if (! (d >= 0.0)) { // foo
 }
 result->params.chip_xfer_latency = d;
}

static int DISKSIM_SSD_PAGE_READ_LATENCY_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_PAGE_READ_LATENCY_loader(struct ssd * result, double d) {
if (! (d >= 0.0)) { // foo
 }
 result->params.page_read_latency = d;
}

static int DISKSIM_SSD_PAGE_WRITE_LATENCY_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_PAGE_WRITE_LATENCY_loader(struct ssd * result, double d) {
if (! (d >= 0.0)) { // foo
 }
 result->params.page_write_latency = d;
}
static int DISKSIM_SSD_BLOCK_ERASE_LATENCY_depend(char *bv) {
return -1;
}

static void DISKSIM_SSD_BLOCK_ERASE_LATENCY_loader(struct ssd * result, double d) {
if (! (d >= 0.0)) { // foo
 }
 result->params.block_erase_latency = d;
}

//vp
static int DISKSIM_SSD_WRITE_POLICY_depend(char *bv) {
    return -1;
}

static void DISKSIM_SSD_WRITE_POLICY_loader(struct ssd *result, int i) {
    if (i > DISKSIM_SSD_MAX_WRITE_POLICIES) {
        fprintf(stderr, "Error: invalid write policy %d in DISKSIM_SSD_WRITE_POLICY_loader\n", i);
        exit(1);
    }
    result->params.write_policy = i;
}

static int DISKSIM_SSD_RESERVE_BLOCKS_depend(char *bv)
{
    return -1;
}

static void DISKSIM_SSD_RESERVE_BLOCKS_loader(struct ssd *result, int i)
{
    if ((i < 0) || (i > DISKSIM_SSD_MAX_RESERVE)) {
        fprintf(stderr, "Error: invalid Reserve blocks percentage %d\n", i);
        exit(1);
    }
    result->params.reserve_blocks = i;
}

static int DISKSIM_SSD_MIN_FREEBLKS_PERCENTAGE_depend(char *bv)
{
    return -1;
}

static void DISKSIM_SSD_MIN_FREEBLKS_PERCENTAGE_loader(struct ssd *result, int i)
{
    result->params.min_freeblks_percent = i;
}


static int DISKSIM_SSD_CLEANING_POLICY_depend(char *bv)
{
    return -1;
}

static void DISKSIM_SSD_CLEANING_POLICY_loader(struct ssd *result, int i)
{
    if (i < 0) {
        fprintf(stderr, "Error: invalid cleaning policy %d\n", i);
        exit(1);
    }

    result->params.cleaning_policy = i;
}

static int DISKSIM_SSD_PLANES_PER_PKG_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_PLANES_PER_PKG_loader(struct ssd *result, int i) {
    if ((i <= 0) || (i > SSD_MAX_PLANES_PER_ELEM)) {
        fprintf(stderr, "Error: invalid planes per package %d\n", i);
        exit(1);
    }
    result->params.planes_per_pkg = i;
}



static int DISKSIM_SSD_BLOCKS_PER_PLANE_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_BLOCKS_PER_PLANE_loader(struct ssd *result, int i) {
    if (i <= 0) {
        fprintf(stderr, "Error: invalid blocks per plane %d\n", i);
        exit(1);
    }
    result->params.blocks_per_plane = i;
}


static int DISKSIM_SSD_PLANE_BLOCK_MAPPING_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_PLANE_BLOCK_MAPPING_loader(struct ssd *result, int i) {
    result->params.plane_block_mapping = i;
}


static int DISKSIM_SSD_COPY_BACK_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_COPY_BACK_loader(struct ssd *result, int i) {
    result->params.copy_back = i;
}


static int DISKSIM_SSD_NUM_PARALLEL_UNITS_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_NUM_PARALLEL_UNITS_loader(struct ssd *result, int i) {
    result->params.num_parunits = i;
}

static int DISKSIM_SSD_ELEMENTS_PER_GANG_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_ELEMENTS_PER_GANG_loader(struct ssd *result, int i) {
    result->params.elements_per_gang = i;
}

static int DISKSIM_SSD_CLEANING_IN_BACKGROUND_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_CLEANING_IN_BACKGROUND_loader(struct ssd *result, int i) {
    result->params.cleaning_in_background = i;
}

static int DISKSIM_SSD_GANG_SHARE_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_GANG_SHARE_loader(struct ssd *result, int i) {
    result->params.gang_share = i;
}


static int DISKSIM_SSD_ALLOC_POOL_LOGIC_depend(char *bv) {
    return -1;
}
static void DISKSIM_SSD_ALLOC_POOL_LOGIC_loader(struct ssd *result, int i) {
    result->params.alloc_pool_logic = i;
}


void * DISKSIM_SSD_loaders[] = {
(void *)DISKSIM_SSD_SCHEDULER_loader,
(void *)DISKSIM_SSD_MAX_QUEUE_LENGTH_loader,
(void *)DISKSIM_SSD_BLOCK_COUNT_loader,
(void *)DISKSIM_SSD_BUS_TRANSACTION_LATENCY_loader,
(void *)DISKSIM_SSD_BULK_SECTOR_TRANSFER_TIME_loader,
(void *)DISKSIM_SSD_NEVER_DISCONNECT_loader,
(void *)DISKSIM_SSD_PRINT_STATS_loader,
(void *)DISKSIM_SSD_COMMAND_OVERHEAD_loader,
(void *)DISKSIM_SSD_MODEL_TYPE_loader,
(void *)DISKSIM_SSD_FLASH_CHIP_ELEMENTS_loader,
(void *)DISKSIM_SSD_FLASH_PAGE_SIZE_loader,
(void *)DISKSIM_SSD_PAGES_PER_BLOCK_loader,
(void *)DISKSIM_SSD_BLOCKS_PER_ELEMENT_loader,
(void *)DISKSIM_SSD_ELEMENT_STRIDE_PAGES_loader,
(void *)DISKSIM_SSD_CHIP_XFER_LATENCY_loader,
(void *)DISKSIM_SSD_PAGE_READ_LATENCY_loader,
(void *)DISKSIM_SSD_PAGE_WRITE_LATENCY_loader,
(void *)DISKSIM_SSD_BLOCK_ERASE_LATENCY_loader,

//vp
(void *)DISKSIM_SSD_WRITE_POLICY_loader,
(void *)DISKSIM_SSD_RESERVE_BLOCKS_loader,
(void *)DISKSIM_SSD_MIN_FREEBLKS_PERCENTAGE_loader,
(void *)DISKSIM_SSD_CLEANING_POLICY_loader,
(void *)DISKSIM_SSD_PLANES_PER_PKG_loader,
(void *)DISKSIM_SSD_BLOCKS_PER_PLANE_loader,
(void *)DISKSIM_SSD_PLANE_BLOCK_MAPPING_loader,
(void *)DISKSIM_SSD_COPY_BACK_loader,
(void *)DISKSIM_SSD_NUM_PARALLEL_UNITS_loader,
(void *)DISKSIM_SSD_ELEMENTS_PER_GANG_loader,
(void *)DISKSIM_SSD_CLEANING_IN_BACKGROUND_loader,
(void *)DISKSIM_SSD_GANG_SHARE_loader,
(void *)DISKSIM_SSD_ALLOC_POOL_LOGIC_loader,

NULL
};

lp_paramdep_t DISKSIM_SSD_deps[] = {
(void*)DISKSIM_SSD_SCHEDULER_depend,
(void*)DISKSIM_SSD_MAX_QUEUE_LENGTH_depend,
(void*)DISKSIM_SSD_BLOCK_COUNT_depend,
(void*)DISKSIM_SSD_BUS_TRANSACTION_LATENCY_depend,
(void*)DISKSIM_SSD_BULK_SECTOR_TRANSFER_TIME_depend,
(void*)DISKSIM_SSD_NEVER_DISCONNECT_depend,
(void*)DISKSIM_SSD_PRINT_STATS_depend,
(void*)DISKSIM_SSD_COMMAND_OVERHEAD_depend,
(void *)DISKSIM_SSD_MODEL_TYPE_depend,
(void *)DISKSIM_SSD_FLASH_CHIP_ELEMENTS_depend,
(void *)DISKSIM_SSD_FLASH_PAGE_SIZE_depend,
(void *)DISKSIM_SSD_PAGES_PER_BLOCK_depend,
(void *)DISKSIM_SSD_BLOCKS_PER_ELEMENT_depend,
(void *)DISKSIM_SSD_ELEMENT_STRIDE_PAGES_depend,
(void *)DISKSIM_SSD_CHIP_XFER_LATENCY_depend,
(void *)DISKSIM_SSD_PAGE_READ_LATENCY_depend,
(void *)DISKSIM_SSD_PAGE_WRITE_LATENCY_depend,
(void *)DISKSIM_SSD_BLOCK_ERASE_LATENCY_depend,

//vp
(void *)DISKSIM_SSD_WRITE_POLICY_depend,
(void *)DISKSIM_SSD_RESERVE_BLOCKS_depend,
(void *)DISKSIM_SSD_MIN_FREEBLKS_PERCENTAGE_depend,
(void *)DISKSIM_SSD_CLEANING_POLICY_depend,
(void *)DISKSIM_SSD_PLANES_PER_PKG_depend,
(void *)DISKSIM_SSD_BLOCKS_PER_PLANE_depend,
(void *)DISKSIM_SSD_PLANE_BLOCK_MAPPING_depend,
(void *)DISKSIM_SSD_COPY_BACK_depend,
(void *)DISKSIM_SSD_NUM_PARALLEL_UNITS_depend,
(void *)DISKSIM_SSD_ELEMENTS_PER_GANG_depend,
(void *)DISKSIM_SSD_CLEANING_IN_BACKGROUND_depend,
(void *)DISKSIM_SSD_GANG_SHARE_depend,
(void *)DISKSIM_SSD_ALLOC_POOL_LOGIC_depend,

NULL
};

