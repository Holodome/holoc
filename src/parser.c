#include "parser.h"

Parser *create_parser(Lexer *lexer, StringStorage *ss, ErrorReporter *er) {
    Parser *parser = arena_bootstrap(Parser, arena);
    parser->er = er;
    parser->ss = ss;
    parser->lexer = lexer;
    return parser;
}

void destroy_parser(Parser *parser) {
    arena_clear(&parser->arena);
}

static void report_unexpected_token(Parser *parser, Token *tok, u32 expected) {
    char expected_str[128], got_str[128];
    fmt_tok_kind(expected_str, sizeof(expected_str), expected);
    fmt_tok_kind(got_str, sizeof(got_str), tok->kind);
    report_error_tok(parser->er, tok, "Token %s expected (got %s %#x)",
        expected_str, got_str, tok->kind);
}

static b32 expect_tok(Parser *parser, Token *tok, u32 kind) {
    b32 result = TRUE;
    if (tok->kind != kind) {
        report_unexpected_token(parser, tok, kind);
        result = FALSE;
    }
    return result;
}

static b32 parse_end_of_statement(Parser *parser, Token *tok) {
    b32 result = expect_tok(parser, tok, ';');
    if (result) {
        eat_tok(parser->lexer);
    }
    return result;
}

static AST *ast_new(Parser *parser, u32 kind) {
    AST *ast = arena_alloc_struct(&parser->arena, AST);
    ast->kind = kind;
    // @TODO more explicit way of taking source location?
    ast->src_loc = peek_tok(parser->lexer)->src_loc;
    return ast;
}

// @TODO(hl): @LEAK: We can optimize memory usage if use hash table for string ids
static AST *create_ident(Parser *parser, StringID id) {
    AST *ident = ast_new(parser, AST_IDENT);
    ident->ident.name = id;
    return ident;
}

static AST *create_int_lit(Parser *parser, i64 value) {
    AST *literal = ast_new(parser, AST_LIT);
    literal->lit.kind = AST_LIT_INT;
    literal->lit.value_int = value;
    return literal;
}

ASTList parse_comma_separated_idents(Parser *parser) {
    ASTList idents = create_ast_list(ast_new(parser, AST_NONE));
    Token *tok = peek_tok(parser->lexer);
    while (tok->kind == TOKEN_IDENT) {
        AST *ident = create_ident(parser, tok->value_str);
        ast_list_add(&idents, ident);
        tok = peek_next_tok(parser->lexer);
        if (tok->kind == ',') {
            tok = peek_next_tok(parser->lexer);
        }
    }
    return idents;
}

AST *parse_decl(Parser *parser);
AST *parse_decl_ident(Parser *parser, AST *ident, b32 end_of_statement);
AST *parse_block(Parser *parser);

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

AST *parse_expr_precedence(Parser *parser, u32 precedence);
static AST *parse_binary_subxepr(Parser *parser, u32 prec, u32 bin_kind, AST *left) {
    AST *result = 0;
    AST *right = parse_expr_precedence(parser, prec - 1);
    if (right) {
        AST *bin = ast_new(parser, AST_BINARY);
        bin->binary.kind = bin_kind;
        bin->binary.left = left;
        bin->binary.right = right; 
        result = bin;
    } else {
        report_error_tok(parser->er, peek_tok(parser->lexer), "Expected expression");
    }
    return result; 
}

