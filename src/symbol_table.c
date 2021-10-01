#include "symbol_table.h"
#include "lib/lists.h"

static SymbolTableScope *
get_new_scope(SymbolTable *table) {
    SymbolTableScope *scope = table->scope_freelist;
    if (scope) {
        STACK_POP(table->scope_freelist);
        scope->entries = 0;
        scope->next = 0;
    } else {
        scope = arena_alloc_struct(table->arena, SymbolTableScope);
    }
    return scope;
}

static void 
add_scope_to_freelist(SymbolTable *table, SymbolTableScope *scope) {
    while (scope->entries) {
        SymbolTableEntry *entry = scope->entries;
        STACK_ADD(table->entry_freelist, entry);
        STACK_POP(scope->entries);
    }
    STACK_ADD(table->scope_freelist, scope);
}

SymbolTable *
create_symbol_table(MemoryArena *arena) {
    SymbolTable *st = arena_alloc_struct(arena, SymbolTable);
    st->arena = arena;
    st->global_scope = get_new_scope(st);
    return st;
}

void 
symbol_table_push_scope(SymbolTable *st) {
    SymbolTableScope *scope = get_new_scope(st);
    STACK_ADD(st->scope_stack, scope);
    ++st->scope_depth;
}

void 
symbol_table_pop_scope(SymbolTable *st) {
    assert(st->scope_depth);
    SymbolTableScope *scope = st->scope_stack;
    STACK_POP(st->scope_stack);
    add_scope_to_freelist(st, scope);
}

void 
symbol_table_add_entry(SymbolTable *st, StringID str, u32 ast_type, SrcLoc loc) {
    SymbolTableScope *scope = st->scope_stack;
}

SymbolTableEntry *
symbol_table_lookup(SymbolTable *st, StringID str) {
    SymbolTableEntry *entry = 0;
    SymbolTableScope *scope = st->scope_stack;
    while (!entry && scope) {
        for (SymbolTableEntry *it = scope->entries;
             it;
             it = it->next) {
            if (it->str.value == str.value) {
                entry = it;
                break;
            }        
        }
    }
    return entry;
}