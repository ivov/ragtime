#include "hashtable.h"

#include <stdlib.h>
#include <string.h>

static unsigned int hash_string(const char *str) {
  unsigned int hash = 5381; // from djb2 algorithm
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }

  return hash % HASHTABLE_SIZE;
}

void hashtable_init(HashTable *hashtable) {
  for (int i = 0; i < HASHTABLE_SIZE; i++) {
    hashtable->slots[i] = NULL;
  }
}

void *hashtable_get(HashTable *hashtable, const char *key) {
  unsigned int slot = hash_string(key);
  HashEntry *entry = hashtable->slots[slot];

  while (entry) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }
    entry = entry->next;
  }

  return NULL;
}

void hashtable_put(HashTable *hashtable, const char *key, void *value) {
  unsigned int slot = hash_string(key);

  HashEntry *entry = malloc(sizeof(HashEntry));
  if (!entry) {
    fprintf(stderr, "Failed to allocate memory for hash entry\n");
    return;
  }

  entry->key = strdup(key);
  if (!entry->key) {
    free(entry);
    return;
  }
  entry->value = value;
  entry->next = hashtable->slots[slot];
  hashtable->slots[slot] = entry;
}