// Parse expression with ascending precedence - lower ones are parsed earlier
// Currently this is made by making 
// @TODO remove recursion for precedence iteration
#define parse_expr(_interp) parse_expr_precedence(_interp, MAX_OPEARTOR_PRECEDENCE)
AST *parse_expr_precedence(Parser *parser, u32 precedence) {
    AST *expr = 0;
    switch (precedence) {
        case 0: {
            Token *tok = peek_tok(parser->lexer);
            switch (tok->kind) {
                case TOKEN_IDENT: {
                    eat_tok(parser->lexer);
                    AST *ident = create_ident(parser, tok->value_str);
                    expr = ident;
                    tok = peek_tok(parser->lexer);
                    // @TODO What if some mangling done to function name???
                    // finder better place for function call
                    if (tok->kind == '(') {
                        eat_tok(parser->lexer);
                        AST *function_call = ast_new(parser, AST_FUNC_CALL);
                        function_call->func_call.callable = ident;
                        ASTList arguments = parse_comma_separated_idents(parser);
                        if (expect_tok(parser, peek_tok(parser->lexer), ')')) {
                            eat_tok(parser->lexer);
                            expr = function_call;
                        }
                        function_call->func_call.arguments = arguments;
                    }
                } break;
                case TOKEN_INT: {
                    eat_tok(parser->lexer);
                    AST *lit = create_int_lit(parser, tok->value_int);
                    expr = lit;
                } break;
                case TOKEN_REAL: {
                    NOT_IMPLEMENTED;
                } break;
            }
        } break;
        case 1: {
            expr = parse_expr_precedence(parser, precedence - 1);
            Token *tok = peek_tok(parser->lexer);
            switch (tok->kind) {
                case '(': {
                    eat_tok(parser->lexer);
                    AST *temp = parse_expr(parser);
                    tok = peek_tok(parser->lexer);
                    if (expect_tok(parser, tok, ')')) {
                        expr = temp;
                        eat_tok(parser->lexer);
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
            Token *tok = peek_tok(parser->lexer);
            switch (tok->kind) {
                case '+': {
                    eat_tok(parser->lexer);
                    AST *unary = ast_new(parser, AST_UNARY);
                    unary->unary.kind = AST_UNARY_PLUS;
                    unary->unary.expr = parse_expr(parser);
                    expr = unary;
                } break;
                case '-': {
                    eat_tok(parser->lexer);
                    AST *unary = ast_new(parser, AST_UNARY);
                    unary->unary.kind = AST_UNARY_MINUS;
                    unary->unary.expr = parse_expr(parser);
                    expr = unary;
                } break;
                case '!': {
                    eat_tok(parser->lexer);
                    AST *unary = ast_new(parser, AST_UNARY);
                    unary->unary.kind = AST_UNARY_LOGICAL_NOT;
                    unary->unary.expr = parse_expr(parser);
                    expr = unary;
                } break;
                case '~': {
                    eat_tok(parser->lexer);
                    AST *unary = ast_new(parser, AST_UNARY);
                    unary->unary.kind = AST_UNARY_NOT;
                    unary->unary.expr = parse_expr(parser);
                    expr = unary;
                } break;
            }
            
            if (!expr) {
                expr = parse_expr_precedence(parser, precedence - 1);
            }
        } break;
        case 3: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == '*' || tok->kind == '/' || tok->kind == '%') {
                u32 bin_kind = 0;
                switch (tok->kind) {
                    case '*': {
                        bin_kind = AST_BINARY_MUL;
                    } break;
                    case '/': {
                        bin_kind = AST_BINARY_DIV;
                    } break;
                    case '%': {
                        bin_kind = AST_BINARY_MOD;
                    } break;
                }
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 4: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == '+' || tok->kind == '-') {
                u32 bin_kind = 0;
                if (tok->kind == '+') {
                    bin_kind = AST_BINARY_ADD;
                } else if (tok->kind == '-') {
                    bin_kind = AST_BINARY_SUB;
                }
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 5: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == TOKEN_LSHIFT || tok->kind == TOKEN_RSHIFT) {
                u32 bin_kind = 0;
                if (tok->kind == TOKEN_RSHIFT) {
                    bin_kind = AST_BINARY_RSHIFT;
                } else if (tok->kind == TOKEN_LSHIFT) {
                    bin_kind = AST_BINARY_SUB;
                }
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 6: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == '<' || tok->kind == '>' || tok->kind == TOKEN_LE || tok->kind == TOKEN_GE) {
                u32 bin_kind = 0;
                if (tok->kind == TOKEN_LE) {
                    bin_kind = AST_BINARY_LE;
                } else if (tok->kind == TOKEN_GE) {
                    bin_kind = AST_BINARY_GE;
                } else if (tok->kind == '<') {
                    bin_kind = AST_BINARY_L;
                } else if (tok->kind == '>') {
                    bin_kind = AST_BINARY_G;
                }
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 7: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == TOKEN_EQ || tok->kind == TOKEN_NEQ) {
                u32 bin_kind = 0;
                if (tok->kind == TOKEN_EQ) {
                    bin_kind = AST_BINARY_EQ;
                } else if (tok->kind == TOKEN_NEQ) {
                    bin_kind = AST_BINARY_NEQ;
                }
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 8: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == '&') {
                u32 bin_kind = AST_BINARY_AND;
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 9: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == '&') {
                u32 bin_kind = AST_BINARY_XOR;
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 10: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == '&') {
                u32 bin_kind = AST_BINARY_OR;
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 11: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == TOKEN_LOGICAL_AND) {
                u32 bin_kind = AST_BINARY_LOGICAL_AND;
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        case 12: {
            expr = parse_expr_precedence(parser, precedence - 1);
            if (!expr) {
                goto end;
            }
            Token *tok = peek_tok(parser->lexer);
            while (tok->kind == TOKEN_LOGICAL_OR) {
                u32 bin_kind = AST_BINARY_LOGICAL_OR;
                eat_tok(parser->lexer);
                expr = parse_binary_subxepr(parser, precedence, bin_kind, expr);
                tok = peek_tok(parser->lexer);
            }
        } break;
        default: assert(FALSE);
    }
end:
    return expr;
}

static AST *parse_type(Parser *parser) {
    AST *type = 0;
    Token *tok = peek_tok(parser->lexer);
    if (tok->kind == TOKEN_KW_INT) {
        type = ast_new(parser, AST_TYPE);
        type->type.kind = AST_TYPE_INT;
        eat_tok(parser->lexer);
    } else if (tok->kind == TOKEN_KW_FLOAT) {
        type = ast_new(parser, AST_TYPE);
        type->type.kind = AST_TYPE_FLOAT;
        eat_tok(parser->lexer);
    } else {
        
    }
    return type;
}

static u32 assign_token_to_binary_kind(u32 tok) {
    u32 result = 0;
    // @TODO This can be optimized if we structure the ast binary enum 
    // to have kinds in the same order as corresponding token kinds
    switch (tok) {
        case TOKEN_IADD: {
            result = AST_BINARY_ADD;
        } break;
        case TOKEN_ISUB: {
            result = AST_BINARY_SUB;
        } break;
        case TOKEN_IMUL: {
            result = AST_BINARY_MUL;
        } break;
        case TOKEN_IDIV: {
            result = AST_BINARY_DIV;
        } break;
        case TOKEN_IMOD: {
            result = AST_BINARY_MOD;
        } break;
        case TOKEN_IAND: {
            result = AST_BINARY_AND;
        } break;
        case TOKEN_IOR: {
            result = AST_BINARY_OR;
        } break;
        case TOKEN_IXOR: {
            result = AST_BINARY_XOR;
        } break;
        case TOKEN_ILSHIFT: {
            result = AST_BINARY_LSHIFT;
        } break;
        case TOKEN_IRSHIFT: {
            result = AST_BINARY_RSHIFT;
        } break;
    }
    return result;
}

AST *parse_assign_ident(Parser *parser, AST *ident) {
    AST *assign = 0;
    Token *tok = peek_tok(parser->lexer);
    if (!is_token_assign(tok->kind)) {
        report_error_tok(parser->er, tok, "Expected assign operator");
        return 0;    
    }
    
    // All incorrect tokens should be handled before
    eat_tok(parser->lexer);
    // @TODO make this more readable...
    if (tok->kind == '=') {
        AST *expr = parse_expr(parser);
        
        assign = ast_new(parser, AST_ASSIGN);
        assign->assign.lvalue = ident;
        assign->assign.rvalue = expr;
    } else {
        u32 binary_kind = assign_token_to_binary_kind(tok->kind);
        assert(binary_kind);
        AST *binary = ast_new(parser, AST_BINARY);
        binary->binary.kind = binary_kind;
        binary->binary.left = ident;
        binary->binary.right = parse_expr(parser);
        assign = ast_new(parser, AST_ASSIGN);
        assign->assign.lvalue = ident;
        assign->assign.rvalue = binary;
    }
    
    tok = peek_tok(parser->lexer);
    if (!parse_end_of_statement(parser, tok)) {
        assign = 0;
    }
    return assign;
}

AST *parse_if_compound(Parser *parser) {
    Token *tok = peek_tok(parser->lexer);
    if (tok->kind != TOKEN_KW_IF) {
        return 0;
    }
    
    tok = peek_next_tok(parser->lexer);
    AST *expr = parse_expr(parser);
    if (!expr) {
        return 0;
    }
    
    AST *if_st = ast_new(parser, AST_IF);
    if_st->ifs.cond = expr;
    tok = peek_tok(parser->lexer);
    if (expect_tok(parser, tok, '{')) {
        AST *block = parse_block(parser);
        if (!block) {
            return 0;
        }
        if_st->ifs.block = block;
        tok = peek_tok(parser->lexer);
        if (tok->kind == TOKEN_KW_ELSE) {
            tok = peek_next_tok(parser->lexer);
            if (tok->kind == TOKEN_KW_IF) {
                AST *else_if_st = parse_if_compound(parser);
                if_st->ifs.else_block = else_if_st;
            } else if (tok->kind == '{') {
                AST *else_block = parse_block(parser);
                if_st->ifs.else_block = else_block;
            }
        }
    }
    return if_st;
}

AST *parse_statement(Parser *parser) {
    AST *statement = 0;
    Token *tok = peek_tok(parser->lexer);
    // Common path for assign and decl
    // @TODO maybe unite assign and declaration
    AST *ident = 0;
    if (tok->kind == TOKEN_IDENT) {
        ident = create_ident(parser, tok->value_str);
        tok = peek_next_tok(parser->lexer);
    } 
    
    if (is_token_assign(tok->kind)) {
        statement = parse_assign_ident(parser, ident);
    } else if (tok->kind == TOKEN_KW_RETURN) {
        tok = peek_next_tok(parser->lexer);
        ASTList return_vars = create_ast_list(ast_new(parser, AST_NONE));
        while (tok->kind != ';') {
            AST *expr = parse_expr(parser);
            if (!expr || is_error_reported(parser->er)) {
                break;
            }
            ast_list_add(&return_vars, expr);
            
            tok = peek_tok(parser->lexer);
            if (tok->kind == ',') {
                tok = peek_next_tok(parser->lexer);
            }
        }
        
        if (parse_end_of_statement(parser, tok)) {
            statement = ast_new(parser, AST_RETURN);
            statement->returns.vars = return_vars;
        }
    } else if (tok->kind == TOKEN_KW_IF) {
        AST *if_st = parse_if_compound(parser);
        statement = if_st;
    } else if (tok->kind == TOKEN_KW_WHILE) {
        eat_tok(parser->lexer);
        AST *condition = parse_expr(parser);
        if (condition) {
            AST *block = parse_block(parser);
            if (block) {
                AST *while_st = ast_new(parser, AST_WHILE);
                while_st->whiles.cond = condition;
                while_st->whiles.block = block;
                
                statement = while_st;
            }
        }
    } else if (tok->kind == TOKEN_KW_PRINT) {
        eat_tok(parser->lexer);
        AST *expr = parse_expr(parser);
        ASTList exprs = create_ast_list(ast_new(parser, AST_NONE));
        ast_list_add(&exprs, expr);
        tok = peek_tok(parser->lexer);
        if (parse_end_of_statement(parser, tok)) {
            AST *print_st = ast_new(parser, AST_PRINT);
            print_st->prints.arguments = exprs;
            statement = print_st;
        }
    } else {
        statement = parse_decl_ident(parser, ident, TRUE);
    }
    
    return statement;
}

AST *parse_block(Parser *parser) {
    AST *block = 0;
    Token *tok = peek_tok(parser->lexer);
    if (expect_tok(parser, tok, '{')) {
        tok = peek_next_tok(parser->lexer);     
    } else {
        return 0;
    }
    
    ASTList statements = create_ast_list(ast_new(parser, AST_NONE));
    while (peek_tok(parser->lexer)->kind != '}') {
        AST *statement = parse_statement(parser);
        if (!statement) {
            break;
        }
        
        if (is_error_reported(parser->er)) {
            break;
        }
        ast_list_add(&statements, statement);
    }
    block = ast_new(parser, AST_BLOCK);
    block->block.statements = statements;
    
    tok = peek_tok(parser->lexer);
    if (expect_tok(parser, tok, '}')) {
        tok = peek_next_tok(parser->lexer);     
    } else {
        return 0;
    }
    return block;
}

AST *parse_function_signature(Parser *parser) {
    Token *tok = peek_tok(parser->lexer);
    if (!expect_tok(parser, tok, '(')) {
        return 0;
    }
    tok = peek_next_tok(parser->lexer);
    // Parse arguments
    ASTList args = create_ast_list(ast_new(parser, AST_NONE));
    while (tok->kind != ')') {
        if (expect_tok(parser, tok, TOKEN_IDENT)) {
            AST *ident = create_ident(parser, tok->value_str);
            eat_tok(parser->lexer);
            AST *decl = parse_decl_ident(parser, ident, FALSE);
            ast_list_add(&args, ident);
            tok = peek_tok(parser->lexer);
        } else {
            break;
        }
    }
    if (is_error_reported(parser->er)) {
        return 0;
    }
    
    AST *sign = ast_new(parser, AST_FUNC_SIGNATURE);
    sign->func_sign.arguments = args;
    
    tok = peek_next_tok(parser->lexer);
    ASTList return_types = create_ast_list(ast_new(parser, AST_NONE));
    if (tok->kind == TOKEN_ARROW) {
        tok = peek_next_tok(parser->lexer);
        
        Token *tok = peek_tok(parser->lexer);
        for (;;) {
            AST *type = parse_type(parser);
            if (type) {
                ast_list_add(&return_types, type);
                tok = peek_tok(parser->lexer);
                if (tok->kind == ',') {
                    tok = peek_next_tok(parser->lexer);
                }
            } else {
                break;
            }
        }
    }
    sign->func_sign.return_types = return_types;
    return sign;
}

AST *parse_decl_ident(Parser *parser, AST *ident, b32 end_of_statement) {
    AST *decl = 0;
    Token *tok = peek_tok(parser->lexer);
    switch (tok->kind) {
        case TOKEN_AUTO_DECL: {
            tok = peek_next_tok(parser->lexer);
            if (tok->kind == '(') {
                AST *func_sign = parse_function_signature(parser);
                if (func_sign) {
                    AST *func_block = parse_block(parser);
                    if (func_block) {
                        decl = ast_new(parser, AST_FUNC_DECL);
                        decl->func_decl.name = ident;
                        decl->func_decl.sign = func_sign;
                        decl->func_decl.block = func_block;
                    }
                }
            } else {
                AST *expr = parse_expr(parser);
                if (expr) {
                    decl = ast_new(parser, AST_DECL);
                    decl->decl.ident = ident;
                    decl->decl.expr = expr;
                    decl->decl.is_immutable = FALSE;
                    
                    tok = peek_tok(parser->lexer);
                    if (end_of_statement && !parse_end_of_statement(parser, tok)) {
                        decl = 0;
                    }
                }
            }
        } break;
        case ':': {
            eat_tok(parser->lexer);
            AST *type = parse_type(parser);
            assert(type);
            decl = ast_new(parser, AST_DECL);
            decl->decl.ident = ident; 
            decl->decl.type = type;  
            tok = peek_tok(parser->lexer);
            if (tok->kind == '=') {
                tok = peek_next_tok(parser->lexer);
                AST *expr = parse_expr(parser);
                decl->decl.expr = expr;
            } 
            
            tok = peek_tok(parser->lexer);
            if (end_of_statement && !parse_end_of_statement(parser, tok)) {
                decl = 0;
            }
        } break;
        default: {
            report_error_tok(parser->er, tok, "Expected := or :: or : in declaration");
        } break;
    }
    
    return decl;
}

AST *parse_decl(Parser *parser) {
    AST *decl = 0;
    Token *tok = peek_tok(parser->lexer);
    if (expect_tok(parser, tok, TOKEN_IDENT)) {
        AST *ident = create_ident(parser, tok->value_str);
        eat_tok(parser->lexer);
        decl = parse_decl_ident(parser, ident, TRUE);
    }
    
    return decl;
}

AST *parser_parse_toplevel(Parser *parser) {
    Token *tok = peek_tok(parser->lexer);
    if (tok->kind == TOKEN_EOS) {
        return 0;
    }
    
    AST *item = parse_decl(parser);
    return item;
}
