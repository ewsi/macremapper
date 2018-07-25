#include "./remapper.h"

#include "./filter_config_accelerator.h"

#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <linux/tcp.h>


#define MRM_MAX_REMAPS 100

struct mrm_filter_node {
  struct mrm_filter_node                *next;
  struct mrm_filter_config               conf;
  struct mrm_filter_config_accelerator   accelerator;
  unsigned                               refcnt;
};


struct mrm_remap_rule {
  struct mrm_filter_node   *filter;
  struct net_device        *replace_dev;
  unsigned char             match_macaddr[6];
  unsigned char             replace_macaddr[6];
};


static struct mrm_filter_node  *_filters = NULL;
static unsigned                 _remap_count = 0;
static struct mrm_remap_rule    _remaps[MRM_MAX_REMAPS];



static inline struct mrm_remap_rule *
mrm_is_targeted_mac_address( const unsigned char * const macaddr ) {
  /* XXX this function needs some serious optimization! */

  unsigned i;

  for (i = 0; i < _remap_count; i++) {
     if (memcmp(_remaps[i].match_macaddr, macaddr, 6) != 0) continue;

     return &_remaps[i];
  }

  return NULL; /* no we aint filtering on this MAC address */
}

static inline int
mrm_perform_ipv4_remap(
  const struct mrm_remap_rule * const remaprule,
  unsigned char * const dst,
  const unsigned transmission_length,
  struct sk_buff * const skb
  ) {

  const struct mrm_filter_rulerefset * ruleref;
  const struct mrm_filter_rule *rule;
  const struct iphdr * iph;
  union {
    const struct udphdr * udph;
    const struct tcphdr * tcph;
  } u;
  unsigned i;
  unsigned short src_port;
  unsigned short dst_port;
  __be32         src_subnet;
  unsigned validate_port;
  const struct mrm_filter_single_family_protocol_ruleset * const target_rules = &remaprule->filter->accelerator.ip4_targeted_rules;

  validate_port = 0;
  iph = ip_hdr(skb);

  switch (ntohs(iph->protocol)) {
  case IPPROTO_TCP:
    validate_port = 1;
    ruleref = &target_rules->tcp_targeted_rules;
    u.tcph  = tcp_hdr(skb);
    src_port = ntohs(u.tcph->source);
    dst_port = ntohs(u.tcph->dest);
    break;
  case IPPROTO_UDP:
    validate_port = 1;
    ruleref = &target_rules->udp_targeted_rules;
    u.udph  = udp_hdr(skb);
    src_port = ntohs(u.udph->source);
    dst_port = ntohs(u.udph->dest);
    break;
  default:
    ruleref = &target_rules->other_targeted_rules;
    break;
  }

  /* this loop only iterates on rules which apply to
     the current incoming packet protocol
     
     this means that at this point, the protocol and 
     family are already validated applicable for this
     packet
  */
  for (i = 0; i < ruleref->rules_active; i++) {
    rule = ruleref->rules[i];

    /* validate transmission length */
    if (transmission_length < rule->payload_size) continue;

    /* validate IP match */
    switch (rule->src_ipaddr.match_type) {
    case MRMIPFILT_MATCHANY:
      break;
    case MRMIPFILT_MATCHSINGLE:
      if (rule->src_ipaddr.ipaddr4.s_addr != iph->saddr) {
        continue;
      }
      break;
    case MRMIPFILT_MATCHSUBNET:
      src_subnet  = iph->saddr;
      src_subnet &= rule->src_ipaddr.ipaddr4_mask.s_addr;
      if (rule->src_ipaddr.ipaddr4.s_addr != src_subnet) {
        continue;
      }
      break;
    case MRMIPFILT_MATCHRANGE:
      if (ntohl(iph->saddr) < ntohl(rule->src_ipaddr.ipaddr4_start.s_addr)) {
        continue;
      }
      if (ntohl(iph->saddr) > ntohl(rule->src_ipaddr.ipaddr4_end.s_addr)) {
        continue;
      }
    }

    if (!validate_port) return 1; /* rule matches... tell caller to perform remap */

    switch (rule->src_port.match_type) {
    case MRMPORTFILT_MATCHANY:
      break;
    case MRMPORTFILT_MATCHSINGLE:
      if (src_port != rule->src_port.portno) continue;
      break;
    case MRMPORTFILT_MATCHRANGE:
      if (src_port < rule->src_port.low_portno) {
        continue;
      }
      if (src_port > rule->src_port.high_portno) {
        continue;
      }
    }

    switch (rule->dst_port.match_type) {
    case MRMPORTFILT_MATCHANY:
      break;
    case MRMPORTFILT_MATCHSINGLE:
      if (dst_port != rule->dst_port.portno) continue;
      break;
    case MRMPORTFILT_MATCHRANGE:
      if (dst_port < rule->dst_port.low_portno) {
        continue;
      }
      if (dst_port > rule->dst_port.high_portno) {
        continue;
      }
    }

    return 1; /* rule matches... tell caller to perform remap */
  }

  return 0; /* no match... dont remap MAC address */
}

