#include "preprocessor.h"

#include "str.h"
#include "pp_lexer.h"
#include "hashing.h"
#include "allocator.h"
#include "bump_allocator.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

// FIXME: All copies from lexer->string_buf act as if it always was an 1-byte aligned array, which in reality it may be not

static preprocessor_macro **
get_macro(preprocessor *pp, uint32_t hash) {
    preprocessor_macro **macrop = hash_table_sc_get_u32(
        pp->macro_hash, sizeof(pp->macro_hash) / sizeof(*pp->macro_hash),
        preprocessor_macro, next, name_hash, 
        hash);
    return macrop;
}

static preprocessor_macro *
get_new_macro(preprocessor *pp) {
    preprocessor_macro *macro = pp->macro_freelist;
    if (macro) {
        pp->macro_freelist = macro->next;
        memset(macro, 0, sizeof(*macro));
    } else {
        macro = bump_alloc(pp->a, sizeof(*macro));
    }
    return macro;
}

static void 
free_macro_data(preprocessor *pp, preprocessor_macro *macro) {
}

static preprocessor_macro_arg *
get_new_macro_arg(preprocessor *pp) {
    preprocessor_macro_arg *arg = pp->macro_arg_freelist;
    if (arg) {
        pp->macro_arg_freelist = arg->next;
        memset(arg, 0, sizeof(*arg));
    } else {
        arg = bump_alloc(pp->a, sizeof(*arg));
    }
    return arg;
}

static preprocessor_token *
get_new_token(preprocessor *pp) {
    preprocessor_token *tok = pp->tok_freelist;
    if (tok) {
        pp->tok_freelist = tok->next;
        memset(tok, 0, sizeof(*tok));
    } else {
        tok = bump_alloc(pp->a, sizeof(*tok));
    }
    return tok;
}

static void 
define_macro(preprocessor *pp) {
    pp_lexer *lexer = pp->lexer;
    if (lexer->tok_kind != PP_TOK_ID) {
        // TODO: Diagnostic
        assert(false);
    }

    string ident_name = string(lexer->tok_buf, lexer->tok_buf_len);
    uint32_t hash = hash_string(ident_name);
    preprocessor_macro **macrop = get_macro(pp, hash);
    if (*macrop) {
        ident_name = (*macrop)->name;
        // Macro is already present
        // TODO: Diagnostic
        assert(false);
        // Free all args and tokens macro is holding
        free_macro_data(pp, *macrop);
        // Delete macro data but keep its next pointer for hash table
        preprocessor_macro *next = (*macrop)->next;
        memset(*macrop, 0, sizeof(**macrop));
        (*macrop)->next = next;
    } else {
        allocator a = bump_get_allocator(pp->a);
        ident_name = string_dup(&a, ident_name);

        preprocessor_macro *new_macro = get_new_macro(pp);
        new_macro->next = *macrop;
        *macrop = new_macro;
    }

    preprocessor_macro *macro = *macrop;
    assert(macro);
    
    macro->name = ident_name;
    macro->name_hash = hash;

    pp_lexer_parse(lexer);
    if (lexer->tok_kind == PP_TOK_PUNCT && 
        lexer->tok_punct_kind == '(' &&
        !lexer->tok_has_whitespace) {
        preprocessor_macro_arg *args = 0;

        pp_lexer_parse(lexer);
        // TODO: This condition can be checked inside the loop body
        while (lexer->tok_kind != PP_TOK_PUNCT || lexer->tok_punct_kind != ')') {
            if (lexer->tok_kind == PP_TOK_PUNCT && 
                lexer->tok_punct_kind == PP_TOK_PUNCT_VARARGS) {
                string arg_name = WRAP_Z("__VA_ARGS__");
                preprocessor_macro_arg *arg = get_new_macro_arg(pp);
                arg->name = arg_name;
                arg->next = args;
                args = arg;

                pp_lexer_parse(lexer);
                if (lexer->tok_kind != ')') {
                    // TODO: Diagnostic
                    assert(false);
                    break;
                }
            } else if (lexer->tok_kind == PP_TOK_ID) {
                allocator a = bump_get_allocator(pp->a);
                string arg_name = string_memdup(&a, lexer->tok_buf);
                preprocessor_macro_arg *arg = get_new_macro_arg(pp);
                arg->name = arg_name;
                arg->next = args;
                args = arg;
                
                pp_lexer_parse(lexer);
                if (lexer->tok_kind == PP_TOK_PUNCT) {
                    if (lexer->tok_punct_kind == ',') {
                        pp_lexer_parse(lexer);
                    } else if (lexer->tok_punct_kind != ')') {
                        // TODO: Diagnostic
                        assert(false);
                        break;
                    }
                } 
            }
        }

        if (lexer->tok_kind == PP_TOK_PUNCT && lexer->tok_punct_kind == ')') {
            pp_lexer_parse(lexer);
        }

        macro->args = args;
        macro->is_function_like = true;
    }

    while (!lexer->tok_at_line_start && lexer->tok_kind != PP_TOK_EOF) {
        preprocessor_token *tok = get_new_token(pp);

        tok->kind = lexer->tok_kind;
        tok->str_kind = lexer->tok_str_kind;
        tok->punct_kind = lexer->tok_punct_kind;
        string data = string(lexer->tok_buf, lexer->tok_buf_len);
        allocator a = bump_get_allocator(pp->a);
        tok->str = string_dup(&a, data);
        
        tok->next = macro->definition;
        macro->definition = tok;
    }
}

