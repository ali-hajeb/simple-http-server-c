#include "../include/hash.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int init_hash_table(HashTable* hash_table, size_t table_size) {
    if (hash_table == NULL) {
        return -1;
    }

    hash_table->entry = malloc(table_size * sizeof(HashEntry*));
    if (hash_table->entry == NULL) {
        err("init_hash_table", "Unable to allocate memory for hash table!");
        return -1;
    }

    for (size_t i = 0; i < table_size; i++) {
        hash_table->entry[i] = NULL;
    }

    return 1;
}

int hash(const char* hashable_data, size_t hashable_data_size, size_t max_table_size) {
    // FNV-1a offset basis
    unsigned int hash_value = 2166136261u;
    for (size_t i = 0; i < hashable_data_size; i++) {
        hash_value = (hash_value * 16777619u) ^ hashable_data[i];
    }
    return hash_value % max_table_size;
}

HashEntry* create_entry(void* data) {
    if (data == NULL) {
        return NULL;
    }

    HashEntry* new_entry = malloc(sizeof(HashEntry));
    if (new_entry == NULL) {
        err("create_entry", "Unable to allocate memory for new entry!");
        return NULL;
    }

    new_entry->data = data;
    new_entry->next = NULL;

    return new_entry;
}

HashEntry* add_hash_entry(HashEntry** hash_entry, void* data) {
    if (data == NULL) {
        return NULL;
    }

    HashEntry* new_entry = create_entry(data);
    new_entry->next = *hash_entry;
    *hash_entry = new_entry;

    return new_entry;
}

void free_hash_table(HashTable* hash_table, size_t hash_table_size) {
    HashEntry* next;
    for (size_t i = 0; i < hash_table_size; i++) {
        for (HashEntry* j = hash_table->entry[i]; j != NULL; j = next) {
            next = j->next;
            free(j->data);
            free(j);
        }
    }
    free(hash_table->entry);
}
