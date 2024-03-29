//
// Created by koncord on 28.01.18.
//

#include <iostream>
#include <vector>
#include <FuckComponents/FuckByteCode.hpp>
#include <fstream>
#include <cstring>

// todo: check in CMake for memchr
#if 1

static void *my_memrchr(const void *s, int c, size_t n) noexcept {
    if (n > 0) {
        const char *p = (const char *) s;
        const char *q = p + n;
        while (true) {
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
            if (--q < p || q[0] == c) break;
        }
        if (q >= p)
            return (void *) q;
    }
    return nullptr;
}

#endif

class Machine {
public:
    Instruction::ArgType currentCellN;
    Instruction::ArgType currentCodeN;
public:
    Instruction::CellType *currentCell;
    std::vector<Instruction> code;
    std::vector<Instruction::CellType> cells;
    //char cells[Instruction::maxCells] {0};

    Machine() noexcept: currentCellN(0), currentCodeN(0) {
        cells.resize(Instruction::maxCells);
        currentCell = &cells[0];
    }

    Instruction *getCodeCell() noexcept {
        if (currentCodeN >= code.size())
            return nullptr;
        return &code[currentCodeN++];
    }

    void JmpCode(Instruction::ArgType n) noexcept {
        currentCodeN = n;
        if (currentCodeN > code.size())
            currentCodeN = 0;
        else if (currentCodeN < 0)
            currentCodeN = static_cast<Instruction::ArgType>(code.size());
    }

    void moveWindow(Instruction::ArgType n) noexcept {
        currentCellN += n;
        /*if (currentCellN > cells.size())
            currentCellN = 0;
        else if (currentCellN < 0)
            currentCellN = static_cast<Instruction::ArgType>(cells.size());*/
        currentCell = &cells[currentCellN];
    }
};

class CMDUnit {
public:
    explicit CMDUnit(const std::vector<Instruction> &codes) noexcept {
        machine.code = codes;
    }

    void Process() noexcept {
        for (;;) {
            auto cellCode = machine.getCodeCell();
            if (cellCode == nullptr) break;
            switch (cellCode->fn) {

                case Instruction::VMOpcode::inc:
                    *(machine.currentCell + cellCode->arg.half.offset) += cellCode->arg.half.arg;
                    break;
                case Instruction::VMOpcode::incWin:
                    machine.moveWindow(cellCode->arg.full);
                    break;
                case Instruction::VMOpcode::outChar: {
                    if (cellCode->arg.half.arg == -1)
                        putchar(*machine.currentCell);
                    else
                        putchar(*(machine.currentCell + cellCode->arg.half.offset));
                    break;
                }
                case Instruction::VMOpcode::inChar: {
                    int ch = getchar();
                    if (ch == EOF)
                        std::exit(0);
                    if (cellCode->arg.half.arg == -1)
                        *machine.currentCell = static_cast<Instruction::CellType>(ch);
                    else
                        *(machine.currentCell + cellCode->arg.half.offset) = static_cast<Instruction::CellType>(ch);
                    break;
                }
                case Instruction::VMOpcode::loopBegin:
                    if (*machine.currentCell != 0) break;
                case Instruction::VMOpcode::loopEnd:
                    machine.JmpCode(cellCode->arg.full);
                    break;
                case Instruction::VMOpcode::set:
                    *(machine.currentCell + cellCode->arg.half.offset) =
                            static_cast<Instruction::CellType>(cellCode->arg.half.arg);
                    break;
                case Instruction::VMOpcode::copy: {
                    *(machine.currentCell + cellCode->arg.half.offset) +=
                            *(machine.currentCell + cellCode->arg.half.arg);
                    break;
                }
                case Instruction::VMOpcode::mov:
                    *(machine.currentCell + cellCode->arg.half.offset) =
                            *(machine.currentCell + cellCode->arg.half.arg);
                    break;
                case Instruction::VMOpcode::mul: {
                    *(machine.currentCell + cellCode->arg.half.offset) +=
                            (*machine.currentCell) * cellCode->arg.half.arg;
                    break;
                }
                case Instruction::VMOpcode::scan: {
                    char *ptr = (&machine.cells[0] + machine.currentCellN);
                    Instruction::ArgType v;
                    if (cellCode->arg.full < 0)
                        v = -(ptr - (char *) my_memrchr(&machine.cells[0], 0, machine.currentCellN + 1));
                    else
                        v = (char *) memchr(&machine.cells[0] + machine.currentCellN, 0, Instruction::maxCells) - ptr;
                    machine.moveWindow(v);
                    break;
                }
                case Instruction::VMOpcode::memset:
                    memset(&machine.currentCell[cellCode->arg.half.offset],
                           static_cast<Instruction::CellType>(cellCode->arg.half.arg),
                           static_cast<Instruction::CellType>(cellCode->arg.half.arg2));
                    break;
                default:
                    break;
            }
        }
    }

private:
    Machine machine;
};

int main(int argc, char **argv) noexcept {
    auto _error = [](const char *msg) {
        std::cerr << "fuckmachine: " << msg << "\nexecution interrupted." << std::endl;
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

    CMDUnit cmduint(codes);
    cmduint.Process();
    return 0;
}
