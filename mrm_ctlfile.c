

#include "./mrm_ctlfile.h"
#include "./mrm_runconf.h"
#include "./macremapper_ioctl.h"
#include "./bufprintf.h"

#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>


#define PROC_FILENAME "macremapctl"


/* declare a mutex to enforce one transcation at a time
   in the event multiple processes/tasks are using this
   file concurrently...
*/
static struct mutex _ctrl_mutex;


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

static ssize_t
mrm_handle_read(struct file *f, char __user *buf, size_t size, loff_t *off) {
  struct bufprintf_buf *tb;
  int offset_content_size;
  int copy_size;

  /* Note: using f->private_data as our text buffer... */
  if (f->private_data == NULL) {
    /* this is the first call to read()... we need to generate the contents of this "virtual file" */
    tb = f->private_data = kmalloc(sizeof(struct bufprintf_buf), GFP_KERNEL);
    if (f->private_data == NULL) {
      return -ENOMEM;
    }
    bufprintf_init(tb);
    mutex_lock(&_ctrl_mutex);
    rcu_read_lock();
    mrm_bufprintf_running_configuration(tb);
    rcu_read_unlock();
    mutex_unlock(&_ctrl_mutex);
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
    struct mrm_filter_config  filt_conf;
    struct mrm_remap_entry    remap_entry;
    unsigned                  count;
  } u;
  int rv;

  mutex_lock(&_ctrl_mutex);
  rcu_read_lock();

  switch (type) {
  /* ioctl()s for working with filters... */
  case MRM_GETFILTERCOUNT:
    u.count = mrm_get_filter_count();
    if (copy_to_user(param, &u.count, _IOC_SIZE(type)) != 0) goto fail_fault;
    rv = 0; /* success */
    break;
  case MRM_GETFILTER:
    if (copy_from_user(&u.filt_conf, param, _IOC_SIZE(type)) != 0) goto fail_fault;
    rv = mrm_get_filter(&u.filt_conf);
    if (rv == 0) {
      /* only copy back to user on success */
      if (copy_to_user(param, &u.filt_conf, _IOC_SIZE(type)) != 0) goto fail_fault;
    }
    break;
  case MRM_SETFILTER:
    if (copy_from_user(&u.filt_conf, param, _IOC_SIZE(type)) != 0) goto fail_fault;
    rv = mrm_set_filter(&u.filt_conf);
    break;
  case MRM_DELETEFILTER:
    if (copy_from_user(&u.filt_conf, param, _IOC_SIZE(type)) != 0) goto fail_fault;
    rv = mrm_delete_filter(&u.filt_conf);
    break;

  /* ioctl()s for working with MAC address remappings... */
  case MRM_GETREMAPCOUNT:
    u.count = mrm_get_remap_count();
    if (copy_to_user(param, &u.count, _IOC_SIZE(type)) != 0) goto fail_fault;
    rv = 0; /* success */
    break;
  case MRM_GETREMAP:
    if (copy_from_user(&u.remap_entry, param, _IOC_SIZE(type)) != 0) goto fail_fault;
    rv = mrm_get_remap_entry(&u.remap_entry);
    if (rv == 0) {
      /* only copy back to user on success */
      if (copy_to_user(param, &u.remap_entry, _IOC_SIZE(type)) != 0) goto fail_fault;
    }
    break;
  case MRM_SETREMAP:
    if (copy_from_user(&u.remap_entry, param, _IOC_SIZE(type)) != 0) goto fail_fault;
    rv = mrm_set_remap_entry(&u.remap_entry);
    break;
  case MRM_DELETEREMAP:
    if (copy_from_user(&u.remap_entry, param, _IOC_SIZE(type)) != 0) goto fail_fault;
    rv = mrm_delete_remap(u.remap_entry.match_macaddr);
    break;

  /* ioctl() for completely blowing away the running configuration */
  case MRM_WIPERUNCONF:
    rcu_read_unlock();
    mrm_destroy_remapper_config(); /* do NOT hold RCU lock to this... */
    rcu_read_lock();
    rv = 0; /* success */
    break;

  default:
    rv = -ENOTTY; /* Inappropriate I/O control operation (POSIX.1) */
    break;
  }

  rcu_read_unlock();
  synchronize_rcu(); /* is this really necessary? */
  mutex_unlock(&_ctrl_mutex);
  return rv;

fail_fault:
  rcu_read_unlock();
  mutex_unlock(&_ctrl_mutex);
  return -EFAULT;

}


static const struct file_operations _fops = {
  owner:           THIS_MODULE,
  open:            &mrm_handle_open,
  release:         &mrm_handle_release,
  read:            &mrm_handle_read,
  unlocked_ioctl:  (void*)&mrm_handle_ioctl,
};

int mrm_init_ctlfile( void ) {
  struct proc_dir_entry *pde;

  pde = proc_create(PROC_FILENAME, 0600, NULL, &_fops);
  if (pde == NULL) {
    return 0; /* failure */
  }

  mutex_init(&_ctrl_mutex);

  return 1; /* success */
}

void mrm_destroy_ctlfile( void ) {
  remove_proc_entry(PROC_FILENAME, NULL);
}






