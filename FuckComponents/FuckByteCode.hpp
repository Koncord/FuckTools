//
// Created by koncord on 28.01.18.
//

#ifndef FUCKTOOLS_FUCKBYTECODE_HPP
#define FUCKTOOLS_FUCKBYTECODE_HPP

#include <string>
#include <vector>
#include <istream>

struct Instruction {
    enum class VMOpcode : char {
        inc = '+',
        sub = '-',
        incWin = '>',
        decWin = '<',
        outChar = '.',
        inChar = ',',
        loopBegin = '[',
        loopEnd = ']',
        set = '=',
        copy = 'c',
        mul = 'm',
        scan = 's',
        mov = 'v',
        invalid = 0,
    };
    typedef short HalfType;
    typedef int ArgType;
    typedef char CellType;
    static const int maxCells = 30000; // classic BF

    VMOpcode fn;
    union Arg {
        ArgType full;
        struct {
            HalfType offset;
            CellType pad0;
            CellType arg;
        } half;
    } arg;

    explicit Instruction(VMOpcode fn = VMOpcode::invalid, Arg arg = Arg{-1}) noexcept: fn(fn), arg(arg) {}

    explicit Instruction(VMOpcode fn, ArgType arg) noexcept: fn(fn) { this->arg.full = arg; }

    static std::vector<Instruction> load(std::istream &bfc) noexcept;
    static void save(const std::string &file, const std::vector<Instruction> &codes) noexcept;
};

#endif //FUCKTOOLS_FUCKBYTECODE_HPP
