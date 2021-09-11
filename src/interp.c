#include "interp.h"
#include "strings.h"
#include "interp.h"

void report_error(Interp *interp, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    erroutf("[ERROR] %s:", get_file_name(interp->file_id));
    verroutf(msg, args);
    erroutf("\n");    
    
    interp->reported_error = TRUE;
    DBG_BREAKPOINT;
}

void report_error_at_internal(Interp *interp, SourceLocation source_loc, const char *msg, va_list args) {
    const FileData *file_data = get_file_data(interp->file_id);
    // @TODO more robust algorithm
    u32 current_line_idx = 1;
    const char *line_start = file_data->str;
    while (current_line_idx != source_loc.line) {
        if (*line_start == '\n') {
            ++current_line_idx;
        }
        ++line_start;
    }
    const char *line_end = line_start;
    while (*line_end != '\n' && (line_end - file_data->str) < file_data->size) {
        ++line_end;
    }
    
    erroutf("%s:%u:%u: \033[31merror\033[0m: ", get_file_name(interp->file_id), source_loc.line, source_loc.symb);
    verroutf(msg, args);
    erroutf("\n%.*s\n%*c\033[33m^\033[0m\n", line_end - line_start, line_start,
        source_loc.symb, ' ');
        
    interp->reported_error = TRUE;
    DBG_BREAKPOINT;
}

void report_error_at(Interp *interp, SourceLocation source_loc, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_at_internal(interp, source_loc, msg, args);
}

void report_error_tok(Interp *interp, Token *tok, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_at_internal(interp, tok->source_loc, msg, args);
}

void report_error_ast(Interp *interp, AST *ast, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    report_error_at_internal(interp, ast->source_loc, msg, args);
        
}

static void report_unexpected_token(Interp *interp, Token *tok, u32 expected) {
    char expected_str[128], got_str[128];
    fmt_tok_kind(expected_str, sizeof(expected_str), expected);
    fmt_tok_kind(got_str, sizeof(got_str), tok->kind);
    report_error_tok(interp, tok, "Token %s expected (got %s)",
        expected_str, got_str);
}

static b32 expect_tok(Interp *interp, Token *tok, u32 kind) {
    b32 result = TRUE;
    if (tok->kind != kind) {
        report_unexpected_token(interp, tok, kind);
        result = FALSE;
    }
    return result;
}

static b32 parse_end_of_statement(Interp *interp, Token *tok) {
    b32 result = expect_tok(interp, tok, ';');
    if (result) {
        eat_tok(interp->tokenizer);
    }
    return result;
}

static AST *ast_new(Interp *interp, u32 kind) {
    AST *ast = arena_alloc_struct(&interp->arena, AST);
    ast->kind = kind;
    // @TODO more explicit way of taking source location?
    ast->source_loc = peek_tok(interp->tokenizer)->source_loc;
    return ast;
}

static AST *create_ident(Interp *interp, const char *name) {
    AST *ident = ast_new(interp, AST_IDENT);
    ident->ident.name = arena_alloc_str(&interp->arena, name);
    return ident;
}

static AST *create_int_lit(Interp *interp, i64 value) {
    AST *literal = ast_new(interp, AST_LITERAL);
    literal->literal.kind = AST_LITERAL_INT;
    literal->literal.value_int = value;
    return literal;
}

AST *parse_comma_separated_idents(Interp *interp) {
    AST *idents = 0;
    Token *tok = peek_tok(interp->tokenizer);
    while (tok->kind == TOKEN_IDENT) {
        AST *ident = create_ident(interp, tok->value_str);
        LLIST_ADD(idents, ident);
        tok = peek_next_tok(interp->tokenizer);
        if (tok->kind == ',') {
            tok = peek_next_tok(interp->tokenizer);
        }
    }
    return idents;
}

AST *parse_expr2(Interp *interp);
AST *parse_expr1(Interp *interp);
AST *parse_expr(Interp *interp);
AST *parse_decl(Interp *interp);
AST *parse_decl_ident(Interp *interp, AST *ident);
AST *parse_block(Interp *interp);

