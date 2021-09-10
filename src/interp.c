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

AST *parse_expr2(Interp *interp);
AST *parse_expr1(Interp *interp);
AST *parse_expr(Interp *interp);

AST *parse_expr2(Interp *interp) {
    AST *expr = 0;
    Token *tok = peek_tok(interp->tokenizer);
    switch (tok->kind) {
        case TOKEN_INT: {
            eat_tok(interp->tokenizer);
            AST *lit = create_int_lit(interp, tok->value_int);
            expr = lit;
        } break;
        case TOKEN_IDENT: {
            eat_tok(interp->tokenizer);
            AST *ident = create_ident(interp, tok->value_str);
            expr = ident;
        } break;
        case '(': {
            eat_tok(interp->tokenizer);
            AST *temp = parse_expr(interp);
            tok = peek_tok(interp->tokenizer);
            if (expect_tok(interp, tok, ')')) {
                expr = temp;
                eat_tok(interp->tokenizer);
            }
        } break;
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
        default: {
            // @TODO find way to provide correct tokens
            char tok_str[128];
            fmt_tok_kind(tok_str, sizeof(tok_str), tok->kind);
            report_error_tok(interp, tok, "Unexpected token %s in expression", tok_str);
        } 
    }
    return expr;
}

AST *parse_expr1(Interp *interp) {
    // If expression is not binary * or /, just pass the value. Otherwise it becomes left-hand value
    AST *expr = parse_expr2(interp);
    Token *tok = peek_tok(interp->tokenizer);
    while (tok->kind == '*' || tok->kind == '/') {
        AST *binary = ast_new(interp, AST_BINARY);
        
        if (tok->kind == '*') {
            binary->binary.kind = AST_BINARY_MUL;
        } else if (tok->kind == '/') {
            binary->binary.kind = AST_BINARY_DIV;
        }
        eat_tok(interp->tokenizer);
        AST *right = parse_expr2(interp);    
        
        binary->binary.left = expr;
        binary->binary.right = right;
        expr = binary;
        // For next cycle
        tok = peek_tok(interp->tokenizer);
    }
    return expr;
}

AST *parse_expr(Interp *interp) {
    AST *expr = parse_expr1(interp);
    Token *tok = peek_tok(interp->tokenizer);
    while (tok->kind == '+' || tok->kind == '-') {
        AST *binary = ast_new(interp, AST_BINARY);
        
        if (tok->kind == '+') {
            binary->binary.kind = AST_BINARY_ADD;
        } else if (tok->kind == '-') {
            binary->binary.kind = AST_BINARY_SUB;
        }
        eat_tok(interp->tokenizer);
        AST *right = parse_expr1(interp);    
        
        binary->binary.left = expr;
        binary->binary.right = right;
        expr = binary;
        // For next cycle
        tok = peek_tok(interp->tokenizer);
    }
    return expr;
}

AST *parse_assign(Interp *interp) {
    AST *assign = 0;
    Token *tok = peek_tok(interp->tokenizer);
    if (expect_tok(interp, tok, TOKEN_IDENT)) {
        AST *ident = create_ident(interp, tok->value_str);
        
        tok = peek_next_tok(interp->tokenizer);
        if (expect_tok(interp, tok, '=')) {
            tok = peek_next_tok(interp->tokenizer);
            AST *expr = parse_expr(interp);
            
            assign = ast_new(interp, AST_ASSIGN);
            assign->assign.ident = ident;
            assign->assign.expr = expr;
        }  
    } 
    return assign;
}

AST *parse_statement(Interp *interp) {
    AST *statement = 0;
    Token *tok = peek_tok(interp->tokenizer);
    if (tok->kind == TOKEN_IDENT) {
        statement = parse_assign(interp);
    } else {
        report_unexpected_token(interp, tok, TOKEN_IDENT);
    }
    tok = peek_tok(interp->tokenizer);
    if (expect_tok(interp, tok, ';')) {
        eat_tok(interp->tokenizer);
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
    for (;;) {
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
        
        AST *outs = 0;
        while (tok->kind == TOKEN_IDENT) {
            AST *ident = create_ident(interp, tok->value_str);
            LLIST_ADD(outs, ident);
            tok = peek_next_tok(interp->tokenizer);
            if (tok->kind == ',') {
                tok = peek_next_tok(interp->tokenizer);
            }
        }
        sign->func_sign.outs = outs;
    }
    return sign;
}

AST *parse_declaration(Interp *interp) {
    AST *decl = 0;
    Token *tok = peek_tok(interp->tokenizer);
    if (expect_tok(interp, tok, TOKEN_IDENT)) {
        AST *ident = create_ident(interp, tok->value_str);
        tok = peek_next_tok(interp->tokenizer);
        switch (tok->kind) {
            case TOKEN_AUTO_DECL: {
                tok = peek_next_tok(interp->tokenizer);
                AST *expr = parse_expr(interp);
                if (expr) {
                    decl = ast_new(interp, AST_DECL);
                    decl->decl.ident = ident;
                    decl->decl.expr = expr;
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
                    }
                }
            } break;
            default: {
                report_error_tok(interp, tok, "Expected := or :: in declaration");
            } break;
        }
    }
    
    tok = peek_tok(interp->tokenizer);
    if (!expect_tok(interp, tok, ';')) {
        return 0;
    } else {
        eat_tok(interp->tokenizer);
    }
    
    return decl;
}

AST *parse_toplevel_item(Interp *interp) {
    Token *tok = peek_tok(interp->tokenizer);
    if (tok->kind == TOKEN_EOS) {
        return 0;
    }
    
    AST *item = parse_declaration(interp);
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
    }
}

