#include "./remapper.h"

#define MRM_MAX_REMAPS 100

struct mrm_filter_node {
  struct mrm_filter_node     *next;
  struct mrm_filter_config    conf;
  unsigned                    refcnt;
};


struct mrm_remap_rule {
  struct mrm_filter_node   *filter;
  /* XXX link to interface remap !!! */
  unsigned char             match_macaddr[6];
  unsigned char             replace_macaddr[6];
};


static struct mrm_filter_node  *_filters = NULL;
static unsigned                 _remap_count = 0;
static struct mrm_remap_rule    _remaps[MRM_MAX_REMAPS];



mrm_remap_t
mrm_is_targeted_mac_address( const unsigned char * const macaddr ) {
  /* XXX this function needs some serious optimization! */

  unsigned i;

  for (i = 0; i < _remap_count; i++) {
     if (memcmp(_remaps[i].match_macaddr, macaddr, 6) != 0) continue;

     return &_remaps[i];
  }

  return NULL; /* no we aint filtering on this MAC address */
}

int
mrm_perform_ipv4_remap( const mrm_remap_t remap, unsigned char * const dst, const unsigned transmission_length, const struct iphdr * const iph ) {

  unsigned i;
  const struct mrm_filter_config * const filter =  &remap->filter->conf;

  for (i = 0; i < filter->rules_active; i++) {
    /**/
  }

  /* XXX WARNING: NOT FILTERING!!! ALWAYS APPLYING */
  memcpy(dst, remap->replace_macaddr, 6);

  return 0; /* success ? */
}

int
mrm_perform_ipv6_remap( const mrm_remap_t remap, unsigned char * const dst, const unsigned transmission_length, const struct ipv6hdr * const ip6h ) {
  unsigned i;
  const struct mrm_filter_config * const filter =  &remap->filter->conf;

  for (i = 0; i < filter->rules_active; i++) {
    /**/
  }

  /* XXX WARNING: NOT FILTERING!!! ALWAYS APPLYING */
  memcpy(dst, remap->replace_macaddr, 6);
  return 0;
}

unsigned
mrm_get_filter_count( void ) {
  struct mrm_filter_node   *f;
  unsigned                  result;

  for (result = 0, f = _filters; f != NULL; f = f->next, result++);
  
  return result;
}

int
mrm_get_filter( struct mrm_filter_config * const output, unsigned id ) {
  struct mrm_filter_node   *f;

  for (f = _filters; f != NULL; f = f->next, id--) {
    if (id != 0) continue;
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

    memcpy(&f->conf, filt, sizeof(*filt));
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

  return 0; /* success */
}

int
mrm_delete_filter( unsigned id ) {
  struct mrm_filter_node   *f, *fprev;

  for (f = _filters, fprev = NULL; f != NULL; fprev = f, f = f->next, id--) {
    if (id != 0) continue;
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

/* XXX "id" parameter redundant? */
int
mrm_get_remap_entry( struct mrm_remap_entry * const output, const unsigned id ) {
  const struct mrm_remap_rule *r;
  if (id >= _remap_count) return -EINVAL;

  r = &_remaps[id];

  output->id = id;
  memcpy(output->match_macaddr, r->match_macaddr, sizeof(r->match_macaddr));
  memcpy(output->replace_macaddr, r->replace_macaddr, sizeof(r->replace_macaddr));
  strncpy(output->filter_name, r->filter->conf.name, sizeof(output->filter_name));

  return 0; /* success */
}

int
mrm_set_remap_entry( const struct mrm_remap_entry * const remap ) {
  struct mrm_remap_rule *r;
  struct mrm_filter_node *f;
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
      return -ENOMEM;
    }

    r = &_remaps[_remap_count];
    memcpy(r->match_macaddr, remap->match_macaddr, sizeof(r->match_macaddr));
    memcpy(r->replace_macaddr, remap->replace_macaddr, sizeof(r->replace_macaddr));
    r->filter = f;
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

  /* thats all folks */
  return 0; /* success */
}

int
mrm_delete_remap( const unsigned id ) {
  struct mrm_remap_rule *r;
  unsigned int after_entries;

  if (id >= _remap_count) return -EINVAL;

  after_entries = _remap_count - id - 1;

  r = &_remaps[id];
  r->filter->refcnt--;

  /* XXX RACY!!! */
  memmove(r, r + 1, after_entries * sizeof(*r));

  --_remap_count;
  return 0; /* success */

}


void mrm_destroy_remapper_config( void ) {
  while (mrm_get_remap_count() > 0) {
    mrm_delete_remap(0);
  }

  while (mrm_get_filter_count() > 0) {
    mrm_delete_filter(0);
  }
}
