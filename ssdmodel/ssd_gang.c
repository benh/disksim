// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#include "ssd.h"
#include "ssd_gang.h"
#include "ssd_timing.h"

static int ssd_invoke_gang_cleaning(int gang_num, ssd_t *s)
{
    int i;
    int elem_num;
    double max_cost = 0;
    double elem_clean_cost;
    int cleaning_invoked = 0;
    gang_metadata *g = &s->gang_meta[gang_num];

    // all the elements in the gang must be free
    ASSERT(g->busy == FALSE);
    ASSERT(g->cleaning == FALSE);

    // invoke cleaning on all the elements
    for (i = 0; i < s->params.elements_per_gang; i ++) {
        elem_num = gang_num * s->params.elements_per_gang + i;
        elem_clean_cost = _ssd_invoke_element_cleaning(elem_num, s);

        // stat
        s->elements[elem_num].stat.tot_clean_time += max_cost;

        if (max_cost < elem_clean_cost) {
            max_cost = elem_clean_cost;
        }
    }

    // cleaning was invoked on all the elements. we can start
    // the next operation on this gang only after the cleaning
    // gets over on all the elements.
    if (max_cost > 0) {
        ioreq_event *tmp;

        g->busy = 1;
        g->cleaning = 1;
        cleaning_invoked = 1;

        // we use the 'blkno' field to store the gang number
        tmp = (ioreq_event *)getfromextraq();
        tmp->devno = s->devno;
        tmp->time = simtime + max_cost;
        tmp->blkno = gang_num;
        tmp->ssd_gang_num = gang_num;
        tmp->type = SSD_CLEAN_GANG;
        tmp->flags = SSD_CLEAN_GANG;
        tmp->busno = -1;
        tmp->bcount = -1;
        stat_update (&s->stat.acctimestats, max_cost);
        addtointq ((event *)tmp);
    }

    return cleaning_invoked;
}

static int ssd_next_elem_in_gang(ssd_t *s, int gang_num, int elem_num)
{
    return ((elem_num+1)%s->params.elements_per_gang + \
        gang_num*s->params.elements_per_gang);
}

/*
 * collects 1 request from each chip in the gang
 */
static void ssd_collect_req_in_gang
(ssd_t *s, int gang_num, ssd_req ***rd_q, ssd_req ***wr_q, int *rd_total, int *wr_total)
{
    int i;
    int start;
    gang_metadata *g;

    g = &s->gang_meta[gang_num];

    // start from the first element of the gang
    start = gang_num * s->params.elements_per_gang;
    i = start;

    *rd_total = 0;
    *wr_total = 0;

    do {
        ssd_element *elem;
        ioreq_event *req;
        int tot_rd_reqs;
        int tot_wr_reqs;
        int j;

        elem = &s->elements[i];
        ASSERT(ioqueue_get_reqoutstanding(elem->queue) == 0);
        j = i % s->params.elements_per_gang;

        // collect the requests
        tot_rd_reqs = 0;
        tot_wr_reqs = 0;
        if ((req = ioqueue_get_next_request(elem->queue)) != NULL) {
            int found;

            if (req->flags & READ) {
                found = ssd_already_present(rd_q[j], tot_rd_reqs, req);
            } else {
                found = ssd_already_present(wr_q[j], tot_wr_reqs, req);
            }

            if (!found) {
                // this is a valid request
                ssd_req *r = malloc(sizeof(ssd_req));
                r->blk = req->blkno;
                r->count = req->bcount;
                r->is_read = req->flags & READ;
                r->org_req = req;
                r->plane_num = -1; // we don't know to which plane this req will be directed at

                if (req->flags & READ) {
                    rd_q[j][tot_rd_reqs] = r;
                    tot_rd_reqs ++;
                } else {
                    wr_q[j][tot_wr_reqs] = r;
                    tot_wr_reqs ++;
                }
            } else {
                // throw this request -- it doesn't make sense
                stat_update (&s->stat.acctimestats, 0);
                req->time = simtime;
                req->ssd_elem_num = i;
                req->ssd_gang_num = gang_num;

                req->type = DEVICE_ACCESS_COMPLETE;
                addtointq ((event *)req);
            }

            ASSERT((tot_rd_reqs < MAX_REQS) && (tot_wr_reqs < MAX_REQS))
        }

        *rd_total = *rd_total + tot_rd_reqs;
        *wr_total = *wr_total + tot_wr_reqs;

        // go to the next element
        i = ssd_next_elem_in_gang(s, gang_num, i);
    } while (i != start);
}

