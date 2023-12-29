# COOL Compiler
---

**Classroom Object Oriented Language (COOL)** is an introductory programming language designed to be implemented quickly. Still, Cool retains many features of modern programming languages, including objects, static typing, and automatic memory management.

You can find much more about COOL, how its syntax is, how it should be compiled, etc., in the "cool-manual" file or this [course homepage](https://web.stanford.edu/class/cs143/).

Because COOL is used for educational reasons, you need to install a virtual machine or use Docker to work with and write a compiler for it. I found Docker more convenient to work with, and I've provided a tutorial on how to use Docker for this project in the Installation & Setup" file in English and "نصب و راه اندازی" file in Persian.

All the following implementations correspond to the "cool-manual" file and are implemented in C and some frameworks. If you have any questions about COOL or part of this project, you can likely find answers in the "Assignment" file next to each implementation or the "cool-manual" file.

## COOL Program
---

In this section, to get familiar with and better understand COOL's syntax and how it works, I implemented the stack class in COOL language with some features explained in the "Assignment" file.

## Lexer
---

In this section, I have implemented syntax analysis (Lexer) for COOL using [Flex](https://westes.github.io/flex/manual/).

## Parser
---

In this section, I have implemented Parser for COOL using [Bison](https://www.gnu.org/software/bison/manual/html_node/index.html).

## Semantic Analysis
___

In this section, I have implemented a semantic analysis for COOL. There is no need to use an extra framework or library in this section, and I implemented it in C language. But, in addition to "cool-manual", you need to read the "cool-tour" to know how to access the data you extract in the Lexer and Parser.