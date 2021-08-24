//
// Created by koncord on 28.01.18.
//

#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <FuckComponents/FuckByteCode.hpp>
#include <fstream>

int main(int argc, char **argv) {
    auto _error = [](const char *msg) {
        std::cerr << "fuck C compiler: " << msg << "\nexecution interrupted." << std::endl;
        exit(1);
    };

    if (argc < 2)
        _error("no input file");

    std::vector<Instruction> codes;

    std::ifstream bfc(argv[1], std::ios::binary);
    if (!bfc.is_open())
        _error("fatal error: file not found");
    codes = Instruction::load(bfc);
    if (codes.empty())
        _error("error: unknown file format");

    bfc.close();

    std::string outfile;
    if (argc < 3) {
        outfile = argv[1];
        outfile = outfile.substr(0, outfile.rfind('.')) + ".c";
    } else
        outfile = argv[2];

    std::ofstream ccode(outfile);

    int tablvl = 1;

    auto insertTabs = [&ccode](int tablvl) {
        for (int i = 0; i < tablvl; ++i)
            ccode << "    ";
    };

    ccode << "#include <stdio.h>\n#include <string.h>\n#include <stdlib.h>\n\n";
    ccode << R"(void *my_memrchr(const void *s, int c, size_t n) {
    if (n > 0) {
        const char *p = (const char *) s;
        const char *q = p + n;
        while (1) {
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
        }
        if (q >= p) return (void *) q;
    }
    return NULL;
}
)" << std::endl;
    ccode << "int main()\n{" << std::endl;
    insertTabs(tablvl);
    ccode << "int i = 0;\n";
    insertTabs(tablvl);
    //ccode << "char arr[" << Instruction::maxCells << "];\n";
    ccode << "char *arr = (char *) calloc(" << Instruction::maxCells << ", 1);\n";
    //insertTabs(tablvl);
    //ccode << "memset(arr, 0, sizeof(arr));" << std::endl;
    //ccode << "memset(arr, 0," << Instruction::maxCells <<");" << std::endl;

    auto pushOffset = [&ccode](Instruction code, bool space = true) {
        if (code.arg.half.offset == 0)
            ccode << "arr[i]";
        else if (code.arg.half.offset > 0)
            ccode << "arr[i + " << code.arg.half.offset << "]";
        else
            ccode << "arr[i - " << -code.arg.half.offset << "]";
        if (space)
            ccode << " ";
    };

    for (auto it = codes.begin(); it != codes.end(); ++it) {
        auto const &code = *it;
        if (code.fn == Instruction::VMOpcode::loopBegin)
            insertTabs(tablvl++);
        else if (code.fn == Instruction::VMOpcode::loopEnd)
            insertTabs(--tablvl);
        else
            insertTabs(tablvl);

        switch (code.fn) {
            case Instruction::VMOpcode::inc: {
                pushOffset(code);

                if (code.arg.half.arg > 0)
                    ccode << "+= " << (int) code.arg.half.arg << ";" << std::endl;
                else
                    ccode << "-= " << (int) -code.arg.half.arg << ";" << std::endl;
                break;
            }
            case Instruction::VMOpcode::incWin:
                if (code.arg.full > 0)
                    ccode << "i += " << code.arg.full << ";" << std::endl;
                else
                    ccode << "i -= " << -code.arg.full << ";" << std::endl;
                break;
            case Instruction::VMOpcode::outChar:
                if (code.arg.half.arg == -1)
                    ccode << "putchar(arr[i]);" << std::endl;
                else {
                    ccode << "putchar(arr[i";
                    if (code.arg.half.offset > 0)
                        ccode << " + " << code.arg.half.offset << "]);" << std::endl;
                    else
                        ccode << " - " << -code.arg.half.offset << "]);" << std::endl;
                }
                break;
            case Instruction::VMOpcode::inChar:
                if (code.arg.half.arg == -1)
                    ccode << "arr[i] = getchar();" << std::endl;
                else {
                    if (code.arg.half.offset > 0)
                        ccode << "arr[i + " << code.arg.half.offset << "]";
                    else
                        ccode << "arr[i - " << -code.arg.half.offset << "]";
                    ccode << " = getchar();" << std::endl;
                }
                break;
            case Instruction::VMOpcode::loopBegin: {
                auto next = (it + 1);
                // optimizes "while(arr[i]) {i += n;}" construction to one line
                if (next->fn == Instruction::VMOpcode::incWin) {
                    auto next2 = (it + 2);
                    if (next2->fn == Instruction::VMOpcode::loopEnd) {
                        ccode << "for (; arr[i]; ";
                        if (next->arg.full > 0)
                            ccode << "i += " << next->arg.full;
                        else
                            ccode << "i -= " << -next->arg.full;
                        ccode << ");" << std::endl;
                        it += 2;
                        tablvl--;
                        break;
                    }
                }
                // todo: optimize while (arr[i]) { /*code*/ i += n;} to the for-loop
                ccode << "while (arr[i]) {" << std::endl;
            }
                break;
            case Instruction::VMOpcode::loopEnd:
                ccode << "}" << std::endl;
                break;
            case Instruction::VMOpcode::set: {
                // concatenate series of nullsets to single memset
                auto val = (int) code.arg.half.arg;
                auto startOff = code.arg.half.offset;
                auto toIt = it;
                int cnt = 1;
                for (auto it2 = std::next(it); it2 != codes.end(); ++it2) {
                    auto code2 = *it2;
                    if (code2.fn == Instruction::VMOpcode::set) {
                        if (code2.arg.half.arg == val && code2.arg.half.offset == ++startOff) {
                            toIt = it2;
                            cnt++;
                        }
                        else
                            break;
                    } else {
                        break;
                    }
                }
                if (toIt != it) {
                    ccode << "memset(&";
                    pushOffset(code, false);
                    ccode << ", " << val << ", " << cnt << ");" << std::endl;
                    it = toIt;
                    break;
                }
                pushOffset(code);
                ccode << "= " << (int) code.arg.half.arg << ";" << std::endl;
            }
                break;
            case Instruction::VMOpcode::mov:
            case Instruction::VMOpcode::copy:
                pushOffset(code);
                if (code.fn == Instruction::VMOpcode::copy)
                    ccode << "+";
                ccode << "= arr[i";
                if (code.arg.half.arg != 0)
                    ccode << " + " << (int) code.arg.half.arg;
                ccode << "];" << std::endl;
                break;
            case Instruction::VMOpcode::mul: {
                pushOffset(code);
                if (code.arg.half.arg == -1)
                    ccode << "-= ";
                else
                    ccode << "+= ";
                ccode << "arr[i]";
                if (std::abs(code.arg.half.arg) != 1)
                    ccode << " * " << (int) code.arg.half.arg;
                ccode << ";" << std::endl;
                break;
            }
            case Instruction::VMOpcode::scan:
                if (code.arg.full < 0)
                    ccode << "i -= (int)((void *)(arr + i) - my_memrchr(arr, 0, i + 1));" << std::endl;
                else
                    ccode << "i += (int)(memchr(arr + i, 0, sizeof(arr)) - (void *)(arr + i));" << std::endl;
                break;
            default:
                return 1;
        }
    }

    insertTabs(tablvl);
    ccode << "free(arr);\n";
    insertTabs(tablvl);
    ccode << "return 0;\n}" << std::endl;

    ccode.close();

    return 0;
}
