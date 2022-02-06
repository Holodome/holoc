#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include "general.h"

typedef struct error_reporter {
    uint32_t error_count;
    uint32_t warning_count;
} error_reporter;

error_reporter *get_error_reporter(void);
void er_print_final_stats(void);

void report_message_internalv(string file_contents, source_loc loc, string message_kind,
                              char *msg, va_list args);

void report_errorv(source_loc loc, char *fmt, va_list args);
void report_error(source_loc loc, char *fmt, ...);
void report_warningv(source_loc loc, char *fmt, va_list args);
void report_warning(source_loc loc, char *fmt, ...);
void report_notev(source_loc loc, char *fmt, va_list args);
void report_note(source_loc loc, char *fmt, ...);

#define ABSTRACT_REPORT_(_what, func, ...) func((_what)->loc, __VA_ARGS__)

#define report_error_pp_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_error, __VA_ARGS__)
#define report_errorv_pp_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_errorv, __VA_ARGS__)
#define report_warning_pp_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_warning, __VA_ARGS__)
#define report_warningv_pp_token(_tok, ...) \
    ABSTRACT_REPORT_(_tok, report_warningv, __VA_ARGS__)
#define report_note_pp_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_note, __VA_ARGS__)
#define report_notev_pp_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_notev, __VA_ARGS__)

#define report_error_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_error, __VA_ARGS__)
#define report_errorv_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_errorv, __VA_ARGS__)
#define report_warning_oken(_tok, ...) ABSTRACT_REPORT_(_tok, report_warning, __VA_ARGS__)
#define report_warningv_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_warningv, __VA_ARGS__)
#define report_note_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_note, __VA_ARGS__)
#define report_notev_token(_tok, ...) ABSTRACT_REPORT_(_tok, report_notev, __VA_ARGS__)

#define report_error_ast(_ast, ...) ABSTRACT_REPORT_(_ast, report_error, __VA_ARGS__)
#define report_errorv_ast(_ast, ...) ABSTRACT_REPORT_(_ast, report_errorv, __VA_ARGS__)
#define report_warningast(_ast, ...) ABSTRACT_REPORT_(_ast, report_warning, __VA_ARGS__)
#define report_warningv_ast(_ast, ...) ABSTRACT_REPORT_(_ast, report_warningv, __VA_ARGS__)
#define report_note_ast(_ast, ...) ABSTRACT_REPORT_(_ast_, report_note, __VA_ARGS__)
#define report_notev_ast(_ast, ...) ABSTRACT_REPORT_(_ast, report_notev, __VA_ARGS__)

#endif