static inline int
mrm_perform_ipv6_remap(
  const struct mrm_remap_rule * const remaprule,
  unsigned char * const dst,
  const unsigned transmission_length,
  struct sk_buff * const skb
  ) {
  return 0; /* XXX NOT IMPLEMENTED!!! */
}

int
mrm_perform_ethernet_remap(unsigned char * const dst, struct sk_buff * const skb) {
  struct mrm_remap_rule * remaprule;
  unsigned transmission_length;

  /* first and foremost, is the traffic targeted for us? */
  remaprule = mrm_is_targeted_mac_address(dst);
  if (remaprule == NULL) {
    return 0; /* traffic not targeted for us */
  }

  transmission_length = skb->len;

  /* determine what kind of traffic this is... */
  switch (htons(skb->protocol)) {
  case ETH_P_IP:
    if (mrm_perform_ipv4_remap(remaprule, dst, transmission_length, skb)) {
      memcpy(dst, remaprule->replace_macaddr, 6);
      if (remaprule->replace_dev != NULL) {
        skb->dev = remaprule->replace_dev;
      }
      return 1; /* remap applied */
    }
    break;
  case ETH_P_IPV6:
    if (mrm_perform_ipv6_remap(remaprule, dst, transmission_length, skb)) {
      memcpy(dst, remaprule->replace_macaddr, 6);
      if (remaprule->replace_dev != NULL) {
        skb->dev = remaprule->replace_dev;
      }
      return 1; /* remap applied */
    }
    break;
  default:
    break; /* not ip4 || ip6... traffic not targeted for us */
  }

  return 0; /* remap not applied */
}

unsigned
mrm_get_filter_count( void ) {
  struct mrm_filter_node   *f;
  unsigned                  result;

  for (result = 0, f = _filters; f != NULL; f = f->next, result++);
  
  return result;
}

int
mrm_get_filter( struct mrm_filter_config * const output ) {
  struct mrm_filter_node   *f;

  for (f = _filters; f != NULL; f = f->next) {
    if (strncmp(f->conf.name, output->name, sizeof(output->name)) != 0) continue;
    memcpy(output, &f->conf, sizeof(f->conf));
    return 0; /* success */
  }

  return -EINVAL;
}

int
mrm_set_filter( const struct mrm_filter_config * const filt ) {
  struct mrm_filter_node   *f;

  for (f = _filters; f != NULL; f = f->next) {
    if (strncmp(f->conf.name, filt->name, sizeof(filt->name)) != 0) continue;

    /* XXX racy!!! */
    /* XXX racy!!! */
    /* XXX racy!!! */
    memcpy(&f->conf, filt, sizeof(*filt));
    mrm_generate_acceleration_tables(&f->accelerator, &f->conf);
    return 0; /* success */
  }

  /* not found and existing... need to create a new filter... */
  if (_filters == NULL) {
    _filters = f = kmalloc(sizeof(*f), GFP_KERNEL);
  }
  else {
    for (f = _filters; f->next != NULL; f = f->next);
    f->next = kmalloc(sizeof(*f), GFP_KERNEL);
    f = f->next;
  }
  if (f == NULL) {
    return -ENOMEM;
  }
  memset(f, 0, sizeof(*f));

  memcpy(&f->conf, filt, sizeof(*filt));
  mrm_generate_acceleration_tables(&f->accelerator, &f->conf);

  return 0; /* success */
}

int
mrm_delete_filter( const struct mrm_filter_config * const filt ) {
  struct mrm_filter_node   *f, *fprev;

  for (f = _filters, fprev = NULL; f != NULL; fprev = f, f = f->next) {
    if (strncmp(f->conf.name, filt->name, sizeof(filt->name)) != 0) continue;
    if (f->refcnt > 0) {
      return -EADDRINUSE; /* cant delete... in use */
    }
    if (fprev) fprev->next = f->next;
    if (f == _filters) _filters = f->next;
    kfree(f);
    return 0; /* success */
  }

  return -EINVAL; /* failure */
}




unsigned
mrm_get_remap_count( void ) {
  return _remap_count;
}

