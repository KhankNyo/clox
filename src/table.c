

#include <stdio.h>
#include <string.h>

#include "include/common.h"
#include "include/table.h"
#include "include/value.h"
#include "include/object.h"
#include "include/memory.h"
#include "include/vm.h"


#define TABLE_MAX_LOAD 3/4

static void adjust_capacity(Table_t* table, size_t newcap);
static Entry_t* find_entry(Entry_t* entries, size_t num_entries, const ObjString_t* key);
static inline bool is_tombstone(const Entry_t* entry);
static inline bool is_empty(const Entry_t* entry);



void Table_Init(Table_t* table, VM_t* vm)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
    table->vm = vm;
}


void Table_Free(Table_t* table)
{
    FREE_ARRAY(table->vm, Entry_t, table->entries, table->capacity);
    Table_Init(table, table->vm);
}




bool Table_Delete(Table_t* table, const ObjString_t* key)
{
    Entry_t* entry = find_entry(table->entries, table->capacity, key);
    if (NULL == entry->key)
    {
        return false;
    }

    /* mark as tombstone */
    entry->key = NULL;
    entry->val = BOOL_VAL(true);
    return true;
}


bool Table_Get(Table_t* table, const ObjString_t* key, Value_t* val)
{
    if (table->count == 0) 
    {
        return false;
    }

    Entry_t* entry = find_entry(table->entries, table->capacity, key);
    if (is_empty(entry) || is_tombstone(entry))
    {
        return false;
    }

    *val = entry->val;
    return true;
}



bool Table_Set(Table_t* table, ObjString_t* key, Value_t val)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        VM_Push(table->vm, OBJ_VAL(key));

        size_t capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);

        VM_Pop(table->vm);
    }


    Entry_t *entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key == NULL;
    if (is_new_key && is_empty(entry))
    {
        table->count++;
    }

    entry->key = key;
    entry->val = val;
    return is_new_key;
}



void Table_AddAll(const Table_t* src, Table_t* dst)
{
    for (size_t i = 0; i < src->capacity; i++)
    {
        const Entry_t* entry = &src->entries[i];
        if (NULL != entry->key)
        {
            Table_Set(dst, entry->key, entry->val);
        }
    }
}



ObjString_t* Table_FindStr(Table_t* table, const char* cstr, int len, uint32_t hash)
{
    if (table->count == 0)
    {
        return NULL;
    }

    uint32_t index = hash % table->capacity;
    while (true)
    {
        Entry_t* entry = &table->entries[index];
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->val)) /* stops if encountered an empty slot */
                return NULL;
        }
        else if ((entry->key->len == len)
                && (entry->key->hash == hash)
                && (memcmp(entry->key->cstr, cstr, len) == 0))
        {
            /* key found */
            return entry->key;
        }

        index += 1;
        if (index >= table->capacity)
            index = 0;
    }
}



void Table_Mark(Table_t* table)
{
    for (size_t i = 0; i < table->capacity; i++)
    {
        Entry_t* entry = &table->entries[i];
        GC_MarkObj(table->vm, (Obj_t*)entry->key);
        GC_MarkVal(table->vm, entry->val);
    }
}

void Table_RemoveWhite(Table_t* table)
{
    for (size_t i = 0; i < table->capacity; i++)
    {
        Entry_t* entry = &table->entries[i];
        if (NULL != entry->key && !entry->key->obj.is_marked)
        {
            Table_Delete(table, entry->key);
        }
    }
}







ObjString_t* Table_FindStrs(Table_t* table, 
        int substr_count, const ObjString_t* substr[static substr_count], 
        uint32_t hash, int total_len)
{
    if (table->count == 0)
    {
        return NULL;
    }

    uint32_t index = hash % table->capacity;
    while (true)
    {
        Entry_t* entry = &table->entries[index];
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->val)) /* stops if encountered an empty slot */
                return NULL;
        }
        else if (entry->key->len == total_len && entry->key->hash == hash) 
        {
            const char* key_substr = entry->key->cstr;
            for (int i = 0; i < substr_count; i++)
            {
                if (memcmp(key_substr, substr[i]->cstr, substr[i]->len) != 0)
                {   /* substr not equal */
                    goto find_next;
                }
                key_substr += substr[i]->len;

            }
            return entry->key;
        }
find_next:


        index += 1;
        if (index >= table->capacity)
            index = 0;
    }

}













static void adjust_capacity(Table_t* table, size_t newcap)
{
    Entry_t* new_entries = ALLOCATE(table->vm, Entry_t, newcap);
    for (size_t i = 0; i < newcap; i++)
    {
        new_entries[i].key = NULL;
        new_entries[i].val = NIL_VAL();
    }

    /* rebuild hash table with new capacity, skip tombstones and empty buckets */
    table->count = 0;
    for (size_t i = 0; i < table->capacity; i++)
    {
        Entry_t* entry = &table->entries[i];
        if (NULL == entry->key)
        {
            continue;
        }

        Entry_t* dest = find_entry(new_entries, newcap, entry->key);
        dest->key = entry->key;
        dest->val = entry->val;
        table->count++;
    }


    FREE_ARRAY(table->vm, Entry_t, table->entries, table->capacity);

    table->entries = new_entries;
    table->capacity = newcap;
}


static Entry_t* find_entry(Entry_t* entries, size_t num_entries, const ObjString_t* key)
{
    uint32_t index = key->hash % num_entries;
    Entry_t* tombstone = NULL;

    /* this is not an infinite loop thanks to the load factor */
    while (true)
    {
        Entry_t* entry = &entries[index];
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->val)) /* deleted */
                return tombstone != NULL ? tombstone : entry;
            else if (tombstone == NULL) /* tombstone, and not encountered tombstone b4 */
                tombstone = entry;
        }
        else if (entry->key == key) 
            /* addr cmp is ok because strings are interned, 
             * so there are no duplicate strs at difference addrs */
        {
            return entry;
        }

        index += 1;
        if (index >= num_entries)
            index = 0;
    }
}



static inline bool is_tombstone(const Entry_t* entry)
{
    return entry->key == NULL && IS_BOOL(entry->val) && AS_BOOL(entry->val);
}


static inline bool is_empty(const Entry_t* entry)
{
    return entry->key == NULL && IS_NIL(entry->val);
}
