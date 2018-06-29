#ifndef REMAPPER_H_INCLUDED
#define REMAPPER_H_INCLUDED

#include <linux/ip.h>
#include <linux/ipv6.h>

#include "./macremapper_ioctl.h" /* XXX consider putting these types in something not specific to ioctl() */

typedef const struct mrm_remap_rule * mrm_remap_t;
mrm_remap_t mrm_is_targeted_mac_address( const unsigned char * const /* macaddr */ );
int mrm_perform_ipv4_remap( const mrm_remap_t /* remap */, unsigned char * const /* dst */, const unsigned transmission_length, const struct iphdr * const /* iph */ );
int mrm_perform_ipv6_remap( const mrm_remap_t /* remap */, unsigned char * const /* dst */, const unsigned transmission_length, const struct ipv6hdr * const /* ip6h */ );

unsigned mrm_get_filter_count( void );
int mrm_get_filter( struct mrm_filter_config * const /* output */, unsigned /* id */ );
int mrm_set_filter( const struct mrm_filter_config * const /* filt */ );
int mrm_delete_filter( unsigned /* id */ );

unsigned mrm_get_remap_count( void );
int mrm_get_remap_entry( struct mrm_remap_entry * const /* output */, const unsigned /* id */ );
int mrm_set_remap_entry( const struct mrm_remap_entry * const /* remap */ );
int mrm_delete_remap( const unsigned /* id */ );

void mrm_destroy_remapper_config( void );

#endif /* #ifndef REMAPPER_H_INCLUDED */
