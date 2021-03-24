#include "parser.hh"

#include "interp.hh"

Ast *Parser::copy_source_location(Ast *ast) {
    ast->line_number = lexer->current_line_number;
    ast->char_number = lexer->current_char_number;
    ast->filename = interp->filename.data;
    return ast;
}

bool Parser::expect_and_eat(TokenKind token_kind) {
    bool result;
    Token *token = lexer->peek_tok();
    if (token->kind != token_kind) {
        interp->report_error(token, "Invalid token. %s expected", token_kind_to_str(token_kind));
        result = false;
    } else {
        lexer->eat_tok();
        result = true;
    }
    return result;
}

AstLiteral *Parser::make_literal(i64 value) {
    AstLiteral *lit = ast_new<AstLiteral>();
    lit->lit_kind = AstLiteralKind::Integer;
    lit->integer_lit = value;
    return lit;
}

AstIdent *Parser::make_ident(const Str &name) {
    AstIdent *ident = ast_new<AstIdent>();
    ident->name = name;
    return ident;
}

AstExpression *Parser::parse_subexpr() {
    AstExpression *expr = 0;
    Token *tok = lexer->peek_tok();
    lexer->eat_tok();
    switch(tok->kind) {
        case TokenKind::Integer: {
            AstLiteral *lit = make_literal(tok->integer);
            expr = (AstExpression *)lit;
        } break;
        case TokenKind::Identifier: {
            AstIdent *ident = make_ident(tok->ident);
            expr = (AstExpression *)ident;
        } break;
        case (TokenKind)'(': {
            expr = parse_expr();
            if (lexer->peek_tok()->kind != (TokenKind)')') {
                interp->report_error("')' Expected in parse_subexpr");
            }
            lexer->eat_tok();
        } break;
        case (TokenKind)'+': {
            AstUnary *unary = new AstUnary;
            unary->un_kind = AstUnaryKind::Plus;
            unary->expr = parse_expr();
            expr = (AstExpression *)unary;
        } break;
        case (TokenKind)'-': {
            AstUnary *unary = new AstUnary;
            unary->un_kind = AstUnaryKind::Minus;
            unary->expr = parse_expr();
            expr = (AstExpression *)unary;
        } break;
        default: {
            interp->report_error(tok, "Unexpected token (parse_subexpr)");  
        };
    }
    
    return expr;
}

AstExpression *Parser::parse_expr_helper() {
    AstExpression *expr = 0;
    expr = parse_subexpr();
    Token *tok = lexer->peek_tok();
    while (tok->kind == (TokenKind)'*' || tok->kind == (TokenKind)'/') {
        AstBinary *bin = new AstBinary;
        bin->left = expr;
        
        if (tok->kind == (TokenKind)'*') {
            bin->bin_kind = AstBinaryKind::Mul;
        } else if (tok->kind == (TokenKind)'/') {
            bin->bin_kind = AstBinaryKind::Div;
        }
        lexer->eat_tok();
        
        AstExpression *right = parse_subexpr();
        bin->right = right;
        
        expr = bin;
        // For next cycle iteration
        tok = lexer->peek_tok();
    }

    return expr;
}

AstExpression *Parser::parse_expr() {
    AstExpression *expr = 0;
    expr = parse_expr_helper();
    Token *tok = lexer->peek_tok();
    while (tok->kind == (TokenKind)'+' || tok->kind == (TokenKind)'-') {
        AstBinary *bin = new AstBinary;
        bin->left = expr;
        
        if (tok->kind == (TokenKind)'+') {
            bin->bin_kind = AstBinaryKind::Add;
        } else if (tok->kind == (TokenKind)'-') {
            bin->bin_kind = AstBinaryKind::Sub;
        }
        lexer->eat_tok();
        
        AstExpression *right = parse_expr_helper();
        bin->right = right;
        
        expr = bin;
        // For next cycle iteration
        tok = lexer->peek_tok();
    }
    
    return expr;
}

AstAssign *Parser::parse_assign() {
    Token *tok = lexer->peek_tok();
    if (tok->kind != TokenKind::Identifier) {
        return 0;
    }
    lexer->eat_tok();
    AstAssign *assign = new AstAssign;
    assign->ident = make_ident(tok->ident);
    tok = lexer->peek_tok();
    if (tok->kind != (TokenKind)'=') {
        interp->report_error(tok, "Expected '=' in assignment");
    }
    lexer->eat_tok();
    assign->expr = parse_expr();
    tok = lexer->peek_tok();
    if (tok->kind != (TokenKind)';') {
        interp->report_error(tok, "';' expected");
    }
    lexer->eat_tok();
    return assign;
}

AstBlock *Parser::parse() {
    AstBlock *global_block = new AstBlock;
    for (;;) {
        Token *tok = lexer->peek_tok();
        if (tok->kind == TokenKind::EOS) {
            break;
        }
    
        AstAssign *assign = parse_assign();
        if (!assign) {
            interp->report_error("Unknown syntax");
            break;
        }
        global_block->expressions.add((AstExpression *)assign);
    }
    
    return global_block;
}