/* 
1: . () []
2: u+ u- ! ~
3: * / %
4: b+ b-
5: << >>
6: < <= > >=
7: == !=
8: &
9: ^
10: | 
11: &&
12: ||
*/
#define MAX_OPEARTOR_PRECEDENCE 12
// Parse expression with ascending precedence - lower ones are parsed earlier
// Currently this is made by making 
#define parse_expr(_interp) parse_expr_precedence(_interp, MAX_OPEARTOR_PRECEDENCE)
AST *parse_expr_precedence(Interp *interp, u32 precedence) {
    AST *expr = 0;
    switch (precedence) {
        case 0: {
            Token *tok = peek_tok(interp->tokenizer);
            switch (tok->kind) {
                case TOKEN_IDENT: {
                    eat_tok(interp->tokenizer);
                    AST *ident = create_ident(interp, tok->value_str);
                    expr = ident;
                } break;
                case TOKEN_INT: {
                    eat_tok(interp->tokenizer);
                    AST *lit = create_int_lit(interp, tok->value_int);
                    expr = lit;
                } break;
                case TOKEN_REAL: {
                    NOT_IMPLEMENTED;
                } break;
            }
        } break;
        case 1: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            switch (tok->kind) {
                case '(': {
                    eat_tok(interp->tokenizer);
                    AST *temp = parse_expr(interp);
                    tok = peek_tok(interp->tokenizer);
                    if (expect_tok(interp, tok, ')')) {
                        expr = temp;
                        eat_tok(interp->tokenizer);
                    }
                } break;
                case '.': {
                    NOT_IMPLEMENTED;
                } break;
                case '[': {
                    NOT_IMPLEMENTED;
                } break;
            }
        } break;
        case 2: {
            Token *tok = peek_tok(interp->tokenizer);
            switch (tok->kind) {
                case '+': {
                    eat_tok(interp->tokenizer);
                    AST *unary = ast_new(interp, AST_UNARY);
                    unary->unary.kind = AST_UNARY_PLUS;
                    unary->unary.expr = parse_expr(interp);
                    expr = unary;
                } break;
                case '-': {
                    eat_tok(interp->tokenizer);
                    AST *unary = ast_new(interp, AST_UNARY);
                    unary->unary.kind = AST_UNARY_MINUS;
                    unary->unary.expr = parse_expr(interp);
                    expr = unary;
                } break;
                case '!': {
                    eat_tok(interp->tokenizer);
                    AST *unary = ast_new(interp, AST_UNARY);
                    unary->unary.kind = AST_UNARY_LOGICAL_NOT;
                    unary->unary.expr = parse_expr(interp);
                    expr = unary;
                } break;
                case '~': {
                    eat_tok(interp->tokenizer);
                    AST *unary = ast_new(interp, AST_UNARY);
                    unary->unary.kind = AST_UNARY_NOT;
                    unary->unary.expr = parse_expr(interp);
                    expr = unary;
                } break;
            }
            
            if (!expr) {
                expr = parse_expr_precedence(interp, precedence - 1);
            }
        } break;
        case 3: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == '*' || tok->kind == '/' || tok->kind == '%') {
                AST *bin = ast_new(interp, AST_BINARY);
                switch (tok->kind) {
                    case '*': {
                        bin->binary.kind = AST_BINARY_MUL;
                    } break;
                    case '/': {
                        bin->binary.kind = AST_BINARY_DIV;
                    } break;
                    case '%': {
                        bin->binary.kind = AST_BINARY_MOD;
                    } break;
                }
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 4: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == '+' || tok->kind == '-') {
                AST *bin = ast_new(interp, AST_BINARY);
                if (tok->kind == '+') {
                    bin->binary.kind = AST_BINARY_ADD;
                } else if (tok->kind == '-') {
                    bin->binary.kind = AST_BINARY_SUB;
                }
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 5: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == TOKEN_LSHIFT || tok->kind == TOKEN_RSHIFT) {
                AST *bin = ast_new(interp, AST_BINARY);
                if (tok->kind == TOKEN_RSHIFT) {
                    bin->binary.kind = AST_BINARY_RSHIFT;
                } else if (tok->kind == TOKEN_LSHIFT) {
                    bin->binary.kind = AST_BINARY_SUB;
                }
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 6: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == '<' || tok->kind == '>' || tok->kind == TOKEN_LE || tok->kind == TOKEN_GE) {
                AST *bin = ast_new(interp, AST_BINARY);
                if (tok->kind == TOKEN_LE) {
                    bin->binary.kind = AST_BINARY_LE;
                } else if (tok->kind == TOKEN_GE) {
                    bin->binary.kind = AST_BINARY_GE;
                } else if (tok->kind == '<') {
                    bin->binary.kind = AST_BINARY_L;
                } else if (tok->kind == '>') {
                    bin->binary.kind = AST_BINARY_G;
                }
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 7: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == TOKEN_EQ || tok->kind == TOKEN_NEQ) {
                AST *bin = ast_new(interp, AST_BINARY);
                if (tok->kind == TOKEN_EQ) {
                    bin->binary.kind = AST_BINARY_EQ;
                } else if (tok->kind == TOKEN_NEQ) {
                    bin->binary.kind = AST_BINARY_NEQ;
                }
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 8: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == '&') {
                AST *bin = ast_new(interp, AST_BINARY);
                bin->binary.kind = AST_BINARY_AND;
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 9: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == '&') {
                AST *bin = ast_new(interp, AST_BINARY);
                bin->binary.kind = AST_BINARY_XOR;
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 10: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == '&') {
                AST *bin = ast_new(interp, AST_BINARY);
                bin->binary.kind = AST_BINARY_OR;
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 11: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == TOKEN_LOGICAL_AND) {
                AST *bin = ast_new(interp, AST_BINARY);
                bin->binary.kind = AST_BINARY_LOGICAL_AND;
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        case 12: {
            expr = parse_expr_precedence(interp, precedence - 1);
            Token *tok = peek_tok(interp->tokenizer);
            while (tok->kind == TOKEN_LOGICAL_OR) {
                AST *bin = ast_new(interp, AST_BINARY);
                bin->binary.kind = AST_BINARY_LOGICAL_OR;
                eat_tok(interp->tokenizer);
                bin->binary.left = expr;
                bin->binary.right = parse_expr_precedence(interp, precedence - 1);
                expr = bin;
                tok = peek_tok(interp->tokenizer);
            }
        } break;
        default: assert(FALSE);
    }
    return expr;
}

