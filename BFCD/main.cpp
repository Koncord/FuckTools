//
// Created by Stanislav "Koncord" Zhukov on 23.08.2021.
//

#include <iostream>
#include <vector>
#include <fstream>
#include <FuckComponents/FuckByteCode.hpp>
#include <functional>

int main(int argc, char **argv) {
    auto _error = [](const char *msg) {
        std::cerr << "Fuck decompiler: " << msg << "\nexecution interrupted." << std::endl;
        exit(1);
    };
    if (argc < 2)
        _error("no input file");

    std::ifstream bfc(argv[1], std::ios::binary);
    if (!bfc.is_open())
        _error("fatal error: file not found");
    auto codes = Instruction::load(bfc);
    if (codes.empty())
        _error("error: unknown file format");

    bfc.close();

    std::string outfile;
    if (argc < 3) {
        outfile = argv[1];
        outfile = outfile.substr(0, outfile.rfind('.')) + ".bfd";
    } else
        outfile = argv[2];

    std::ofstream bfcode(outfile);

    auto pushStr = [&bfcode](std::string const &str) {
        static int cnt = 0;
        for (char i : str) {
            if (cnt == 80) {
                cnt = 0;
                bfcode << "\n";
            }
            bfcode << i;
            ++cnt;
        }
    };

    auto winMov = [&pushStr](Instruction const &code, std::function<void (Instruction const &)> const &fn) {
        for (int i = 0; i < abs(code.arg.half.offset); ++i)
            pushStr(code.arg.half.offset < 0 ? "<" : ">");
        fn(code);
        for (int i = 0; i < abs(code.arg.half.offset); ++i)
            pushStr(code.arg.half.offset < 0 ? ">" : "<");
    };

    for (auto const &code : codes) {
        switch (code.fn) {
            case Instruction::VMOpcode::invalid:
                break;
            case Instruction::VMOpcode::inc:
            case Instruction::VMOpcode::sub:
                winMov(code, [&pushStr](auto const &code) {
                    for (int i = 0; i < abs(code.arg.half.arg); ++i)
                        pushStr(code.arg.half.arg > 0 ? "+" : "-");
                });
                break;
            case Instruction::VMOpcode::incWin:
            case Instruction::VMOpcode::decWin:
                for (int i = 0; i < abs(code.arg.full); ++i)
                    pushStr(code.arg.full < 0 ? "<" : ">");
                break;
            case Instruction::VMOpcode::outChar:
                if (code.arg.half.arg == -1)
                    pushStr(".");
                else {
                    winMov(code, [&pushStr](auto const &code) {
                        for (int i = 0; i < abs(code.arg.half.arg); ++i)
                            pushStr(".");
                    });
                }
                break;
            case Instruction::VMOpcode::inChar:
                if (code.arg.half.arg == -1)
                    pushStr(",");
                else {
                    winMov(code, [&pushStr](auto const &code) {
                        for (int i = 0; i < abs(code.arg.half.arg); ++i)
                            pushStr(".");
                    });
                }
            case Instruction::VMOpcode::loopBegin:
                pushStr("[");
                break;
            case Instruction::VMOpcode::loopEnd:
                pushStr("]");
                break;
            case Instruction::VMOpcode::set:
                if (code.arg.full == 0) {
                    pushStr("[-]");
                } else {
                    winMov(code, [&pushStr](auto const &code) {
                        pushStr("[-]");
                        for (int i = 0; i < abs(code.arg.half.arg); ++i)
                            pushStr(code.arg.half.arg > 0 ? "+" : "-");
                    });
                }
                break;
            case Instruction::VMOpcode::copy:
                throw std::runtime_error("copy not implemented");
                break;
            case Instruction::VMOpcode::mul:
                /*if (code.arg.half.offset == 0) {
                    pushStr("[");

                    pushStr("]");
                }*/
                throw std::runtime_error("mul not implemented");
                break;
            case Instruction::VMOpcode::scan:
                pushStr(code.arg.full < 0 ? "[<]" : "[>]");
                break;
            case Instruction::VMOpcode::mov:
                throw std::runtime_error("mov not implemented");
                break;
        }
    }
}
