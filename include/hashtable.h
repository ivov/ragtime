#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <JavaScriptCore/JavaScript.h>

#define HASHTABLE_SIZE 64

typedef struct HashEntry {
  char *key;
  void *value;
  struct HashEntry *next;
} HashEntry;

typedef struct {
  HashEntry *slots[HASHTABLE_SIZE];
} HashTable;

void hashtable_init(HashTable *ht);
void *hashtable_get(HashTable *ht, const char *key);
void hashtable_put(HashTable *ht, const char *key, void *value);

#endif
