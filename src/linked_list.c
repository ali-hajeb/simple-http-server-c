#include "../include/linked_list.h"
#include "../include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_list(List* list) {
    if (list == NULL) {
        return;
    }
    list->size = 0;
    list->items = NULL;
}

ListItem* create_item(const char* key, const void* value, size_t value_size) {
    if (key == NULL || value == NULL || value_size == 0) {
        err("dict_create", "Null key or value!");
        return NULL;
    }

    ListItem* item = malloc(sizeof(ListItem));
    if (item == NULL) {
        err("dict_create", "Unable to allocate memory for new dictionary entry!");
        return NULL;
    }

    item->key = strdup(key);
    if (item->key == NULL) {
        err("dict_create", "Unable to allocate memory for key!");
        free(item);
        return NULL;
    }

    item->value = malloc(value_size);
    if (item->value == NULL) {
        err("dict_create", "Unable to allocate memory for key value!");
        free(item->key);
        free(item);
        return NULL;
    }
    memcpy(item->value, value, value_size);
    item->next = NULL;
    item->value_size = value_size;

    return item;
}

int list_set_item(List* list, const char* key, const void* value, size_t value_size) {
    if (list == NULL || key == NULL || value == NULL) {
        err("list_set_item", "Null params!");
        printf("%p %p %pn", list, key, value);
        return -1;
    }

    // Check if key already exists
    ListItem* existing_key = list_get_item(list, key);
    if (existing_key != NULL) {
        if (existing_key->value_size == value_size) {
            memcpy(existing_key->value, value, value_size);
        } else {
            existing_key->value = realloc(existing_key->value, value_size);
            if (existing_key->value == NULL) {
                err("dict_create", "Unable to re-allocate memory for key value!");
                return -1;
            }
            memcpy(existing_key->value, value, value_size);
            existing_key->value_size = value_size;
        }
        return 0;
    }

    // If key does not exists
    ListItem* new_item = create_item(key, value, value_size);
    if (new_item == NULL) {
        return -1;
    }
    new_item->next = list->items;
    list->items = new_item;
    list->size++;
    return 1;
}

ListItem* list_get_item(List* list, const char* key) {
    if (list == NULL || key == NULL) {
        return NULL;
    }
    for (ListItem* i = list->items; i != NULL; i = i->next) {
        if (strcmp(key, i->key) == 0) {
            return i;
        }
    }
    return NULL;
}

size_t free_list(List* list) {
    size_t items_freed = 0;
    ListItem* next;
    for (ListItem* i = list->items; i != NULL; i = next) {
        next = i->next;
        free(i->key);
        free(i->value);
        free(i);
        items_freed++;
    }

    list->items = NULL;
    list->size = 0;
    return items_freed;
}
