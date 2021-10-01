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

#define IDENT_DEPTH(_depth) (_depth) ? out_streamf(st, "%*c", (_depth), ' ') : (void)0

void 
fmt_ast_tree(StringStorage *ss, OutStream *st, AST *ast, u32 depth) {
    if (!ast) {
        out_streamf(st, "%*cNULL\n", depth, ' ');
        return;
    }
    
    switch (ast->kind) {
        case AST_BLOCK: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Block:\n");
            AST_LIST_ITER(&ast->block.statements, it) {
                fmt_ast_tree(ss, st, it, depth + 1);
            }
        } break;
        case AST_LIT: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Lit kind=%s ", AST_LITERAL_STRS[ast->lit.kind]);
            switch (ast->lit.kind) {
                case AST_LIT_INT: {
                    out_streamf(st, "int=%lld\n", ast->lit.value_int);
                } break;
                case AST_LIT_REAL: {
                    out_streamf(st, "real=%f\n", ast->lit.value_real);
                } break;
                case AST_LIT_STR: {
                    char lit_str[4096];
                    string_storage_get(ss, ast->lit.value_str, lit_str, sizeof(lit_str));
                    out_streamf(st, "str=%s\n", lit_str);
                } break;
            }
        } break; 
        case AST_IDENT: {
            IDENT_DEPTH(depth);
            char lit_str[4096];
            string_storage_get(ss, ast->ident.name, lit_str, sizeof(lit_str));
            out_streamf(st, "Ident %s\n", lit_str);
        } break;
        case AST_TYPE: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Type %s\n", AST_TYPE_STRS[ast->type.kind]);
        } break;
        case AST_UNARY: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Unary kind=%s\n", AST_UNARY_STRS[ast->unary.kind]);
            fmt_ast_tree(ss, st, ast->unary.expr, depth + 1);
        } break; 
        case AST_BINARY: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Binary kind=%s\n", AST_BINARY_STRS[ast->binary.kind]);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Left:\n");
            fmt_ast_tree(ss, st, ast->binary.left, depth + 2);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Right:\n");
            fmt_ast_tree(ss, st, ast->binary.right, depth + 2);
        } break; 
        case AST_ASSIGN: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Assign\n");
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "LValue:\n");
            fmt_ast_tree(ss, st, ast->assign.lvalue, depth + 2);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "RValue:\n");
            fmt_ast_tree(ss, st, ast->assign.rvalue, depth + 2);
        } break; 
        case AST_PRINT: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Print:\n");
            AST_LIST_ITER(&ast->prints.arguments, it) {
                fmt_ast_tree(ss, st, it, depth + 1);
            }
        } break;
        case AST_DECL: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Decl:\n");
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Identifier:\n");
            fmt_ast_tree(ss, st, ast->decl.ident, depth + 2);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Type:\n");
            fmt_ast_tree(ss, st, ast->decl.type, depth + 2);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Value:\n");
            fmt_ast_tree(ss, st, ast->decl.expr, depth + 2);
        } break; 
        case AST_RETURN: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Return:\n");
            AST_LIST_ITER(&ast->returns.vars, it) {
                fmt_ast_tree(ss, st, it, depth + 1);
            }
        } break; 
        case AST_IF: {
            IDENT_DEPTH(depth);
            out_streamf(st, "If:\n");
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Condition:\n");
            fmt_ast_tree(ss, st, ast->ifs.cond, depth + 2);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Block:\n");
            fmt_ast_tree(ss, st, ast->ifs.block, depth + 2);
            if (ast->ifs.else_block) {
                IDENT_DEPTH(depth + 1);
                out_streamf(st, "Else:\n");
                fmt_ast_tree(ss, st, ast->ifs.else_block, depth + 2);
            }
        } break; 
        case AST_WHILE: {
            IDENT_DEPTH(depth);
            out_streamf(st, "While:\n");
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Condition:\n");
            fmt_ast_tree(ss, st, ast->whiles.cond, depth + 2);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Block:\n");
            fmt_ast_tree(ss, st, ast->whiles.block, depth + 2);
        } break; 
        case AST_FUNC_SIGNATURE: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Func sign:\n");
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Arguments:\n");
            AST_LIST_ITER(&ast->func_sign.arguments, it) {
                fmt_ast_tree(ss, st, it, depth + 2);
            }
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Return types:\n");
            AST_LIST_ITER(&ast->func_sign.return_types, it) {
                fmt_ast_tree(ss, st, it, depth + 2);
            }
        } break;
        case AST_FUNC_DECL: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Func:\n");
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Name:\n");
            fmt_ast_tree(ss, st, ast->func_decl.name, depth + 2);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Signature:\n");
            fmt_ast_tree(ss, st, ast->func_decl.sign, depth + 2);
            fmt_ast_tree(ss, st, ast->func_decl.block, depth + 1);
        } break; 
        case AST_FUNC_CALL: {
            IDENT_DEPTH(depth);
            out_streamf(st, "Func call:\n");
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Callable:\n");
            fmt_ast_tree(ss, st, ast->func_call.callable, depth + 2);
            IDENT_DEPTH(depth + 1);
            out_streamf(st, "Arguments:\n");
            AST_LIST_ITER(&ast->func_call.arguments, it) {
                fmt_ast_tree(ss, st, it, depth + 2);
            }
        } break;
        INVALID_DEFAULT_CASE;
    }
}
