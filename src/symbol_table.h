/* 
Author: Holodome
Date: 01.10.2021 
File: pkby/src/symbol_table.h
Version: 0
*/
#pragma once
#include "lib/general.h"
#include "platform/memory.h"
#include "lib/hashing.h"

#include "ast.h"
#include "string_storage.h"

#define SYMBOL_TABLE_HASH_SIZE 8192

typedef struct Symbol_Table_Entry {
    String_ID str;
    u32 ast_type;
    Src_Loc declare_loc;
    bool is_initialized;
    
    struct Symbol_Table_Entry *next;
} Symbol_Table_Entry;

typedef struct Symbol_Table_Scope {
    Symbol_Table_Entry *entries;
    
    // parent
    struct Symbol_Table_Scope *next;
} Symbol_Table_Scope;

typedef struct {
    Memory_Arena *arena;
    Hash64 hash; // @TODO(hl): Not used
    
    Symbol_Table_Scope *global_scope;
    Symbol_Table_Scope *scope_stack;
    u32 scope_depth;
    
    Symbol_Table_Scope *scope_freelist;
    Symbol_Table_Entry *entry_freelist;
} Symbol_Table;

Symbol_Table *create_symbol_table(Memory_Arena *arena);
void symbol_table_push_scope(Symbol_Table *st);
void symbol_table_pop_scope(Symbol_Table *st);
void symbol_table_add_entry(Symbol_Table *st, String_ID str, u32 ast_type, Src_Loc loc);
Symbol_Table_Entry *symbol_table_lookup(Symbol_Table *st, String_ID str);