#include "interp.hh"

Interp::Interp(const Str &filename) {
    this->filename = filename;
    FILE *file = fopen(filename.data, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    file_data_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    file_data = (u8 *)Mem::alloc(file_data_size + 1);
    fread(file_data, file_data_size, 1, file);
    file_data[file_data_size] = 0;
    fclose(file);
    
    lexer.init(file_data, file_data_size + 1);
    parser.interp = this;
    parser.lexer = &lexer;
}

Interp::~Interp() {
    Mem::free(file_data);
}

void verror_at(const char *filename, const char *file_data,
               u32 line_number, u32 char_index, const char *format, va_list args) {
    assert(char_index--);
    assert(line_number--);
    
    const char *line_start = file_data;
    u32 counter = 0;
    while (counter < line_number) {
        counter += *line_start++ == '\n';
    }
    
    // while (file_data < line_start && *(line_start - 1) != '\n') {
    //     --line_start; 
    // }
    const char *line_end = line_start;
    while (*line_end && *line_end != '\n') {
        ++line_end;
    }

    u32 indent = fprintf(stderr, "%s:%d,%d: ", filename, line_number, char_index);
    fprintf(stderr, "%.*s\n", (int)(line_end - line_start), line_start);
    u32 indent_until_cursor = indent + char_index;
    fprintf(stderr, "%*s", indent_until_cursor, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
}

void Interp::report_error(Token *token, const char *format, ...) {
    va_list args;
    va_start(args, format);
    verror_at(filename.data, (const char *)file_data, token->line_number, 
              token->char_number, format, args);
    va_end(args);
}

void Interp::report_error(Ast *ast, const char *format, ...) {
    va_list args;
    va_start(args, format);
    verror_at(filename.data, (const char *)file_data, ast->line_number, 
              ast->char_number, format, args);
    va_end(args);
}

void Interp::report_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    Token *token = lexer.peek_tok();
    verror_at(filename.data, (const char *)file_data, token->line_number, 
              token->char_number, format, args);
    va_end(args);
}

void Interp::interp() {
    AstBlock *global_block = parser.parse();
   
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
