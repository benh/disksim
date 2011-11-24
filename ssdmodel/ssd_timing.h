// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#ifndef DISKSIM_SSD_TIMING_H
#define DISKSIM_SSD_TIMING_H

typedef struct _ssd_timing_t {
    int     (*choose_element)(struct _ssd_timing_t *t, int blkno);
    void    (*free)(struct _ssd_timing_t *t);
} ssd_timing_t;

        // Modules implementing this interface choose an alignment boundary for requests.
        // They enforce this boundary by returning counts less than requested from choose_aligned_count
        // Typically the alignment just past the last sector in a request is zero mod 8 or 16,
        // and in no cases will a returned count cross a stride or block boundary.
        //
        // The results of compute_delay are not meaningful if a count is supplied that was not
        // dictated by an earlier call to choose_aligned_count.

// get a timing object ... params pointer is valid for lifetime of element
ssd_timing_t   *ssd_new_timing_t(ssd_timing_params *params);
int ssd_choose_aligned_count(int page_size, int blkno, int count);
void ssd_compute_access_time(ssd_t *s, int elem_num, ssd_req **reqs, int total);

#endif
