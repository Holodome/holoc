#include "ast.h"
#include "lib/strings.h"
#include "lib/lists.h"

const char *AST_TYPE_STRS[] = {
    "None",
    "Int",
    "Float",
    "Bool",
    "Str"
};

const char *AST_LITERAL_STRS[] = {
    "None",
    "Int",
    "Real",
    "String"
};

const char *AST_UNARY_STRS[] = {
    "None",
    "Minus",
    "Plus",
    "LogicalNot",
    "Not"
};

const char *AST_UNARY_SYMBS[] = {
    "None",
    "-",
    "+",
    "!",
    "~"  
};

const char *AST_BINARY_STRS[] = {
    "None",
    "Add",
    "Sub",
    "Mul",
    "Div",
    "IAdd",
    "ISub",
    "IMul",
    "LessEquals",
    "Less",
    "GreaterEquals",
    "Equals",
    "NotEquals",
    "And",
    "Or",
    "Xor",
    "IAnd",
    "IOr",
    "IXor",  
    "LShift",
    "ILShift",
    "RShift",
    "IRShift"
};

const char *AST_BINARY_SYMBS[] = {
    "None"
    "+",
    "-",
    "*",
    "/",    
    "%",    
    "<=",    
    "<",    
    ">=",    
    ">",    
    "==",    
    "!=",    
    "&",    
    "|",    
    "^",    
    "<<",    
    ">>",    
    "&&",    
    "||",    
};

ASTList 
create_ast_list(AST *sentinel) {
    ASTList result = {0};
    result.DBG_is_initialized = TRUE;
    result.sentinel = sentinel;
    CDLIST_INIT(result.sentinel);
    return result;    
}

void 
ast_list_add(ASTList *list, AST *ast) {
    assert(list->DBG_is_initialized);
    ++list->DBG_len;
    CDLIST_ADD_LAST(list->sentinel, ast);
}

void 
fmt_ast_tree(StringStorage *ss, OutStream *st, AST *ast, u32 depth) {
    if (!ast) {
        out_streamf(st, "%*cNULL\n", depth, ' ');
        return;
    }
    
    switch (ast->kind) {
        case AST_BLOCK: {
            
        } break;
        case AST_LIT: {
            
        } break; 
        case AST_IDENT: {
            
        } break;
        case AST_TYPE: {
            
        } break; 
        case AST_UNARY: {
            
        } break; 
        case AST_BINARY: {
            
        } break; 
        case AST_ASSIGN: {
            
        } break; 
        case AST_PRINT: {
            
        } break;
        case AST_DECL: {
            
        } break; 
        case AST_RETURN: {
            
        } break; 
        case AST_IF: {
            
        } break; 
        case AST_WHILE: {
            
        } break; 
        case AST_FUNC_SIGNATURE: {
            
        } break;
        case AST_FUNC_DECL: {
            
        } break; 
        case AST_FUNC_CALL: {
            
        } break;
        INVALID_DEFAULT_CASE;
    }
}