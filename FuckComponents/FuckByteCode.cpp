//
// Created by koncord on 28.01.18.
//

#include <fstream>
#include "FuckByteCode.hpp"

std::vector<Instruction> Instruction::load(std::istream &bfc) noexcept {
    std::vector<Instruction> codes;
    VMOpcode op;

    while ( bfc.read((char*) &op, sizeof(VMOpcode))) {


        Arg a{};

        switch (op) {
            case VMOpcode::inc:
            case VMOpcode::sub:
            case VMOpcode::outChar:
            case VMOpcode::inChar:
            case VMOpcode::set:
            case VMOpcode::copy:
            case VMOpcode::mul:
            case VMOpcode::mov:
                /*bfc.read((char*)&a.half.offset, sizeof(HalfType));
                bfc.read((char*)&a.half.arg, sizeof(CellType));
                break;*/
            case VMOpcode::incWin:
            case VMOpcode::decWin:
            case VMOpcode::loopBegin:
            case VMOpcode::loopEnd:
            case VMOpcode::scan:
                bfc.read((char*)&a.full, sizeof(ArgType));
                break;
            default:
                codes.clear();
                return codes;
        }

        codes.emplace_back(op, a);
    }

    return codes;
}

void Instruction::save(const std::string &file, const std::vector<Instruction> &codes) noexcept {
    std::ofstream bfc(file, std::ios::binary);

    for (const auto &c : codes) {
        bfc.write(reinterpret_cast<const char *>(&c.fn), sizeof(VMOpcode));
        switch (c.fn) {
            case VMOpcode::inc:
            case VMOpcode::sub:
            case VMOpcode::outChar:
            case VMOpcode::inChar:
            case VMOpcode::set:
            case VMOpcode::copy:
            case VMOpcode::mul:
            case VMOpcode::mov:
                /*bfc.write((char*) &c.arg.half.offset, sizeof(HalfType));
                bfc.write((char*) &c.arg.half.arg, sizeof(CellType));
                break;*/
            default:
                bfc.write(reinterpret_cast<const char *>(&c.arg.full), sizeof(ArgType));
        }
    }
    bfc.close();
}
