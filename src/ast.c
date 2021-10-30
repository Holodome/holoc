#include "ast.h"

#include "lib/lists.h"
#include "lib/memory.h"

Ast_List 
ast_list() {
    Ast_List list = {0};
    CDLIST_INIT(&list.sentinel);
    list.is_initialized = true;
    return list;    
}

void 
ast_list_add(Ast_List *list, Ast *ast) {
    assert(list->is_initialized);
    CDLIST_ADD_LAST(&list->sentinel, &ast->link);    
}

const static u64 AST_KIND_SIZES[] = {
    sizeof(Ast_Ident),
    sizeof(Ast_String_Lit),
    sizeof(Ast_Number_Lit),
    sizeof(Ast_Unary),
    sizeof(Ast_Binary),
    sizeof(Ast_Cond),
    sizeof(Ast_If),
    sizeof(Ast_For),
    sizeof(Ast_Do),
    sizeof(Ast_Switch),
    sizeof(Ast_Case),
    sizeof(Ast_Block),
    sizeof(Ast_Goto),
    sizeof(Ast_Label),
    sizeof(Ast_Func_Call),
    sizeof(Ast_Cast),
    sizeof(Ast_Member),
    sizeof(Ast_Return),
};

Ast *
ast_new(struct Memory_Arena *arena, u32 kind) {
    u64 size = AST_KIND_SIZES[kind];
    Ast *ast = (Ast *)arena_alloc(arena, size);
    ast->ast_kind = kind;
    return ast;
}