int choose_gang(int blk, ssd_t *s)
{
    int num_gangs = SSD_NUM_GANG(s);
    int page = blk / s->params.page_size;

    // assuming stripping across the gangs
    return ((page/s->params.elements_per_gang) % num_gangs);
}

int ssd_gang_index(int blk, ssd_t *s)
{
    int page = blk/s->params.page_size;
    int i = page/s->params.elements_per_gang;
    return (i/SSD_NUM_GANG(s));
}

int choose_elem_in_gang(int blk, int gang_num, ssd_t *s)
{
    int j;
    int elem_num;
    int num_gangs = SSD_NUM_GANG(s);
    int page = blk / s->params.page_size;
    int pg_index_in_gang;
    gang_metadata *gm = &s->gang_meta[gang_num];

    pg_index_in_gang = ssd_gang_index(blk, s);

    // assuming: stripping within the gang, across all its elements
    elem_num = (gang_num)*s->params.elements_per_gang + page%s->params.elements_per_gang;
    j = (page % s->params.elements_per_gang) * s->data_pages_per_elem + pg_index_in_gang;

    ASSERT(elem_num == (int)gm->pg2elem[j].e);
    ASSERT((elem_num >= (gang_num)*s->params.elements_per_gang) && (elem_num < (gang_num+1)*s->params.elements_per_gang));

    return elem_num;
}

void choose_gang_and_element(int blk, ioreq_event *req, ssd_t *s, int *gang_num, int *elem_num)
{
    int gnum = choose_gang(blk, s);
    int e = choose_elem_in_gang(blk, gnum, s);
    *gang_num = gnum;
    *elem_num = e;

    return;
}

double ssd_gang_read_sync(int gang_num, int blkno, int count, ssd_t *s)
{
    gang_metadata *g;
    double req_time = 0;
    int blk;
    int gindex;

    g = &s->gang_meta[gang_num];
    blk = blkno;
    req_time = 0;
    ASSERT(count <= SSD_GANG_PAGE_SIZE(s));

    gindex = ssd_gang_index(blkno, s);

    // calculate the time
    while (count > 0) {
        ssd_element *elem;
        int elem_num;
        double acctime = 0;

        ASSERT(gindex == ssd_gang_index(blk, s));
        elem_num = choose_elem_in_gang(blk, gang_num, s);
        elem = &s->elements[elem_num];

        // first request means include the read time too
        //if (blk == blkno) {
            acctime = s->params.page_read_latency;
        //}

        // for other requests, since the read is overlapped
        // just include the transfer time.
        acctime += ssd_data_transfer_cost(s, s->params.page_size);

        // stat
        elem->stat.tot_reqs_issued ++;
        elem->stat.tot_time_taken += acctime;
        stat_update (&s->stat.acctimestats, acctime);
        req_time += acctime;

        //printf("Adding gang %d blk %d time %f\n",
        //  gang_num, blkno, simtime+req_time);

        // go to next block
        blk += s->params.page_size;
        count -= s->params.page_size;
    }

    return req_time;
}

/*
 * issues write synchronously -- in a read-modify-write fashion.
 */
