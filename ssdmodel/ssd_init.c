// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#include "ssd.h"
#include "ssd_timing.h"
#include "ssd_clean.h"
#include "ssd_utils.h"
#include "ssd_init.h"

/* read-only globals used during readparams phase */
static char *statdesc_acctimestats  =   "Access time";

static void ssd_statinit (int devno, int firsttime)
{
   ssd_t *currdisk;

   currdisk = getssd (devno);
   if (firsttime) {
      stat_initialize(statdeffile, statdesc_acctimestats, &currdisk->stat.acctimestats);
   } else {
      stat_reset(&currdisk->stat.acctimestats);
   }

   currdisk->stat.requestedbus = 0.0;
   currdisk->stat.waitingforbus = 0.0;
   currdisk->stat.numbuswaits = 0;
}

void ssd_initialize_diskinfo ()
{
   disksim->ssdinfo = malloc (sizeof(ssdinfo_t));
   bzero ((char *)disksim->ssdinfo, sizeof(ssdinfo_t));
   disksim->ssdinfo->ssds = malloc(MAXDEVICES * (sizeof(ssd_t)));
   disksim->ssdinfo->ssds_len = MAXDEVICES;
   bzero ((char *)disksim->ssdinfo->ssds, (MAXDEVICES * (sizeof(ssd_t))));
}


static int ssd_first_page_in_next_block(int ppage, ssd_t *currdisk)
{
    int skip_by = ppage % currdisk->params.pages_per_block;
    ppage += currdisk->params.pages_per_block - skip_by;
    return ppage;
}

int ssd_elem_export_size(ssd_t *currdisk)
{
    unsigned int reserved_blocks, usable_blocks;
    unsigned int reserved_blocks_per_plane, usable_blocks_per_plane;

    reserved_blocks_per_plane = (currdisk->params.reserve_blocks * currdisk->params.blocks_per_plane) / 100;
    usable_blocks_per_plane = currdisk->params.blocks_per_plane - reserved_blocks_per_plane;
    reserved_blocks = reserved_blocks_per_plane * currdisk->params.planes_per_pkg;
    usable_blocks = usable_blocks_per_plane * currdisk->params.planes_per_pkg;

    return (usable_blocks * SSD_DATA_PAGES_PER_BLOCK(currdisk));
}

