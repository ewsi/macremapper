#ifndef MACREMAPPER_IOCTL_H_INCLUDED
#define MACREMAPPER_IOCTL_H_INCLUDED

#include <asm/ioctl.h>

#define MRM_FILTER_MAX_RULES 15
#define MRM_FILTER_NAME_MAX  24


#define MRM_IOCTL_TYPE 77

/* ioctl()s for working with filters... */
#define MRM_GETFILTERCOUNT _IOR  (MRM_IOCTL_TYPE, 0, unsigned)
#define MRM_GETFILTER      _IOWR (MRM_IOCTL_TYPE, 1, struct mrm_io_filter)
#define MRM_SETFILTER      _IOW  (MRM_IOCTL_TYPE, 2, struct mrm_io_filter)
#define MRM_DELETEFILTER   _IOW  (MRM_IOCTL_TYPE, 3, unsigned)

/* ioctl()s for working with MAC address remappings... */
#define MRM_GETREMAPCOUNT  _IOR  (MRM_IOCTL_TYPE, 10, unsigned)
#define MRM_GETREMAP       _IOWR (MRM_IOCTL_TYPE, 11, struct mrm_remap_entry)
#define MRM_SETREMAP       _IOW  (MRM_IOCTL_TYPE, 12, struct mrm_remap_entry)
#define MRM_DELETEREMAP    _IOW  (MRM_IOCTL_TYPE, 13, unsigned)


/* filter data types */
struct mrm_ipaddr_filter {
  enum {
    MRMIPFILT_MATCHANY     = 0,
    MRMIPFILT_MATCHSINGLE,
    MRMIPFILT_MATCHSUBNET,
    MRMIPFILT_MATCHRANGE,
  } match_type;

  union {
    unsigned char ipaddr4[4];
    unsigned char ipaddr6[16];
  };
};

struct mrm_ipproto_filter {
  enum {
    /* OR these to form combinations... */
    MRMIPPFILT_MATCHANY     = 0,
    MRMIPPFILT_MATCHFAMILY  = 1,
    MRMIPPFILT_MATCHUDP     = 2,
    MRMIPPFILT_MATCHTCP     = 4,
  } match_type;
};

struct mrm_port_filter {
  enum {
    MRMPORTFILT_MATCHANY     = 0,
    MRMPORTFILT_MATCHSINGLE,
    MRMPORTFILT_MATCHRANGE,
  } match_type;

  union {
    unsigned short portno;
    unsigned short low_portno;
  };
  unsigned short high_portno;
};

struct mrm_filter_rule {
  unsigned payload_size; /* >= this to trigger... 0 = match all */

  int                        family; /* AF_INET || AF_INET6 */
  struct mrm_ipproto_filter  proto;
  struct mrm_ipaddr_filter   src_ipaddr;
  struct mrm_port_filter     src_port;
  struct mrm_port_filter     dst_port;
};

struct mrm_filter_config {
  char                    name[MRM_FILTER_NAME_MAX];
  unsigned                rules_active; /* count of active rules... cant be > MRM_FILTER_MAX_RULES */
  struct mrm_filter_rule  rules[MRM_FILTER_MAX_RULES];
};

struct mrm_io_filter {
  unsigned                  id; /* only used as an input parameter for  MRM_GETFILTER */
  struct mrm_filter_config  conf;
};


/* remap data types */
struct mrm_remap_entry {
  unsigned        id; /* only used as an input parameter for MRM_GETREMAP */
  unsigned char   match_macaddr[6];
  unsigned char   replace_macaddr[6];
  /* char            replace_ifname[...]; */
  char            filter_name[MRM_FILTER_NAME_MAX];
};


#endif /* #ifndef MACREMAPPER_IOCTL_H_INCLUDED */
