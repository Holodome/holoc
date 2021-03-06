#if !defined(INTERP_HH)

#include "lexer.hh"
#include "ast.hh"

struct Object {
    i64 value;
};  

struct Parser {
    Lexer *lexer;
    
    Parser(Lexer *lexer) 
        : lexer(lexer) {
    }
    
    AstLiteral *make_literal(i64 value) {
        AstLiteral *lit = new AstLiteral;
        lit->lit_kind = AstLiteralKind::Integer;
        lit->integer_lit = value;
        return lit;
    }
    AstIdent *make_ident(const Str &name) {
        AstIdent *ident = new AstIdent;
        ident->name = name;
        return ident;
    }
    
    AstExpression *parse_subexpr() {
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
                    lexer->error("')' Expected in parse_subexpr");
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
                lexer->error(tok, "Unexpected token (parse_subexpr)");  
            };
        }
        
        return expr;
    }
    
    AstExpression *parse_expr_helper() {
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
    
    AstExpression *parse_expr() {
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
    
    AstAssign *parse_assign() {
        Token *tok = lexer->peek_tok();
        if (tok->kind != TokenKind::Identifier) {
            return 0;
        }
        lexer->eat_tok();
        AstAssign *assign = new AstAssign;
        assign->ident = make_ident(tok->ident);
        tok = lexer->peek_tok();
        if (tok->kind != (TokenKind)'=') {
            lexer->error(tok, "Expected '=' in assignment");
        }
        lexer->eat_tok();
        assign->expr = parse_expr();
        tok = lexer->peek_tok();
        if (tok->kind != (TokenKind)';') {
            lexer->error(tok, "';' expected");
        }
        lexer->eat_tok();
        return assign;
    }
    
    AstBlock *parse() {
        AstBlock *global_block = new AstBlock;
        for (;;) {
            Token *tok = lexer->peek_tok();
            if (tok->kind == TokenKind::EOS) {
                break;
            }
        
            AstAssign *assign = parse_assign();
            global_block->expressions.add((AstExpression *)assign);
            if (!assign) {
                lexer->error(lexer->peek_tok(), "Unknown syntax");
            }
        }
        
        return global_block;
    }
};

struct Interp {
    Array<Str> var_names;
    Array<Object> objects;
    
    Object *get_object(const Str &name) {
        Object *result = 0;
        for (size_t i = 0; i < var_names.len; ++i) {
            if (var_names[i] == name) {
                result = &objects[i];
            }
        }
        return result;
    }
    
    i64 evaluate_expression(AstExpression *expr) {
        switch (expr->kind) {
            case AstKind::Binary: {
                AstBinary *bin = (AstBinary *)expr;
                switch(bin->bin_kind) {
                    case AstBinaryKind::Add: {
                        return evaluate_expression(bin->left) + evaluate_expression(bin->right);
                    } break;
                    case AstBinaryKind::Sub: {
                        return evaluate_expression(bin->left) - evaluate_expression(bin->right);
                    } break;
                    case AstBinaryKind::Mul: {
                        return evaluate_expression(bin->left) * evaluate_expression(bin->right);
                    } break;
                    case AstBinaryKind::Div: {
                        return evaluate_expression(bin->left) / evaluate_expression(bin->right);
                    } break;
                    default: {
                        assert(false);
                    };
                }  
            } break;
            case AstKind::Unary: {
                AstUnary *unary = (AstUnary *)expr;
                switch (unary->un_kind) {
                    case AstUnaryKind::Plus: {
                        return -evaluate_expression(unary->expr);
                    } break;
                    case AstUnaryKind::Minus: {
                        return evaluate_expression(unary->expr);
                    } break;
                    default: {
                        assert(false);
                    };
                }
            } break;
            case AstKind::Literal: {
                AstLiteral *lit = (AstLiteral *)expr;
                switch (lit->lit_kind) {
                    case AstLiteralKind::Integer: {
                        return lit->integer_lit;
                    } break;
                    default: {
                        assert(false);  
                    };
                }
            } break;
            case AstKind::Ident: {
                AstIdent *ident = (AstIdent *)expr;
                Object *object = get_object(ident->name);
                if (object) {
                    return object->value;
                } else {
                    printf("'%.*s': undeclared identifier", ident->name.len, ident->name.data);
                    assert(false);
                }
            } break;
            default: {
                assert(false);
            };
        }
		assert(false);
		return 0;
    }
    
    Interp(AstBlock *global_block) {
        for (size_t i = 0; i < global_block->expressions.len; ++i) {
            AstExpression *expr = global_block->expressions[i];
            assert(expr->kind == AstKind::Assign);
            AstAssign *assign = (AstAssign *)expr;
            
            Object *obj = get_object(assign->ident->name);
            if (!obj) {
                var_names.add(assign->ident->name);
                Object object_;
                objects.add(object_);
                obj = get_object(assign->ident->name);
            }
            assert(obj);

			obj->value = evaluate_expression(assign->expr);
        }
        
        for (size_t i = 0; i < var_names.len; ++i) {
            printf("%.*s: %lli\n", var_names[i].len, var_names[i].data, objects[i].value);
        }
    }
};

#define INTERP_HH 1
#endif
