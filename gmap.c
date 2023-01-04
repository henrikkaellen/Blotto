#include "gmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char *gmap_error = "error";

//(key, value) pair stored in struct
typedef struct _entry
{
    void *key;
    void *value;
    struct _entry *next;
} entry;


/**
 * Returns the index where key is located in the given map, or the index
 * where it would go if it is not present.
 * 
 * @param table a table with at least one free slot, non-NULL
 * @param size the size of the table
 * @param capacity the capacity of the table
 * @param key a string, non-NULL
 * @param hash the hash function used for the keys, non-NULL
 * @param compare a comparison function for keys, non-NULL
 * @param copy a function for copying keys, non-NULL
 * @param free a function for freeing keys, non-NULL
 */
struct _gmap
{
    entry **table;
    size_t capacity;
    size_t size;
    size_t (*hash)(const void *);
    int (*compare)(const void *, const void *);
    void *(*copy)(const void *);
    void (*free)(void *);
};

size_t gmap_compute_index(const void *key, size_t (*hash)(const void*), size_t capacity);
entry *gmap_table_find_key(entry **table, const void *key, size_t (*hash)(const void *), int (*compare)(const void *, const void *), size_t capacity);
void gmap_embiggen(gmap *m, size_t n);
void gmap_table_add(entry **table, entry *n, size_t (*hash)(const void* ), size_t capacity);
void gmap_store_key_in_array(const void *key, void *value, void *arg);

//initial capcity of table
#define GMAP_INITIAL_CAPACITY 100


gmap *gmap_create(void *(*cp)(const void *), int (*comp)(const void *, const void *), size_t (*h)(const void *s), void (*f)(void *))
{
    if (cp == NULL || comp == NULL || h == NULL || f == NULL)
    {
        return NULL;
    }

    gmap *result = malloc(sizeof(gmap));

    if (result != NULL)
    {
        //add the functions to the struct
        result->copy = cp;
        result->compare = comp;
        result->hash = h;
        result->free = f;

        //initialize the table
        result->table = malloc(GMAP_INITIAL_CAPACITY * sizeof(entry*));
        result->capacity = (result->table != NULL ? GMAP_INITIAL_CAPACITY : 0);
        result->size = 0;

        //each element in table should be NULL when initialized
        for (size_t i = 0; i < result->capacity; i++)
        {
            result->table[i] = NULL;
        }
    }

    return result;
}

size_t gmap_size(const gmap *m)
{
    if (m == NULL)
    {
        return 0;
    }

    return m->size;
}

//function to find index of specific key
size_t gmap_compute_index(const void *key, size_t (*hash)(const void*), size_t capacity)
{
    return hash(key) % capacity;
}


//function for sequential search
entry *gmap_table_find_key(entry **table, const void *key, size_t (*hash)(const void *), int (*compare)(const void *, const void *), size_t capacity)
{
    //determine which chain to search from
    size_t ind = gmap_compute_index(key, hash, capacity);

    //sequential search
    entry *curr = table[ind];
    while (curr != NULL && compare(curr->key, key) != 0)
    {
        curr = curr->next;
    }
    return curr;
}


//function for increasing the chains and rehashing keys
void gmap_embiggen(gmap *m, size_t n)
{
    entry **new_table = malloc(n * sizeof(entry*));

    //each element in table should be NULL when initialized
    for (size_t i = 0; i < n; i++)
    {
        new_table[i] = NULL;
    }

    for (size_t chain = 0; chain < m->capacity; chain++)
    {
        entry *curr = m->table[chain];
        while (curr != NULL)
        {
            entry *temp = curr->next;
            gmap_table_add(new_table, curr, m->hash, n);
            curr = temp;
        }
    }

    m->capacity = n;
    free(m->table);

    m->table = new_table;
}

void gmap_table_add(entry **table, entry *n, size_t (*hash)(const void* ), size_t capacity)
{
    //get index of chain
    size_t ind = gmap_compute_index(n->key, hash, capacity);

    //add to the front of the chain
    n->next = table[ind];
    table[ind] = n;
}

