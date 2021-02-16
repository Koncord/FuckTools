//
// Created by Stanislav "Koncord" Zhukov on 16.02.2021.
//

# pragma once

#include <vector>
#include <istream>
#include <FuckComponents/FuckByteCode.hpp>

class FuckTranslator {
public:
    explicit FuckTranslator(std::istream &file) : file(file) {
    }

    std::vector<Instruction> translate() {
        unsigned char ch;
        std::vector<Instruction> code;
        Instruction::Arg arg{0};
        arg.half.arg = 1;

        Instruction::Arg arg2{0};
        arg2.half.arg = -1;

        while (file >> ch) {
            switch ((Instruction::VMOpcode) ch) {
#define FUCK_PARSE(opcode, arg) case opcode: code.emplace_back(opcode, arg); break;
                FUCK_PARSE(Instruction::VMOpcode::inc, arg);
                case Instruction::VMOpcode::sub:
                    code.emplace_back(Instruction::VMOpcode::inc, arg2);
                    break;
                FUCK_PARSE(Instruction::VMOpcode::incWin, 1);
                case Instruction::VMOpcode::decWin:
                    code.emplace_back(Instruction::VMOpcode::incWin, -1);
                    break;
                FUCK_PARSE(Instruction::VMOpcode::inChar, arg2);
                FUCK_PARSE(Instruction::VMOpcode::outChar, arg2);
                FUCK_PARSE(Instruction::VMOpcode::loopBegin, 0x0000DEAD);
                FUCK_PARSE(Instruction::VMOpcode::loopEnd, 0x0000DEAD);
#undef FUCK_PARSE
                default:
                    //std::cerr << "Unknown opcode: " << ch << std::endl;
                    break;
            }
        }
        std::cout << std::endl;
        return code;
    }

private:
    std::istream &file;
};
