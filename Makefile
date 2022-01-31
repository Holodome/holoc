ERROR_POLICY = -pedantic -Wshadow -Wextra -Wall -Werror -Wno-unused-function -Wno-unused-parameter -Wno-missing-braces
CFLAGS = -g -O0 -std=c99 -Isrc -D_CRT_SECURE_NO_WARNINGS $(ERROR_POLICY)

DIR = build
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:%.c=$(DIR)/%.o)
DEPS = $(wildcard src/*.h)

TESTS = $(wildcard tests/*.c)
TEST_EXES = $(TESTS:%.c=$(DIR)/%.exe)

# all: format holoc 
all: holoc 

holoc: $(OBJS) | $(DIR) $(DEPS)
	$(CC) -o $(DIR)/$@ $^ $(LDFLAGS)

run: holoc 
	$(DIR)/holoc

$(DIR)/%.i: %.c
	mkdir -p $(dir $@)
	$(CC) -E $(CFLAGS) -c -o $@ $<  

$(DIR)/%.o: %.c | $(DEPS)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<  

$(DIR):
	mkdir -p $@

clean:
	rm -rf $(DIR)

format:
	find src -iname *.h -o -iname *.c | xargs clang-format -i --style=file --verbose

$(DIR)/tests/%.exe: tests/%.c | $(DEPS) $(SRCS)
	mkdir -p $(dir $@)
	$(CC) -o $@ tests/$*.c $(CFLAGS) 

test: $(TEST_EXES) | $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; done 
	
.PHONY: clean format test
