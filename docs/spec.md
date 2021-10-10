# Spec

## Introduction

PKBY is an educational project of first-grade student done in free time.
It is intended to be an introduction to the world of compilers and formal languages.

Thus, the goals are very simple. Maximum functionality and experiment with minimum optimization.

Features implemented in this language are often can be thought about as additions to the C language, ignoring ++ part of it as for being too bloated.

Main goals in language design are advanced metaprogramming and consistency.

### Metaprogramming

Includes full compile-time code execution, as what C is totally lacking besides some constant propagation.

### Consistency

Many features of C and languages derived from it are very error-prone and compilcated - like switch statement, typedef, function pointers and many others. For example, there are 4 meanings of static keyword depending on context, -> vs . in struct member access (more of refactorability problem), having to break the switch cases for one-in million case where you wouldn't want that. 

## Basic syntax

All examples here are followed by their c counterparts

### Main

```c
int main(int argc, char **argv) {
    
}
```

There are several problems with C main: 
different signatures, and int as return type.

```
main :: () {
    
}
```

### Variable declaration

```c++
int x;
x = 2;
int a = x;
// c++
auto b = 2;
```

```
x : int;
x = 2;
a : int = x;
b := 2;
```

### If statements

```c
int sign;
if (x > 0) {
   sign = 1; 
} else if (x == 0) {
    sign = 0;
} else {
    sign = -1;
}
```

```
sign : int;
if x > 0 {
    sign = 1;
} else if x == 0 {
    sign = 0;
} else {
    sign = -1;
}
```

Not that {} are forced

### Loops

```c
// infinite 
for (;;) { }
// pre-condition
while (x > 0) { }
// post-condition
do { } while (x > 0); 
```

```
// infinite
loop { }
// pre-condition
while x > 0 { }
```

Although it is not ideal having two keywords doing the same thing, but not having while would result in code much code bloat

### Loop control flow

```c
// Nested loop break
bool is_valid;
while (x < 0 && is_valid) {
    while (x < -100) {
        if (x == -105) {
            is_valid = false;
            break;
        }
    }
}
// Break and continue
while (x > 0) {
    if (x % 2 == 0) continue;
    break;
}
```

```
// Nested loop break
loop a {
    if x < 0 { break; }
    while x < -100 {
        if x == -105 break a; 
    }
}
// Break and continue
while x > 0 {
    if x % 2 == 0 { continue; }
    break;
}
```

### Functions

```c
int gcd(int a, int b) {
    ...
}
```

```
gcd :: (a : int, b : int) -> int {
    
}
```

### Function pointer types
```c
int (*gcd)(int a, int b) = 0;
```

```
gcd : (a : int, b : int) -> int = 0; 
```

### Pointers
```c
int *a = malloc(sizeof(int));
*a = 2;
foo(a);
```

```
a : *int = new int;
*a = 2;
foo(a);
```


### Type casting
```c
int a = (int)0.5f;
```

```
a := cast int 0.5f;
```

### Structs
```c
struct Foo {
    int bar;
};
```

```
Foo :: struct {
    bar : int;
};
```

### Struct pointers
```c
Foo *foo = malloc(sizeof(Foo));
foo->bar = 1;
Foo *foo_copy = malloc(sizeof(Foo));
*foo_copy = *foo; 
```

```
foo : *Foo = new Foo;
foo.bar = 1; 
foo_copy : *Foo = new Foo;
*foo_copy = *foo;
```

## Metaprogramming

```c++
constexpr int factorial(int a) {
    if (a == 1) {
        return 1;
    } 
    return a * factorial(a - 1);
}
const int precomputed_factorial = factorial(a);
```

```
factorial := (a : int) -> int {
    if a == 1 { return 1; }
    return a * factorial(a - 1);
}
precomputed_factorial := #run factorial(10); 

do_arbitrary_complex_stuff :: () {
    file := open_file("log.log");
    write_file(file, "hello world");
    close_file(file);
}
#run do_arbitrary_complex_stuff();
```