int
mrm_get_remap_entry( struct mrm_remap_entry * const e) {
  unsigned i;
  const struct mrm_remap_rule *r;

  for (i = 0; i < _remap_count; i++) {
    r = &_remaps[i];
    if (memcmp(r->match_macaddr, e->match_macaddr, sizeof(r->match_macaddr)) != 0) continue;

    memcpy(e->replace_macaddr, r->replace_macaddr, sizeof(r->replace_macaddr));
    strncpy(e->filter_name, r->filter->conf.name, sizeof(e->filter_name));
    return 0; /* success */
  }

  return -EINVAL; /* entry not found */
}

int
mrm_set_remap_entry( const struct mrm_remap_entry * const remap ) {
  struct mrm_remap_rule *r;
  struct mrm_filter_node *f;
  struct net_device *dev;
  struct net_device *freedev;
  unsigned i;

  /* XXX RACY!!! */
  /* XXX RACY!!! */
  /* XXX RACY!!! */
  /* XXX RACY!!! */
  /* XXX RACY!!! */

  /* first find the specified filter by name... */
  for (f = _filters; f != NULL; f = f->next) {
    if (strncmp(f->conf.name, remap->filter_name, sizeof(remap->filter_name)) == 0) break;;
  }
  if (f == NULL) {
    /* given filter name does not exist! */
    printk(KERN_WARNING "MRM Invalid Filter Name!\n");
    return -EINVAL;
  }

  /* resolve the given interface name... */
  dev = NULL;
  if (remap->replace_ifname[0] != '\0') {
    if (strnlen(remap->replace_ifname, sizeof(remap->replace_ifname)) == sizeof(remap->replace_ifname)) {
      return -EINVAL; /* sanity check to ensure the string is "\0" terminated */
    }
    /* XXX WARNING: using "&init_net" here makes it use the 
                    "main system" network namespace...
                    if this is invoked by an ioctl() from
                    a process within a container, this
                    may be a surity issue... TBD...
    */
    dev = dev_get_by_name(&init_net, remap->replace_ifname);
    if (dev == NULL) {
      return -EINVAL; /* failed to lookup device by name */
    }
  }

  /* IMPORTANT: as of here, the reference count has been increased on dev */

  /* check to see if we have an existing remap entry */
  for (r=NULL, i = 0; i < _remap_count; i++) {
    if (memcmp(_remaps[i].match_macaddr, remap->match_macaddr, sizeof(_remaps[i].match_macaddr)) == 0) {
      r = &_remaps[i];
      break;
    }
  }

  /* create a new remap entry if not found... */
  if (r == NULL) {
    /* didnt find an existing remap entry with matching mac address...
       create one... */
    if (_remap_count == MRM_MAX_REMAPS) {
      /* were full... cant insert any more remaps */
      if (dev != NULL) dev_put(dev);
      return -ENOMEM;
    }

    r = &_remaps[_remap_count];
    memcpy(r->match_macaddr, remap->match_macaddr, sizeof(r->match_macaddr));
    memcpy(r->replace_macaddr, remap->replace_macaddr, sizeof(r->replace_macaddr));
    r->filter = f;
    r->replace_dev = NULL;
    f->refcnt++;
    ++_remap_count;
  }

  /* ok so now both the remap and filter variable are pointing to the correct place... */
  if (f != r->filter) {
    /* filter changed... update both reference counters and swap */
    f->refcnt++;
    r->filter->refcnt--;
    r->filter = f;
  }

  /* copy over the new replace value (if any) */
  memcpy(r->replace_macaddr, remap->replace_macaddr, sizeof(r->replace_macaddr));

  /* apply the interface device replace setting */
  if (dev == r->replace_dev) {
    /* replace device is already correct... no need to change */
    if (dev != NULL) dev_put(dev);
  }
  else {
    /* need to replace the device */
    freedev = r->replace_dev;
    r->replace_dev = dev;
    if (freedev) dev_put(freedev);
  }

  /* thats all folks */
  return 0; /* success */
}

int
mrm_delete_remap( const unsigned char * const macaddr ) {
  struct mrm_remap_rule *r;
  unsigned i;
  unsigned after_entries;

  /* find the remap entry */
  for (r=NULL, i = 0; i < _remap_count; i++) {
    r = &_remaps[i];
    if (memcmp(r->match_macaddr, macaddr, sizeof(r->match_macaddr)) != 0) continue;

    /* remote the filter reference counter */
    r->filter->refcnt--;

    /* decrement the net device reference count */
    if (r->replace_dev != NULL) {
      dev_put(r->replace_dev);
      r->replace_dev = NULL;
    }

    /* XXX racy!!! */
    /* compute how many entries are in the array afterwards, 
       then shift them down in the array by one element */
    after_entries = _remap_count - i - 1;
    memmove(r, r + 1, after_entries * sizeof(*r));

    /* finally decrement the amount of entries in the array */
    --_remap_count;
    return 0; /* success */
  }

  return -EINVAL; /* entry not found */
}


