#define A (2 + 3)
#define C (1 / 1)
// #define B (A * C)

// int x = B;

// #define B(_a, _c) (_a * _c)

// int y = B(A, C);

// #define D() B(A, C)

// int yy = D();

// #define E(_a, ...) (_a + 1), __VA_ARGS__
// int E(a, b, c, d);

#define E(a, ...) __VA_ARGS__
int E(a, b, c, d);