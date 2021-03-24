#if !defined(RUNTIME_HH)

#include "general.hh"
#include "array.hh"
#include "str.hh"

struct Object {
    i64 value;
};  
 
struct Runtime {
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

};

#define RUNTIME_HH 1
#endif
