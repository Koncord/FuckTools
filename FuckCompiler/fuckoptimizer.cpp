//
// Created by Stanislav "Koncord" Zhukov on 16.02.2021.
//

#include "fuckoptimizer.hpp"
#include <iostream>
#include <map>

FuckOptimizer::FuckOptimizer(Container &code) : code(code) {
}

FuckOptimizer::Iterator FuckOptimizer::findEndOfLoop(Iterator const &begin, int *loopsInside) const {
    int loops = 0;
    int nloop = 0;
    auto it = std::find_if(begin, code.end(), [&nloop, &loops](const auto &ins) {
        if (ins.fn == Instruction::VMOpcode::loopBegin) {
            ++loops;
            ++nloop;
        }
        if (ins.fn == Instruction::VMOpcode::loopEnd)
            --nloop;
        return nloop == 0 && ins.fn == Instruction::VMOpcode::loopEnd;
    });

    if (loopsInside != nullptr)
        *loopsInside = loops - 1;
    return it;
}

void FuckOptimizer::compile(FuckOptimizer::OptLevel opt) {
    if (code[0].fn == Instruction::VMOpcode::loopBegin) //remove [] in start because all cells are 0
    {
        auto endOfLoop = findEndOfLoop(code.begin());

        if (endOfLoop != code.end() && ((endOfLoop - code.begin() + 1) <= code.size()))
            code.erase(code.begin(), endOfLoop + 1);
    }

    if (opt >= OptLevel::basic) {
        contract(); // contract series of >> or +++ to 1 command
    }
    if (opt >= OptLevel::kidLevel) {
        incToSet(); // change INC to SET where it's possible
        memZero(); // [+] and [-]
        scanMem(); // [>] and [<]
    }
    if (opt >= OptLevel::aggressive) {
        copyOrMulLoops();     // [->>+>+<<<]
        copyOrMulLoops(true); // [->>+>+++<<<]
    }
    if (opt >= OptLevel::motherfucker) {
        useOffsets();// use offsets instead bunch of > or <
        setAndInc(); // optimize set(k, a) + inc(k, b) to set(k, a + b)
        cloneCell(); // optimize set(k, 0) + addCell(k, v) to movCell(k, v)
    }
    calcLoops();
}

