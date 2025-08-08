#ifndef HASH_H
#define HASH_H
#include <stddef.h>
#include <stdio.h>

typedef struct hash_entry {
    void* data;
    struct hash_entry* next;
} HashEntry;

typedef struct {
    HashEntry** entry;
} HashTable;

int init_hash_table(HashTable* hash_table, size_t table_size);
int hash(const char* hashable_data, size_t hashable_data_size, size_t max_table_size);
HashEntry* create_entry(void* data);
HashEntry* add_hash_entry(HashEntry** hash_entry, void* data);
#endif
