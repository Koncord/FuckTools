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

        while (file >> ch) {
            switch (ch) {
                case '+':
                    code.emplace_back(Instruction::VMOpcode::inc, 0, 1);
                    break;
                case '-':
                    code.emplace_back(Instruction::VMOpcode::inc, 0, -1);
                    break;
                case '>':
                    code.emplace_back(Instruction::VMOpcode::incWin, 1);
                    break;
                case '<':
                    code.emplace_back(Instruction::VMOpcode::incWin, -1);
                    break;
                case ',':
                    code.emplace_back(Instruction::VMOpcode::inChar, 0, -1);
                    break;
                case '.':
                    code.emplace_back(Instruction::VMOpcode::outChar, 0, -1);
                    break;
                case '[':
                    code.emplace_back(Instruction::VMOpcode::loopBegin, 0x0000DEAD);
                    break;
                case ']':
                    code.emplace_back(Instruction::VMOpcode::loopEnd, 0x0000DEAD);
                    break;
                default:
                    //std::cerr << "Unknown opcode: " << ch << std::endl;
                    break;
            }
        }
        //std::cout << std::endl;
        return code;
    }

private:
    std::istream &file;
};
