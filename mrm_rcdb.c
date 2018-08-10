

#include "./mrm_rcdb.h"

#include <linux/etherdevice.h> /* ether_addr_equal() */
#include <linux/slab.h>
#include <linux/string.h>




#define MRM_MAX_REMAPS 100

static struct mrm_runconf_filter_node   *_filters = NULL;
static unsigned                          _remap_count = 0;
static struct mrm_runconf_remap_entry    _remaps[MRM_MAX_REMAPS];


void
mrm_rcdb_destroy( void ) {
  while (_remap_count > 0) {
    mrm_rcdb_delete_remap_entry(_remaps);
  }

  while (_filters != NULL) {
    mrm_rcdb_delete_filter(_filters);
  }

}


/* filter functions... */

unsigned
mrm_rcdb_get_filter_count( void ) {
  struct mrm_runconf_filter_node   *f;
  unsigned                          result;

  for (result = 0, f = _filters; f != NULL; f = f->next, result++);
  
  return result;
}

struct mrm_runconf_filter_node *
mrm_rcdb_lookup_filter_by_name(const char * const name) {
  struct mrm_runconf_filter_node   *f;

  for (f = _filters; f != NULL; f = f->next) {
    if (strncmp(f->conf.name, name, sizeof(f->conf.name)) == 0)
      return f;
  }

  return NULL; /* not found */
}

struct mrm_runconf_filter_node *mrm_rcdb_lookup_filter_by_index(unsigned index ) {
  struct mrm_runconf_filter_node   *f;

  for (f = _filters; (f != NULL) && (index > 0); f = f->next, index--);
  return f;
}

struct mrm_runconf_filter_node *
mrm_rcdb_insert_filter( const char * const name) {
  struct mrm_runconf_filter_node   *f;
  struct mrm_runconf_filter_node   *rv;

  rv = mrm_rcdb_lookup_filter_by_name(name);
  if (rv != NULL) return rv; /* filter by said name already exists */

  rv = kmalloc(sizeof(*rv), GFP_KERNEL);
  if (rv == NULL) return NULL; /* out of memory */
  memset(rv, 0, sizeof(*rv));

  if (_filters == NULL) {
    _filters = rv;
  }
  else {
    for (f = _filters; f->next != NULL; f = f->next);
    f->next = rv;
  }

  return rv;
}

int
mrm_rcdb_delete_filter( struct mrm_runconf_filter_node * const filter ) {
  struct mrm_runconf_filter_node   *f, *fprev;

  for (f = _filters, fprev = NULL; f != NULL; fprev = f, f = f->next) {
    if (filter != f) continue;
    if (f->refcnt > 0) return -EADDRINUSE; /* cant delete... in use */
    if (fprev) fprev->next = f->next;
    if (f == _filters) _filters = f->next;
    kfree(f);
    return 0; /* success */
  }

  return -EINVAL; /* failed to lookup */
}





/* remap entry functions... */

unsigned
mrm_rcdb_get_remap_count( void ) {
  return _remap_count;
}

struct mrm_runconf_remap_entry *
mrm_rcdb_lookup_remap_entry_by_macaddr(const unsigned char * const macaddr) {
  unsigned i;
  struct mrm_runconf_remap_entry *r;

  for (i = 0; i < _remap_count; i++) {
    r = &_remaps[i];
    if (ether_addr_equal(r->match_macaddr, macaddr))
      return r; /* success */
  }

  return NULL; /* lookup failed */
}

struct mrm_runconf_remap_entry *mrm_rcdb_lookup_remap_entry_by_index(const unsigned index) {
  if (index >= _remap_count) return NULL;
  return &_remaps[index];
}

struct mrm_runconf_remap_entry *
mrm_rcdb_insert_remap_entry(const unsigned char * const macaddr) {
  struct mrm_runconf_remap_entry *rv;

  /* does a remap entry by this name already exist? if so, return the existing one... */
  rv = mrm_rcdb_lookup_remap_entry_by_macaddr(macaddr);
  if (rv != NULL) return rv;

  if (_remap_count == MRM_MAX_REMAPS) {
    return NULL; /* were full... cant insert any more remaps */
  }

  rv = &_remaps[_remap_count];
  memset(rv, 0, sizeof(*rv));
  memcpy(rv->match_macaddr, macaddr, sizeof(rv->match_macaddr));
  ++_remap_count;

  return rv;
}

int
mrm_rcdb_delete_remap_entry(struct mrm_runconf_remap_entry * const remap_entry) {
  unsigned idx;
  unsigned after_entries;
  struct mrm_runconf_filter_node *filter;

  /* first find the index of the given remap entry pointer in the list */
  if (remap_entry < _remaps) return -EINVAL;
  idx = remap_entry - _remaps;
  if (idx >= _remap_count) return -EINVAL;

  /* preserve the filter pointer... */
  filter = remap_entry->filter;

  /* XXX possibly the worst way to update a list... */
  after_entries = _remap_count - idx - 1;
  memmove(remap_entry, remap_entry + 1, after_entries * sizeof(*remap_entry));

  /* finally decrement the amount of entries in the array */
  --_remap_count;

  /* decrement the filter reference count if applicable */
  if (filter) filter->refcnt--;

  return 0; /* success */
}

void
mrm_rcdb_set_remap_entry_filter(struct mrm_runconf_remap_entry * const remap_entry, struct mrm_runconf_filter_node * const filter) {
  struct mrm_runconf_filter_node * old_filter;
  
  
  if (remap_entry->filter == filter) return; /* nothing to do... */

  if (filter != NULL) filter->refcnt++;

  old_filter = remap_entry->filter;
  remap_entry->filter = filter;

  if (old_filter != NULL) old_filter->refcnt--;
}

