#define str(a) #a
// str(asdasd)
// str(123)
#define CONCAT(a, b) a##b
str(CONCAT(2, 3))