static void ssd_activate_gang_sync(int gang_num, ssd_t *s)
{
    ioreq_event *req;
    gang_metadata *g;

    g = &s->gang_meta[gang_num];
    if (g->busy) {
        return;
    }

    if (s->params.cleaning_in_background) {
        // if cleaning was invoked, wait until it is over ...
        if (ssd_invoke_gang_cleaning(gang_num, s)) {
            return;
        }
    }

    if (g->reqs_waiting > 0) {
        double req_time = 0;

        // if we have to clean in the foreground, then we invoke
        // cleaning only when there are requests waiting
        if (!s->params.cleaning_in_background) {
            // if cleaning was invoked, wait until it is over ...
            if (ssd_invoke_gang_cleaning(gang_num, s)) {
                return;
            }
        }

        req = ioqueue_get_next_request(g->queue);
        ASSERT(req != NULL);

        // check the request size
        ASSERT(req->bcount <= SSD_GANG_PAGE_SIZE(s));

        // if this is a read request, issue it simply
        if (req->flags & READ) {
            g->busy = TRUE;
            req_time = ssd_gang_read_sync(gang_num, req->blkno, req->bcount, s);

            // add an event for request completion
            req->time = simtime + req_time;
            req->type = DEVICE_ACCESS_COMPLETE;
            req->ssd_gang_num = gang_num;
            addtointq ((event *)req);
            //printf("Gang %d blk %d end %f\n", gang_num, req->blkno, req->time);
        } else {
            // writes are slightly trickier. in a synchronous gang,
            // we treat a logical page to consist of pages from all
            // the elements and span the entire gang. to update a
            // portion of such a logical page, we need to read
            // other portions of the same page and write them all.

            double read_time = 0;
            double xfer_time = 0;
            int count, blk, pg, start_pg;
            int gindex;
            int wrote_summary = 0;
            ssd_req *elem_req[1];

            g->busy = TRUE;

            // get the starting page of the stripe in the gang
            pg = req->blkno / s->params.page_size;
            start_pg = pg - pg % s->params.elements_per_gang;
            gindex = ssd_gang_index(req->blkno, s);

            if (req->bcount != SSD_GANG_PAGE_SIZE(s)) {
                blk = start_pg * s->params.page_size;
                count = SSD_GANG_PAGE_SIZE(s);

                // calculate the read time
                while (count > 0) {

                    ASSERT(gindex == ssd_gang_index(blk, s));
                    if ((req->blkno <= blk) && ((blk+s->params.page_size-1) < (req->blkno+req->bcount))) {
                        // don't have to read this page
                    } else {
                        // read this page
                        // but we don't have to transfer the data across the
                        // serial pins. we just need to read the data into the
                        // register.
                        // read_time += ssd_data_transfer_cost(s, s->params.page_size);
                    }

                    count -= s->params.page_size;
                    blk += s->params.page_size;
                }

                read_time += s->params.page_read_latency;
            }

            // we need to issue a new write request, so allocate
            elem_req[0] = malloc(sizeof(ssd_req));
            memset(elem_req[0], 0, sizeof(ssd_req));

            blk = start_pg * s->params.page_size;
            count = SSD_GANG_PAGE_SIZE(s);
            req_time = read_time;

            // calculate the write time
            while (count > 0) {
                ssd_element *elem;
                int elem_num;
                double xfertime;
                double acctime;

                ASSERT(gindex == ssd_gang_index(blk, s));
                elem_num = choose_elem_in_gang(blk, gang_num, s);
                elem = &s->elements[elem_num];
                elem_req[0]->blk = blk;
                elem_req[0]->count = s->params.page_size;
                elem_req[0]->is_read = 0;

                // issue one req.
                // this is just to update the status in each element
                ssd_compute_access_time(s, elem_num, &elem_req[0], 1);

                // stat
                xfertime = ssd_data_transfer_cost(s, s->params.page_size);
                acctime = read_time + elem_req[0]->acctime;
                elem->stat.tot_reqs_issued ++;
                elem->stat.tot_time_taken += acctime;
                stat_update (&s->stat.acctimestats, acctime);
                req_time += xfertime;

                // see if a summary page was written
                if (elem_req[0]->acctime > (xfertime + s->params.page_write_latency)) {
                    wrote_summary = 1;
                }

                // go to next block
                blk += s->params.page_size;
                count -= s->params.page_size;
            }

            // if a summary page was written, include its time too
            if (wrote_summary) {
                req_time += req_time; // add the time for transferring the summary pages too
                req_time += 2 * s->params.page_write_latency;
            } else {
                req_time += s->params.page_write_latency;
            }

            // add an event for request completion
            req->time = simtime + req_time;
            req->type = DEVICE_ACCESS_COMPLETE;
            req->ssd_gang_num = gang_num;
            addtointq ((event *)req);

            // free
            free(elem_req[0]);
        }

        // we've processed one request from the queue
        g->reqs_waiting --;
    }
}

