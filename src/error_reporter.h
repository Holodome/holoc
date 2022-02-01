#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include "types.h"

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

#endif 
