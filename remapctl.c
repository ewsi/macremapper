

#include "./remapctl.h"
#include "./remapper.h"
#include "./macremapper_ioctl.h"

#include <linux/proc_fs.h>
#include <linux/uaccess.h>


#define PROC_FILENAME "macremapctl"


/* gets called when a userland process does a open("/proc/macremapctl",...) */
static int
mrm_handle_open (struct inode *in, struct file *f) {
  f->private_data = NULL; /* currently only being used for the read() function result text */
  return 0; /* success ? */
}

/* gets called when a userland process closes a file descriptor opened from "/proc/macremapctl" */
static int
mrm_handle_release (struct inode *in, struct file *f) {
  if (f->private_data != NULL) {
    /* currently only being used for the read() function result text */
    kfree(f->private_data);
    f->private_data = NULL; /* defensive */
  }
  return 0; /* success ? */
}

struct mrm_text_buf {
  unsigned  len;
  char      buf[ 20 * 1024 ]; /* 20k should be plenty here... */
};


static void
generate_text_report(struct mrm_text_buf * const tb) {
  char *ptr;
  unsigned i,j;
  unsigned count;
  struct mrm_filter_config f;
  struct mrm_remap_entry   r;
  struct mrm_filter_rule  *rule;

  #define ptrprintf(...) {ptr += snprintf(ptr, (sizeof(tb->buf) - (ptr - tb->buf)), __VA_ARGS__ ); if ((ptr - tb->buf) >= sizeof(tb->buf)) ptr = tb->buf + sizeof(tb->buf);}

  ptr = tb->buf;

  ptrprintf("MAC Address Re-Mapper Running Configuration:\n");


  count = mrm_get_filter_count();
  ptrprintf("  Filters: (Total Count %u)\n", count);
  for (i = 0; i < count; i++) {
    if (mrm_get_filter(&f, i) != 0) {
      ptrprintf("    FILTER GET FAILED IDX %u\n\n", i);
      continue;
    }
    ptrprintf("    Name: %.*s\n", sizeof(f.name), f.name);
    ptrprintf("    Rules: (Total Count %u)\n", f.rules_active);
    for (j = 0; j < f.rules_active; j++) {
      rule = &f.rules[j];
      ptrprintf("      ");
      if (rule->payload_size == 0) {
        ptrprintf("*:");
      }
      else {
        ptrprintf("%u:", rule->payload_size);
      }
      switch(rule->src_ipaddr.match_type) {
      case MRMIPFILT_MATCHANY:
        ptrprintf("*:");
        break;
      case MRMIPFILT_MATCHSINGLE:
        switch(rule->family) {
        case AF_INET:
          ptrprintf("%u.%u.%u.%u:",
                    (unsigned) rule->src_ipaddr.ipaddr4[0],
                    (unsigned) rule->src_ipaddr.ipaddr4[1],
                    (unsigned) rule->src_ipaddr.ipaddr4[2],
                    (unsigned) rule->src_ipaddr.ipaddr4[3]
          );
          break;
        case AF_INET6:
          ptrprintf("[%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X]:",
                    (unsigned) rule->src_ipaddr.ipaddr6[0],
                    (unsigned) rule->src_ipaddr.ipaddr6[1],
                    (unsigned) rule->src_ipaddr.ipaddr6[2],
                    (unsigned) rule->src_ipaddr.ipaddr6[3],
                    (unsigned) rule->src_ipaddr.ipaddr6[4],
                    (unsigned) rule->src_ipaddr.ipaddr6[5],
                    (unsigned) rule->src_ipaddr.ipaddr6[6],
                    (unsigned) rule->src_ipaddr.ipaddr6[7],
                    (unsigned) rule->src_ipaddr.ipaddr6[8],
                    (unsigned) rule->src_ipaddr.ipaddr6[9],
                    (unsigned) rule->src_ipaddr.ipaddr6[10],
                    (unsigned) rule->src_ipaddr.ipaddr6[11],
                    (unsigned) rule->src_ipaddr.ipaddr6[12],
                    (unsigned) rule->src_ipaddr.ipaddr6[13],
                    (unsigned) rule->src_ipaddr.ipaddr6[14],
                    (unsigned) rule->src_ipaddr.ipaddr6[15]
          );
          break;
        default:
          ptrprintf("\?\?\?:");
          break;
        }
        break;
      default:
        ptrprintf("\?\?\?:");
        break;
      }
      switch (rule->src_port.match_type) {
      case MRMPORTFILT_MATCHANY:
        ptrprintf("*:");
        break;
      case MRMPORTFILT_MATCHSINGLE:
        ptrprintf("%hu:", rule->src_port.portno);
        break;
      case MRMPORTFILT_MATCHRANGE:
        ptrprintf("%hu-%hu:", rule->src_port.low_portno, rule->src_port.high_portno);
        break;
      default:
        ptrprintf("\?\?\?:");
        break;
      }
      if (rule->proto.match_type == MRMIPPFILT_MATCHANY) {
        ptrprintf("*:");
      }
      else {
        if (rule->proto.match_type & MRMIPPFILT_MATCHUDP) {
          ptrprintf("udp");
          break;
        }
        else if (rule->proto.match_type & MRMIPPFILT_MATCHTCP) {
          ptrprintf("tcp");
          break;
        }
        else {
          ptrprintf("\?\?\?");
          break;
        }
        if (rule->proto.match_type & MRMIPPFILT_MATCHFAMILY) {
          switch (rule->family) {
          case AF_INET:
            ptrprintf("4");
            break;
          case AF_INET6:
            ptrprintf("6");
            break;
          default:
            ptrprintf("?");
            break;
          }
        }
        ptrprintf(":");
      }
      switch (rule->dst_port.match_type) {
      case MRMPORTFILT_MATCHANY:
        ptrprintf("*");
        break;
      case MRMPORTFILT_MATCHSINGLE:
        ptrprintf("%hu", rule->dst_port.portno);
        break;
      case MRMPORTFILT_MATCHRANGE:
        ptrprintf("%hu-%hu", rule->dst_port.low_portno, rule->dst_port.high_portno);
        break;
      default:
        ptrprintf("\?\?\?");
        break;
      }
    }
    ptrprintf("\n");
  }

  count = mrm_get_remap_count();
  ptrprintf("  Remap Entries: (Total Count %u)\n", count);
  for(i = 0; i < count; i++) {
    if (mrm_get_remap_entry(&r, i) != 0) {
      ptrprintf("    REMAP ENTRY GET FAILED IDX %u\n\n", i);
      continue;
    }
    ptrprintf("    Match MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
              (unsigned)r.match_macaddr[0],
              (unsigned)r.match_macaddr[1],
              (unsigned)r.match_macaddr[2],
              (unsigned)r.match_macaddr[3],
              (unsigned)r.match_macaddr[4],
              (unsigned)r.match_macaddr[5]
    );
    ptrprintf("    Replace MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
              (unsigned)r.replace_macaddr[0],
              (unsigned)r.replace_macaddr[1],
              (unsigned)r.replace_macaddr[2],
              (unsigned)r.replace_macaddr[3],
              (unsigned)r.replace_macaddr[4],
              (unsigned)r.replace_macaddr[5]
    );
    ptrprintf("    Filter: %.*s\n", sizeof(r.filter_name), r.filter_name);
    ptrprintf("\n");
  }


  tb->len = ptr - tb->buf;
}