void mrm_destroy_remapper_config( void ) {
  while (mrm_get_remap_count() > 0) {
    mrm_delete_remap(_remaps[0].match_macaddr);
  }

  while (mrm_get_filter_count() > 0) {
    mrm_delete_filter(&_filters->conf);
  }
}






#include "./bufprintf.h"


static void
dump_single_mac_address(struct bufprintf_buf * const tb, const unsigned char * const macaddr) {
  bufprintf(tb, "%02X:%02X:%02X:%02X:%02X:%02X\n",
            (unsigned)macaddr[0],
            (unsigned)macaddr[1],
            (unsigned)macaddr[2],
            (unsigned)macaddr[3],
            (unsigned)macaddr[4],
            (unsigned)macaddr[5]
  );
}

static void
dump_single_ip(struct bufprintf_buf * const tb, const int family, const unsigned char * const ip) {

  switch (family) {
  case AF_INET:
    bufprintf(tb, "%u.%u.%u.%u",
              (unsigned) ip[0],
              (unsigned) ip[1],
              (unsigned) ip[2],
              (unsigned) ip[3]
    );
    break;
  case AF_INET6:
    bufprintf(tb, "[%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X]",
              (unsigned) ip[0],
              (unsigned) ip[1],
              (unsigned) ip[2],
              (unsigned) ip[3],
              (unsigned) ip[4],
              (unsigned) ip[5],
              (unsigned) ip[6],
              (unsigned) ip[7],
              (unsigned) ip[8],
              (unsigned) ip[9],
              (unsigned) ip[10],
              (unsigned) ip[11],
              (unsigned) ip[12],
              (unsigned) ip[13],
              (unsigned) ip[14],
              (unsigned) ip[15]
    );
    break;
  default:
    bufprintf(tb, "bad_family");
    break;
  }
}

static void
dump_single_port_filter(struct bufprintf_buf * const tb, const struct mrm_port_filter * const pf) {
  switch (pf->match_type) {
  case MRMPORTFILT_MATCHANY:
    bufprintf(tb, "any");
    break;
  case MRMPORTFILT_MATCHSINGLE:
    bufprintf(tb, "%hu", pf->portno);
    break;
  case MRMPORTFILT_MATCHRANGE:
    bufprintf(tb, "%hu-%hu", pf->low_portno, pf->high_portno);
    break;
  default:
    bufprintf(tb, "unknown");
    break;
  }
}

static void
dump_single_ruleset(struct bufprintf_buf * const tb, const char * const text, const struct mrm_filter_rulerefset * const ruleset) {

  unsigned i;
  const struct mrm_filter_rule *rule;

  bufprintf(tb, "    \"%s Rules\" (Total Count %u):\n", text, ruleset->rules_active);

  for ( i = 0; i < ruleset->rules_active; i++) {
    rule = ruleset->rules[i];
    bufprintf(tb, "      ");
    bufprintf(tb, "payload_size=%u", rule->payload_size);
    bufprintf(tb, " family=");
    switch(rule->family) {
    case AF_UNSPEC: bufprintf(tb, "AF_UNSPEC"); break;
    case AF_INET:   bufprintf(tb, "AF_INET"); break;
    case AF_INET6:  bufprintf(tb, "AF_INET6"); break;
    default:        bufprintf(tb, "Unknown(%d)", rule->family); break;
    }

    bufprintf(tb, " proto=");
    if ((rule->proto.match_type & MRMIPPFILT_MATCHUDP) == MRMIPPFILT_MATCHUDP) {
      bufprintf(tb, "udp");
    }
    else if ((rule->proto.match_type & MRMIPPFILT_MATCHTCP) == MRMIPPFILT_MATCHTCP) {
      bufprintf(tb, "tcp");
    }
    else if ((rule->proto.match_type & (~MRMIPPFILT_MATCHFAMILY)) == MRMIPPFILT_MATCHANY) {
      bufprintf(tb, "any");
    }
    else {
      bufprintf(tb, "unknown");
    }
    if ((rule->proto.match_type & MRMIPPFILT_MATCHFAMILY) == MRMIPPFILT_MATCHFAMILY) {
      bufprintf(tb, "%d", (rule->family == AF_INET) ? 4 : 6);
    }

    bufprintf(tb, " srcip=");
    switch(rule->src_ipaddr.match_type) {
    case MRMIPFILT_MATCHANY: bufprintf(tb, "any"); break;
    case MRMIPFILT_MATCHSINGLE: 
      dump_single_ip(tb, rule->family, (rule->family == AF_INET) ? (const void*)&rule->src_ipaddr.ipaddr4       : (const void*)&rule->src_ipaddr.ipaddr6);
      break;
    case MRMIPFILT_MATCHSUBNET: 
      dump_single_ip(tb, rule->family, (rule->family == AF_INET) ? (const void*)&rule->src_ipaddr.ipaddr4       : (const void*)&rule->src_ipaddr.ipaddr6);
      bufprintf(tb, "/");
      dump_single_ip(tb, rule->family, (rule->family == AF_INET) ? (const void*)&rule->src_ipaddr.ipaddr4_mask  : (const void*)&rule->src_ipaddr.ipaddr6_mask);
      break;
    case MRMIPFILT_MATCHRANGE: 
      dump_single_ip(tb, rule->family, (rule->family == AF_INET) ? (const void*)&rule->src_ipaddr.ipaddr4_start : (const void*)&rule->src_ipaddr.ipaddr6_start);
      bufprintf(tb, "-");
      dump_single_ip(tb, rule->family, (rule->family == AF_INET) ? (const void*)&rule->src_ipaddr.ipaddr4_end   : (const void*)&rule->src_ipaddr.ipaddr6_end);
      break;
    }

    bufprintf(tb, " srcport=");
    dump_single_port_filter(tb, &rule->src_port);

    bufprintf(tb, " dstport=");
    dump_single_port_filter(tb, &rule->dst_port);

    bufprintf(tb, "\n");
  }
}

