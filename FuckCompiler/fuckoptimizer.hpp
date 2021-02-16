//
// Created by Stanislav "Koncord" Zhukov on 16.02.2021.
//

# pragma once

#include <vector>
#include "FuckComponents/FuckByteCode.hpp"

class FuckOptimizer {
public:
    typedef std::vector<Instruction> Container;
    typedef Container::iterator Iterator;

    explicit FuckOptimizer(Container &code);

    Iterator findEndOfLoop(const Iterator &begin, int *loopsInside = nullptr) const;

    enum class OptLevel : uint8_t {
        none = 0,
        basic,
        kidLevel,
        aggressive,
        motherfucker
    };

    void compile(OptLevel opt = OptLevel::motherfucker);

    // optimize
    //   arr[i + k] = v;
    //   arr[i + k] += 5;
    // to
    //   arr[i + k] = v + 5;
    void setAndInc();

    // optimize
    //  arr[i + k] = 0;
    //  arr[i + k] += arr[i + v];
    // to
    //  arr[i + k] = arr[i + v];
    void cloneCell();

    void useOffsets();

    // [>] scanRight and [<] scanLeft
    void scanMem();

    // "[->>+>+<<<]" copy
    // "[->>+>+++<<<]" mul
    void copyOrMulLoops(bool mul = false);

    void contract();

    void memZero();

    void calcLoops();


private:
    std::vector<Instruction> &code;
};
