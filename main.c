#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netfilter.h>

#include <linux/netfilter_bridge.h>

#include "./remapper.h"
#include "./remapctl.h"



/* this function get called per each frame about to go out of a bridge (brctl) */
static unsigned int
mrm_bridge_outbound_hook(
  unsigned int hooknum,
  struct sk_buff *skb,
  const struct net_device *in,
  const struct net_device *out,
  int (*okfn)(struct sk_buff *)
  ) {

  unsigned char *dstmac;
  mrm_remap_t remap;

  if (skb == NULL) {
    printk(KERN_WARNING "MRM NULL SKB\n");
    return NF_ACCEPT;
  }

  dstmac = skb_mac_header(skb);
  if (dstmac == NULL) {
    printk(KERN_WARNING "MRM NULL SKB MAC Header\n");
    return NF_ACCEPT;
  }

  remap = mrm_is_targeted_mac_address(dstmac);
  if (remap == NULL) {
    /* our filter is not targeting this MAC address... pass this traffic through */
    return NF_ACCEPT;
  }

  /* the filter rules engine only cares about IPv4 or IPv6 traffic */
  switch (skb->protocol) {
  case ETH_P_IP:
    mrm_perform_ipv4_remap(remap, dstmac, skb->len, ip_hdr(skb));
    break;
  case ETH_P_IPV6:
    mrm_perform_ipv6_remap(remap, dstmac, skb->len, ipv6_hdr(skb));
    break;
  }

  /* always return NF_ACCEPT as we dont intend to filter out any traffic */
  return NF_ACCEPT;
}

static struct nf_hook_ops _hops = {
  owner:     THIS_MODULE,
  hook:      &mrm_bridge_outbound_hook,
  pf:        NFPROTO_BRIDGE,
  hooknum:   NF_BR_POST_ROUTING,
  priority:  NF_BR_PRI_FIRST,
};

static int __init
modinit( void ) {
  printk(KERN_INFO "MRM The MAC Address Re-Mapper is in the kernel\n");
  nf_register_hook(&_hops);
  mrm_init_remapctl(); /* XXX not checking for failure! */

  return 0; /* all is good */
}

static void __exit
modexit( void ) {
  mrm_destroy_remapctl();
  mrm_destroy_remapper_config();
  nf_unregister_hook(&_hops);
  printk(KERN_INFO "MRM The MAC Address Re-Mapper gone bye-bye\n");
}

module_init(modinit);
module_exit(modexit);

#warning What license should we use?
MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Jon Dennis <j.dennis@cablelabs.com>");
MODULE_DESCRIPTION("MAC Address Re-Mapper");

