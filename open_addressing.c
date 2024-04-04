
#include "open_addressing.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN_SIZE 8

const int hash_func_table_size = 256;

void tabulation_sample(void *index) {
  uint32_t *start = index;
  uint32_t *end = start + hash_func_table_size / sizeof(uint32_t);
  while (start != end)
    *(start++) = rand();
}

// tabulation hashing, r=4, q=32
uint32_t hash(uint32_t x, uint8_t *T) {
  const int r = 4;
  const uint32_t no_cols = 1 << r;
  const uint32_t mask = (1 << r) - 1;

  // clang-format off
  uint32_t *T_ = (uint32_t *)T;
  uint32_t y = T_[0 * no_cols + (x & mask)];  x >>= r;
  y ^= T_[1 * no_cols + (x & mask)];          x >>= r;
  y ^= T_[2 * no_cols + (x & mask)];          x >>= r;
  y ^= T_[3 * no_cols + (x & mask)];          x >>= r;
  y ^= T_[4 * no_cols + (x & mask)];          x >>= r;
  y ^= T_[5 * no_cols + (x & mask)];          x >>= r;
  y ^= T_[6 * no_cols + (x & mask)];          x >>= r;
  y ^= T_[7 * no_cols + (x & mask)];
  // clang-format on

  return y;
}

unsigned int static p(unsigned int k, unsigned int i, unsigned int m) {
  return (k + i) & (m - 1);
}

static void init_table(struct hash_table *table, unsigned int size,
                       struct bin *begin, struct bin *end) {
  // Initialize table members
  struct bin *bins = malloc(size * sizeof *bins);
  *table = (struct hash_table){.bins = bins,
                               .size = size,
                               .used = 0,
                               .active = 0.,
                               .ops_since_rehash = 0};

  // Initialize bins
  struct bin empty_bin = {.in_probe = false, .is_empty = true};
  for (unsigned int i = 0; i < table->size; i++) {
    table->bins[i] = empty_bin;
  }

  // Copy the old bins to the new table
  for (struct bin *bin = begin; bin != end; bin++) {
    if (!bin->is_empty) {
      insert_key(table, bin->key);
    }
  }

  table->hash_func_index = malloc(hash_func_table_size);
  tabulation_sample(table->hash_func_index);
}

static void resize(struct hash_table *table, unsigned int new_size) {
  // remember the old bins until we have moved them.
  struct bin *old_bins_begin = table->bins,
             *old_bins_end = old_bins_begin + table->size;

  // Resample hash function if we are resizing...
  tabulation_sample(table->hash_func_index);
  table->ops_since_rehash = 0;

  // Update table and copy the old active bins to it.
  init_table(table, new_size, old_bins_begin, old_bins_end);

  // finally, free memory for old bins
  free(old_bins_begin);
}

static void rehash(struct hash_table *table) {
  // Resizing and rehashing is the same code, except we don't change the size.
  resize(table, table->size);
}

struct hash_table *new_table() {
  struct hash_table *table = malloc(sizeof *table);
  init_table(table, MIN_SIZE, NULL, NULL);
  return table;
}

void delete_table(struct hash_table *table) {
  free(table->bins);
  free(table->hash_func_index);
  free(table);
}

// Find the bin containing key, or the first bin past the end of its probe
struct bin *find_key(struct hash_table *table, unsigned int key) {
  for (unsigned int i = 0; i < table->size; i++) {
    struct bin *bin = table->bins + p(key, i, table->size);
    if (bin->key == key || !bin->in_probe)
      return bin;
  }
  // The table is full. This should not happen!
  assert(false);
}

// Find the first empty bin in its probe.
struct bin *find_empty(struct hash_table *table, unsigned int key) {
  for (unsigned int i = 0; i < table->size; i++) {
    struct bin *bin = table->bins + p(key, i, table->size);
    if (bin->is_empty)
      return bin;
  }
  // The table is full. This should not happen!
  assert(false);
}

void insert_key(struct hash_table *table, unsigned int key) {
  if (!contains_key(table, key)) {
    struct bin *key_bin = find_empty(table, key);

    table->active++;
    if (!key_bin->in_probe)
      table->used++; // We are using a new bin

    *key_bin = (struct bin){.in_probe = true, .is_empty = false, .key = key};

    if (table->used > table->size / 2)
      resize(table, table->size * 2);
    if (table->ops_since_rehash++ > table->size)
      rehash(table);
  }
}

bool contains_key(struct hash_table *table, unsigned int key) {
  if (table->ops_since_rehash++ > table->size)
    rehash(table);

  struct bin *bin = find_key(table, key);
  return bin->key == key && !bin->is_empty;
}

void delete_key(struct hash_table *table, unsigned int key) {
  struct bin *bin = find_key(table, key);
  if (bin->key != key)
    return; // Nothing more to do

  bin->is_empty = true; // Delete the bin
  table->active--;      // Same bins in use but one less active

  if (table->active < table->size / 8 && table->size > MIN_SIZE)
    resize(table, table->size / 2);
  if (table->ops_since_rehash++ > table->size)
    rehash(table);
}

void print_table(struct hash_table *table) {
  for (unsigned int i = 0; i < table->size; i++) {
    if (i > 0 && i % 8 == 0) {
      printf("\n");
    }
    struct bin *bin = table->bins + i;
    if (bin->in_probe && !bin->is_empty) {
      printf("[%u]", bin->key);
    } else if (bin->in_probe && bin->is_empty) {
      printf("[*]");
    } else {
      printf("[ ]");
    }
  }
  printf("\n----------------------\n");
}
