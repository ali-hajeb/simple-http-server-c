#ifndef LINKED_LIST_H 
#define LINKED_LIST_H 
#include <stdio.h>

typedef struct list_node {
    size_t value_size;
    char* key;
    void* value;
    struct list_node* next;
} ListItem;

typedef struct {
    size_t size;
    ListItem* items;
} List;

/*
 * Function: create_item
 *
 * ---------------------
 *
 *  Creates a list item.
 *
 *  returns: Pointer to the item.
 */
ListItem* create_item(const char* key, const void* value, size_t value_size);

/*
 * Function: list_set_item
 *
 * -----------------------
 *
 *  Adds an item to the list. If item key exists, updates the value.
 *
 *  list: pointer to the linked list.
 *  key: item key.
 *  value: pointer to the item value.
 *  value_size: item value size.
 *
 *  returns: If failed (-1), on update (0), on add (1).
 */
int list_set_item(List* list, const char* key, const void* value, size_t value_size);

/*
 * Function: list_get_item
 *
 * -----------------------
 *
 *  Returns a pointer to the desired item.
 *
 *  list: pointer to linked list struct.
 *  key: item key.
 *
 *  returns: pointer to the desired item. returns NULL, if item not found.
 */
ListItem* list_get_item(List* list, const char* key);

/*
 * Function: free_list
 *
 * -------------------
 *
 *  Frees linked list.
 *
 *  list: pointer to the linked list.
 *
 *  returns: Number of freed items.
 */
size_t free_list(List* list);
#endif
