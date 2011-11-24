// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#include "disksim_global.h"
#include "disksim_iosim.h"
#include "disksim_stat.h"
#include "disksim_ioqueue.h"
#include "disksim_bus.h"
#include "config.h"

#ifndef DISKSIM_SSD_H
#define DISKSIM_SSD_H

struct ssd *getssd (int devno);

/* default ssd dev header */
extern struct device_header ssd_hdr_initializer;

//#define SSD_ASSERT_ALL                1

#define SSD_MAX_ELEMENTS            64
#define SSD_MAX_PLANES_PER_ELEM     16
#define SSD_MAX_PARUNITS_PER_ELEM   SSD_MAX_PLANES_PER_ELEM
#define SSD_MAX_ELEMS_PER_GANG      SSD_MAX_ELEMENTS// if you're changing this, do change the following bits too
#define SSD_BITS_ELEMS_PER_GANG     8


typedef struct {
   statgen acctimestats;
   double  requestedbus;
   double  waitingforbus;
   int     numbuswaits;
} ssd_stat_t;

/*
 * statistics per ssd element
 */
typedef struct _ssd_element_stat {
    int num_clean;                  // no of times cleaning has been invoked on this element
    int pages_moved;                // no of pages moved due to cleaning
    double tot_xfer_cost;           // total transfer cost across planes (must be 0 for cpbk)
    int avg_lifetime;               // average remaining lifetime on all the blocks
    int tot_reqs_issued;            // total requests issued at this chip
    double tot_time_taken;          // sum of access time of each req
    double tot_clean_time;          // sum of cleaning time
} ssd_element_stat;

/*
 * different states a block can be in.
 */
typedef enum {
  SSD_BLOCK_CLEAN,
  SSD_BLOCK_INUSE,
  SSD_BLOCK_SEALED
} SSD_BLOCK_STATES;


/*
 * this is the metadata stored on each block.
 * it contains a list of valid pages (in the real system,
 * we can find if a page in a block is valid by comparing
 * a version number stored on each block) and some stat
 * about its remaining lifetime to help wear-leveling and
 * cleaning program.
 */
typedef struct _block_metadata {
    int         block_num;              //
    int         plane_num;              // the plane to which this block belongs to

    int         *page;                  // size of the array = pages_per_block
                                        // holds all the valid logical page numbers present
                                        // in a block

    int         num_valid;              // represents the no of valid pages in the block (that is,
                                        // the no of non '-1' entries in the above page array)
                                        // don't use it as an index to add pages in the above array.

    int         rem_lifetime;           // remaining life time for this block. when the system
                                        // starts for the first time, this value will be initialized
                                        // to total num of erasures that can be performed on a block.
                                        // after every time this block is erased, this count is
                                        // decremented.

    double      time_of_last_erasure;   // holds the time when the last erasure was performed.

    int         state;                  // state of the block
                                        // a block could be in one of the following states:
                                        // CLEAN - when the block is erased and clean to take new writes
                                        // INUSE - when its pages are being filled
                                        // SEALED - when all the free pages are written and the block
                                        // is sealed for future writes

    unsigned int    bsn;                // block sequence number (version number for blocks)
} block_metadata;


/*
 * holds the plane specific metadata
 */
typedef struct _plane_metadata {

    int free_blocks;                // num of free blocks in this plane

    int valid_pages;                // num of valid pages (note that a block might not
                                    // be free but not all its pages should be valid)

    unsigned int active_page;       // this points to the next page to write inside an
                                    // active block.

    int clean_in_progress;          // a flag that is set to 1 when some cleaning is
                                    // going on in this plane
    int clean_in_block;             // which block is being cleaned?
    int block_alloc_pos;            // block allocation position in a plane
    int parunit_num;                // parallel unit number
    int num_cleans;                 // number of times cleaning was invoked on this
} plane_metadata;

typedef struct _parunit {
    int plane_to_clean;
} parunit;

/*
 * defining a structure to hold the metadata
 * of each ssd element.
 */
