# Codestyle

## Naming conventions

All functions, variables and types should be in snake case.
Preprocessor macros should be in upper case, except for cases of function-like macros that try to look like a function (and act like one, like wrapper).
Small macros that wrap some expression should be left macros to signal for inlining.
Enum values should be in upper case.

Variable names can be short, like tok instead of token. This is not allowed in other cases.
Type names should be the most descriptive, function names can be shortened.

Functions and types that form some logical group should be prefixed with 2-3 letters.
