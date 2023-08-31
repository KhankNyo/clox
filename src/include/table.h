#ifndef _CLOX_TABLE_H_
#define _CLOX_TABLE_H_



#include "common.h"
#include "memory.h"
#include "object.h"

typedef struct Entry_t
{
    ObjString_t* key;
    Value_t val;
} Entry_t;


typedef struct Table_t
{
    Allocator_t* alloc;
    size_t count;
    size_t capacity;
    Entry_t* entries;
} Table_t;


/*
 *  Initializes the hash table
 */
void Table_Init(Table_t* table, Allocator_t* alloc);

/*
 *  Frees the table itself, the keys and values are left in tact
 */
void Table_Free(Table_t* table);





/*
 *  get a value in the table that has the corresponding key
 *  \returns true if the key was found
 *  \returns false if the key was not found, val_out is untouched
 */
bool Table_Get(Table_t* table, const ObjString_t* key, Value_t* val_out);

/* 
 *  Set a key in the table to the correspond value, will override the 
 *  entry's value if an entry with the given key already exist
 *  \returns true if the key did not exist before, an entry will be created with the given key
 *  \returns false if the key already exist, the value associated with the key will be overwritten 
 *  to hold the given key and value
 */
bool Table_Set(Table_t* table, ObjString_t* key, Value_t val);

/* 
 *  marks an entry of the table that has the key as tombstone
 *  \returns true if the key exits
 *  \returns false if it doesn't, and do nothing
 */
bool Table_Delete(Table_t* table, const ObjString_t* key);

/*
 *  copies all valid entries from src to dst,
 *  valid entries are not tombstone and empty entries
 */
void Table_AddAll(const Table_t* src, Table_t* dst);

/* 
 *  find a key corresponding to the cstr and the hash given
 *  \returns NULL if not found
 *  \returns a pointer to the key if found
 */
ObjString_t* Table_FindStr(Table_t* table, const char* cstr, int len, uint32_t hash);

/*
 *  find a key corresponding to the substrs and the hash given
 *  the substr will be combined together to form a full string
 *  that string will be used to find the key
 *  \returns NULL if not found
 *  \returns a pointer to the entry's key if found
 */
ObjString_t* Table_FindStrs(Table_t* table, 
        int substr_count, const ObjString_t* substr[static substr_count], 
        uint32_t hash, int total_len
);


#endif /* _CLOX_TABLE_H_ */


