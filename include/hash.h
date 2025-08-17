#ifndef HASH_H
#define HASH_H
#include <stdio.h>

typedef struct hash_entry {
    void* data;
    struct hash_entry* next;
} HashEntry;

typedef struct {
    HashEntry** entry;
    size_t size;
} HashTable;

/*
* Function: init_hash_table
* 
* -------------------------
*
*  Initiates a hash table struct.
*
*  hash_table: Pointer to the hash table.
*  table_size: Size of the hash_table.
*
*  returns: If failed (-1), on success (1).
*/
int init_hash_table(HashTable* hash_table, size_t table_size);

/*
 * Function: hash
 *
 * --------------
 *
 *  Generates a hash value of a hashable data.
 *
 *  hashable_data: String value to hash.
 *  hashable_data_size: Size of hashable data.
 *  max_table_size: Hash table size.
 *
 *  returns: Hash value.
 */
int hash(const char* hashable_data, size_t hashable_data_size, size_t max_table_size);

/*
 * Function create_entry
 * 
 * ---------------------
 *
 *  Creates a new entry.
 *
 *  data: Pointer to the data.
 *
 *  returns: Pointer to the new entry.
 */
HashEntry* create_entry(void* data);

/*
 * Function: add_hash_entry
 *
 * ------------------------
 *
 *  Creates a new entry and adds the entry to the hash table.
 *
 *  hash_entry: Pointer to the entry pointer.
 *  data: Pointer to the data.
 *
 *  returns: Pointer to the entry.
 */
HashEntry* add_hash_entry(HashEntry** hash_entry, void* data);

/*
 * Function: free_hash_table
 *
 * -------------------------
 *
 *  Frees the hash table.
 *
 *  hash_table: Pointer to the hash table.
 */
void free_hash_table(HashTable* hash_table);
#endif
