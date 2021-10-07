#include "ast.h"
#include "lib/strings.h"
#include "lib/lists.h"

#include "compiler_ctx.h"

const char *AST_TYPE_STRS[] = {
    "None",
    "Int",
    "Float",
    "Bool",
    "Str",
    "Proc"
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
    "Lectx->ssEquals",
    "Lectx->ss",
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
    "None",
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

AST_List 
create_ast_list(AST *sentinel) {
    AST_List result = {0};
    result.DBG_is_initialized = true;
    result.sentinel = sentinel;
    CDLIST_INIT(result.sentinel);
    return result;    
}

void 
ast_list_add(AST_List *list, AST *ast) {
    assert(list->DBG_is_initialized);
    ++list->DBG_len;
    CDLIST_ADD_LAST(list->sentinel, ast);
}

#define IDENT_DEPTH(_depth) (_depth) ? out_streamf(stream, "%*c", (_depth), ' ') : (void)0

void 
fmt_ast_tree(Compiler_Ctx *ctx, Out_Stream *stream, AST *ast, u32 depth) {
    if (!ast) {
        out_streamf(stream, "%*cNULL\n", depth, ' ');
        return;
    }
    
    switch (ast->kind) {
    case AST_BLOCK: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Block:\n");
        AST_LIST_ITER(&ast->block.statements, it) {
            fmt_ast_tree(ctx, stream, it, depth + 1);
        }
    } break;
    case AST_LIT: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Lit kind=%s ", AST_LITERAL_STRS[ast->lit.kind]);
        switch (ast->lit.kind) {
            case AST_LIT_INT: {
                out_streamf(stream, "int=%lld\n", ast->lit.value_int);
            } break;
            case AST_LIT_REAL: {
                out_streamf(stream, "real=%f\n", ast->lit.value_real);
            } break;
            case AST_LIT_STR: {
                char lit_str[4096];
                string_storage_get(ctx->ss, ast->lit.value_str, lit_str, sizeof(lit_str));
                out_streamf(stream, "str=%s\n", lit_str);
            } break;
        }
    } break; 
    case AST_IDENT: {
        IDENT_DEPTH(depth);
        char lit_str[4096];
        string_storage_get(ctx->ss, ast->ident.name, lit_str, sizeof(lit_str));
        out_streamf(stream, "Ident %s\n", lit_str);
    } break;
    case AST_TYPE: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Type %s\n", AST_TYPE_STRS[ast->type.kind]);
    } break;
    case AST_UNARY: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Unary kind=%s\n", AST_UNARY_STRS[ast->unary.kind]);
        fmt_ast_tree(ctx, stream, ast->unary.expr, depth + 1);
    } break; 
    case AST_BINARY: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Binary kind=%s\n", AST_BINARY_STRS[ast->binary.kind]);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Left:\n");
        fmt_ast_tree(ctx, stream, ast->binary.left, depth + 2);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Right:\n");
        fmt_ast_tree(ctx, stream, ast->binary.right, depth + 2);
    } break; 
    case AST_ASSIGN: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Actx->ssign\n");
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "LValue:\n");
        fmt_ast_tree(ctx, stream, ast->assign.lvalue, depth + 2);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "RValue:\n");
        fmt_ast_tree(ctx, stream, ast->assign.rvalue, depth + 2);
    } break; 
    case AST_PRINT: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Print:\n");
        AST_LIST_ITER(&ast->prints.arguments, it) {
            fmt_ast_tree(ctx, stream, it, depth + 1);
        }
    } break;
    case AST_DECL: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Decl:\n");
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Identifier:\n");
        fmt_ast_tree(ctx, stream, ast->decl.ident, depth + 2);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Type:\n");
        fmt_ast_tree(ctx, stream, ast->decl.type, depth + 2);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Value:\n");
        fmt_ast_tree(ctx, stream, ast->decl.expr, depth + 2);
    } break; 
    case AST_RETURN: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Return:\n");
        AST_LIST_ITER(&ast->returns.vars, it) {
            fmt_ast_tree(ctx, stream, it, depth + 1);
        }
    } break; 
    case AST_IF: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "If:\n");
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Condition:\n");
        fmt_ast_tree(ctx, stream, ast->ifs.cond, depth + 2);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Block:\n");
        fmt_ast_tree(ctx, stream, ast->ifs.block, depth + 2);
        if (ast->ifs.else_block) {
            IDENT_DEPTH(depth + 1);
            out_streamf(stream, "Else:\n");
            fmt_ast_tree(ctx, stream, ast->ifs.else_block, depth + 2);
        }
    } break; 
    case AST_WHILE: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "While:\n");
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Condition:\n");
        fmt_ast_tree(ctx, stream, ast->whiles.cond, depth + 2);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Block:\n");
        fmt_ast_tree(ctx, stream, ast->whiles.block, depth + 2);
    } break; 
    case AST_FUNC_SIGNATURE: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Func sign:\n");
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Arguments:\n");
        AST_LIST_ITER(&ast->func_sign.arguments, it) {
            fmt_ast_tree(ctx, stream, it, depth + 2);
        }
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Return types:\n");
        AST_LIST_ITER(&ast->func_sign.return_types, it) {
            fmt_ast_tree(ctx, stream, it, depth + 2);
        }
    } break;
    case AST_FUNC_DECL: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Func:\n");
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Name:\n");
        fmt_ast_tree(ctx, stream, ast->func_decl.name, depth + 2);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Signature:\n");
        fmt_ast_tree(ctx, stream, ast->func_decl.sign, depth + 2);
        fmt_ast_tree(ctx, stream, ast->func_decl.block, depth + 1);
    } break; 
    case AST_FUNC_CALL: {
        IDENT_DEPTH(depth);
        out_streamf(stream, "Func call:\n");
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Callable:\n");
        fmt_ast_tree(ctx, stream, ast->func_call.callable, depth + 2);
        IDENT_DEPTH(depth + 1);
        out_streamf(stream, "Arguments:\n");
        AST_LIST_ITER(&ast->func_call.arguments, it) {
            fmt_ast_tree(ctx, stream, it, depth + 2);
        }
    } break;
    INVALID_DEFAULT_CASE;
    }
}
