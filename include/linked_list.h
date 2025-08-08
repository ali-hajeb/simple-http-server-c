#ifndef DICT_H
#define DICT_H
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

ListItem* create_item(const char* key, const void* value, size_t value_size);
int list_set_item(List* list, const char* key, const void* value, size_t value_size);
ListItem* list_get_item(List* list, const char* key);
size_t free_list(List* list);
#endif
