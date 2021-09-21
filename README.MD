# PKBY

This is complete complier and runner for bytecode-compiled language.

This project is a journey into exploring world of programming languages and is not positioning itself as an example of well-written or optimized product. 

## Idea

Idea behind this project as whole is to learn how to write compilers for programming languages. This includes a few of different steps: tokenizing text-based input, formalizing tokenized data, compiling to whatever format will be used and then executing compiled code. 

Each of program steps tries to be written in a modular manner that is suitable for later usage in futher projects. That invloves spending some time on designing the API - thing that learning project could as well leave overboard.

## Features

> List may change

* Memory allocator with allocation tracking
* Compilation to bytecode
* Bytecode execution
* Metaprogramming features (ability to to separate compilation step result of which is used in actual program)
* Ability to link to C libraries
* Compiler does not use c standard library, but it is possible to link to 
* Garbage collection

## Inspiration

First of all, this project is not positioning itself as completely original product. Thus, some acknowledgements have to be made.

* [Jonathan Blow](https://www.youtube.com/user/jblow888) - inspiration about syntax and language design
* [Casey Muratori](https://www.youtube.com/c/MollyRocket) - huge influence on code style 
* [Niklas Gray](https://www.gamasutra.com/blogs/author/NiklasGray/1016727/) - his articles on data structures pretty much reflect author's point of view (see 'Minimalist container library in C' and 'Data Structures')

## Goal

The goal here can be split into several parts. 
First of all, compiling and executing code should be possible. Secondly, it is to write an example project using this language that can show off its benefits (or features). Because on early stages of development it is not clear what whole set of features will look like, it is complicated to describe the project. 

One possibility could make use of some multy-platform library that can be used to make games, because author of this project has some expirience in that sphere.