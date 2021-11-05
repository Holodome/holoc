/* 
    Stages of translation

Encoding is defined for all source files to be the same. All files are opened and encoded into
UTF8. This was chosen beacause it supports all ASCII characters needed to aprse language enytax while
also allowing unicode in places where we need it.

The C standard says that \\n in source should be removed in order co concatenate lines. This behaviour is
not done directly in order to preserve the source code locations line numbers,
 however, in places where this affect the program behavoiur (namely, macros & string literals)
this is implemented in-place.

Trigraphs are not implemented.

'\r\n' is not replaced for ' \n' for now as there are no placed where it would affect the code generation
 
After ending the preparation of source file (if any), the porgram strarts to generate tokens.

Token generation is split into three stages. 
In first, token is prepared via parsing it to a buffer. This is done to implement some of the language behaviours.
Second stage involves textual manipulation of tokens, as described by the preprocessor, or concatenation of 
string literals.
The third and the last stage does the actual token generation from temporary buffer.

Preprocessor behaviour is implemented in a way to try not to generate tokens ahead, or tokenize whole file at once
This may seem like unwanted complication, but it has been decided to be a way to go.
Thus, the program needs to support some kind of data structure to allow storing state of different parsing locations.
This is implemented via simple stack.
For example, when the compiler does include of new file, it appens new buffer with file source, and stores location
of previously parsed file. This is also used to do macro text replacement - instead of manipulating wiht text,
it pushes new buffers into stack.
*/
#include "lib/general.h"

#include "lib/memory.h"
#include "lib/strings.h"
#include "lib/stream.h"

#include "compiler_ctx.h"
#include "lexer.h"

static void 
fmt_src_loc(Out_Stream *stream, const Src_Loc *loc, File_Registry *fr, u32 depth) {
    switch (loc->kind) {
    case SRC_LOC_FILE: {
        out_streamf(stream, "%*c FILE '%s'%u:%u ", 
            depth, ' ',
            get_file_path(fr, loc->file), loc->line + 1, loc->symb + 1);
    } break;
    case SRC_LOC_MACRO: {
        out_streamf(stream, "%*c MACRO %u", depth, ' ', loc->symb + 1);
        // fmt_src_loc(stream, loc->parent, fr, depth + 1);
    } break;
    case SRC_LOC_MACRO_ARG: {
        out_streamf(stream, "%*c MACRO ARG: %u", depth, ' ', loc->symb + 1);
    } break;
    }
    if (loc->parent) {
        out_streamf(stream, "\n");
        fmt_src_loc(stream, loc->parent, fr, depth + 1);
    }
}

int 
main(int argc, char **argv) {
    const char *include_paths[] = {
        "examples/tests/pp",
        "examples/tests",
        "src"
    };
    
    Compiler_Ctx *ctx = create_compiler_ctx();
    ctx->fr->include_seach_paths_count = ARRAY_SIZE(include_paths);
    ctx->fr->include_search_paths = include_paths;
    Lexer *lexer = create_lexer(ctx, "sample.c");
    preprocess(lexer, STDOUT);
    
    UNUSED(argc);
    UNUSED(argv);
    return 0;
}