static void _ssd_activate_gang
(ssd_t *s, gang_metadata *g, int gang_num, ssd_req ***reqs_queue, int read_flag, int total)
{
    double after = 0;
    ssd_req ***first;
    int i;

    if (total == 0) {
        return;
    }

    first = (ssd_req ***) malloc (s->params.elements_per_gang * sizeof(ssd_req **));
    for (i = 0; i < s->params.elements_per_gang; i ++) {
        first[i] = &reqs_queue[i][0];
    }

    while (1) {
        int skip = 0;
        double acctime = 0;
        int reqs_issued = 0;

        // issue one req for each element in the gang
        for (i = 0; i < s->params.elements_per_gang; i ++) {

            // if there is a valid request, then issue it
            if (reqs_queue[i][0]) {
                ssd_req *to_free;
                ssd_element *elem;
                int elem_num;

                elem_num = gang_num * s->params.elements_per_gang + i;
                elem = &s->elements[elem_num];

                g->busy = 1;
                elem->media_busy = TRUE;

                // issue just one req
                ssd_compute_access_time(s, elem_num, reqs_queue[i], 1);

                // find the maximum time taken by a request
                if (acctime < reqs_queue[i][0]->acctime) {
                  acctime = reqs_queue[i][0]->acctime;
                }

                elem->stat.tot_reqs_issued ++;
                elem->stat.tot_time_taken += reqs_queue[i][0]->acctime;

                // add an event for request completion
                stat_update (&s->stat.acctimestats, reqs_queue[i][0]->acctime);
                reqs_queue[i][0]->org_req->time = simtime + after + reqs_queue[i][0]->acctime;
                reqs_queue[i][0]->org_req->type = DEVICE_ACCESS_COMPLETE;
                reqs_queue[i][0]->org_req->ssd_elem_num = elem_num;
                reqs_queue[i][0]->org_req->ssd_gang_num = gang_num;

                addtointq ((event *)reqs_queue[i][0]->org_req);
                //printf("Adding gang %d blk %d time %f\n",
                //  gang_num, reqs_queue[i][0]->org_req->blkno, reqs_queue[i][0]->org_req->time);
                to_free = reqs_queue[i][0];

                // go to the next request in this element
                reqs_queue[i] = &reqs_queue[i][1];
                free(to_free);

                reqs_issued ++;

                // if data lines are also shared, we can just issue
                // one request at a time
                if (s->params.gang_share == SSD_SHARED_BUS_GANG) {
                    if (reqs_issued == 1) {
                        break;
                    }
                }
            } else {
                skip ++;
            }
        }

        // next round can start only after finishing all
        // the requests in this round.
        after = after + acctime;

        // if none of the elements have a request to issue, quit.
        if (skip == s->params.elements_per_gang) {
            break;
        }
    }

    // restore the addresses
    for (i = 0; i < s->params.elements_per_gang; i ++) {
        reqs_queue[i] = first[i];
    }

    free(first);
}

/*
 * activates a gang of chips. if there are requests waiting,
 * they're collected and issued one per chip.
 */
static void ssd_activate_gang(ssd_t *s, int gang_num)
{
    int i;
    ssd_req ***rd_q;
    ssd_req ***wr_q;
    gang_metadata *g;

    g = &s->gang_meta[gang_num];
    if (g->busy) {
        return;
    }

    if (s->params.cleaning_in_background) {
        // if cleaning was invoked, wait until
        // it is over ...
        if (ssd_invoke_gang_cleaning(gang_num, s)) {
            return;
        }
    }

    if (g->reqs_waiting > 0) {
        int rd_total;
        int wr_total;

        // if we have to clean in the foreground, then we invoke
        // cleaning only when there are requests waiting
        if (!s->params.cleaning_in_background) {
            // if cleaning was invoked, wait until
            // it is over ...
            if (ssd_invoke_gang_cleaning(gang_num, s)) {
                return;
            }
        }

        // allocate
        rd_q = (ssd_req ***) malloc(s->params.elements_per_gang * sizeof (ssd_req **));
        wr_q = (ssd_req ***) malloc(s->params.elements_per_gang * sizeof (ssd_req **));
        for (i = 0; i < s->params.elements_per_gang; i ++) {
            rd_q[i] = malloc(MAX_REQS * sizeof(ssd_req *));
            wr_q[i] = malloc(MAX_REQS * sizeof(ssd_req *));
            memset(rd_q[i], 0, MAX_REQS * sizeof(ssd_req *));
            memset(wr_q[i], 0, MAX_REQS * sizeof(ssd_req *));
        }

        // get the requests and issue them
        ssd_collect_req_in_gang(s, gang_num, rd_q, wr_q, &rd_total, &wr_total);

        // issue reads first and then writes
        _ssd_activate_gang(s, g, gang_num, rd_q, 1, rd_total);
        _ssd_activate_gang(s, g, gang_num, wr_q, 0, wr_total);

        g->reqs_waiting -= (rd_total + wr_total);

        // free
        for (i = 0; i < s->params.elements_per_gang; i ++) {
            free(rd_q[i]);
            free(wr_q[i]);
        }
        free(rd_q);
        free(wr_q);
    }
}