typedef struct _ssd_element_metadata {
    int *lba_table;                 // a table mapping the lba to the physical pages
                                    // on the chip.

    char *free_blocks;              // each bit indicates whether a block in the
                                    // ssd_element is free or in use. number of bits
                                    // in free_blocks is given by
                                    // (struct ssd*)->params.blocks_per_element

    unsigned int tot_free_blocks;   // total number of free blocks in the system. i'm
                                    // having this variable just for convenience. it can be
                                    // computed from the above free_blocks list.

    unsigned int active_page;       // this points to the next page to write inside an
                                    // active block.

    plane_metadata plane_meta[SSD_MAX_PLANES_PER_ELEM];

    block_metadata *block_usage;    // contains the number of valid pages in each block.
                                    // we also store the valid page numbers here. this is useful
                                    // during cleaning.

    unsigned int bsn;               // block sequence number for this ssd element

    int plane_to_clean;             // which plane to clean?
    int plane_to_write;             // which plane to write next?
    int block_alloc_pos;            // start allocating block from this position

    parunit parunits[SSD_MAX_PARUNITS_PER_ELEM];

    int gang_num;                   // the gang to which this element belongs
    int reqs_waiting;               //
    int tot_migrations;             //
    int tot_pgs_migrated;           //
    double mig_cost;                //
} ssd_element_metadata;

/*
 * a ssd plane consists of a bunch of blocks (2048 according to Samsung specifications)
 * and a register for transferring data in and out.
 */
typedef struct _ssd_plane {
    //struct ioq *queue;                // requests that are active on this plane
    //struct ioq *completion_queue;     // requests that complete in this plane

    int media_busy;                 // tells you whether the plane is currently being used or not
    int pair_plane;                 // certain planes are paired (in Samsung chips) and two-plane
                                    // operations can be performed only within a plane-pair.
    int num_blocks;                 // no of blocks in this plane.
} ssd_plane;


/*
 * the ssd element (a single package) internally consists of multiple
 * planes and the entire package has only one set of pins. so, when
 * transfering data to/from the package, the access to the pins must
 * be serialized. however, operations on multiple planes can be
 * performed concurrently.
 */
typedef struct _ssd_element {
   struct ioq *queue;
   int media_busy;
   ssd_element_metadata metadata;               // contains the mapping info b/w lba and flash pages
   ssd_element_stat stat;                       // statistics about each ssd element

   int pin_busy;                                // state to hold the busy state of the package pins
   int num_planes;                              // number of planes in this package
   ssd_plane plane[SSD_MAX_PLANES_PER_ELEM];    // an array of flash planes
} ssd_element;

typedef struct _ssd_elem_number {
    int e:SSD_BITS_ELEMS_PER_GANG;
} ssd_elem_number;

typedef struct _gang_metadata {
    struct ioq *queue;
    int cleaning;                               // set to 1 if cleaning is in progress
    int busy;                                   // set to 1 if at least one element is busy in this gang
    int reqs_waiting;                           // num of reqs waiting in this gang
    double oldest;                              // time at which the oldest of the waiting reqs arrived
    int elem_free_pages[SSD_MAX_ELEMENTS];      // free pages on each element
    ssd_elem_number *pg2elem;
} gang_metadata;


#define SSD_SIMPLE_MODEL                        1
#define SSD_SIMPLE_DW_MODEL                     2
#define SSD_LOGDISK_MODEL                       3

//vp - different write policies
#define DISKSIM_SSD_MAX_WRITE_POLICIES          2
#define DISKSIM_SSD_WRITE_POLICY_SIMPLE         1
#define DISKSIM_SSD_WRITE_POLICY_OSR            2

//vp - maximum percentage of pages one can reserve
//we don't want someone to set 100% of the space to reserve
#define DISKSIM_SSD_MAX_RESERVE                 50

//vp - sector size and metadata size per sector
#define SSD_DATA_BYTES_PER_SECTOR               512
#define SSD_MDATA_BYTES_PER_SECTOR              16
#define SSD_BYTES_PER_SECTOR                    (SSD_DATA_BYTES_PER_SECTOR + SSD_MDATA_BYTES_PER_SECTOR)

//vp - summary page size
#define SSD_SECTORS_PER_SUMMARY_PAGE            1

/////////////////////////////////////////////////////////////////
//vp - blocks in a plane can be arranged in different ways

// blocks can be concatenated (chained) from each plane
//
// plane 0    plane 1    plane 2    plane 3
// ------------------------------------------
// blk 0      blk 2048   blk 4096   blk 6144
// blk 1      blk 2049   blk 4097   blk 6145
// ...        ...
// blk 2047   blk 4095   blk 6143   blk 8191
#define PLANE_BLOCKS_CONCAT                 1


// blocks can be stripped across every pair of planes
// this is how samsung has designed its dies.
// according to the samsung chip design, blocks are
// stripped across every pair of planes. that is, the
// block numbers are arranged as follows.
//
// plane 0    plane 1    plane 2    plane 3
// ------------------------------------------
// blk 0      blk 1      blk 4096   blk 4097
// blk 2      blk 3      blk 4098   blk 4099
// ...        ...
// blk 4094   blk 4095   blk 8190   blk 8191
#define PLANE_BLOCKS_PAIRWISE_STRIPE        2