void
mrm_bufprintf_running_configuration(struct bufprintf_buf * const tb) {
  unsigned count;
  unsigned i;
  const struct mrm_filter_node  *f;
  const struct mrm_remap_rule   *r;
  struct mrm_filter_rulerefset   all_rrs;

  bufprintf(tb, "MAC Address Re-Mapper Running Configuration:\n");

  count = mrm_get_filter_count();
  bufprintf(tb, "  Filters: (Total Count %u)\n", count);
  for (f = _filters; f != NULL; f = f->next) {

    all_rrs.rules_active = f->conf.rules_active;
    for (i = 0; i < all_rrs.rules_active; i++) {
      all_rrs.rules[i] = &f->conf.rules[i];
    }

    bufprintf(tb, "    Name: %.*s\n", sizeof(f->conf.name), f->conf.name);
    bufprintf(tb, "    Remap Reference Count: %u\n", f->refcnt);
    bufprintf(tb, "    Total Rule Count: %u\n", f->conf.rules_active);
    dump_single_ruleset(tb, "All Configured", &all_rrs);
    dump_single_ruleset(tb, "TCP/IP4-Only", &f->accelerator.ip4_targeted_rules.tcp_targeted_rules);
    dump_single_ruleset(tb, "UDP/IP4-Only", &f->accelerator.ip4_targeted_rules.udp_targeted_rules);
    dump_single_ruleset(tb, "Other/IP4-Only", &f->accelerator.ip4_targeted_rules.other_targeted_rules);
    dump_single_ruleset(tb, "TCP/IP6-Only", &f->accelerator.ip6_targeted_rules.tcp_targeted_rules);
    dump_single_ruleset(tb, "UDP/IP6-Only", &f->accelerator.ip6_targeted_rules.udp_targeted_rules);
    dump_single_ruleset(tb, "Other/IP6-Only", &f->accelerator.ip6_targeted_rules.other_targeted_rules);
    bufprintf(tb, "\n");
  }
  count = mrm_get_remap_count();
  bufprintf(tb, "  Remap Entries: (Total Count %u)\n", _remap_count);
  for(i = 0; i < _remap_count; i++) {
    r = &_remaps[i];

    bufprintf(tb, "    Match MAC Address: ");
    dump_single_mac_address(tb, r->match_macaddr);
    bufprintf(tb, "    Replace MAC Address: ");
    dump_single_mac_address(tb, r->replace_macaddr);
    bufprintf(tb, "    Replace Interface: ");
    if (r->replace_dev == NULL) {
      bufprintf(tb, "(None)\n");
    }
    else {
      bufprintf(tb, "%.*s\n", sizeof(r->replace_dev->name), r->replace_dev->name);
    }

    bufprintf(tb, "    Filter: %.*s\n", sizeof(r->filter->conf.name), r->filter->conf.name);
    bufprintf(tb, "\n");
  }
}

