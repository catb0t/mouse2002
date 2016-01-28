# {Curly Numbers};

In the esoteric programming language Curly, programs consist solely of curly braces `{}` and semicolons `;`. Despite this humble toolset, Curly has literals that can represent any nonnegative integer. The format is a little hard for the uninitiated to read, though, so let's write some code to do the conversion for us.

## Format of numbers

Curly numbers are structured according to the following rules:

1. Adding a semicolon adds one to the number.
1. A number enclosed in curly braces is multiplied by four.
1. Curly-brace groups may be nested but not concatenated. Braces must match properly.
1. Semicolons outside a set of curly braces must come afterward, not before.
1. To avoid ambiguity in parsing, a number must always start with a curly brace.

Some examples:

```
{;;}     2*4 = 8
{{;};};  (1*4+1)*4+1 = 21
{};;;    0*4+3 = 3
```
(Note that rule 5 means the numbers 0 to 3 must start with an empty pair of curly braces.)

And some invalid examples:

```
{{;}{;;}}  Curly brace groups side-by-side, not nested
{;}}       Unmatched brace
{;{;}}     Semicolon before curly-brace group
;;;        Number does not start with curly brace
```
Here's a BNF grammar for Curly numbers:
```
<number> ::= "{" <inner> "}" <semis>
<inner>  ::= <semis>
           | <number>
<semis>  ::= ";" <semis>
           | ""
```
Numbers like `{;;;;}` (more than 3 semicolons in a row) or `{{};}` (unnecessary empty brace groups) are called *improper* Curly numbers. They obey the above grammar and can be evaluated in the usual way, but they are also capable of shorter representations (for the above examples, `{{;}}` and `{;}` respectively).

## The challenge

Write a program or function that inputs/receives a string. If the string is a nonnegative decimal integer, output/return the *proper* (i.e. shortest possible) Curly representation for that integer. If the string is a Curly number, output/return its decimal representation.

**Input** can be received via STDIN, command-line argument, or function parameter. It *must* be a string; that is, you may not write a function that accepts strings for Curly numbers but integers for decimal numbers.

**Output** can be printed to STDOUT or returned from the function. A function *may* return an integer when appropriate, or it may return strings in all situations.

Your program does not have to handle bad input (Curly numbers that break the formatting rules, floating point numbers, negative integers, random text), and it is **not** required to handle improper Curly numbers (but see below). Input will consist only of printable ASCII characters.

## Scoring

The shortest code in bytes wins. If your program can do **both** of the following:

1. correctly handle improper Curly numbers, and
1. when given a Curly number, ignore any extra characters that aren't `{};`

then subtract 10% from your score. (Integer input will never have extraneous characters, even for the bonus.)

## Test cases

    Input       Output
    {;;}        8
    {{;};};     21
    {};;;       3
    {{{{;}}};}  260
    {}          0
    4           {;}
    17          {{;}};
    1           {};
    0           {}
    96          {{{;};;}}

For the bonus:

    {};;;;;     5
    {{;;;;};;}  72
    c{u;r;l}y;! 9
    42{;} ;;;;  8

<sub>Note: Curly is not yet implemented. But if this question does well, I may develop it further.</sub>