static ssize_t
mrm_handle_read(struct file *f, char __user *buf, size_t size, loff_t *off) {
  struct mrm_text_buf *tb;
  int offset_content_size;
  int copy_size;

  /* Note: using f->private_data as our text buffer... */
  if (f->private_data == NULL) {
    /* this is the first call to read()... we need to generate the contents of this "virtual file" */
    f->private_data = kmalloc(sizeof(struct mrm_text_buf), GFP_KERNEL);
    if (f->private_data == NULL) {
      return -ENOMEM;
    }
    generate_text_report(f->private_data);
  }
  tb = f->private_data;

  offset_content_size = tb->len - *off;
  copy_size = (size > offset_content_size) ? offset_content_size : size;

  if ((*off) >= tb->len) {
    return 0; /* return no bytes read as we hit the end of our content */
  }

  if (copy_to_user(buf, &tb->buf[*off], copy_size) != 0) return -EFAULT;
  (*off) += copy_size;

  return copy_size;
}

static long
mrm_handle_ioctl(struct file *f, unsigned int type, void __user *param) {
  union {
    struct mrm_io_filter      io_filt;
    struct mrm_remap_entry    remap_entry;
    unsigned                  id;
    unsigned                  count;
  } u;
  int rv;

  switch (type) {
  /* ioctl()s for working with filters... */
  case MRM_GETFILTERCOUNT:
    u.count = mrm_get_filter_count();
    if (copy_to_user(param, &u.count, _IOC_SIZE(type)) != 0) return -EFAULT;
    return 0; /* success */
  case MRM_GETFILTER:
    if (copy_from_user(&u.io_filt, param, _IOC_SIZE(type)) != 0) return -EFAULT;
    rv = mrm_get_filter(&u.io_filt.conf, u.io_filt.id);
    if (rv == 0) {
      /* only copy back to user on success */
      if (copy_to_user(param, &u.io_filt, _IOC_SIZE(type)) != 0) return -EFAULT;
    }
    return rv;
  case MRM_SETFILTER:
    if (copy_from_user(&u.io_filt, param, _IOC_SIZE(type)) != 0) return -EFAULT;
    return mrm_set_filter(&u.io_filt.conf);
  case MRM_DELETEFILTER:
    if (copy_from_user(&u.id, param, _IOC_SIZE(type)) != 0) return -EFAULT;
    return mrm_delete_filter(u.id);

  /* ioctl()s for working with MAC address remappings... */
  case MRM_GETREMAPCOUNT:
    u.count = mrm_get_remap_count();
    if (copy_to_user(param, &u.count, _IOC_SIZE(type)) != 0) return -EFAULT;
    return 0; /* success */
  case MRM_GETREMAP:
    if (copy_from_user(&u.remap_entry, param, _IOC_SIZE(type)) != 0) return -EFAULT;
    rv = mrm_get_remap_entry(&u.remap_entry, u.io_filt.id);
    if (rv == 0) {
      /* only copy back to user on success */
      if (copy_to_user(param, &u.remap_entry, _IOC_SIZE(type)) != 0) return -EFAULT;
    }
    return rv;
  case MRM_SETREMAP:
    if (copy_from_user(&u.remap_entry, param, _IOC_SIZE(type)) != 0) return -EFAULT;
    return mrm_set_remap_entry(&u.remap_entry);
  case MRM_DELETEREMAP:
    if (copy_from_user(&u.id, param, _IOC_SIZE(type)) != 0) return -EFAULT;
    return mrm_delete_remap(u.id);

  default:
    return -ENOTTY; /* Inappropriate I/O control operation (POSIX.1) */
  }
  return -ENOTTY; /* Inappropriate I/O control operation (POSIX.1) */
}


static const struct file_operations _fops = {
  owner:           THIS_MODULE,
  open:            &mrm_handle_open,
  release:         &mrm_handle_release,
  read:            &mrm_handle_read,
  unlocked_ioctl:  (void*)&mrm_handle_ioctl,
};

int mrm_init_remapctl( void ) {
  /**/
  struct proc_dir_entry *pde;

  pde = proc_create(PROC_FILENAME, 0600, NULL, &_fops);
  if (pde == NULL) {
    return 0; /* failure */
  }

  return 1; /* success */
}

void mrm_destroy_remapctl( void ) {
  remove_proc_entry(PROC_FILENAME, NULL);
}