AST *parse_assign_ident(Interp *interp, AST *ident) {
    AST *assign = 0;
    Token *tok = peek_tok(interp->tokenizer);
    if (!is_token_assign(tok->kind)) {
        report_error_tok(interp, tok, "Expected assign operator");
        return 0;    
    }
    
    // All incorrect tokens should be handled before
    eat_tok(interp->tokenizer);
    // @TODO make this more readable...
    if (tok->kind == '=') {
        AST *expr = parse_expr(interp);
        
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = expr;
    } else if (tok->kind == TOKEN_IADD) { 
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_ADD;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_ISUB) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_SUB;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_IMUL) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_MUL;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_IDIV) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_DIV;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_IMOD) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_MOD;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_IAND) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_AND;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_IOR) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_OR;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_IXOR) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_XOR;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_ILSHIFT) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_LSHIFT;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    } else if (tok->kind == TOKEN_IRSHIFT) {
        AST *expr = parse_expr(interp);
        AST *add = ast_new(interp, AST_BINARY);
        add->binary.kind = AST_BINARY_RSHIFT;
        add->binary.left = ident;
        add->binary.right = expr;
        assign = ast_new(interp, AST_ASSIGN);
        assign->assign.ident = ident;
        assign->assign.expr = add;
    }
    
    tok = peek_tok(interp->tokenizer);
    if (!parse_end_of_statement(interp, tok)) {
        assign = 0;
    }
    return assign;
}

