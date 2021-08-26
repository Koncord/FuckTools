//
// Created by koncord on 28.01.18.
//

#ifndef FUCKTOOLS_FUCKBYTECODE_HPP
#define FUCKTOOLS_FUCKBYTECODE_HPP

#include <string>
#include <vector>
#include <istream>

#pragma pack(push, 1)
struct Instruction {
    enum class VMOpcode : char {
        invalid = 0,
        inc,
        sub,
        incWin,
        decWin,
        outChar,
        inChar,
        loopBegin,
        loopEnd,
        set,
        copy,
        mul,
        scan,
        mov,
        memset,
    };
    typedef short HalfType;
    typedef int ArgType;
    typedef char CellType;
    static const int maxCells = 30000; // classic BF
    static const int CellMaxValue = UCHAR_MAX;

    VMOpcode fn;
    union Arg {
        Arg() : full(0) {}

        Arg(ArgType full) : full(full) {}

        Arg(HalfType offset, CellType arg) : half{offset, 0, arg} {}

        ArgType full;
        struct {
            HalfType offset;
            CellType arg2;
            CellType arg;
        } half;
    } arg;

    explicit Instruction(VMOpcode fn = VMOpcode::invalid, Arg arg = Arg{-1}) noexcept: fn(fn), arg(arg) {}

    explicit Instruction(VMOpcode fn = VMOpcode::invalid, HalfType offset = -1, CellType arg = -1) noexcept: fn(fn), arg(offset, arg) {}

    explicit Instruction(VMOpcode fn, ArgType arg) noexcept: fn(fn) { this->arg.full = arg; }

    static std::vector<Instruction> load(std::istream &bfc) noexcept;
    static void save(const std::string &file, const std::vector<Instruction> &codes) noexcept;
};
#pragma pack(pop)

#endif //FUCKTOOLS_FUCKBYTECODE_HPP
