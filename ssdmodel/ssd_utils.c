// DiskSim SSD support
// ©2008 Microsoft Corporation. All Rights Reserved

#include "ssd_utils.h"

//////////////////////////////////////////////////////////////////////////////
//                 code for bit manipulation routines
//////////////////////////////////////////////////////////////////////////////


//clears a particular bit.
//pos 0 corresponds to msb
void ssd_clear_bit(unsigned char *c, int pos)
{
    int byte = pos / 8;
    int bit_pos = 7 - (pos % 8);

    c[byte] = (c[byte] & (~(0x1 << bit_pos)));

    return;
}


//sets a particular bit.
//pos 0 corresponds to msb and pos 7 corresponds
//to lsb.
void ssd_set_bit(unsigned char *c, int pos)
{
    int byte = pos / 8;
    int bit_pos = 7 - (pos % 8);

    c[byte] = (c[byte] | (0x1 << bit_pos));

    return;
}

//returns true if a bit is set
int ssd_bit_on(unsigned char *c, int pos)
{
    int byte = pos / 8;
    int bit_pos = 7 - (pos % 8);

    return (c[byte] & (0x1 << bit_pos));
}

//finds the position of the first zero-th
//bit in an unsigned char array. the positions are
//numbered starting from 0 from msb to lsb. returns -1
//if all the bits are already set. 'total' specifies the
//number of bits to consider in the array. 'start' gives
//the location from which to start the search.
int ssd_find_zero_bit(unsigned char *c, int total, int start)
{
    int bit = start;
    int temp = total;

    while (temp) {
        int byte = bit / 8;
        if ((c[byte] >> (7 - (bit % 8))) & 0x1) {
            bit = (bit + 1) % total;
            temp --;
        } else {
            return bit;
        }
    }

    return -1;
}

//////////////////////////////////////////////////////////////////////////////
//             adding some code for a linked list module
//////////////////////////////////////////////////////////////////////////////

void ll_create(listnode **start)
{
    *start = (listnode *)malloc(sizeof(listnode));
    memset(*start, 0, sizeof(listnode));

    (*start)->data = (header_data *)malloc(sizeof(header_data));
    ((header_data *)(*start)->data)->size = 0;
}

// Free all the nodes in the list.
void ll_release(listnode *start)
{
    free(start->data);

    while (start) {
        listnode *next = start->next;

        if (next) {
            next->prev = NULL;

            if (next == start) {
                next = NULL;
            }
        }

        if (start->prev) {
            start->prev->next = NULL;
        }

        /* we just release the node. we don't release the
         * data that is contained in the node. that is the
         * responsibility of the function that is using
         * this linked list */
        free(start);

        start = next;
    }

    return;
}

listnode *_ll_insert_at_tail(listnode *start, listnode *toinsert)
{
    if ((!start) || (!toinsert)) {
        fprintf(stderr, "Error: invalid value to _ll_insert_at_tail\n");
        exit(-1);
    } else {
        if (start->prev) {
            listnode *prevnode = start->prev;

            toinsert->next = prevnode->next;
            prevnode->next->prev = toinsert;
            prevnode->next = toinsert;
            toinsert->prev = prevnode;
        } else {
            start->prev = toinsert;
            start->next = toinsert;
            toinsert->prev = start;
            toinsert->next = start;
        }

        return toinsert;
    }
}

listnode *_ll_insert_at_head(listnode *start, listnode *toinsert)
{
    if ((!start) || (!toinsert)) {
        fprintf(stderr, "Error: invalid value to _ll_insert_at_head\n");
        exit(-1);
    } else {
        if (start->next) {
            listnode *nextnode = start->next;

            toinsert->prev = nextnode->prev;
            nextnode->prev->next = toinsert;
            nextnode->prev = toinsert;
            toinsert->next = nextnode;
        } else {
            start->next = toinsert;
            start->prev = toinsert;
            toinsert->next = start;
            toinsert->prev = start;
        }

        return toinsert;
    }
}

/*
 * Insert the data at the tail.
 */
listnode *ll_insert_at_tail(listnode *start, void *data)
{
    /* allocate a new node */
    listnode *newnode = malloc(sizeof(listnode));
    newnode->data = data;

    /* increment the number of entries */
    ((header_data *)(start->data))->size ++;

    return _ll_insert_at_tail(start, newnode);
}

/*
 * Insert the data at the head.
 */
listnode *ll_insert_at_head(listnode *start, void *data)
{
    /* allocate a new node */
    listnode *newnode = malloc(sizeof(listnode));
    newnode->data = data;

    /* increment the number of entries */
    ((header_data *)(start->data))->size ++;

    return _ll_insert_at_head(start, newnode);
}

/*
 * Release a node from the linked list.
 */
void ll_release_node(listnode *start, listnode *node)
{
    if ((!node) || (!start)) {
        fprintf(stderr, "Error: invalid items on the list\n");
        exit(-1);
    } else {
        ((header_data *)(start->data))->size --;

        if (((header_data *)(start->data))->size > 0) {
            listnode *nodesprev = node->prev;

            nodesprev->next = node->next;
            node->next->prev = nodesprev;
        } else {
            /* do some sanity checking */

            /* there is just one element left in the list
             * and we want to release it. so, check if it
             * matches the prev and next of the start node. */
            if ((start->next != node) || (start->prev != node)) {
                fprintf(stderr, "Error: sanity check failed\n");
                exit(-1);
            }

            /* this is the last element - just release it */
            start->next = NULL;
            start->prev = NULL;
        }

        /* we just release the node. we don't release the
         * data that is contained in the node. that is the
         * responsibility of the function that is using
         * this linked list */
        free(node);
    }
}

/*
 * Release the tail node from the linked list.
 */
void ll_release_tail(listnode *start)
{
    listnode *tail = start->prev;

    if ((!tail) || (tail == start)) {
        fprintf(stderr, "Warning: the list is empty. no element to release\n");
        return;
    } else {
        ll_release_node(start, tail);
    }
}

listnode *ll_get_tail(listnode *start)
{
    return (start->prev->data);
}

/*
 * Return the total number of nodes in the list.
 */
int ll_get_size(listnode *start)
{
    return ((header_data *)(start->data))->size;
}

/*
 * For debugging purposes.
 * We can make some improvements on performance,
 * for e.g., by searching from the tail if n is
 * greater than size/2.
 */
listnode *ll_get_nth_node(listnode *start, int n)
{
    int i;
    int reverse;
    listnode *node;

    if (n >= ll_get_size(start)) {
        fprintf(stderr, "Error: n (%d) is greater than size %d\n",
            n, ll_get_size(start));
        return NULL;
    }

    if (n > ll_get_size(start)/2) {
        i = ll_get_size(start) - 1;
        reverse = 1;
        node = start->prev;
    } else {
        i = 0;
        reverse = 0;
        node = start->next;
    }

    /* go over each node and print it */
    while ((node) && (node != start)) {
        if (i == n) {
            return node;
        }

        if (reverse) {
            node = node->prev;
            i --;
        } else {
            node = node->next;
            i ++;
        }
    }

    fprintf(stderr, "Error: cannot find the %d node in list\n", n);
    return NULL;
}
