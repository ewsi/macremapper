/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#include "./mrm_runconf.h"

#include "./mrm_private.h"
#include "./mrm_rcdb.h"
#include "./filter_config_accelerator.h"

#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/udp.h>
#include <linux/tcp.h>




static inline int
mrm_perform_ipv4_remap(
  const struct mrm_runconf_remap_entry * const remaprule,
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
    const void *          transportptr;
  } u;
  unsigned i;
  unsigned short src_port;
  unsigned short dst_port;
  __be32         src_subnet;
  unsigned validate_port;
  const struct mrm_filter_single_family_protocol_ruleset * const target_rules = &remaprule->filter->accelerator.ip4_targeted_rules;

  validate_port = 0;
  iph = ip_hdr(skb);

  /*
    compute the pointer to the transport (udp/tcp/whatever l4 protocol) header here...

    unfortunately the sk_buff does not always have it's transport_header member
    populated. I am hesitant to make a call to skb_set_transport_header() anywhere
    in here (which feels more like "the right way" to do this) because I want
    to minimize changes in state to the sk_buff.

    i really probably should be using ip_hdrlen() here, but in the spirit of
    performance and since we already have the pointer to the iphdr struct, a
    simple * 4 to the ihl field will do.
  */
  u.transportptr = ((unsigned char *)iph) + (iph->ihl * 4);

  switch (iph->protocol) {
  case IPPROTO_TCP:
    validate_port = 1;
    ruleref = &target_rules->tcp_targeted_rules;
    src_port = ntohs(u.tcph->source);
    dst_port = ntohs(u.tcph->dest);
    break;
  case IPPROTO_UDP:
    validate_port = 1;
    ruleref = &target_rules->udp_targeted_rules;
    src_port = ntohs(u.udph->source);
    dst_port = ntohs(u.udph->dest);
    break;
  default:
    ruleref = &target_rules->other_targeted_rules;
    src_port = 0;
    dst_port = 0;
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
  const struct mrm_runconf_remap_entry * const remaprule,
  unsigned char * const dst,
  const unsigned transmission_length,
  struct sk_buff * const skb
  ) {
  return 0; /* XXX NOT IMPLEMENTED!!! */
}

static inline void
mrm_apply_remap(
    struct mrm_runconf_remap_entry * const remaprule,
    unsigned char * const dst,
    struct sk_buff * const skb
  ) {
  /* this is THE function that actually moves the frame elsewhere... */

  /* implement a basic "round-robin" replacement policy... */
  const unsigned replace_idx = remaprule->replace_idx;
  if (++remaprule->replace_idx >= remaprule->replace_count) remaprule->replace_idx = 0;

  memcpy(dst, remaprule->replace[replace_idx].macaddr, 6);
  if (remaprule->replace[replace_idx].dev != NULL) {
    skb->dev = remaprule->replace[replace_idx].dev;
  }
}

