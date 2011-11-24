
#ifndef _SSDMODEL_MODULES_H
#define _SSDMODEL_MODULES_H   


#include "ssdmodel_ssd_param.h"


static struct lp_mod *ssdmodel_mods[] = {
 &ssdmodel_ssd_mod 
};

typedef enum {
  SSDMODEL_MOD_SSD
} ssdmodel_mod_t;

#define SSDMODEL_MAX_MODULE 0
#endif // _SSDMODEL_MODULES_H
