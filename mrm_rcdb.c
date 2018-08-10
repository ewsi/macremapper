

#include "./mrm_rcdb.h"

#include <linux/etherdevice.h> /* ether_addr_equal() */
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/jhash.h>
#include <linux/random.h>



/* filter storage... */
static struct kmem_cache                *_filter_cache __read_mostly;
static struct list_head                  _filter_list;
#define filter_for_each(pos) list_for_each_entry(pos, &_filter_list, list)


/* remap storage... */
#define MRM_MAX_REMAPS 100
#define REMAP_HASH_BITS 8
#define REMAP_HASH_COUNT (1 << REMAP_HASH_BITS)
static u32                               _remap_hash_salt;
static struct hlist_head                 _remap_hash[REMAP_HASH_COUNT];
static struct kmem_cache                *_remap_cache  __read_mostly;
#define remap_for_each(pos, headidx) hlist_for_each_entry(pos, &_remap_hash[headidx], hlist)

int
mrm_rcdb_init( void ) {
  _filter_cache = NULL;
  _remap_cache  = NULL;

  _filter_cache = kmem_cache_create("mrm_filter_cache", sizeof(struct mrm_runconf_filter_node), 0, SLAB_HWCACHE_ALIGN, NULL);
  if (_filter_cache == NULL) goto failed;

  _remap_cache = kmem_cache_create("mrm_rcdb_cache", sizeof(struct mrm_runconf_remap_entry), 0, SLAB_HWCACHE_ALIGN, NULL);
  if (_remap_cache == NULL) goto failed;

  INIT_LIST_HEAD(&_filter_list);

  memset(_remap_hash, 0, sizeof(_remap_hash)); /* XXX is there a better way to initialize? */
  get_random_bytes(&_remap_hash_salt, sizeof(_remap_hash_salt));

  return 0; /* success */

failed:
  if (_filter_cache != NULL) kmem_cache_destroy(_filter_cache);
  if (_remap_cache != NULL) kmem_cache_destroy(_remap_cache);


  _filter_cache = NULL;
  _remap_cache  = NULL;
  
  return -ENOMEM;
}

void
mrm_rcdb_destroy( void ) {
  mrm_rcdb_clear(); /* TDB if needed */

  kmem_cache_destroy(_remap_cache);
  kmem_cache_destroy(_filter_cache);
}

void
mrm_rcdb_clear( void ) {
  while (mrm_rcdb_get_remap_count() > 0) {
    mrm_rcdb_delete_remap_entry(mrm_rcdb_lookup_remap_entry_by_index(0));
  }

  while (mrm_rcdb_get_filter_count() > 0) {
    mrm_rcdb_delete_filter(mrm_rcdb_lookup_filter_by_index(0));
  }
}


/* filter functions... */

unsigned
mrm_rcdb_get_filter_count( void ) {
  struct mrm_runconf_filter_node   *f;
  unsigned                          result;

  result = 0;
  filter_for_each(f) {
    ++result;
  }
  return result;
}

struct mrm_runconf_filter_node *
mrm_rcdb_lookup_filter_by_name(const char * const name) {
  struct mrm_runconf_filter_node   *f;

  filter_for_each(f) {
    if (strncmp(f->conf.name, name, sizeof(f->conf.name)) == 0)
      return f;
  }

  return NULL; /* not found */
}

struct mrm_runconf_filter_node *mrm_rcdb_lookup_filter_by_index(unsigned index) {
  struct mrm_runconf_filter_node   *f;

  filter_for_each(f) {
    if (index-- == 0) break;
  }

  return f;
}

