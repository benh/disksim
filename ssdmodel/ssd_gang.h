// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#ifndef _DISKSIM_SSD_GANG_H
#define _DISKSIM_SSD_GANG_H

#include "ssd.h"
#include "modules/ssdmodel_ssd_param.h"

void ssd_media_access_request_gang_sync(ioreq_event *curr);
void ssd_media_access_request_gang(ioreq_event *curr);
void ssd_clean_gang_complete(ioreq_event *curr);
void ssd_access_complete_gang_sync(ioreq_event *curr);
void ssd_access_complete_gang(ioreq_event *curr);

#endif
