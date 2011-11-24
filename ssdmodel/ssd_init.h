// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#ifndef DISKSIM_SSD_INIT_H
#define DISKSIM_SSD_INIT_H

#include "ssd.h"

void ssd_initialize_diskinfo();
int ssd_elem_export_size(ssd_t *currdisk);
void ssd_element_metadata_init(int elem_number, ssd_element_metadata *metadata, ssd_t *currdisk);
void ssd_plane_init(ssd_element *elem, ssd_t *s, int devno);
void ssd_verify_parameters(ssd_t *currdisk);
void ssd_initialize (void);
void ssd_resetstats (void);

#endif
