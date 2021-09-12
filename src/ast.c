#include "ast.h"
#include "strings.h"

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

void ast_list_add(ASTList *list, AST *ast) {
    if (!list->first) {
        list->first = ast;
    }
    LLIST_ADD(list->last, ast);
}

void fmt_ast_tree_recursive(FmtBuffer *buf, AST *ast, u32 depth) {
    if (!ast) {
        fmt_buf(buf, "%*cNULL\n");
        return;
    }
    
    switch (ast->kind) {
        case AST_BLOCK: {
            fmt_buf(buf, "%*cBLOCK:\n", depth, ' ');
            for (AST *statement = ast->block.first_statement;
                 statement;
                 statement = statement->next) {
                fmt_ast_tree_recursive(buf, statement, depth + 1);        
            }
        } break;
        case AST_LITERAL: {
            fmt_buf(buf, "%*cLIT %s: ", depth, ' ', AST_LITERAL_STRS[ast->literal.kind]);
            switch (ast->literal.kind) {
                case AST_LITERAL_INT: {
                    fmt_buf(buf, "%lld", ast->literal.value_int);
                } break;
                case AST_LITERAL_REAL: {
                    fmt_buf(buf, "%f", ast->literal.value_real);
                } break;
                case AST_LITERAL_STRING: {
                    // @TODO
                    // fmt_buf(buf, "%s", ast->literal.value_str);
                } break;
                default: {
                    fmt_buf(buf, "NONE");
                } break;
            }
            fmt_buf(buf, "\n");
        } break;
        case AST_IDENT: {
            fmt_buf(buf, "%*cIDENT: %s\n", depth, ' ', ast->ident.name);
        } break;
        case AST_UNARY: {
            fmt_buf(buf, "%*cUNARY: %s EXPR:\n", depth, ' ', AST_UNARY_STRS[ast->unary.kind]);
            fmt_ast_tree_recursive(buf, ast->unary.expr, depth + 1);
        } break;
        case AST_BINARY: {
            fmt_buf(buf, "%*cBINARY: %s LEFT:\n", depth, ' ', AST_BINARY_STRS[ast->binary.kind]);
            fmt_ast_tree_recursive(buf, ast->binary.left, depth + 1);
            fmt_buf(buf, "%*cRIGHT:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->binary.right, depth + 1);
        } break;
        case AST_ASSIGN: {
            fmt_buf(buf, "%*cASSIGN: IDENT:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->assign.ident, depth + 1);
            fmt_buf(buf, "%*cEXPR:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->assign.expr, depth + 1);
        } break;
        case AST_PRINT: {
            
        } break;
        case AST_DECL: {
            fmt_buf(buf, "%*cDECL: IDENT:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->decl.ident, depth + 1);
            fmt_buf(buf, "%*cEXPR:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->decl.expr, depth + 1);
        } break;
        case AST_RETURN: {
            fmt_buf(buf, "%*cRETURN:\n", depth, ' ');
            for (AST *var = ast->return_st.vars;
                 var;
                 var = var->next) {
                fmt_ast_tree_recursive(buf, var, depth + 1);
            }
        } break;
        case AST_IF: {
            fmt_buf(buf, "%*cIF: COND:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->if_st.cond, depth + 1);
            fmt_buf(buf, "%*cBODY:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->if_st.block, depth + 1);
            if (ast->if_st.else_block) {
                fmt_buf(buf, "%*cELSE:\n", depth, ' ');
                fmt_ast_tree_recursive(buf, ast->if_st.else_block, depth + 1);
            }
        } break;
        case AST_FUNC_SIGNATURE: {
            fmt_buf(buf, "%*cFUNC SIGN: ARGS:\n", depth, ' ');
            for (AST *arg = ast->func_sign.arguments;
                 arg;
                 arg = arg->next) {
                fmt_ast_tree_recursive(buf, arg, depth + 1);        
            }
            fmt_buf(buf, "%*cOUTS:\n", depth, ' ');
            for (AST *arg = ast->func_sign.outs;
                 arg;
                 arg = arg->next) {
                fmt_ast_tree_recursive(buf, arg, depth + 1);        
            }
        } break;
        case AST_FUNC_DECL: {
            fmt_buf(buf, "%*cFUNC: NAME:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->func_decl.name, depth + 1);
            fmt_buf(buf, "%*cSIGNATURE:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->func_decl.sign, depth + 1);
            fmt_buf(buf, "%*cBODY:\n", depth, ' ');
            fmt_ast_tree_recursive(buf, ast->func_decl.block, depth + 1);
        } break;
        default: {
            fmt_buf(buf, "%*cUNKNOWN %u\n", depth, ' ', ast->kind);
        } break;
    }    
}