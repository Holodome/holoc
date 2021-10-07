#include "symbol_table.h"
#include "lib/lists.h"

static Symbol_Table_Scope *
get_new_scope(Symbol_Table *table) {
    Symbol_Table_Scope *scope = table->scope_freelist;
    if (scope) {
        STACK_POP(table->scope_freelist);
        mem_zero(scope, sizeof(*scope));
    } else {
        scope = arena_alloc_struct(table->arena, Symbol_Table_Scope);
    }
    return scope;
}

static Symbol_Table_Entry *
get_new_entry(Symbol_Table *table) {
    Symbol_Table_Entry *entry = table->entry_freelist;
    if (entry) {
        STACK_POP(table->entry_freelist);
        mem_zero(entry, sizeof(*entry));
    } else {
        entry = arena_alloc_struct(table->arena, Symbol_Table_Entry);
    }
    return entry;
}

static void 
add_scope_to_freelist(Symbol_Table *table, Symbol_Table_Scope *scope) {
    while (scope->entries) {
        Symbol_Table_Entry *entry = scope->entries;
        STACK_ADD(table->entry_freelist, entry);
        STACK_POP(scope->entries);
    }
    STACK_ADD(table->scope_freelist, scope);
}

Symbol_Table *
create_symbol_table(Memory_Arena *arena) {
    Symbol_Table *st = arena_alloc_struct(arena, Symbol_Table);
    st->arena = arena;
    st->global_scope = get_new_scope(st);
    st->scope_stack = st->global_scope;
    return st;
}

void 
symbol_table_push_scope(Symbol_Table *st) {
    Symbol_Table_Scope *scope = get_new_scope(st);
    STACK_ADD(st->scope_stack, scope);
    ++st->scope_depth;
}

void 
symbol_table_pop_scope(Symbol_Table *st) {
    assert(st->scope_depth);
    Symbol_Table_Scope *scope = st->scope_stack;
    STACK_POP(st->scope_stack);
    add_scope_to_freelist(st, scope);
}

void 
symbol_table_add_entry(Symbol_Table *st, String_ID str, u32 ast_type, Src_Loc loc) {
    Symbol_Table_Scope *scope = st->scope_stack;
    Symbol_Table_Entry *entry = get_new_entry(st);
    STACK_ADD(scope->entries, entry);
    entry->ast_type = ast_type;
    entry->declare_loc = loc;
    entry->str = str;
}

Symbol_Table_Entry *
symbol_table_lookup(Symbol_Table *st, String_ID str) {
    Symbol_Table_Entry *entry = 0;
    Symbol_Table_Scope *scope = st->scope_stack;
    while (!entry && scope) {
        for (Symbol_Table_Entry *it = scope->entries;
             it;
             it = it->next) {
            if (it->str.value == str.value) {
                entry = it;
                break;
            }        
        }
        scope = scope->next;
    }
    return entry;
}