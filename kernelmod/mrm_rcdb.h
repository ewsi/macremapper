/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#ifndef MRM_RCDC_H_INCLUDED
#define MRM_RCDC_H_INCLUDED

#include "./mrm_private.h"

int mrm_rcdb_init( void );
void mrm_rcdb_destroy( void );
void mrm_rcdb_clear( void );

/* filter functions... */
unsigned mrm_rcdb_get_filter_count( void );
struct mrm_runconf_filter_node *mrm_rcdb_lookup_filter_by_name(const char * const /* name */);
struct mrm_runconf_filter_node *mrm_rcdb_lookup_filter_by_index(unsigned /* index */);
struct mrm_runconf_filter_node *mrm_rcdb_insert_filter( const char * const /* name */);
int mrm_rcdb_delete_filter( struct mrm_runconf_filter_node * const /* filter */ );


/* remap entry functions... */
unsigned mrm_rcdb_get_remap_count( void );
struct mrm_runconf_remap_entry *mrm_rcdb_lookup_remap_entry_by_macaddr(const unsigned char * const /* macaddr */);
struct mrm_runconf_remap_entry *mrm_rcdb_lookup_remap_entry_by_index(unsigned /* index */);
struct mrm_runconf_remap_entry *mrm_rcdb_update_remap_entry(const unsigned char * const /* match_macaddr */, struct mrm_runconf_filter_node * const /* filter */, const unsigned char * const /* replace_macaddr */, struct net_device * const /* replace_dev */);
void mrm_rcdb_delete_remap_entry(struct mrm_runconf_remap_entry * const /* remap_entry */);


#endif /* #ifndef MRM_RCDC_H_INCLUDED */
