# Simple Tokenizer

A simple tokenizer that reads a source file and lists the tokens that it finds.
This tokenizer is not designed specifically for any language. Instead, it is written as a general tokenizer that can help us better understand tokenizer and how it is used in compilers or interpreters.


#### Example

For instance, the following input line:
```
50 plus 150
is equal to 200.
```

Will produce the following listing:
```
1 0: 50 plus 150
    >> <NUMBER>         50
    >> <WORD>           plus
    >> <NUMBER>         150
2 0: is equal to 200
    >> <WORD>           is
    >> <WORD>           equal
    >> <WORD>           to
    >> <WORD>           200
    >> <PERIOD>         .
```

#### Usage

Generate the executable using `gcc`:
`gcc token.c -o token`

Run the executable with the path of the source file as an argument
`token <sourcefile>`