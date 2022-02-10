#!/usr/bin/env python3

import sys

def nodeinfo(n):
    return (n.kind, n.is_anonymous(), n.spelling, n.type.spelling)

import clang.cindex
index = clang.cindex.Index.create()
tu = index.parse("examples/example.c", args=["-std=c99"])
for t in tu.cursor.walk_preorder():
    print(dir(t))
    print(nodeinfo(t))
    print(repr(t))
