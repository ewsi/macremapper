/*
* Copyright (c) 2018 Cable Television Laboratories, Inc. ("CableLabs")
*                    and others.  All rights reserved.
*
* Created by Jon Dennis (j.dennis@cablelabs.com)
*/

#ifndef FILTER_CONFIG_ACCELERATOR_H_INCLUDED
#define FILTER_CONFIG_ACCELERATOR_H_INCLUDED

#include "./macremapper_filter_config.h"

/*
  this file contains structures used to accelerate the 
  packet filtering process
*/



struct mrm_filter_rulerefset {
  unsigned                      rules_active;
  const struct mrm_filter_rule *rules[MRM_FILTER_MAX_RULES];
};


/*
    A single family ruleset is used to define references
    to rule instances based on the protocol.

    Currently there are three rule sets:
     . TCP -- TCP traffic consults this and only this ruleset
     . UDP -- UDP traffic consults this and only this ruleset
     . Other -- any traffic NOT TCP or UDP consults this and only this ruleset
*/
struct mrm_filter_single_family_protocol_ruleset {
  struct mrm_filter_rulerefset other_targeted_rules;
  struct mrm_filter_rulerefset tcp_targeted_rules;
  struct mrm_filter_rulerefset udp_targeted_rules;
};



/*
  the configuration accelerator structure
  in a nutshell, this is used to provide a set of 
  references to filter rules based on protocol

  current there are two rulesets:
   . IP4 -- IP4 traffic consults this and only this ruleset
   . IP6 -- IP6 traffic consults this and only this ruleset
*/
struct mrm_filter_config_accelerator {
  struct mrm_filter_single_family_protocol_ruleset ip4_targeted_rules;
  struct mrm_filter_single_family_protocol_ruleset ip6_targeted_rules;
};



/* functions used to work with these data structures */
void mrm_generate_acceleration_tables(struct mrm_filter_config_accelerator * const /* output */, const struct mrm_filter_config * const /* input */);



#endif /* #ifndef REMAPPER_CONFIG_ACCELERATOR_H_INCLUDED */