void ssd_media_access_request_gang_sync(ioreq_event *curr)
{
    int i;
    int gang_to_activate[SSD_MAX_ELEMENTS];
    ssd_t *currdisk = getssd(curr->devno);
    int blkno = curr->blkno;
    int count = curr->bcount;

    memset(gang_to_activate, 0, SSD_MAX_ELEMENTS * sizeof(int));

    /* **** CAREFUL ... HIJACKING tempint2 and tempptr2 fields here **** */
    curr->tempint2 = count;
    while (count != 0) {
        int gang_num;
        ioreq_event *tmp;
        gang_metadata *g;

        gang_num = choose_gang(blkno, currdisk);
        g = &currdisk->gang_meta[gang_num];

        // this gang must be activated
        gang_to_activate[gang_num] = 1;

        // create a new sub-request for the gang
        tmp = (ioreq_event *)getfromextraq();
        tmp->devno = curr->devno;
        tmp->busno = curr->busno;
        tmp->flags = curr->flags;
        tmp->blkno = blkno;
        tmp->tempptr2 = curr;
        tmp->bcount = ssd_choose_aligned_count(SSD_GANG_PAGE_SIZE(currdisk), blkno, count);
        ASSERT(tmp->bcount <= SSD_GANG_PAGE_SIZE(currdisk));

        blkno += tmp->bcount;
        count -= tmp->bcount;

        // add the request to the corresponding element's queue
        ioqueue_add_new_request(g->queue, (ioreq_event *)tmp);

        // increment the number of requests waiting in this gang.
        // if this is the first request, then this is the oldest
        // waiting request.
        g->reqs_waiting ++;
        if (g->reqs_waiting == 1) {
            g->oldest = simtime;
        }
    }

    // if we added a request to this gang, activate it
    for (i = 0; i < SSD_NUM_GANG(currdisk); i ++) {
        if (gang_to_activate[i]) {
            ssd_activate_gang_sync(i, currdisk);
        }
    }
}

void ssd_media_access_request_gang (ioreq_event *curr)
{
    int i;
    int gang_to_activate[SSD_MAX_ELEMENTS];
    ssd_t *currdisk = getssd(curr->devno);
    int blkno = curr->blkno;
    int count = curr->bcount;

   memset(gang_to_activate, 0, SSD_MAX_ELEMENTS * sizeof(int));

   /* **** CAREFUL ... HIJACKING tempint2 and tempptr2 fields here **** */
   curr->tempint2 = count;
   while (count != 0) {
        int gang_num;
        int elem_num;
        ssd_element *elem;
        ioreq_event *tmp;
        gang_metadata *g;

       // find the gang to direct the request
        choose_gang_and_element(blkno, curr, currdisk, &gang_num, &elem_num);

        // this gang must be activated
        gang_to_activate[gang_num] = 1;

        g = &currdisk->gang_meta[gang_num];
       elem = &currdisk->elements[elem_num];

       // create a new sub-request for the element
       tmp = (ioreq_event *)getfromextraq();
       tmp->devno = curr->devno;
       tmp->busno = curr->busno;
       tmp->flags = curr->flags;
       tmp->blkno = blkno;
       tmp->bcount = ssd_choose_aligned_count(currdisk->params.page_size, blkno, count);
       ASSERT(tmp->bcount == currdisk->params.page_size);

       tmp->tempptr2 = curr;
       blkno += tmp->bcount;
       count -= tmp->bcount;

       // add the request to the corresponding element's queue
       ioqueue_add_new_request(elem->queue, (ioreq_event *)tmp);

       // increment the number of requests waiting in this gang.
       // if this is the first request, then this is the oldest
       // waiting request.
       g->reqs_waiting ++;
       if (g->reqs_waiting == 1) {
           g->oldest = simtime;
       }
   }

   // if we added a request to this gang, activate it
   for (i = 0; i < SSD_NUM_GANG(currdisk); i ++) {
       if (gang_to_activate[i]) {
            ssd_activate_gang(currdisk, i);
       }
   }
}