AST *parse_if_compound(Interp *interp) {
    Token *tok = peek_tok(interp->tokenizer);
    if (tok->kind != TOKEN_KW_IF) {
        return 0;
    }
    
    tok = peek_next_tok(interp->tokenizer);
    AST *expr = parse_expr(interp);
    if (!expr) {
        return 0;
    }
    
    AST *if_st = ast_new(interp, AST_IF);
    if_st->if_st.cond = expr;
    tok = peek_tok(interp->tokenizer);
    if (expect_tok(interp, tok, '{')) {
        AST *block = parse_block(interp);
        if (!block) {
            return 0;
        }
        if_st->if_st.block = block;
        tok = peek_tok(interp->tokenizer);
        if (tok->kind == TOKEN_KW_ELSE) {
            tok = peek_next_tok(interp->tokenizer);
            if (tok->kind == TOKEN_KW_IF) {
                AST *else_if_st = parse_if_compound(interp);
                if_st->if_st.else_block = else_if_st;
            } else if (tok->kind == '{') {
                AST *else_block = parse_block(interp);
                if_st->if_st.else_block = else_block;
            }
        }
    }
    return if_st;
}

AST *parse_statement(Interp *interp) {
    AST *statement = 0;
    Token *tok = peek_tok(interp->tokenizer);
    // Common path for assign and decl
    // @TODO maybe unite assign and declaration
    AST *ident = 0;
    if (tok->kind == TOKEN_IDENT) {
        ident = create_ident(interp, tok->value_str);
        tok = peek_next_tok(interp->tokenizer);
    } 
    
    if (is_token_assign(tok->kind)) {
        statement = parse_assign_ident(interp, ident);
    } else if (tok->kind == TOKEN_KW_RETURN) {
        tok = peek_next_tok(interp->tokenizer);
        AST *return_vars = 0;
        while (tok->kind != ';') {
            AST *expr = parse_expr(interp);
            if (!expr || interp->reported_error) {
                break;
            }
            LLIST_ADD(return_vars, expr);
            
            tok = peek_tok(interp->tokenizer);
            if (tok->kind == ',') {
                tok = peek_next_tok(interp->tokenizer);
            }
        }
        
        if (parse_end_of_statement(interp, tok)) {
            statement = ast_new(interp, AST_RETURN);
            statement->return_st.vars = return_vars;
        }
    } else if (tok->kind == TOKEN_KW_IF) {
        AST *if_st = parse_if_compound(interp);
        statement = if_st;
    } else if (tok->kind == TOKEN_KW_WHILE) {
    } else if (tok->kind == TOKEN_KW_PRINT) {
    } else {
        statement = parse_decl_ident(interp, ident);
    }
    
    return statement;
}

AST *parse_block(Interp *interp) {
    AST *block = 0;
    Token *tok = peek_tok(interp->tokenizer);
    if (expect_tok(interp, tok, '{')) {
        tok = peek_next_tok(interp->tokenizer);     
    } else {
        return 0;
    }
    
    AST *statements = 0;
    while (peek_tok(interp->tokenizer)->kind != '}') {
        AST *statement = parse_statement(interp);
        if (!statement) {
            break;
        }
        
        if (interp->reported_error) {
            break;
        }
        LLIST_ADD(statements, statement);
    }
    block = ast_new(interp, AST_BLOCK);
    block->block.first_statement = statements;
    
    tok = peek_tok(interp->tokenizer);
    if (expect_tok(interp, tok, '}')) {
        tok = peek_next_tok(interp->tokenizer);     
    } else {
        return 0;
    }
    return block;
}

