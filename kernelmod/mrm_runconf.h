/* SPDX-License-Identifier: GPL-2.0-only */
/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#ifndef MRM_RUNCONF_H_INCLUDED
#define MRM_RUNCONF_H_INCLUDED

#include "./macremapper_filter_config.h"

#include <linux/skbuff.h>

int mrm_perform_ethernet_remap( unsigned char * const /* dst */, struct sk_buff * const /* skb */);

unsigned mrm_get_filter_count( void );
int mrm_get_filter( struct mrm_filter_config * const /* output */ );
int mrm_set_filter( const struct mrm_filter_config * const /* filt */ );
int mrm_delete_filter( const struct mrm_filter_config * const /* filt */ );

unsigned mrm_get_remap_count( void );
int mrm_get_remap_entry( struct mrm_remap_entry * const /* e */);
int mrm_set_remap_entry( const struct mrm_remap_entry * const /* remap */ );
int mrm_delete_remap( const unsigned char * const /* macaddr */ );

void mrm_destroy_remapper_config( void );


struct bufprintf_buf;
void mrm_bufprintf_running_configuration(struct bufprintf_buf * const /* tb */);

#endif /* #ifndef MRM_RUNCONF_H_INCLUDED */
