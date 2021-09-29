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

void ast_list_add(ASTList *list, AST *ast) {
    assert(list->DBG_is_initialized);
    ++list->DBG_len;
    CDLIST_ADD_LAST(list->sentinel, ast);
}

void fmt_ast_tree(StringStorage *ss, OutStream *st, AST *ast, u32 depth) {
    if (!ast) {
        out_streamf(st, "%*cNULL\n", depth, ' ');
        return;
    }
    
    switch (ast->kind) {
        case AST_BLOCK: {
            out_streamf(st, "%*cBLOCK:\n", depth, ' ');
            AST_LIST_ITER(&ast->block.statements, statement) {
                fmt_ast_tree(ss, st, statement, depth + 1);        
            }
        } break;
        case AST_LIT: {
            out_streamf(st, "%*cLIT %s: ", depth, ' ', AST_LITERAL_STRS[ast->lit.kind]);
            switch (ast->lit.kind) {
                case AST_LIT_INT: {
                    out_streamf(st, "%lld", ast->lit.value_int);
                } break;
                case AST_LIT_REAL: {
                    out_streamf(st, "%f", ast->lit.value_real);
                } break;
                case AST_LIT_STRING: {
                    // @TODO
                    // out_streamf(st, "%s", ast->literal.value_str);
                } break;
                default: {
                    out_streamf(st, "NONE");
                } break;
            }
            out_streamf(st, "\n");
        } break;
        case AST_IDENT: {
            out_streamf(st, "%*cIDENT: %s\n", depth, ' ', string_storage_get(ss, ast->ident.name));
        } break;
        case AST_UNARY: {
            out_streamf(st, "%*cUNARY: %s EXPR:\n", depth, ' ', AST_UNARY_STRS[ast->unary.kind]);
            fmt_ast_tree(ss, st, ast->unary.expr, depth + 1);
        } break;
        case AST_BINARY: {
            out_streamf(st, "%*cBINARY: %s LEFT:\n", depth, ' ', AST_BINARY_STRS[ast->binary.kind]);
            fmt_ast_tree(ss, st, ast->binary.left, depth + 1);
            out_streamf(st, "%*cRIGHT:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->binary.right, depth + 1);
        } break;
        case AST_ASSIGN: {
            out_streamf(st, "%*cASSIGN: IDENT:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->assign.ident, depth + 1);
            out_streamf(st, "%*cEXPR:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->assign.expr, depth + 1);
        } break;
        case AST_PRINT: {
            out_streamf(st, "%*cPRINT: EXPRS:\n", depth, ' ');
            AST_LIST_ITER(&ast->prints.arguments, statement) {
                fmt_ast_tree(ss, st, statement, depth + 1);
            }
        } break;
        case AST_DECL: {
            out_streamf(st, "%*cDECL: IDENT:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->decl.ident, depth + 1);
            out_streamf(st, "%*cEXPR:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->decl.expr, depth + 1);
        } break;
        case AST_RETURN: {
            out_streamf(st, "%*cRETURN:\n", depth, ' ');
            AST_LIST_ITER(&ast->returns.vars, var) {
                fmt_ast_tree(ss, st, var, depth + 1);
            }
        } break;
        case AST_IF: {
            out_streamf(st, "%*cIF: COND:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->ifs.cond, depth + 1);
            out_streamf(st, "%*cBODY:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->ifs.block, depth + 1);
            if (ast->ifs.else_block) {
                out_streamf(st, "%*cELSE:\n", depth, ' ');
                fmt_ast_tree(ss, st, ast->ifs.else_block, depth + 1);
            }
        } break;
        case AST_FUNC_SIGNATURE: {
            out_streamf(st, "%*cFUNC SIGN: ARGS:\n", depth, ' ');
            AST_LIST_ITER(&ast->func_sign.arguments, arg) {
                fmt_ast_tree(ss, st, arg, depth + 1);        
            }
            out_streamf(st, "%*cOUTS:\n", depth, ' ');
            AST_LIST_ITER(&ast->func_sign.return_types, arg) {
                fmt_ast_tree(ss, st, arg, depth + 1);        
            }
        } break;
        case AST_FUNC_DECL: {
            out_streamf(st, "%*cFUNC: NAME:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->func_decl.name, depth + 1);
            out_streamf(st, "%*cSIGNATURE:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->func_decl.sign, depth + 1);
            out_streamf(st, "%*cBODY:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->func_decl.block, depth + 1);
        } break;
        case AST_FUNC_CALL: {
            out_streamf(st, "%*cFUNC CALL: NAME:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->func_call.callable, depth + 1);
            out_streamf(st, "%*cARGUMENTS:\n", depth, ' ');
            AST_LIST_ITER(&ast->func_call.arguments, arg) {
                fmt_ast_tree(ss, st, arg, depth + 1);        
            }
        } break;
        case AST_TYPE: {
            out_streamf(st, "%*cTYPE: %s", depth, ' ', AST_TYPE_STRS[ast->type.kind]);
        } break;
        case AST_WHILE: {
            out_streamf(st, "%*cWHILE: COND:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->whiles.cond, depth + 1);
            out_streamf(st, "%*cBLOCK:\n", depth, ' ');
            fmt_ast_tree(ss, st, ast->whiles.block, depth + 1);
        } break;
        default: {
            out_streamf(st, "%*cUNKNOWN %u\n", depth, ' ', ast->kind);
            out_stream_flush(st);
            DBG_BREAKPOINT;
        } break;
    }    
}

void fmt_ast_expr(StringStorage *ss, OutStream *st, AST *ast) {
    if (!ast) {
        out_streamf(st, "NULL\n");
        return;
    }
    
    switch (ast->kind) {
        case AST_LIT: {
            switch (ast->lit.kind) {
                case AST_LIT_INT: {
                    out_streamf(st, "%lld", ast->lit.value_int);
                } break;
                case AST_LIT_REAL: {
                    out_streamf(st, "%f", ast->lit.value_real);
                } break;
                INVALID_DEFAULT_CASE;
            }
        } break;
        case AST_BINARY: {
            out_streamf(st, "(");
            fmt_ast_expr(ss, st, ast->binary.left);
            const char *op_str = AST_BINARY_SYMBS[ast->binary.kind];
            out_streamf(st, "%s", op_str);
            fmt_ast_expr(ss, st, ast->binary.right);
            out_streamf(st, ")");
        } break;
        case AST_UNARY: {
            const char *op_str = AST_UNARY_SYMBS[ast->unary.kind];
            out_streamf(st, "(");
            fmt_ast_expr(ss, st, ast->unary.expr);
            out_streamf(st, ")");
        } break;
        case AST_IDENT: {
            out_streamf(st, "%s", string_storage_get(ss, ast->ident.name));
        } break;
        INVALID_DEFAULT_CASE;
    }
}