void *gmap_put(gmap *m, const void *key, void *value)
{
    if (m == NULL || key == NULL)
    {
        return false;
    }

    entry *n = gmap_table_find_key(m->table, key, m->hash, m->compare, m->capacity);
    if (n != NULL)
    {
        //key already present
        void *old_value = n->value;
        n->value = value;
        return old_value;
    }
    else
    {
        //make a copy og key
        void *copy = m->copy(key);

        if (copy != NULL)
        {
            //check if load factor is too high
            if (m->size >= m->capacity)
            {
                //add chains and rehash
                gmap_embiggen(m, m->capacity*2);
            }

            //add to table
            entry *n = malloc(sizeof(entry));
            if (n != NULL)
            {
                n->key = copy;
                n->value = value;
                gmap_table_add(m->table, n, m->hash, m->capacity);
                m->size++;
                return NULL;
            }
            else
            {
                m->free(copy);
                return gmap_error;
            }
        }
        else
        {
            return gmap_error;
        }
    }
}

void *gmap_remove(gmap *m, const void *key)
{
    if (m == NULL || key == NULL)
    {
        return NULL;
    }

    void *val;
    //determine which chain to search from
    size_t ind = gmap_compute_index(key, m->hash, m->capacity);

    //sequential search
    entry *curr = m->table[ind];
    entry *prev = NULL;
    while (curr != NULL && m->compare(curr->key, key) != 0)
    {
        prev = curr;
        curr = curr->next;
    }
    if (curr == NULL)
    {
        return NULL;
    }

    //in case of prev is NULL, curr is first element
    else if (prev == NULL)
    {
        entry *temp = curr;
        m->table[ind] = curr->next;
        val = temp->value;
        m->free(temp->key);
        free(temp);

        m->size--;
        return val;
    }

    else
    {
        prev->next = curr->next;
        val = curr->value;

        m->free(curr->key); 
        free(curr);

        m->size--;
        
        return val;   
    }

    return NULL;
}

bool gmap_contains_key(const gmap *m, const void *key)
{
    if (gmap_table_find_key(m->table, key, m->hash, m->compare, m->capacity) == NULL)
    {
        return false;
    }

    return true;
}

void *gmap_get(gmap *m, const void *key)
{
    if (m == NULL || key == NULL)
    {
        return NULL;
    }

    entry *n = gmap_table_find_key(m->table, key, m->hash, m->compare, m->capacity);
    if (n != NULL)
    {
        //return the value in that node
        return n->value;
    }
    else
    {
        //key not found
        return NULL;
    }
}

void gmap_for_each(gmap *m, void (*f)(const void *, void *, void *), void *arg)
{
    if (m == NULL || f == NULL)
    {
        return;
    }

    //repeat function for each chain in gmap
    for (size_t chain = 0; chain < m->capacity; chain++)
    {
        entry *curr = m->table[chain];
        while (curr != NULL)
        {
            f(curr->key, curr->value, arg);
            curr = curr->next;
        }
    }
}

/**
 * A location in an array where a key can be stored. The location is
 * represented by a (array, index) pair.
 */
typedef struct _gmap_store_location
{
    const void **arr;
    size_t index;
} gmap_store_location;

void gmap_store_key_in_array(const void *key, void *value, void *arg)
{
    gmap_store_location *where = arg;
    where->arr[where->index] = key;
    where->index++;
}

const void **gmap_keys(gmap *m)
{
    const void **keys = malloc(sizeof(*keys) * m->size);

    if (keys != NULL)
    {
        gmap_store_location loc = {keys, 0};
        gmap_for_each(m, gmap_store_key_in_array, &loc);
    }

    return keys;
}

void gmap_destroy(gmap *m)
{
    if (m == NULL)
    {
        return;
    }

    for (size_t i = 0; i < m->capacity; i++)
    {
        entry *curr = m->table[i];
        while (curr != NULL)
        {
            //free key
            m->free(curr->key);

            //remember where to go next
            entry *next = curr->next;

            //free entry
            free(curr);

            //go to next entry
            curr = next;
        }
    }  
    free(m->table);
    free(m);