struct mrm_runconf_filter_node *
mrm_rcdb_insert_filter( const char * const name) {
  struct mrm_runconf_filter_node   *rv;

  /* first make sure a filter by the same name dont already exist */
  rv = mrm_rcdb_lookup_filter_by_name(name);
  if (rv != NULL) return rv; /* filter by said name already exists */

  /* allocate a new filter */
  rv = kmem_cache_alloc(_filter_cache, GFP_ATOMIC);
  if (rv == NULL) return NULL; /* out of memory */

  /* initialize it */
  memset(rv, 0, sizeof(*rv));
  INIT_LIST_HEAD(&rv->list);
  strncpy(rv->conf.name, name, sizeof(rv->conf.name));

  /* add it to the list */
  list_add(&rv->list, &_filter_list);

  return rv;
}

int
mrm_rcdb_delete_filter( struct mrm_runconf_filter_node * const filter ) {

  if (filter->refcnt > 0) return -EADDRINUSE;

  list_del(&filter->list);
  kmem_cache_free(_filter_cache, filter);

  return 0; /* success */
}





/* remap entry functions... */

static inline int
mrm_rcsb_hash_macaddr(const unsigned char * const macaddr) {
  /* hashing algorithm inspired by br_mac_hash() in br_fdb.c
     key is last four bytes of macaddr
  */
  const u32 key = get_unaligned((const u32*)&macaddr[2]);
  return jhash_1word(key, _remap_hash_salt) & (REMAP_HASH_COUNT - 1);
}

unsigned
mrm_rcdb_get_remap_count( void ) {
  struct mrm_runconf_remap_entry *r;
  unsigned headidx;
  unsigned result;

  /* theres gotta be a better way of doing this */

  result = 0;
  for (headidx = 0; headidx < REMAP_HASH_COUNT; headidx++) {
    remap_for_each(r, headidx) {
      ++result;
    }
  }
  return result;
}

struct mrm_runconf_remap_entry *
mrm_rcdb_lookup_remap_entry_by_macaddr(const unsigned char * const macaddr) {
  struct mrm_runconf_remap_entry *r;
  const int headidx = mrm_rcsb_hash_macaddr(macaddr);

  remap_for_each(r, headidx) {
    if (ether_addr_equal(r->match_macaddr, macaddr))
      return r; /* success */
  }

  return NULL; /* lookup failed */
}

struct mrm_runconf_remap_entry *mrm_rcdb_lookup_remap_entry_by_index(unsigned index) {
  struct mrm_runconf_remap_entry *r;
  unsigned headidx;

  /* theres gotta be a better way of doing this */

  for (headidx = 0; headidx < REMAP_HASH_COUNT; headidx++) {
    remap_for_each(r, headidx) {
      if (index-- == 0)
        return r;
    }
  }
  return NULL; /*lookup failed */
}

struct mrm_runconf_remap_entry *
mrm_rcdb_insert_remap_entry(const unsigned char * const macaddr) {
  struct mrm_runconf_remap_entry *rv;

  /* does a remap entry by this name already exist? if so, return the existing one... */
  rv = mrm_rcdb_lookup_remap_entry_by_macaddr(macaddr);
  if (rv != NULL) return rv;

  if (mrm_rcdb_get_remap_count() == MRM_MAX_REMAPS) {
    return NULL; /* were full... cant insert any more remaps */
  }

  /* allocate a new remap entry... */
  rv = kmem_cache_alloc(_remap_cache, GFP_ATOMIC);
  if (rv == NULL) {
    return NULL; /* out of memory... */
  }

  /* initialize it... */
  memset(rv, 0, sizeof(*rv));
  memcpy(rv->match_macaddr, macaddr, sizeof(rv->match_macaddr));

  /* add it to the hash */
  hlist_add_head(&rv->hlist, &_remap_hash[mrm_rcsb_hash_macaddr(rv->match_macaddr)]);

  return rv;
}

int
mrm_rcdb_delete_remap_entry(struct mrm_runconf_remap_entry * const remap_entry) {
  struct mrm_runconf_filter_node *filter;

  /* preserve the filter pointer... */
  filter = remap_entry->filter;

  /* remove it from the hash list and free it... */
  hlist_del(&remap_entry->hlist);
  kmem_cache_free(_remap_cache, remap_entry);

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

