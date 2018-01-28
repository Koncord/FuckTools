//
// Created by koncord on 28.01.18.
//

#include <stdexcept>
#include <fstream>
#include <iostream>
#include "FuckByteCode.hpp"

std::vector<Instruction> Instruction::load(std::istream &bfc) noexcept
{

    bfc.seekg(0, std::ios::end);
    auto numCodes = static_cast<size_t>(bfc.tellg() / size());
    bfc.seekg(0, std::ios::beg);

    std::vector<Instruction> codes(numCodes);

    for (int i = 0; i < numCodes; ++i)
    {
        char buff[size()];
        bfc.read(buff, size());
        switch (static_cast<VMOpcode>(buff[0]))
        {
            case VMOpcode::inc:
            case VMOpcode::sub:
            case VMOpcode::incWin:
            case VMOpcode::decWin:
            case VMOpcode::outChar:
            case VMOpcode::inChar:
            case VMOpcode::loopBegin:
            case VMOpcode::loopEnd:
            case VMOpcode::set:
            case VMOpcode::copy:
            case VMOpcode::mul:
            case VMOpcode::scan:
            case VMOpcode::mov:
                break;
            default:
                codes.clear();
                return codes;
        }

        codes[i] = Instruction(static_cast<VMOpcode>(buff[0]), *reinterpret_cast<Arg *>(&buff[1]));
    }

    return codes;
}

void Instruction::save(const std::string &file, const std::vector<Instruction> &codes) noexcept
{
    std::ofstream bfc(file, std::ios::binary);

    for (const auto &c : codes)
    {
        bfc.write(reinterpret_cast<const char *>(&c.fn), sizeof(VMOpcode));
        bfc.write(reinterpret_cast<const char *>(&c.arg), sizeof(Arg));
    }
    bfc.close();
}
