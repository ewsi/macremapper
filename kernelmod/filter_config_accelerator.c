// SPDX-License-Identifier: GPL-2.0-only
/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#include "./filter_config_accelerator.h"

#include <linux/string.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

static inline int
does_rule_apply_for_family(const struct mrm_filter_rule * const rule, const int af) {
  if (rule->family == af) return 1; /* specified family matches... no need to go further */
  if (rule->proto.match_type & MRMIPPFILT_MATCHFAMILY) return 0; /* protocol match family is set */
  if (rule->src_ipaddr.match_type != MRMIPFILT_MATCHANY) return 0; /* match address is not any */
  return 1; /* specified address family does not match, however, protocol and ip are match any */
}


static inline void
push_rule(
  struct mrm_filter_rulerefset * const ruleset,
  const struct mrm_filter_rule * const rule
  ) {
  if (ruleset->rules_active >= MRM_FILTER_MAX_RULES) {
    printk(KERN_WARNING "MRM Ruleset Full. Dropping rule!");
    return; /* full. */
  }
  ruleset->rules[ruleset->rules_active++] = rule;
}

void
mrm_generate_acceleration_tables(
  struct mrm_filter_config_accelerator * const output,
  const struct mrm_filter_config * const input
  ) {
  unsigned i;
  const struct mrm_filter_rule * rule;

  struct mrm_filter_rulerefset * const r_ip4udp = &output->ip4_targeted_rules.udp_targeted_rules;
  struct mrm_filter_rulerefset * const r_ip4tcp = &output->ip4_targeted_rules.tcp_targeted_rules;
  struct mrm_filter_rulerefset * const r_ip4oth = &output->ip4_targeted_rules.other_targeted_rules;
  struct mrm_filter_rulerefset * const r_ip6udp = &output->ip6_targeted_rules.udp_targeted_rules;
  struct mrm_filter_rulerefset * const r_ip6tcp = &output->ip6_targeted_rules.tcp_targeted_rules;
  struct mrm_filter_rulerefset * const r_ip6oth = &output->ip6_targeted_rules.other_targeted_rules;

  memset(output, 0, sizeof(*output));

  for (i = 0; i < input->rules_active; i++) {
    rule = &input->rules[i];

    /* ipv4 ? */
    if (does_rule_apply_for_family(rule, AF_INET)) {
      /* udp traffic */
      if (
        ((rule->proto.match_type & MRMIPPFILT_MATCHUDP) == MRMIPPFILT_MATCHUDP) ||
        ((rule->proto.match_type & (~MRMIPPFILT_MATCHFAMILY)) == MRMIPPFILT_MATCHANY) 
      ) {
        push_rule(r_ip4udp, rule);
      }
      /* tcp traffic */
      if (
        ((rule->proto.match_type & MRMIPPFILT_MATCHTCP) == MRMIPPFILT_MATCHTCP) ||
        ((rule->proto.match_type & (~MRMIPPFILT_MATCHFAMILY)) == MRMIPPFILT_MATCHANY) 
      ) {
        push_rule(r_ip4tcp, rule);
      }
      /* traffic the is neither tcp nor udp */
      if ((rule->proto.match_type & (~MRMIPPFILT_MATCHFAMILY)) == MRMIPPFILT_MATCHANY) {
        /* only insert if both src and dst port are match any
           because the "other" traffic type probably dont use ports */
        if ((rule->src_port.match_type == MRMPORTFILT_MATCHANY) && (rule->dst_port.match_type == MRMPORTFILT_MATCHANY)) {
          push_rule(r_ip4oth, rule);
        }
      }
    }

    /* ipv6 ? */
    if (does_rule_apply_for_family(rule, AF_INET6)) {
      /* udp traffic */
      if (
        ((rule->proto.match_type & MRMIPPFILT_MATCHUDP) == MRMIPPFILT_MATCHUDP) ||
        ((rule->proto.match_type & (~MRMIPPFILT_MATCHFAMILY)) == MRMIPPFILT_MATCHANY) 
      ) {
        push_rule(r_ip6udp, rule);
      }
      /* tcp traffic */
      if (
        ((rule->proto.match_type & MRMIPPFILT_MATCHTCP) == MRMIPPFILT_MATCHTCP) ||
        ((rule->proto.match_type & (~MRMIPPFILT_MATCHFAMILY)) == MRMIPPFILT_MATCHANY) 
      ) {
        push_rule(r_ip6tcp, rule);
      }
      /* traffic the is neither tcp nor udp */
      if ((rule->proto.match_type & (~MRMIPPFILT_MATCHFAMILY)) == MRMIPPFILT_MATCHANY) {
        /* only insert if both src and dst port are match any
           because the "other" traffic type probably dont use ports */
        if ((rule->src_port.match_type == MRMPORTFILT_MATCHANY) && (rule->dst_port.match_type == MRMPORTFILT_MATCHANY)) {
          push_rule(r_ip6oth, rule);
        }
      }
    }
  }
}