// blocks can be stripped across all the planes
//
// plane 0    plane 1    plane 2    plane 3
// ------------------------------------------
// blk 0      blk 1      blk 2      blk 3
// blk 4      blk 5      blk 6      blk 7
// ...        ...
// blk 8188   blk 8189   blk 8190   blk 8191
//
#define PLANE_BLOCKS_FULL_STRIPE            3


//vp - how many active pages are in an element
#define SSD_COPY_BACK_DISABLE                   0
#define SSD_COPY_BACK_ENABLE                    1

// how we define the allocation pool size
#define SSD_ALLOC_POOL_GANG                     0
#define SSD_ALLOC_POOL_CHIP                     1   // each element is an allocation pool
#define SSD_ALLOC_POOL_PLANE                    2

// what do the gangs share?
#define SSD_SHARED_BUS_GANG                     1  // shares both data and control
#define SSD_SHARED_CONTROL_GANG                 2  // shares only the control

typedef struct _ssd_timing_params {
    int    ssd_model;                   // e.g. SSD_*_MODEL above
    int    nelements;                   // how many concurrent entities (chips)
    int    page_size;                   // sectors per page
    int    pages_per_block;             // pages per block
    int    blocks_per_element;          // total blocks per chip
    int    element_stride_pages;        // usually 1 or 2

    //vp - changing the chip_xfer_latency to per byte transfer cost
    //instead of per sector transfer cost to make more accurate estimate
    double chip_xfer_latency;           // time to get a byte to/from the chip register
    double page_read_latency;           // time to read a page into chip register
    double page_write_latency;          // time to write a page from chip register
    double block_erase_latency;         // time to erase a block

    int     write_policy;               // policy followed when writing a block
                                        // follow the above definitions
                                        // (e.g., DISKSIM_SSD_WRITE_POLICY_SIMPLE)

    int     reserve_blocks;             // percentage of blocks to reserve

    int     min_freeblks_percent;       // min free blocks percentage

    int     cleaning_policy;            // cleaning & wear-leveling policy used to
                                        // clean blocks

    int     planes_per_pkg;             // num of planes in a single SSD package

    unsigned int    blocks_per_plane;   // num of flash blocks in a single plane

    int     plane_block_mapping;        // how blocks are mapped in a plane

    int     copy_back;      // how active pages are assigned in an element

    int     num_parunits;               // number of parallel units in each element

    int     elements_per_gang;          // number of packages in a gang

    int     gang_share;                 // is this a shared bus or shared control gang?

    int     cleaning_in_background;     // do we want to do the cleaning in foreground/background?

    int     alloc_pool_logic;           // static or dynamic allocation
} ssd_timing_params;

struct _ssd_timing_t;    // forward def for timing module.
//typedef struct _ssd_timing_t *ssd_timing_t;

typedef struct ssd {
   struct device_header hdr;
   ssd_timing_params  params;
    struct _ssd_timing_t   *timing_t;

   double overhead;
   double bus_transaction_latency;
   int numblocks;
   int devno;
   int inited;
   int reconnect_reason;

   ioreq_event *channel_activity;
   ioreq_event *completion_queue;
   struct ioq *queue;

   ssd_element elements[SSD_MAX_ELEMENTS];

   // for ganging elements
   unsigned int data_pages_per_elem;    // number of pages that can be used to store data
   gang_metadata gang_meta[SSD_MAX_ELEMENTS];

   double blktranstime;
   int maxqlen;
   int busowned;
   ioreq_event *buswait;
   int neverdisconnect;
   int numinbuses;
   int inbuses[MAXINBUSES];
   int depth[MAXINBUSES];
   int slotno[MAXINBUSES];

   int printstats;
   ssd_stat_t stat;
} ssd_t;

typedef struct ssd_info {
   struct ssd **ssds;
   int numssds;
  int ssds_len; /* allocated size of ssds */
} ssdinfo_t;

/* request structure */
typedef struct _ssd_req {
    int blk;
    int count;
    int is_read;
    int plane_num;
    ioreq_event *org_req;
    double acctime;
    double schtime;         // when to schedule this event?
    int include;            // should we include its cost in acctime stats
                            // if we're getting parallel ios, then we don't need to include all of 'em
} ssd_req;

