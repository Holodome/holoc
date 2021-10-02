#include "lib/clarg_parse.h"
#include "lib/lists.h"
#include "lib/strings.h"

typedef struct {
    CLArgInfo *infos;
    u32 ninfos;
    
    u32 argc;
    char **argv;
    u32 cursor;
} CLArgCtx;

static bool 
ctx_is_not_finished(CLArgCtx *ctx) {
    return ctx->cursor < ctx->argc;
}

static bool 
ctx_has_space_for(CLArgCtx *ctx, u32 n) {
    return ctx->cursor + n < ctx->argc;
}

static CLArgInfo *
find_info(CLArgCtx *ctx, const char *opt) {
    CLArgInfo *info = 0;
    for (u32 i = 0; i < ctx->ninfos; ++i) {
        CLArgInfo *test = ctx->infos + i;
        if (str_eq(test->name, opt)) {
            info = test;
            break;
        }
    }
    return info;
}

static void 
write_arg(void *out, u32 type, const char *opt) {
    switch (type) {
    case CLARG_TYPE_F64: {
        f64 value = str_to_f64(opt);
        *((f64 *)out) = value;
    } break;
    case CLARG_TYPE_I64: {
        i64 value = str_to_i64(opt);
        *((i64 *)out) = value;
    } break;
    case CLARG_TYPE_STR: {
        char *str = mem_alloc_str(opt);
        *((char **)out) = str;
    } break;
    default: {
        NOT_IMPLEMENTED;
    } break;
    }
}

void 
clarg_parse(void *out_bf, CLArgInfo *infos, u32 ninfos, u32 argc, char ** const argv) {
    CLArgCtx ctx = {0};
    ctx.infos = infos;
    ctx.ninfos = ninfos;
    ctx.argc = argc;
    ctx.argv = argv;
    ctx.cursor = 1;    
    
    while (ctx_is_not_finished(&ctx)) {
        const char *opt = argv[ctx.cursor];
        CLArgInfo *info = find_info(&ctx, opt);
        
        if (!info) {
            // Try to find variadic
            for (u32 i = 0; i < ninfos; ++i) {
                CLArgInfo *test = infos + i;
                if (test->narg == CLARG_NARG) {
                    info = test;
                    break;
                }
            }
        }
        
        if (!info) {
            erroutf("Unknown option %s\n", opt);
            ++ctx.cursor;
            continue;
        }
        
        void *out = (u8 *)out_bf + info->output_offset;
        if (info->narg == CLARG_NARG) {
            assert(info->type != CLARG_TYPE_BOOL);
            u32 narg_start_cursor = ctx.cursor;
            ++ctx.cursor;
            while (ctx_is_not_finished(&ctx)) {
                opt = argv[ctx.cursor];
                if (find_info(&ctx, opt)) {
                    break;
                }
                ++ctx.cursor;
            }
            
            u32 narg_count = ctx.cursor - narg_start_cursor;
            void *arr = mem_alloc(8 * (narg_count + 1)); // @HACK Should accumulate size 
            for (u32 i = 0; i < narg_count; ++i) {
                u32 cursor = narg_start_cursor + i;
                write_arg((u8 *)arr + 8 * i, info->type, ctx.argv[cursor]);
            }
            ((u64 *)arr)[narg_count] = 0;
            *(void **)out = arr;
            continue;   
        }
        
        if (info->narg == 0) {
            assert(info->type == CLARG_TYPE_BOOL);
            *(u8 *)out = true;
            ++ctx.cursor;
        } else if (info->narg == 1) {
            if (!ctx_has_space_for(&ctx, 1)) {
                NOT_IMPLEMENTED;
                break;
            } else {
                ++ctx.cursor;
            }
            write_arg(out, info->type, ctx.argv[ctx.cursor++]);
        } else {
            
        }
    }
}