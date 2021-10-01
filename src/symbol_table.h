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

typedef struct SymbolTableEntry {
    StringID str;
    u32 ast_type;
    SrcLoc declare_loc;
    
    struct SymbolTableEntry *next;
} SymbolTableEntry;

typedef struct SymbolTableScope {
    SymbolTableEntry *entries;
    
    // parent
    struct SymbolTableScope *next;
} SymbolTableScope;

typedef struct {
    MemoryArena *arena;
    Hash64 hash; // @TODO(hl): Not used
    
    SymbolTableScope *global_scope;
    SymbolTableScope *scope_stack;
    u32 scope_depth;
    
    SymbolTableScope *scope_freelist;
    SymbolTableEntry *entry_freelist;
} SymbolTable;

SymbolTable *create_symbol_table(MemoryArena *arena);
void symbol_table_push_scope(SymbolTable *st);
void symbol_table_pop_scope(SymbolTable *st);
void symbol_table_add_entry(SymbolTable *st, StringID str, u32 ast_type, SrcLoc loc);
SymbolTableEntry *symbol_table_lookup(SymbolTable *st, StringID str);