# FuckTools
Tools to build, translate and execute classic BrainFuck language

## FuckCompiler
A tool to build BrainFuck to intermediate bytecode
```
fuckc mandelbrot.bf mandelbrot.bfc motherfucker
```
Optimization levels:
```
none - switch off all optimizations
basic - contract series of >> or +++ to 1 command
kidlevel - compiles series of [+] and [-] to memzero opcode and [>] and [<] to scan opcode
aggressive - compiles [->>+>+<<<] and [->>+>+++<<<] to mul and copy opcodes
motherfucker - lota motherfucker level optimizes 
```

## FuckMachine
Execute BrainFuck bytecode in VM
```
fuckmachine mandelbrot.bfc
```

## BFCC
Translate BrainFuck bytecode to the C code
```
fuckcc mandelbrot.bfc mandelbrot.c
gcc mandelbrot.c mandelbrot
./mandelbrot
```
