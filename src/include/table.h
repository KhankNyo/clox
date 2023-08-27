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



void Table_Init(Table_t* table, Allocator_t* alloc);
void Table_Free(Table_t* table);



bool Table_Get(Table_t* table, const ObjString_t* key, Value_t* val);
bool Table_Set(Table_t* table, ObjString_t* key, Value_t val);
bool Table_Delete(Table_t* table, const ObjString_t* key);
void Table_AddAll(const Table_t* src, Table_t* dst);
ObjString_t* Table_FindStr(Table_t* strings, const char* cstr, int len, uint32_t hash);



#endif /* _CLOX_TABLE_H_ */


