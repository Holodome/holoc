#if !defined(PARSER_HH)

#include "general.hh"
#include "lexer.hh"
#include "ast.hh"

struct Interp;

// Parser is responsible for turning tokenized input in ast input
struct Parser {
    Lexer *lexer;
    Interp *interp;
    
    // Used to make new asts
    Ast *copy_source_location(Ast *ast);
    template <typename T> 
    T *ast_new() {
       T *value = new T;
       copy_source_location((Ast *)value);
       return value; 
    }
    
    AstLiteral *make_literal(i64 value);
    AstIdent *make_ident(const Str &name);
    
    AstExpression *parse_subexpr();
    AstExpression *parse_expr_helper();
    AstExpression *parse_expr();
    AstAssign *parse_assign();
    
    AstBlock *parse();
    
    bool expect_and_eat(TokenKind token_kind);
};

#define PARSER_HH 1
#endif