void FuckOptimizer::setAndInc() {
    for (auto it = code.begin(); it != code.end();) {
        // look for INC opcode that is not first
        if (it->fn == Instruction::VMOpcode::inc && it != code.begin()) {
            auto prev = it - 1;
            // if the previous opcode was SET with the same offset, shrink it
            if (prev->fn == Instruction::VMOpcode::set && prev->arg.half.offset == it->arg.half.offset) {
                prev->arg.full = it->arg.full;
                it = code.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void FuckOptimizer::cloneCell() {
    for (auto it = code.begin(); it != code.end();) {
        if (it->fn == Instruction::VMOpcode::copy && it != code.begin()) {
            //auto next = it + 1;
            auto prev = it - 1;
            if (prev->fn == Instruction::VMOpcode::set && prev->arg.half.arg == 0
                && prev->arg.half.offset == it->arg.half.offset) {
                prev->fn = Instruction::VMOpcode::mov;
                prev->arg.full = it->arg.full;
                it = code.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void FuckOptimizer::useOffsets() {
    for (auto it = code.begin(); it != code.end();) {
        // find of offsetable instructions
        auto findOfsetable = [this](const Iterator &begin, bool reverse = false) {
            auto offsetable = begin;
            bool run = true;
            while (run && offsetable != code.end()) {
                switch (offsetable->fn) {
                    case Instruction::VMOpcode::inc:
                    case Instruction::VMOpcode::incWin:
                    case Instruction::VMOpcode::outChar:
                    case Instruction::VMOpcode::inChar:
                    case Instruction::VMOpcode::set:
                    case Instruction::VMOpcode::copy:
                        //case Instruction::VMOpcode::mul: //not possible without 3rd argument
                        if (reverse)
                            run = false;
                        else
                            ++offsetable;
                        break;
                    default:
                        if (reverse)
                            ++offsetable;
                        else
                            run = false;
                        break;
                }
            }
            return offsetable;
        };

        auto beginOffsetable = findOfsetable(it, true);
        if (beginOffsetable == code.end())
            break;

        auto endOffsetable = findOfsetable(beginOffsetable, false);
        it = endOffsetable;

        std::cout << "offsetable range [" << beginOffsetable - code.begin()
                  << ", " << endOffsetable - code.begin() << ")" << std::endl;

        std::vector<Instruction> optimized;
        optimized.reserve(endOffsetable - beginOffsetable);
        Instruction::ArgType p = 0;
        for (auto it2 = beginOffsetable; it2 != endOffsetable; ++it2) {
            if (it2->fn == Instruction::VMOpcode::incWin) {
                p += it2->arg.full;
                continue;
            }

            Instruction ins = *it2;

            if (it2->fn == Instruction::VMOpcode::inc || it2->fn == Instruction::VMOpcode::set) {
                ins.arg.half.offset = p;
            } else if (it2->fn == Instruction::VMOpcode::copy) {
                Instruction::Arg tmp = ins.arg;
                ins.arg.half.offset += p;
                ins.arg.half.arg = p;
            }
                /*else if(it2->fn == Instruction::VMOpcode::mul)
                {
                    Instruction::Arg tmp = ins.arg;
                    ins.arg.half.offset += p;
                    ins.arg.half.arg = p;
                }*/
            else // outChar, inChar
            {
                if (p != 0) {
                ins.arg.half.arg = 1; // use offset version of the outChar or inchar
                ins.arg.half.offset = p;
            }
            }
            optimized.push_back(ins);
        }

        optimized.shrink_to_fit();

        if (p != 0)
            optimized.emplace_back(Instruction::VMOpcode::incWin, p);

        p = endOffsetable - beginOffsetable;

        auto newIt = code.insert(beginOffsetable, optimized.begin(), optimized.end());
        std::cout << p << " " << newIt + optimized.size() - code.begin() << std::endl;
        it = code.erase(newIt + optimized.size(), newIt + +optimized.size() + p);

        std::cout << "Optimized size: " << optimized.size() << std::endl;
    }
    int i = 0;
}

void FuckOptimizer::scanMem() {
    // TODO: FIX CRASH ON hello.bf
    for (auto it = code.begin(); it != code.end(); ++it) {
        auto sz = ((it - code.begin()) + 3);
        if (it->fn == Instruction::VMOpcode::loopBegin && sz <= code.size()) {
            auto next = ++it;
            if (next->fn == Instruction::VMOpcode::incWin && (++it)->fn == Instruction::VMOpcode::loopEnd) {
                if (std::abs(next->arg.full) == 1) {
                    it = code.erase(it - 2, it);
                    it->fn = Instruction::VMOpcode::scan;
                    it->arg = next->arg;
                }
            }
        }
    }
}

void FuckOptimizer::copyOrMulLoops(bool mul) {
    Iterator begin;
    for (auto it = code.begin(); it != code.end();) {

        if (it->fn == Instruction::VMOpcode::loopBegin)
            begin = it;
        else if (it->fn == Instruction::VMOpcode::loopEnd) {
            //check for simple operations "><+-"
            bool simple = true;
            for (auto it2 = begin + 1; it2 != it;) {
                switch (it2->fn) {
                    case Instruction::VMOpcode::inc:
                    case Instruction::VMOpcode::incWin:
                        ++it2;
                        break;
                    default:
                        simple = false;
                        it2 = it;
                        break;
                }
            }
            if (!simple) {
                ++it;
                continue;
            }

            // call the loop track pointer position and what arithmetic operations it carries out

            std::map<Instruction::ArgType, Instruction::CellType> cells;
            Instruction::ArgType p = 0;
            for (auto it2 = begin + 1; it2 != it; ++it2) {
                if (it2->fn == Instruction::VMOpcode::inc) {
                    cells[p] += it2->arg.half.arg;
                } else if (it2->fn == Instruction::VMOpcode::incWin)
                    p += it2->arg.full;
            }


            // if pointer ended up where it started (cell 0) and we subtracted exactly 1 from cell 0,
            // then this loop can be replaced with a Mul instruction

            if (p != 0 || cells[0] != -1) {
                ++it;
                continue;
            }

            cells.erase(0);


            bool isCopy = true;

            for (auto &cell : cells) {
                if (cell.second != 1) {
                    isCopy = false;
                    break;
                }
            }
            if (!(mul || isCopy)) {
                ++it;
                continue;
            }

            //replace the loop with copy operation

            std::cout << (char) (begin->fn) << std::endl;
            for (auto &cell : cells) {
                if (mul) {
                    begin->fn = Instruction::VMOpcode::mul;
                    begin->arg.half.offset = cell.first;
                    begin->arg.half.arg = cell.second;
                    std::cout << "multiple " << cell.first << std::endl;
                } else {
                    begin->fn = Instruction::VMOpcode::copy;
                    begin->arg.half.offset = cell.first;
                    begin->arg.half.arg = 0;
                    std::cout << "copy " << cell.first << std::endl;
                }
                ++begin;
            }
            begin->fn = Instruction::VMOpcode::set;
            begin->arg.full = 0;
            ++begin;
            it = code.erase(begin, it + 1);
            continue;
        }

        ++it;
    }
}

void FuckOptimizer::contract() {
    auto prevInstr = code.begin();
    for (auto it = code.begin() + 1; it != code.end();) {
        switch (it->fn) {

            case Instruction::VMOpcode::inc:
            case Instruction::VMOpcode::sub:
                if (it->fn == prevInstr->fn) {
                    prevInstr->arg.half.offset += it->arg.half.offset; // combine inc
                    prevInstr->arg.half.arg += it->arg.half.arg; // combine inc
                    it = code.erase(it);
                    break;
                }
            case Instruction::VMOpcode::incWin:
            case Instruction::VMOpcode::decWin:
                if (it->fn == prevInstr->fn) {
                    prevInstr->arg.full += it->arg.full; // combine inc
                    it = code.erase(it);
                    break;
                }
            default:
                prevInstr = it;
                ++it;
                break;
        }
    }
}

void FuckOptimizer::memZero() // [-] and [+] to arr[i] = 0
{
    for (auto it = code.begin(); it != code.end(); ++it) {
        if (it->fn == Instruction::VMOpcode::loopBegin && ((it - code.begin()) + 3) <= code.size()) {
            if ((++it)->fn == Instruction::VMOpcode::inc && (++it)->fn == Instruction::VMOpcode::loopEnd) {
                it = code.erase(it - 2, it);
                it->fn = Instruction::VMOpcode::set;
                it->arg.full = 0;
            }
        }
    }
}

void FuckOptimizer::calcLoops() {
    for (auto it = code.begin() + 1; it != code.end(); ++it) {
        if (it->fn != Instruction::VMOpcode::loopBegin)
            continue;

        int skip = 0;
        for (auto it2 = it + 1; it2 != code.end(); ++it2) {
            if (it2->fn == Instruction::VMOpcode::loopBegin)
                ++skip;

            if (it2->fn == Instruction::VMOpcode::loopEnd) {
                if (skip > 0)
                    --skip;
                else {
                    it2->arg.full = static_cast<Instruction::ArgType>(it - code.begin());
                    it->arg.full = static_cast<Instruction::ArgType>((it2 - code.begin()) + 1);
                    break;
                }
            }
        }
    }
}

void FuckOptimizer::incToSet() {
    for (auto it = code.begin(); it != code.end(); ++it) {
        if (it->fn != Instruction::VMOpcode::inc)
            continue;
        bool canChange = true;
        auto it2 = code.begin();
        // TODO: look for previous operations that emmit current offset
        for (; it2 != it; ++it2) {
            if (it->arg.half.offset == it2->arg.half.offset) {
                canChange = false;
                break;
            }
        }
        if (!canChange)
            continue;

        it->fn = Instruction::VMOpcode::set;
        // no any changes of this offset
        if (it2 == it) {
            continue;
        }

        // no major changes of value, so just add them
        it->arg.half.arg += it2->arg.half.arg;
        auto arg = it->arg;
    }
}