/* some more definitions */
#define SSD_PAGE_TO_BLOCK(pnum, s)      ((pnum)/(s->params.pages_per_block))
#define SSD_PAGES_PER_ELEM(s)           ((s)->params.blocks_per_element * (s)->params.pages_per_block)
#define ADDRESSABLE_PAGES_PER_ELEM(s)   ((s)->data_pages_per_elem)
#define SSD_PLANES_PER_ELEM(s)          ((s)->params.planes_per_pkg)
#define SSD_PARUNITS_PER_ELEM(s)        ((s)->params.num_parunits)
#define SSD_PLANES_PER_PARUNIT(s)       (SSD_PLANES_PER_ELEM(s)/SSD_PARUNITS_PER_ELEM(s))
#define SSD_DATA_PAGES_PER_BLOCK(s)     ((s)->params.pages_per_block - 1)
#define SSD_GANG_PAGE_SIZE(s)           ((s)->params.elements_per_gang * (s)->params.page_size)
#define numssds                         (disksim->ssdinfo->numssds)

#define ALLOC_UNIT_GANG             1
#define ALLOC_UNIT_ELEM             2
#define SSD_NUM_GANG(s)             ((s)->params.nelements / (s)->params.elements_per_gang)
#define SSD_GANG_SIZE(s)            (ADDRESSABLE_PAGES_PER_ELEM(s) * (s)->params.elements_per_gang)
#define MIN_REQS                    99999999
#define MAX_REQS                    100
#define MAX_REQS_ELEM_QUEUE         100
#define SYNC_GANG                   1
#define MIGRATE                     1

/* externalized disksim_ssd.c functions */
void    ssd_read_toprints (FILE *parfile);
void    ssd_read_specs (FILE *parfile, int devno, int copies);
void    ssd_set_syncset (int setstart, int setend);
void    ssd_param_override (char *paramname, char *paramval, int first, int last);
void    ssd_setcallbacks (void);
void    ssd_initialize (void);
void    ssd_resetstats (void);
void    ssd_printstats (void);
void    ssd_printsetstats (int *set, int setsize, char *sourcestr);
void    ssd_cleanstats (void);
int     ssd_set_depth (int devno, int inbusno, int depth, int slotno);
int     ssd_get_depth (int devno);
int     ssd_get_inbus (int devno);
int     ssd_get_busno (ioreq_event *curr);
int     ssd_get_slotno (int devno);
int     ssd_get_number_of_blocks (int devno);
int     ssd_get_maxoutstanding (int devno);
int     ssd_get_numdisks (void);
int     ssd_get_numcyls (int devno);
double  ssd_get_blktranstime (ioreq_event *curr);
int     ssd_get_avg_sectpercyl (int devno);
void    ssd_get_mapping (int maptype, int devno, int blkno, int *cylptr, int *surfaceptr, int *blkptr);
void    ssd_event_arrive (ioreq_event *curr);
int     ssd_get_distance (int devno, ioreq_event *req, int exact, int direction);
double  ssd_get_servtime (int devno, ioreq_event *req, int checkcache, double maxtime);
double  ssd_get_acctime (int devno, ioreq_event *req, double maxtime);
void    ssd_bus_delay_complete (int devno, ioreq_event *curr, int sentbusno);
void    ssd_bus_ownership_grant (int devno, ioreq_event *curr, int busno, double arbdelay);

void    ssd_assert_free_blocks(ssd_t *s, ssd_element_metadata *metadata);
void    ssd_assert_valid_pages(int plane_num, ssd_element_metadata *metadata, ssd_t *s);
double  ssd_data_transfer_cost(ssd_t *s, int sectors_count);
int     ssd_last_page_in_block(int page_num, ssd_t *s);
double  _ssd_write_page_osr(ssd_t *s, ssd_element_metadata *metadata, int lpn);
int     ssd_logical_pageno(int blkno, ssd_t *s);
int     ssd_block_to_bitpos(ssd_t *currdisk, int block);
int     ssd_bitpos_to_block(int bitpos, ssd_t *s);
void    _ssd_alloc_active_block(int plane_num, int elem_num, ssd_t *s);
int     ssd_free_bits(int plane_num, int elem_num, ssd_element_metadata *metadata, ssd_t *s);
void    ssd_assert_plane_freebits(int plane_num, int elem_num, ssd_element_metadata *metadata, ssd_t *s);
double  ssd_read_policy_simple(int count, ssd_t *s);
void    ssd_complete_parent(ioreq_event *curr, ssd_t *currdisk);
double _ssd_invoke_element_cleaning(int elem_num, ssd_t *s);
int     ssd_already_present(ssd_req **reqs, int total, ioreq_event *req);

#endif   /* DISKSIM_ssd_H */