AST *parse_function_signature(Interp *interp) {
    Token *tok = peek_tok(interp->tokenizer);
    if (!expect_tok(interp, tok, '(')) {
        return 0;
    }
    tok = peek_next_tok(interp->tokenizer);
    // Parse arguments
    AST *arguments = 0;
    while (tok->kind != ')') {
        if (expect_tok(interp, tok, TOKEN_IDENT)) {
            AST *ident = create_ident(interp, tok->value_str);
            LLIST_ADD(arguments, ident);
            
            tok = peek_next_tok(interp->tokenizer);
            if (expect_tok(interp, tok, ':')) {
                tok = peek_next_tok(interp->tokenizer);
                if (expect_tok(interp, tok, TOKEN_IDENT)) {
                    tok = peek_next_tok(interp->tokenizer);
                    if (tok->kind == ',') {
                        tok = peek_next_tok(interp->tokenizer);
                    } 
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    if (interp->reported_error) {
        return 0;
    }
    
    AST *sign = ast_new(interp, AST_FUNC_SIGNATURE);
    sign->func_sign.arguments = arguments;
    
    tok = peek_next_tok(interp->tokenizer);
    if (tok->kind == TOKEN_ARROW) {
        tok = peek_next_tok(interp->tokenizer);
        
        AST *outs = parse_comma_separated_idents(interp);
        sign->func_sign.outs = outs;
    }
    return sign;
}

AST *parse_decl_ident(Interp *interp, AST *ident) {
    AST *decl = 0;
    Token *tok = peek_tok(interp->tokenizer);
    switch (tok->kind) {
        case TOKEN_AUTO_DECL: {
            tok = peek_next_tok(interp->tokenizer);
            AST *expr = parse_expr(interp);
            if (expr) {
                decl = ast_new(interp, AST_DECL);
                decl->decl.ident = ident;
                decl->decl.expr = expr;
                
                tok = peek_tok(interp->tokenizer);
                if (!parse_end_of_statement(interp, tok)) {
                    decl = 0;
                }
            }
        } break;
        case TOKEN_CONSTANT: {
            tok = peek_next_tok(interp->tokenizer);
            if (tok->kind == '(') {
                AST *func_sign = parse_function_signature(interp);
                if (func_sign) {
                    AST *func_block = parse_block(interp);
                    if (func_block) {
                        decl = ast_new(interp, AST_FUNC_DECL);
                        decl->func_decl.name = ident;
                        decl->func_decl.sign = func_sign;
                        decl->func_decl.block = func_block;
                    }
                }
            } else {
                AST *expr = parse_expr(interp);
                if (expr) {
                    decl = ast_new(interp, AST_DECL);
                    decl->decl.ident = ident;
                    decl->decl.expr = expr;
                    
                    tok = peek_tok(interp->tokenizer);
                    if (!parse_end_of_statement(interp, tok)) {
                        decl = 0;
                    }
                }
            }
        } break;
        case ':': {
            tok = peek_next_tok(interp->tokenizer);
            // @TODO parse type 
            AST *type = parse_expr(interp);
            tok = peek_tok(interp->tokenizer);
            decl = ast_new(interp, AST_DECL);
            decl->decl.ident = ident;   
            // @TODO use type
            if (tok->kind == '=') {
                tok = peek_next_tok(interp->tokenizer);
                AST *expr = parse_expr(interp);
                if (expr) {
                    decl->decl.expr = expr;
                }
            } 
            
            tok = peek_tok(interp->tokenizer);
            if (!parse_end_of_statement(interp, tok)) {
                decl = 0;
            }
        } break;
        default: {
            report_error_tok(interp, tok, "Expected := or :: or : in declaration");
        } break;
    }
    
    
    
    return decl;
}

AST *parse_decl(Interp *interp) {
    AST *decl = 0;
    Token *tok = peek_tok(interp->tokenizer);
    if (expect_tok(interp, tok, TOKEN_IDENT)) {
        AST *ident = create_ident(interp, tok->value_str);
        eat_tok(interp->tokenizer);
        decl = parse_decl_ident(interp, ident);
    }
    
    return decl;
}

AST *parse_toplevel_item(Interp *interp) {
    Token *tok = peek_tok(interp->tokenizer);
    if (tok->kind == TOKEN_EOS) {
        return 0;
    }
    
    AST *item = parse_decl(interp);
    return item;
}

Interp create_interp(const char *filename) {
    Interp interp = {0};
    interp.file_id = get_file_id_for_filename(filename);
    interp.tokenizer = create_tokenizer(interp.file_id);
    return interp;
}

void do_interp(Interp *interp) {
    for (;;) {
        AST *toplevel = parse_toplevel_item(interp);
        if (!toplevel) {
            break;
        }
        
        if (interp->reported_error) {
            break;
        }
        char buffer[4096] = {0};
        FmtBuffer buf = create_fmt_buf(buffer, sizeof(buffer));
        fmt_ast_tree_recursive(&buf, toplevel, 0);
        outf(buffer);
    }
}