/*
 * vp
 * description: this routine allocates and initializes the ssd element metadata
 * structures. FIXME: if the systems is powered up, this init routine has to
 * populate the structures by scanning the summary pages (to implement this,
 * we can read from a disk checkpoint file). but, this is future work.
*/
void ssd_element_metadata_init(int elem_number, ssd_element_metadata *metadata, ssd_t *currdisk)
{
    gang_metadata *g;
    unsigned int ppage;
    unsigned int i;
    unsigned int bytes_to_alloc;
    unsigned int tot_blocks = currdisk->params.blocks_per_element;
    unsigned int tot_pages = tot_blocks * currdisk->params.pages_per_block;
    unsigned int reserved_blocks, usable_blocks, export_size;
    unsigned int reserved_blocks_per_plane, usable_blocks_per_plane;
    unsigned int bitpos;
    unsigned int active_block;
    unsigned int elem_index;
    unsigned int bsn = 1;
    int plane_block_mapping = currdisk->params.plane_block_mapping;

    //////////////////////////////////////////////////////////////////////////////
    // active page starts at the 1st page on the reserved section
    reserved_blocks_per_plane = (currdisk->params.reserve_blocks * currdisk->params.blocks_per_plane) / 100;
    usable_blocks_per_plane = currdisk->params.blocks_per_plane - reserved_blocks_per_plane;
    reserved_blocks = reserved_blocks_per_plane * currdisk->params.planes_per_pkg;
    usable_blocks = usable_blocks_per_plane * currdisk->params.planes_per_pkg;

    //////////////////////////////////////////////////////////////////////////////
    // initialize the free blocks and free pages
    metadata->tot_free_blocks = reserved_blocks;

    //////////////////////////////////////////////////////////////////////////////
    // assign the gang and init the element's free pages
    metadata->gang_num = elem_number / currdisk->params.elements_per_gang;
    currdisk->gang_meta[metadata->gang_num].elem_free_pages[elem_number] = \
        metadata->tot_free_blocks * SSD_DATA_PAGES_PER_BLOCK(currdisk);
    g = &currdisk->gang_meta[metadata->gang_num];
    elem_index = elem_number % currdisk->params.elements_per_gang;

    //////////////////////////////////////////////////////////////////////////////
    // let's begin cleaning with the first plane
    metadata->plane_to_clean = 0;
    metadata->plane_to_write = 0;
    metadata->block_alloc_pos = 0;
    metadata->reqs_waiting = 0;
    metadata->tot_migrations = 0;
    metadata->tot_pgs_migrated = 0;
    metadata->mig_cost = 0;

    //////////////////////////////////////////////////////////////////////////////
    // init the plane metadata
    for (i = 0; i < (unsigned int)currdisk->params.planes_per_pkg; i ++) {
        int blocks_to_skip;

        switch(plane_block_mapping) {
            case PLANE_BLOCKS_CONCAT:
                blocks_to_skip = (i*currdisk->params.blocks_per_plane + usable_blocks_per_plane);
                break;

            case PLANE_BLOCKS_PAIRWISE_STRIPE:
                blocks_to_skip = (i/2)*(2*currdisk->params.blocks_per_plane) + (2*usable_blocks_per_plane) + i%2;
                break;

            case PLANE_BLOCKS_FULL_STRIPE:
                blocks_to_skip = (currdisk->params.planes_per_pkg * usable_blocks_per_plane) + i;
                break;

            default:
                fprintf(stderr, "Error: unknown plane_block_mapping %d\n", plane_block_mapping);
                exit(1);
        }

        metadata->plane_meta[i].active_page = blocks_to_skip*currdisk->params.pages_per_block;
        metadata->plane_meta[i].free_blocks = reserved_blocks_per_plane;
        metadata->plane_meta[i].valid_pages = 0;
        metadata->plane_meta[i].clean_in_progress = 0;
        metadata->plane_meta[i].clean_in_block = -1;
        metadata->plane_meta[i].block_alloc_pos = i*currdisk->params.blocks_per_plane;
        metadata->plane_meta[i].parunit_num = i / SSD_PLANES_PER_PARUNIT(currdisk);
        metadata->plane_meta[i].num_cleans = 0;
    }

    //////////////////////////////////////////////////////////////////////////////
    // init the next plane to clean in a parunit
    for (i = 0; i < (unsigned int) SSD_PARUNITS_PER_ELEM(currdisk); i ++) {
        metadata->parunits[i].plane_to_clean = SSD_PLANES_PER_PARUNIT(currdisk)*i;
    }

    //////////////////////////////////////////////////////////////////////////////
    // init the element's active page
    switch(plane_block_mapping) {
        case PLANE_BLOCKS_CONCAT:
            metadata->active_page = usable_blocks_per_plane * currdisk->params.pages_per_block;
            break;

        case PLANE_BLOCKS_PAIRWISE_STRIPE:
            metadata->active_page = (2 * usable_blocks_per_plane) * currdisk->params.pages_per_block;
            break;

        case PLANE_BLOCKS_FULL_STRIPE:
            metadata->active_page = (currdisk->params.planes_per_pkg * usable_blocks_per_plane) * currdisk->params.pages_per_block;
            break;

        default:
            fprintf(stderr, "Error: unknown plane_block_mapping %d\n", plane_block_mapping);
            exit(1);
    }

    ASSERT(metadata->active_page == metadata->plane_meta[0].active_page);
    active_block = metadata->active_page / currdisk->params.pages_per_block;

    // since we reserve one page out of every block to store the summary info,
    // the size exported by the flash disk is little less.
    export_size = usable_blocks * SSD_DATA_PAGES_PER_BLOCK(currdisk);
    currdisk->data_pages_per_elem = export_size;
    //printf("res blks = %d, use blks = %d act page = %d exp size = %d\n",
    //  reserved_blocks, usable_blocks, metadata->active_page, export_size);

    //////////////////////////////////////////////////////////////////////////////
    // allocate the lba table
    if ((metadata->lba_table = (int *)malloc(export_size * sizeof(int))) == NULL) {
        fprintf(stderr, "Error: malloc to lba table in ssd_element_metadata_init failed\n");
        fprintf(stderr, "Allocation size = %d\n", export_size * sizeof(int));
        exit(1);
    }

    //////////////////////////////////////////////////////////////////////////////
    // allocate the free blocks bit map
    // what if the no of blocks is not divisible by 8?
    if ((tot_blocks % (sizeof(unsigned char) * 8)) != 0) {
        fprintf(stderr, "This case is not yet handled\n");
        exit(1);
    }

    bytes_to_alloc = tot_blocks / (sizeof(unsigned char) * 8);
    if (!(metadata->free_blocks = (unsigned char *)malloc(bytes_to_alloc))) {
        fprintf(stderr, "Error: malloc to free_blocks in ssd_element_metadata_init failed\n");
        fprintf(stderr, "Allocation size = %d\n", bytes_to_alloc);
        exit(1);
    }
    bzero(metadata->free_blocks, bytes_to_alloc);

    //////////////////////////////////////////////////////////////////////////////
    // allocate the block usage array and initialize it
    if (!(metadata->block_usage = (block_metadata *)malloc(tot_blocks * sizeof(block_metadata)))) {
        fprintf(stderr, "Error: malloc to block_usage in ssd_element_metadata_init failed\n");
        fprintf(stderr, "Allocation size = %d\n", tot_blocks * sizeof(block_metadata));
        exit(1);
    }
    bzero(metadata->block_usage, tot_blocks * sizeof(block_metadata));

    for (i = 0; i < tot_blocks; i ++) {
        int j;

        metadata->block_usage[i].block_num = i;
        metadata->block_usage[i].page = (int*)malloc(sizeof(int) * currdisk->params.pages_per_block);

        for (j = 0; j < currdisk->params.pages_per_block; j ++) {
            metadata->block_usage[i].page[j] = -1;
        }

        // assign the plane number to each block
        switch(plane_block_mapping) {
            case PLANE_BLOCKS_CONCAT:
                metadata->block_usage[i].plane_num = i / currdisk->params.blocks_per_plane;
                break;

            case PLANE_BLOCKS_PAIRWISE_STRIPE:
                metadata->block_usage[i].plane_num = (i/(2*currdisk->params.blocks_per_plane))*2 + i%2;
                break;

            case PLANE_BLOCKS_FULL_STRIPE:
                metadata->block_usage[i].plane_num = i % currdisk->params.planes_per_pkg;
                break;

            default:
                fprintf(stderr, "Error: unknown plane_block_mapping %d\n", plane_block_mapping);
                exit(1);
        }

        // set the remaining life time and time of last erasure
        metadata->block_usage[i].rem_lifetime = SSD_MAX_ERASURES;
        metadata->block_usage[i].time_of_last_erasure = simtime;

        // set the block state
        metadata->block_usage[i].state = SSD_BLOCK_CLEAN;

        // init the bsn to be zero
        metadata->block_usage[i].bsn = 0;
    }

    //////////////////////////////////////////////////////////////////////////////
    // initially, we assume that every logical page is mapped
    // onto a physical page. we start from the first phy page
    // and continue to map, leaving the last page of every block
    // to store the summary information.
    ppage = 0;
    i = 0;
    while (i < export_size) {
        int pgnum_in_gang;
        int pp_index;
        int plane_num;
        unsigned int block = SSD_PAGE_TO_BLOCK(ppage, currdisk);

        ASSERT(block < (unsigned int)currdisk->params.blocks_per_element);

        // if this is the last page in the block
        if (ssd_last_page_in_block(ppage, currdisk)) {
            // leave this physical page for summary page and
            // seal the block
            metadata->block_usage[block].state = SSD_BLOCK_SEALED;

            // go to next block
            ppage ++;
            block = SSD_PAGE_TO_BLOCK(ppage, currdisk);
        }

        // if this block is in the reserved section, skip it
        // and go to the next block.
        switch(plane_block_mapping) {
            case PLANE_BLOCKS_CONCAT:
            {
                unsigned int block_index = block % currdisk->params.blocks_per_plane;
                if ((block_index >= usable_blocks_per_plane) && (block_index < currdisk->params.blocks_per_plane)) {
                    // go to next block
                    ppage = ssd_first_page_in_next_block(ppage, currdisk);
                    continue;
                }
            }
            break;

            case PLANE_BLOCKS_PAIRWISE_STRIPE:
            {
                unsigned int block_index = block % (2*currdisk->params.blocks_per_plane);
                if ((block_index >= 2*usable_blocks_per_plane) && (block_index < 2*currdisk->params.blocks_per_plane)) {
                    ppage = ssd_first_page_in_next_block(ppage, currdisk);
                    continue;
                }
            }
            break;

            case PLANE_BLOCKS_FULL_STRIPE:
                // ideally the control should not come here ...
                if ((block >= usable_blocks) && (block < (unsigned int)currdisk->params.blocks_per_element)) {
                    printf("Error: the control should not come here ...\n");
                    ppage = ssd_first_page_in_next_block(ppage, currdisk);
                    continue;
                }
            break;

            default:
                fprintf(stderr, "Error: unknown plane_block_mapping %d\n", plane_block_mapping);
                exit(1);
        }

        // when the control comes here, 'ppage' contains the next page
        // that can be assigned to a logical page.
        // find the index of the phy page within the block
        pp_index = ppage % currdisk->params.pages_per_block;

        // populate the lba table
        metadata->lba_table[i] = ppage;
        pgnum_in_gang = elem_index * export_size + i;
        g->pg2elem[pgnum_in_gang].e = elem_number;

        // mark this block as not free and its state as 'in use'.
        // note that a block could be not free and its state be 'sealed'.
        // it is enough if we set it once while working on the first phy page.
        // also increment the block sequence number.
        if (pp_index == 0) {
            bitpos = ssd_block_to_bitpos(currdisk, block);
            ssd_set_bit(metadata->free_blocks, bitpos);
            metadata->block_usage[block].state = SSD_BLOCK_INUSE;
            metadata->block_usage[block].bsn = bsn ++;
        }

        // increase the usage count per block
        plane_num = metadata->block_usage[block].plane_num;
        metadata->block_usage[block].page[pp_index] = i;
        metadata->block_usage[block].num_valid ++;
        metadata->plane_meta[plane_num].valid_pages ++;

        // go to the next physical page
        ppage ++;

        // go to the next logical page
        i ++;
    }

    //////////////////////////////////////////////////////////////////////////////
    // mark the block that corresponds to the active page
    // as not free and 'in_use'.
    switch(currdisk->params.copy_back) {
        case SSD_COPY_BACK_DISABLE:
            bitpos = ssd_block_to_bitpos(currdisk, active_block);
            ssd_set_bit(metadata->free_blocks, bitpos);
            metadata->block_usage[active_block].state = SSD_BLOCK_INUSE;
            metadata->block_usage[active_block].bsn = bsn ++;
        break;

        case SSD_COPY_BACK_ENABLE:
            for (i = 0; i < (unsigned int)currdisk->params.planes_per_pkg; i ++) {
                int plane_active_block = SSD_PAGE_TO_BLOCK(metadata->plane_meta[i].active_page, currdisk);

                bitpos = ssd_block_to_bitpos(currdisk, plane_active_block);
                ssd_set_bit(metadata->free_blocks, bitpos);
                metadata->block_usage[plane_active_block].state = SSD_BLOCK_INUSE;
                metadata->block_usage[plane_active_block].bsn = bsn ++;
                metadata->tot_free_blocks --;
                metadata->plane_meta[i].free_blocks --;
            }
        break;

        default:
            fprintf(stderr, "Error: invalid copy back policy %d\n",
                currdisk->params.copy_back);
            exit(1);
    }

    //////////////////////////////////////////////////////////////////////////////
    // set the bsn for the ssd element
    metadata->bsn = bsn;
    //printf("set the bsn to %d\n", bsn);
}

