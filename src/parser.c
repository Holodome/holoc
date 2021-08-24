#include "parser.h"

#include "strings.h"
#include "tokenizer.h"
#include "ast.h"

static void report_error(Token *token, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    // @TODO capture source location
    voutf(msg, args);
    va_end(args);
}

static void report_unexpected_token(Token *token, u32 expected) {
    report_error(token, "%s:%u:%u Token %u expected (got %u)\n",
        token->source_loc.source_name, token->source_loc.line, token->source_loc.symb,
        expected, token->kind);
}

static b32 expect_tok(Token *token, u32 kind) {
    b32 result = TRUE;
    if (token->kind != kind) {
        report_unexpected_token(token, kind);
        result = FALSE;
    }
    return result;
}

static AST *ast_new(Parser *parser, u32 kind) {
    AST *ast = arena_alloc_struct(&parser->arena, AST);
    ast->kind = kind;
    // @TODO Do we want to reallocate source location here?
    ast->source_loc = peek_tok(parser->tokenizer)->source_loc;
    return ast;
}

static AST *create_ident(Parser *parser, const char *name) {
    AST *ident = ast_new(parser, AST_IDENT);
    ident->ident.name = arena_alloc_str(&parser->arena, name);
    return ident;
}

static AST *create_int_lit(Parser *parser, i64 value) {
    AST *literal = ast_new(parser, AST_LITERAL);
    literal->literal.kind = AST_LITERAL_INT;
    literal->literal.value_int = value;
    return literal;
}

Parser create_parser(struct Tokenizer *tokenizer) {
    Parser result = {0};
    result.tokenizer = tokenizer;
    return result;
}

AST *parse_expr2(Parser *parser);
AST *parse_expr1(Parser *parser);
AST *parse_expr(Parser *parser);

AST *parse_expr2(Parser *parser) {
    AST *expr = 0;
    Token *token = peek_tok(parser->tokenizer);
    switch (token->kind) {
        case TOKEN_INT: {
            eat_tok(parser->tokenizer);
            AST *lit = create_int_lit(parser, token->value_int);
            expr = lit;
        } break;
        case TOKEN_IDENT: {
            eat_tok(parser->tokenizer);
            AST *ident = create_ident(parser, token->value_str);
            expr = ident;
        } break;
        case '(': {
            eat_tok(parser->tokenizer);
            AST *temp = parse_expr(parser);
            token = peek_tok(parser->tokenizer);
            if (expect_tok(token, ')')) {
                expr = temp;
                eat_tok(parser->tokenizer);
            }
        } break;
        case '+': {
            eat_tok(parser->tokenizer);
            AST *unary = ast_new(parser, AST_UNARY);
            unary->unary.kind = AST_UNARY_PLUS;
            unary->unary.expr = parse_expr(parser);
            expr = unary;
        } break;
        case '-': {
            eat_tok(parser->tokenizer);
            AST *unary = ast_new(parser, AST_UNARY);
            unary->unary.kind = AST_UNARY_MINUS;
            unary->unary.expr = parse_expr(parser);
            expr = unary;
        } break;
        default: {
            // @TODO find way to provide correct tokens
            report_unexpected_token(token, TOKEN_NONE);
        } 
    }
    return expr;
}

AST *parse_expr1(Parser *parser) {
    // If expression is not binary * or /, just pass the value. Otherwise it becomes left-hand value
    AST *expr = parse_expr2(parser);
    Token *token = peek_tok(parser->tokenizer);
    while (token->kind == '*' || token->kind == '/') {
        AST *binary = ast_new(parser, AST_BINARY);
        
        if (token->kind == '*') {
            binary->binary.kind = AST_BINARY_MUL;
        } else if (token->kind == '/') {
            binary->binary.kind = AST_BINARY_DIV;
        }
        eat_tok(parser->tokenizer);
        AST *right = parse_expr2(parser);    
        
        binary->binary.left = expr;
        binary->binary.right = right;
        expr = binary;
        // For next cycle
        token = peek_tok(parser->tokenizer);
    }
    return expr;
}

AST *parse_expr(Parser *parser) {
    AST *expr = parse_expr1(parser);
    Token *token = peek_tok(parser->tokenizer);
    while (token->kind == '+' || token->kind == '-') {
        AST *binary = ast_new(parser, AST_BINARY);
        
        if (token->kind == '+') {
            binary->binary.kind = AST_BINARY_ADD;
        } else if (token->kind == '-') {
            binary->binary.kind = AST_BINARY_SUB;
        }
        eat_tok(parser->tokenizer);
        AST *right = parse_expr1(parser);    
        
        binary->binary.left = expr;
        binary->binary.right = right;
        expr = binary;
        // For next cycle
        token = peek_tok(parser->tokenizer);
    }
    return expr;
}

AST *parse_assign(Parser *parser) {
    AST *assign = 0;
    Token *token = peek_tok(parser->tokenizer);
    if (expect_tok(token, TOKEN_IDENT)) {
        AST *ident = create_ident(parser, token->value_str);
        
        token = peek_next_tok(parser->tokenizer);
        if (expect_tok(token, '=')) {
            token = peek_next_tok(parser->tokenizer);
            AST *expr = parse_expr(parser);
            
            assign = ast_new(parser, AST_ASSIGN);
            assign->assign.ident = ident;
            assign->assign.expr = expr;
        }  
    } 
    return assign;
}

AST *parse_statement(Parser *parser) {
    AST *statement = 0;
    Token *token = peek_tok(parser->tokenizer);
    if (token->kind == TOKEN_IDENT) {
        statement = parse_assign(parser);
    } else {
        report_unexpected_token(token, TOKEN_IDENT);
    }
    token =  peek_tok(parser->tokenizer);
    if (expect_tok(token, ';')) {
        eat_tok(parser->tokenizer);
    }
    return statement;
}

struct AST *parse(Parser *parser) {
    AST *result = ast_new(parser, AST_BLOCK);
    for (;;) {
        Token *token = peek_tok(parser->tokenizer);
        if (token->kind == TOKEN_EOS) {
            break;
        }
        
        AST *statement = parse_statement(parser);
        if (statement) {
            LLIST_ADD(result->block.first_statement, statement);
        }
    }
    return result;    
}