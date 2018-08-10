#ifndef MRM_PRIVATE_H_INCLUDED
#define MRM_PRIVATE_H_INCLUDED


#include "./filter_config_accelerator.h"

#include <linux/list.h>

struct mrm_runconf_filter_node {
  struct list_head                       list;
  struct mrm_filter_config               conf;
  struct mrm_filter_config_accelerator   accelerator;
  unsigned                               refcnt;
};


struct mrm_runconf_remap_entry {
  struct mrm_runconf_filter_node   *filter;
  struct net_device                *replace_dev;
  unsigned char                     match_macaddr[6];
  unsigned char                     replace_macaddr[6];
};

#endif /* #ifndef MRM_PRIVATE_H_INCLUDED */