static void 
undef_macro(preprocessor *pp) {
    pp_lexer *lexer = pp->lexer;
    if (lexer->tok_kind != PP_TOK_ID) {
        // TODO: Diagnostic
        assert(false);
    }

    string ident_name = string(lexer->tok_buf, lexer->tok_buf_len);
    uint32_t hash = hash_string(ident_name);
    preprocessor_macro **macrop = get_macro(pp, hash);
    if (*macrop) {
        preprocessor_macro *macro = *macrop;
        free_macro_data(pp, macro);
        *macrop = macro->next;
        macro->next = pp->macro_freelist;
        pp->macro_freelist = macro;
    } else {
        // TODO: Diagnostic
        assert(false);
    }
}

static void 
process_pp_directive(preprocessor *pp) {
    pp_lexer *lexer = pp->lexer;
    if (lexer->tok_kind != PP_TOK_ID) {
        // TODO: Diagnostic
        assert(false);
    }

    string directive = string(lexer->tok_buf, lexer->tok_buf_len);
    if (string_eq(directive, WRAP_Z("include"))) {
    } else if (string_eq(directive, WRAP_Z("define"))) {
        pp_lexer_parse(lexer);
        define_macro(pp);
    } else if (string_eq(directive, WRAP_Z("undef"))) {
        pp_lexer_parse(lexer);
        undef_macro(pp);
    } else if (string_eq(directive, WRAP_Z("if"))) {
    } else if (string_eq(directive, WRAP_Z("elif"))) {
    } else if (string_eq(directive, WRAP_Z("else"))) {
    } else if (string_eq(directive, WRAP_Z("endif"))) {
    } else if (string_eq(directive, WRAP_Z("ifdef"))) {
    } else if (string_eq(directive, WRAP_Z("ifndef"))) {
    } else if (string_eq(directive, WRAP_Z("line"))) {
        // Ignored
    } else if (string_eq(directive, WRAP_Z("pragma"))) {
    } else if (string_eq(directive, WRAP_Z("error"))) {
    } else {
        // TODO: Diagnostic
    }
}

token 
preprocessor_get_token(preprocessor *pp) {
   pp_lexer_parse(pp->lexer); 
   for (;;) {
       if (pp->lexer->tok_kind == PP_TOK_PUNCT && 
           pp->lexer->tok_punct_kind == '#') {
           if (pp->lexer->tok_at_line_start) {
               pp_lexer_parse(pp->lexer);
               process_pp_directive(pp);
               continue;
           } else {
               assert(false); // TODO: Diagnostic
           }
       }
   }
}
