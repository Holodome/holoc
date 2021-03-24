#if !defined(AST_HH)

#include "general.hh"
#include "str.hh"
#include "array.hh"

enum struct AstKind {
    None = 0x0,
    Block,
    Literal,
    Assign,
    Ident,
    Unary,
    Binary,
    Print,
    Count
};

enum struct AstLiteralKind {
    None = 0x0,
    Integer,
    Count
};  

enum struct AstUnaryKind {
    None = 0x0,
    Minus, 
    Plus,
    Count 
};  

enum struct AstBinaryKind {
    None = 0x0,
    Add,
    Sub,
    Mul,
    Div,
    Count
};

struct Ast;
struct AstExpression;
struct AstDeclaration;
struct AstBlock;
struct AstLiteral;
struct AstUnary;
struct AstBinary;
struct AstIdent;
struct AstTypeDefenition;

struct Ast {
    AstKind kind = AstKind::None;
    
    u32 line_number;
    u32 char_number;
    const char *filename;
};


struct AstExpression : public Ast {
    
};

struct AstBlock : public Ast {
    AstBlock() { kind = AstKind::Block; }
    AstBlock *parent = 0;
    Array<AstExpression *> expressions;
};

struct AstLiteral : public AstExpression {
    AstLiteral() { kind = AstKind::Literal; }
    AstLiteralKind lit_kind;
    
    Str string_lit;
    i64 integer_lit;    
};

struct AstIdent : public AstExpression {
    AstIdent() { kind = AstKind::Ident; }
    
    Str name;
};  

struct AstUnary : public AstExpression {
    AstUnary() { kind = AstKind::Unary; }
    
    AstUnaryKind un_kind = AstUnaryKind::None;
    AstExpression *expr = 0;
};

struct AstBinary : public AstExpression {
    AstBinary() { kind = AstKind::Binary; }
    
    AstBinaryKind bin_kind = AstBinaryKind::None;
    AstExpression *left = 0;
    AstExpression *right = 0;
};

struct AstAssign : public AstExpression {
    AstAssign() { kind = AstKind::Assign; }
    
    AstIdent *ident = 0;
    AstExpression *expr = 0;
};  

struct AstPrint : public AstExpression {
    AstPrint() { kind = AstKind::Print; }  
    
    
};

#define AST_HH 1
#endif