int
mrm_perform_ethernet_remap(unsigned char * const dst, struct sk_buff * const skb) {
  struct mrm_runconf_remap_entry * remaprule;
  unsigned transmission_length;

  /* first and foremost, is the traffic targeted for us? */
  remaprule = mrm_rcdb_lookup_remap_entry_by_macaddr(dst);
  if (remaprule == NULL) {
    return 0; /* traffic not targeted for us */
  }

  if (remaprule->filter == NULL) {
    printk(KERN_WARNING "MRM No filter associated for matched remap\n");
    return 0; /* dont have a filter for this rule... */
  }

  transmission_length = skb->len;

  /* determine what kind of traffic this is... */
  switch (htons(skb->protocol)) {
  case ETH_P_IP:
    if (mrm_perform_ipv4_remap(remaprule, dst, transmission_length, skb)) {
      mrm_apply_remap(remaprule, dst, skb);
      return 1; /* remap applied */
    }
    break;
  case ETH_P_IPV6:
    if (mrm_perform_ipv6_remap(remaprule, dst, transmission_length, skb)) {
      mrm_apply_remap(remaprule, dst, skb);
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
  return mrm_rcdb_get_filter_count(); /* XXX redundant */
}

int
mrm_get_filter( struct mrm_filter_config * const output ) {
  struct mrm_runconf_filter_node   *f;

  f = mrm_rcdb_lookup_filter_by_name(output->name);
  if (f == NULL) return -EINVAL;
  memcpy(output, &f->conf, sizeof(f->conf));

  return 0; /* success */
}

int
mrm_set_filter( const struct mrm_filter_config * const filt ) {
  struct mrm_runconf_filter_node   *f;

  f = mrm_rcdb_insert_filter(filt->name);
  if (f == NULL) return -ENOMEM;

  memcpy(&f->conf, filt, sizeof(*filt));
  mrm_generate_acceleration_tables(&f->accelerator, &f->conf);
  return 0; /* success */
}

int
mrm_delete_filter( const struct mrm_filter_config * const filt ) {
  struct mrm_runconf_filter_node *f;

  f = mrm_rcdb_lookup_filter_by_name(filt->name);
  if (f == NULL) return -EINVAL; /* filter not found */
  return mrm_rcdb_delete_filter(f);
}




unsigned
mrm_get_remap_count( void ) {
  return mrm_rcdb_get_remap_count(); /* XXX redundant */
}

int
mrm_get_remap_entry( struct mrm_remap_entry * const e) {
  const struct mrm_runconf_remap_entry *r;
  unsigned i;

  r = mrm_rcdb_lookup_remap_entry_by_macaddr(e->match_macaddr);
  if (r == NULL) return -EINVAL; /* remap entry not found */

  strncpy(e->filter_name, r->filter->conf.name, sizeof(e->filter_name));

  for (i = 0; i < r-> replace_count; ++i) {
    memcpy(e->replace[i].macaddr, r->replace[i].macaddr, sizeof(r->replace[i].macaddr));

    /* XXX WARNING!
       this is not currently populating the device name!!
    */
  }
  return 0; /* success */
}

int
mrm_set_remap_entry( const struct mrm_remap_entry * const remap ) {
  struct mrm_runconf_filter_node *f;
  const unsigned char *replace_macaddrs[MRM_MAX_REPLACE];
  struct net_device *dev[MRM_MAX_REPLACE];
  unsigned i;
  int rv;

  /* initial values... */
  memset(&dev, 0, sizeof(dev));
  memset(&replace_macaddrs, 0, sizeof(replace_macaddrs));
  rv = 0; /* sucess until proven otherwise */

  /* validate the replacement targets... */
  if ((remap->replace_count < 1) || (remap->replace_count > MRM_MAX_REPLACE)) {
    printk(KERN_WARNING "MRM Bad remap replace count!\n");
    rv = -EINVAL;
    goto done;
  }

  /* find the specified filter by name... */
  f = mrm_rcdb_lookup_filter_by_name(remap->filter_name);
  if (f == NULL) {
    /* given filter name does not exist! */
    printk(KERN_WARNING "MRM Invalid Filter Name!\n");
    rv = -EINVAL;
    goto done;
  }

  /* resolve the interface name & copy MAC address pointers for each given replacement... */
  for (i = 0; i < remap->replace_count; ++i) {
    if (remap->replace[i].ifname[0] != '\0') {
      if (strnlen(remap->replace[i].ifname, sizeof(remap->replace[i].ifname)) == sizeof(remap->replace[i].ifname)) {
        printk(KERN_WARNING "MRM Replace interface name too long!\n");
        rv = -EINVAL;
        goto done; /* sanity check to ensure the string is "\0" terminated */
      }
      /* XXX WARNING: using "&init_net" here makes it use the 
                      "main system" network namespace...
                      if this is invoked by an ioctl() from
                      a process within a container, this
                      may be a security issue... TBD...
      */
      dev[i] = dev_get_by_name(&init_net, remap->replace[i].ifname);
      if (dev[i] == NULL) {
        printk(KERN_WARNING "MRM Bad interface name: '%s'!\n", remap->replace[i].ifname);
        rv = -EINVAL;
        goto done; /* failed to lookup device by name */
      }
    }
    
    replace_macaddrs[i] = remap->replace[i].macaddr;
  }

  /* IMPORTANT: as of here, the reference count has been increased on dev */

  /* insert/update remap entry... */
  if (mrm_rcdb_update_remap_entry(remap->match_macaddr, f, remap->replace_count, replace_macaddrs, dev) == NULL) {
    /* failed for some reason... most likely were full */
    rv = -ENOMEM;
    goto done;
  }

  /* note: once a remap entry is successfully inserted, it is now the 
           responsibility of "mrm_rcdb.c" to "dev_put()" the
           referenced net_device...
  */


done:
  if (rv < 0) {
    /* something failed, need to cleanup any references... */
    for (i = 0; i < (sizeof(dev) / sizeof(dev[0])); ++i) {
      if (dev[i] != NULL) dev_put(dev[i]);
    }
  }

  /* thats all folks */
  return rv;
}

int
mrm_delete_remap( const unsigned char * const macaddr ) {
  struct mrm_runconf_remap_entry *r;

  /* first lookup the remap entry */
  r = mrm_rcdb_lookup_remap_entry_by_macaddr(macaddr);
  if (r == NULL)
    return -EINVAL; /* remap entry not found */

  /* attempt to remove the remap entry... */
  mrm_rcdb_delete_remap_entry(r);

  return 0; /* success */
}


void mrm_destroy_remapper_config( void ) {
  mrm_rcdb_clear(); /* XXX redundant */
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
  unsigned i,j;
  unsigned filter_count;
  unsigned remap_count;
  const struct mrm_runconf_filter_node  *f;
  const struct mrm_runconf_remap_entry  *r;
  struct mrm_filter_rulerefset           all_rrs;

  bufprintf(tb, "MAC Address Re-Mapper Running Configuration:\n");

  filter_count = mrm_get_filter_count();
  bufprintf(tb, "  Filters: (Total Count %u)\n", filter_count);
  for (i = 0; i < filter_count; i++) {
    f = mrm_rcdb_lookup_filter_by_index(i);

    all_rrs.rules_active = f->conf.rules_active;
    for (j = 0; j < all_rrs.rules_active; j++) {
      all_rrs.rules[j] = &f->conf.rules[j];
    }

    bufprintf(tb, "    Name: %.*s\n", (int)sizeof(f->conf.name), f->conf.name);
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
  remap_count = mrm_get_remap_count();
  bufprintf(tb, "  Remap Entries: (Total Count %u)\n", remap_count);
  for(i = 0; i < remap_count; i++) {
    r = mrm_rcdb_lookup_remap_entry_by_index(i);

    bufprintf(tb, "    Match MAC Address: ");
    dump_single_mac_address(tb, r->match_macaddr);
    bufprintf(tb, "    Replacements: (Total Count %u)\n", r->replace_count);
    for (j = 0; j <  r->replace_count; ++j) {
      bufprintf(tb, "      MAC Address %u: ", j);
      dump_single_mac_address(tb, r->replace[j].macaddr);
      bufprintf(tb, "      Interface %u: ", j);
      if (r->replace[j].dev == NULL) {
        bufprintf(tb, "(None)\n");
      }
      else {
        bufprintf(tb, "%.*s\n", (int)sizeof(r->replace[j].dev->name), r->replace[j].dev->name);
      }
    }

    bufprintf(tb, "    Filter: %.*s\n", (int)sizeof(r->filter->conf.name), r->filter->conf.name);
    bufprintf(tb, "\n");
  }
}

