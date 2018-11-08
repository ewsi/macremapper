#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter.h>

#include <linux/netfilter_bridge.h>

#include "./mrm_runconf.h"
#include "./mrm_rcdb.h"
#include "./mrm_ctlfile.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
#error Linux Kernel Version 3.7+ is required!
#endif



/* this function get called per each frame about to go out of a bridge (brctl) */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
static unsigned int
mrm_bridge_outbound_hook(
  unsigned int hooknum,
  struct sk_buff *skb,
  const struct net_device *in,
  const struct net_device *out,
  int (*okfn)(struct sk_buff *)
  ) {
#else
static unsigned int
mrm_bridge_outbound_hook(
  void *priv,
  struct sk_buff *skb,
  const struct nf_hook_state *state
  ) {

#endif

  unsigned char *dstmac;

  if (skb == NULL) {
    printk(KERN_WARNING "MRM NULL SKB\n");
    return NF_ACCEPT;
  }

  dstmac = skb_mac_header(skb);
  if (dstmac == NULL) {
    printk(KERN_WARNING "MRM NULL SKB MAC Header\n");
    return NF_ACCEPT;
  }

  /*
    it is unclear if we are always called with the RCU lock or not...
    (primarily due to the NF hooks in "br_netfilter.c")

    therefor, acquiring the RCU lock here...

    this should be ok... the RCU lock is "recursive" according to this:
    http://www.rdrop.com/users/paulmck/RCU/whatisRCU.html

  */


  rcu_read_lock();
  mrm_perform_ethernet_remap(dstmac, skb); /* XXX return value ? */
  rcu_read_unlock();

  /* always return NF_ACCEPT as we dont intend to filter out any traffic */
  return NF_ACCEPT;
}

static struct nf_hook_ops _hops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
  owner:     THIS_MODULE,
#endif
  hook:      &mrm_bridge_outbound_hook,
  pf:        NFPROTO_BRIDGE,
  hooknum:   NF_BR_POST_ROUTING,
  priority:  NF_BR_PRI_FIRST,
};

static int __init
modinit( void ) {
  int rv;

  rv = mrm_rcdb_init();
  if (rv != 0) return rv;

  nf_register_hook(&_hops);
  mrm_init_ctlfile(); /* XXX not checking for failure! */

  printk(KERN_INFO "MRM The MAC Address Re-Mapper is now in the kernel\n");

  return 0; /* all is good */
}

static void __exit
modexit( void ) {
  mrm_destroy_ctlfile();
  nf_unregister_hook(&_hops);
  mrm_rcdb_destroy(); /* imperative that this happens last */
  printk(KERN_INFO "MRM The MAC Address Re-Mapper gone bye-bye\n");
}

module_init(modinit);
module_exit(modexit);

MODULE_LICENSE("GPL"); /* GPL required for RCU sync stuff */
MODULE_AUTHOR("Jon Dennis <j.dennis@cablelabs.com>");
MODULE_DESCRIPTION("MAC Address Re-Mapper");