/*
 * cleaning in a gang is over.
 */
void ssd_clean_gang_complete(ioreq_event *curr)
{
   ssd_t *currdisk;
   int i;
   int gang_num;

   currdisk = getssd (curr->devno);
   gang_num = curr->ssd_gang_num;

   ASSERT(currdisk->gang_meta[gang_num].busy == TRUE);
   ASSERT(currdisk->gang_meta[gang_num].cleaning == TRUE);

   // make sure no element of this gang is busy
   for (i = 0; i < currdisk->params.elements_per_gang; i ++) {
        int e = gang_num * currdisk->params.elements_per_gang + i;
        ASSERT (currdisk->elements[e].media_busy == FALSE);
   }

   // release this event
   addtoextraq((event *) curr);

   // activate the gang to serve the next set of requests
   currdisk->gang_meta[gang_num].busy = 0;
   currdisk->gang_meta[gang_num].cleaning = 0;

#if SYNC_GANG
   ssd_activate_gang_sync(gang_num, currdisk);
#else
   ssd_activate_gang(currdisk, gang_num);
#endif
}

void ssd_access_complete_gang_sync(ioreq_event *curr)
{
    ssd_t *currdisk;
    int gang_num;
    gang_metadata *gm;

    currdisk = getssd (curr->devno);
    gang_num = curr->ssd_gang_num;
    gm = &currdisk->gang_meta[gang_num];

    if (ioqueue_physical_access_done(gm->queue,curr) == NULL) {
        fprintf(stderr, "ssd_access_complete_gang_sync: ioreq_event not found by ioqueue_physical_access_done call\n");
        ASSERT(0);
    }

    // all the reqs are over
    ASSERT(currdisk->gang_meta[gang_num].cleaning == 0);
    if (ioqueue_get_reqoutstanding(gm->queue) == 0) {
        gm->busy = FALSE;
    }

    ssd_complete_parent(curr, currdisk);
    addtoextraq((event *) curr);
    ssd_activate_gang_sync(gang_num, currdisk);
}

void ssd_access_complete_gang (ioreq_event *curr)
{
   ssd_t *currdisk;
   int elem_num;
   ssd_element  *elem;
   ioreq_event *x;
   int i;
   int gang_num;
   int some_elem_busy;

   // fprintf (outputfile, "Entering ssd_access_complete: %12.6f\n", simtime);

   currdisk = getssd (curr->devno);
   elem_num = curr->ssd_elem_num;
   elem = &currdisk->elements[elem_num];
   gang_num = curr->ssd_gang_num;
   ASSERT(gang_num == elem->metadata.gang_num);

   if ((x = ioqueue_physical_access_done(elem->queue,curr)) == NULL) {
      fprintf(stderr, "ssd_access_complete:  ioreq_event not found by ioqueue_physical_access_done call\n");
      exit(1);
   }

   // all the reqs are over
   if (ioqueue_get_reqoutstanding(elem->queue) == 0) {
    elem->media_busy = FALSE;
   }

   some_elem_busy = 0;
   ASSERT(currdisk->gang_meta[gang_num].cleaning == 0);
   for (i = 0; i < currdisk->params.elements_per_gang; i ++) {
        int e = gang_num * currdisk->params.elements_per_gang + i;
        if (currdisk->elements[e].media_busy) {
            some_elem_busy = 1;
            break;
        }
   }

   if (!some_elem_busy) {
       currdisk->gang_meta[gang_num].busy = 0;
   }

    ssd_complete_parent(curr, currdisk);
    addtoextraq((event *) curr);
    ssd_activate_gang(currdisk, gang_num);
}
