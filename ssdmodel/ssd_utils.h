// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#ifndef __DISKSIM_SSD_UTILS_H__
#define __DISKSIM_SSD_UTILS_H__
#include "ssd.h"

//////////////////////////////////////////////////////////////////////////////
//                 code for bit manipulation routines
//////////////////////////////////////////////////////////////////////////////

void ssd_clear_bit(unsigned char *c, int pos);
void ssd_set_bit(unsigned char *c, int pos);
int ssd_bit_on(unsigned char *c, int pos);
int ssd_find_zero_bit(unsigned char *c, int total, int start);


//////////////////////////////////////////////////////////////////////////////
//             adding some code for a linked list module
//////////////////////////////////////////////////////////////////////////////

struct _listnode;

typedef struct _listnode {
    struct _listnode *prev;
    struct _listnode *next;
    void *data;
} listnode;

typedef struct _header_data {
    unsigned int size;
} header_data;

void ll_create(listnode **start);
void ll_release(listnode *start);
listnode *ll_insert_at_tail(listnode *start, void *data);
listnode *ll_insert_at_head(listnode *start, void *data);
void ll_release_node(listnode *start, listnode *node);
void ll_release_tail(listnode *start);
listnode *ll_get_tail(listnode *start);
int ll_get_size(listnode *start);
listnode *ll_get_nth_node(listnode *start, int n);

#endif