void ssd_plane_init(ssd_element *elem, ssd_t *s, int devno)
{
    int i;

    // set the num of planes per package
    elem->num_planes = s->params.planes_per_pkg;

    // init all the planes
    for (i = 0; i < elem->num_planes; i ++) {
        elem->plane[i].media_busy = FALSE;
        elem->plane[i].num_blocks = s->params.blocks_per_plane;

        // just flip the LSB to find the pair
        elem->plane[i].pair_plane = i ^ 0x1;
    }
}

/* vp
 * verifies if a valid combination of parameters are given.
 */
void ssd_verify_parameters(ssd_t *currdisk)
{
    //vp - some verifications:
    ASSERT(currdisk->params.min_freeblks_percent < currdisk->params.reserve_blocks);

    if (currdisk->params.write_policy == 1) {
        printf("Simple write policy with block-sized pages (256 KB) is not supported\n");
        ASSERT(0);
    }

    ASSERT((currdisk->params.planes_per_pkg * currdisk->params.blocks_per_plane) == currdisk->params.blocks_per_element);

    if (currdisk->params.alloc_pool_logic == 2) { // plane specific
        ASSERT(currdisk->params.copy_back == SSD_COPY_BACK_ENABLE); // we can do GC only w/in a plane
    }
}

void ssd_alloc_queues(ssd_t *t)
{
   // gross hack !!!!!
   // copy the queue for the gangs and element queues from the
   // main queue
   // printf("Total gang = %d\n", SSD_NUM_GANG(result));
   int i;
   for (i = 0; i < SSD_NUM_GANG(t); i ++) {
          struct ioq *q = malloc(sizeof(ioqueue));
          memcpy(q, t->queue, sizeof(ioqueue));
          t->gang_meta[i].queue = q;
          assert(q != NULL);
   }

   for (i=0;i<t->params.nelements;i++) {
          struct ioq *q = malloc(sizeof(ioqueue));
          memcpy(q, t->queue, sizeof(ioqueue));
          t->elements[i].queue = q;
          assert(q != NULL);
    }
}

