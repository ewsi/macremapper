/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#ifndef MRM_PRIVATE_H_INCLUDED
#define MRM_PRIVATE_H_INCLUDED


#include "./filter_config_accelerator.h"

#include <linux/list.h>
#include <linux/hash.h>

struct mrm_runconf_filter_node {
  struct list_head                       list;
  struct rcu_head                        rcu;
  struct mrm_filter_config               conf;
  struct mrm_filter_config_accelerator   accelerator;
  unsigned                               refcnt;
};


struct mrm_runconf_remap_entry {
  struct hlist_node                 hlist;
  struct rcu_head                   rcu;
  struct mrm_runconf_filter_node   *filter;
  unsigned char                     match_macaddr[6];
  unsigned                          replace_count; /* total count of elements in the replace[] member */
  unsigned                          replace_idx;   /* used by the "critical path" to round-robin which replace[] member is to be used */
  struct {
    unsigned char                     macaddr[6];
    struct net_device                *dev;
  } replace[MRM_MAX_REPLACE];
};

#endif /* #ifndef MRM_PRIVATE_H_INCLUDED */