void ssd_initialize (void)
{
    static print1 = 1;
   int i, j;

   if (disksim->ssdinfo == NULL) {
      ssd_initialize_diskinfo ();
   }
/*
fprintf (outputfile, "Entered ssd_initialize - numssds %d\n", numssds);
*/
   ssd_setcallbacks();

   // fprintf(stdout, "MAXDEVICES = %d, numssds %d\n", MAXDEVICES, numssds);
   // vp - changing the MAXDEVICES in the below 'for' loop to numssds
   for (i=0; i<numssds; i++) {
       int exp_size;
      ssd_t *currdisk = getssd (i);
      ssd_alloc_queues(currdisk);

      //vp - some verifications:
      ssd_verify_parameters(currdisk);

      //vp - this was not initialized and caused so many bugs
      currdisk->devno = i;

      if (!currdisk) continue;
/*        if (!currdisk->inited) { */
         currdisk->numblocks = currdisk->params.nelements *
                   currdisk->params.blocks_per_element *
                   currdisk->params.pages_per_block *
                   currdisk->params.page_size;
         currdisk->reconnect_reason = -1;
         addlisttoextraq ((event **) &currdisk->buswait);
         currdisk->busowned = -1;
         currdisk->completion_queue = NULL;
         /* hack to init queue structure */
         ioqueue_initialize (currdisk->queue, i);
         ssd_statinit(i, TRUE);
         currdisk->timing_t = ssd_new_timing_t(&currdisk->params);

         // initialize the gang
         exp_size = ssd_elem_export_size(currdisk);
         for (j = 0; j < SSD_NUM_GANG(currdisk); j ++) {
             int tot_pages = exp_size * currdisk->params.elements_per_gang;
             currdisk->gang_meta[j].busy = 0;
             currdisk->gang_meta[j].cleaning = 0;
             currdisk->gang_meta[j].reqs_waiting = 0;
             currdisk->gang_meta[j].oldest = 0;
             currdisk->gang_meta[j].pg2elem = malloc(sizeof(ssd_elem_number) * tot_pages);
             memset(currdisk->gang_meta[j].pg2elem, 0, sizeof(ssd_elem_number) * tot_pages);

             ioqueue_initialize (currdisk->gang_meta[j].queue, i);
         }

         for (j=0; j<currdisk->params.nelements; j++) {
             ssd_element *elem = &currdisk->elements[j];
             ioqueue_initialize (elem->queue, i);

             /* hack to init queue structure */
             elem->media_busy = FALSE;

             // vp - pins are also free
             elem->pin_busy = FALSE;

             // vp - initialize the planes in the element
             ssd_plane_init(elem, currdisk, i);

            // vp - initialize the ssd element metadata
            // FIXME: where to free these data?
             memset(&elem->stat, 0, sizeof(elem->stat));
            if (currdisk->params.write_policy == DISKSIM_SSD_WRITE_POLICY_OSR) {
                ssd_element_metadata_init(j, &(elem->metadata), currdisk);
            }

            //vp - initialize the stat structure
            memset(&elem->stat, 0, sizeof(ssd_element_stat));
         }
/*        } */
   }
}

void ssd_resetstats (void)
{
   int i;

   for (i=0; i<MAXDEVICES; i++) {
      ssd_t *currdisk = getssd (i);
      if (currdisk) {
         int j;
         ioqueue_resetstats(currdisk->queue);
         for (j=0; j<currdisk->params.nelements; j++) {
             ioqueue_resetstats(currdisk->elements[j].queue);
         }
         ssd_statinit(i, 0);
      }
   }
}

