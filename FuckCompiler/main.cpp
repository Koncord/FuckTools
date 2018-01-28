//
// Created by koncord on 28.01.18.
//

#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <FuckComponents/FuckByteCode.hpp>
#include <fstream>
#include <algorithm>
#include <map>

struct ciChar_traits : public std::char_traits<char> {
    static bool eq(char c1, char c2)
    {
        return std::toupper(c1) == std::toupper(c2);
    }

    static bool ne(char c1, char c2)
    {
        return std::toupper(c1) != std::toupper(c2);
    }

    static bool lt(char c1, char c2)
    {
        return std::toupper(c1) < std::toupper(c2);
    }

    static int compare(const char *s1, const char *s2, size_t n)
    {
        while (n-- != 0)
        {
            if (std::toupper(*s1) < std::toupper(*s2)) return -1;
            if (std::toupper(*s1) > std::toupper(*s2)) return 1;
            ++s1;
            ++s2;
        }
        return 0;
    }

    static const char *find(const char *s, int n, char a)
    {
        while (n-- > 0 && std::toupper(*s) != std::toupper(a))
        {
            ++s;
        }
        return s;
    }
};

typedef std::basic_string<char, ciChar_traits> ciString;

class FuckTranslator
{
public:
    explicit FuckTranslator(std::istream &file) : file(file)
    {
    }

    std::vector<Instruction> translate()
    {
        unsigned char ch;
        std::vector<Instruction> code;
        Instruction::Arg arg {0};
        arg.half.arg = 1;

        Instruction::Arg arg2 {0};
        arg2.half.arg = -1;

        while (file >> ch)
        {
            switch ((Instruction::VMOpcode) ch)
            {
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

class FuckOptimizer
{
public:
    typedef std::vector<Instruction> Container;
    typedef Container::iterator Iterator;

    explicit FuckOptimizer(Container &code) : code(code)
    {
    }

    Iterator findEndOfLoop(const Iterator &begin, int *loopsInside = nullptr) const
    {
        int loops = 0;
        int nloop = 0;
        auto it = std::find_if(begin, code.end(), [&nloop, &loops](const auto &ins) {
            if (ins.fn == Instruction::VMOpcode::loopBegin)
            {
                ++loops;
                ++nloop;
            }
            if (ins.fn == Instruction::VMOpcode::loopEnd)
                --nloop;
            return nloop == 0 && ins.fn == Instruction::VMOpcode::loopEnd;
        });

        if(loopsInside != nullptr)
            *loopsInside = loops - 1;
        return it;
    }

    enum class OptLevel : uint8_t
    {
        none = 0,
        basic,
        kidLevel,
        aggressive,
        motherfucker
    };

    void compile(OptLevel opt = OptLevel::motherfucker)
    {
        if(code[0].fn == Instruction::VMOpcode::loopBegin) //remove [] in start because all cells are 0
        {
            auto endOfLoop = findEndOfLoop(code.begin());

            if(endOfLoop != code.end() && ((endOfLoop - code.begin() + 1) <= code.size()))
                code.erase(code.begin(), endOfLoop + 1);
        }

        if(opt >= OptLevel::basic)
        {
            contract(); // contract series of >> or +++ to 1 command
        }
        if(opt >= OptLevel::kidLevel)
        {
            memZero(); // [+] and [-]
            scanMem(); // [>] and [<]
        }
        if(opt >= OptLevel::aggressive)
        {
            copyOrMulLoops();     // [->>+>+<<<]
            copyOrMulLoops(true); // [->>+>+++<<<]
        }
        if(opt >= OptLevel::motherfucker)
        {
            useOffsets();// use offsets instead bunch of > or <
            setAndInc(); // optimize set(k, a) + inc(k, b) to set(k, a + b)
            cloneCell(); // optimize set(k, 0) + addCell(k, v) to movCell(k, v)
        }
        calcLoops();
    }

    // optimize
    //   arr[i + k] = v;
    //   arr[i + k] += 5;
    // to
    //   arr[i + k] = v + 5;
    void setAndInc()
    {
        for (auto it = code.begin(); it != code.end();)
        {
            if(it->fn == Instruction::VMOpcode::inc && it != code.begin())
            {
                auto prev = it - 1;
                if(prev->fn == Instruction::VMOpcode::set && prev->arg.half.offset == it->arg.half.offset)
                {
                    prev->arg.full = it->arg.full;
                    it = code.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    // optimize
    //  arr[i + k] = 0;
    //  arr[i + k] += arr[i + v];
    // to
    //  arr[i + k] = arr[i + v];
    void cloneCell()
    {
        for (auto it = code.begin(); it != code.end();)
        {
            if(it->fn == Instruction::VMOpcode::copy && it != code.begin())
            {
                //auto next = it + 1;
                auto prev = it - 1;
                if(prev->fn == Instruction::VMOpcode::set && prev->arg.half.arg == 0
                   && prev->arg.half.offset == it->arg.half.offset)
                {
                    prev->fn = Instruction::VMOpcode::mov;
                    prev->arg.full = it->arg.full;
                    it = code.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    void useOffsets()
    {
        for (auto it = code.begin(); it != code.end();)
        {
            // find of offsetable instructions
            auto findOfsetable = [this](const Iterator &begin, bool reverse = false) {
                auto offsetable = begin;
                bool run = true;
                while (run && offsetable != code.end())
                {
                    switch (offsetable->fn)
                    {
                        case Instruction::VMOpcode::inc:
                        case Instruction::VMOpcode::incWin:
                        case Instruction::VMOpcode::outChar:
                        case Instruction::VMOpcode::inChar:
                        case Instruction::VMOpcode::set:
                        case Instruction::VMOpcode::copy:
                        //case Instruction::VMOpcode::mul: //not possible without 3rd argument
                            if(reverse)
                                run = false;
                            else
                                ++offsetable;
                            break;
                        default:
                            if(reverse)
                                ++offsetable;
                            else
                                run = false;
                            break;
                    }
                }
                return offsetable;
            };

            auto beginOffsetable = findOfsetable(it, true);
            if(beginOffsetable == code.end())
                break;

            auto endOffsetable = findOfsetable(beginOffsetable, false);
            it = endOffsetable;

            std::cout << "offsetable range [" << beginOffsetable - code.begin()
                      << ", " << endOffsetable - code.begin() << ")"<< std::endl;

            std::vector<Instruction> optimized;
            optimized.reserve(endOffsetable - beginOffsetable);
            Instruction::ArgType p = 0;
            for(auto it2 = beginOffsetable; it2 != endOffsetable; ++it2)
            {
                if(it2->fn == Instruction::VMOpcode::incWin)
                {
                    p += it2->arg.full;
                    continue;
                }

                Instruction ins = *it2;

                if(it2->fn == Instruction::VMOpcode::inc
                   || it2->fn == Instruction::VMOpcode::set)
                {
                    ins.arg.half.offset = p;
                }
                else if(it2->fn == Instruction::VMOpcode::copy)
                {
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
                    ins.arg.half.arg = 1; // use offset version of the outChar or inchar
                    ins.arg.half.offset = p;
                }
                optimized.push_back(ins);
            }

            optimized.shrink_to_fit();

            if(p != 0)
                optimized.emplace_back(Instruction::VMOpcode::incWin, p);

            p = endOffsetable - beginOffsetable;

            auto newIt = code.insert(beginOffsetable, optimized.begin(), optimized.end());
            std::cout << p << " " << newIt + optimized.size() - code.begin() << std::endl;
              it = code.erase(newIt + optimized.size(), newIt + + optimized.size() + p);

            std::cout << "Optimized size: " << optimized.size() << std::endl;
        }
        int i = 0;
    }

    void scanMem() // [>] scanRight and [<] scanLeft
    {

        for (auto it = code.begin(); it != code.end(); ++it)
        {
            if(it->fn == Instruction::VMOpcode::loopBegin && ((it - code.begin()) + 3) <= code.size())
            {
                auto next = ++it;
                if(next->fn == Instruction::VMOpcode::incWin && (++it)->fn == Instruction::VMOpcode::loopEnd)
                {
                    if(std::abs(next->arg.full) == 1)
                    {
                        it = code.erase(it - 2, it);
                        it->fn = Instruction::VMOpcode::scan;
                        it->arg = next->arg;
                    }
                }
            }
        }
    }

    // "[->>+>+<<<]" copy
    // "[->>+>+++<<<]" mul
    void copyOrMulLoops(bool mul = false)
    {
        Iterator begin;
        for (auto it = code.begin(); it != code.end();)
        {

            if(it->fn == Instruction::VMOpcode::loopBegin)
                begin = it;
            else if(it->fn == Instruction::VMOpcode::loopEnd)
            {
                //check for simple operations "><+-"
                bool simple = true;
                for (auto it2 = begin + 1; it2 != it;)
                {
                    switch (it2->fn)
                    {
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
                if (!simple)
                {
                    ++it;
                    continue;
                }

                // call the loop track pointer position and what arithmetic operations it carries out

                std::map<Instruction::ArgType, Instruction::CellType> cells;
                Instruction::ArgType p = 0;
                for (auto it2 = begin + 1; it2 != it; ++it2)
                {
                    if(it2->fn == Instruction::VMOpcode::inc)
                    {
                        cells[p] += it2->arg.half.arg;
                    }
                    else if(it2->fn == Instruction::VMOpcode::incWin)
                        p += it2->arg.full;
                }


                // if pointer ended up where it started (cell 0) and we subtracted exactly 1 from cell 0,
                // then this loop can be replaced with a Mul instruction

                if(p != 0 or cells[0] != -1)
                {
                    ++it;
                    continue;
                }

                cells.erase(0);


                bool isCopy = true;

                for (auto &cell : cells)
                {
                    if(cell.second != 1)
                    {
                        isCopy = false;
                        break;
                    }
                }
                if(!(mul || isCopy))
                {
                    ++it;
                    continue;
                }

                //replace the loop with copy operation

                std::cout << (char)(begin->fn) << std::endl;
                for (auto &cell : cells)
                {
                    if(mul)
                    {
                        begin->fn = Instruction::VMOpcode::mul;
                        begin->arg.half.offset = cell.first;
                        begin->arg.half.arg = cell.second;
                        std::cout << "multiple " << cell.first << std::endl;
                    }
                    else
                    {
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


    void contract()
    {
        auto prevInstr = code.begin();
        for (auto it = code.begin() + 1; it != code.end();)
        {
            switch (it->fn)
            {

                case Instruction::VMOpcode::inc:
                case Instruction::VMOpcode::sub:
                    if (it->fn == prevInstr->fn)
                    {
                        prevInstr->arg.half.offset += it->arg.half.offset; // combine inc
                        prevInstr->arg.half.arg += it->arg.half.arg; // combine inc
                        it = code.erase(it);
                        break;
                    }
                case Instruction::VMOpcode::incWin:
                case Instruction::VMOpcode::decWin:
                    if (it->fn == prevInstr->fn)
                    {
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

    void memZero() // [-] and [+] to arr[i] = 0
    {
        for (auto it = code.begin(); it != code.end(); ++it)
        {
            if(it->fn == Instruction::VMOpcode::loopBegin && ((it - code.begin()) + 3) <= code.size())
            {
                if((++it)->fn == Instruction::VMOpcode::inc && (++it)->fn == Instruction::VMOpcode::loopEnd)
                {
                    it = code.erase(it-2, it);
                    it->fn = Instruction::VMOpcode::set;
                    it->arg.full = 0;
                }
            }
        }
    }

    void calcLoops()
    {
        for (auto it = code.begin() + 1; it != code.end(); ++it)
        {
            if (it->fn != Instruction::VMOpcode::loopBegin)
                continue;

            int skip = 0;
            for (auto it2 = it + 1; it2 != code.end(); ++it2)
            {
                if (it2->fn == Instruction::VMOpcode::loopBegin)
                    ++skip;

                if (it2->fn == Instruction::VMOpcode::loopEnd)
                {
                    if (skip > 0)
                        --skip;
                    else
                    {
                        it2->arg.full = static_cast<Instruction::ArgType>(it - code.begin());
                        it->arg.full = static_cast<Instruction::ArgType>((it2 - code.begin()) + 1);
                        break;
                    }
                }
            }
        }
    }


private:
    std::vector<Instruction> &code;
};

int main(int argc, char **argv)
{
    auto _error = [](const char *msg){
        std::cerr << "fuckc: " << msg << "\nexecution interrupted." << std::endl;
        exit(1);
    };

    if (argc < 2)
        _error("no input file");



    FuckOptimizer::OptLevel optLevel = FuckOptimizer::OptLevel::none;

    if(argc >= 4)
    {
        ciString optLevelStr = argv[3];

        if (optLevelStr == "basic")
            optLevel = FuckOptimizer::OptLevel::basic;
        else if (optLevelStr == "kidlevel")
            optLevel = FuckOptimizer::OptLevel::kidLevel;
        else if (optLevelStr == "aggressive")
            optLevel = FuckOptimizer::OptLevel::aggressive;
        else if (optLevelStr == "motherfucker")
            optLevel = FuckOptimizer::OptLevel::motherfucker;
    }

    std::ifstream file(argv[1]);

    if (!file.is_open())
        _error("fatal error: file not found");

    FuckTranslator translator(file); // basic translations

    auto code = translator.translate();

    for (const auto &c : code)
        std::cout << "\"" << (char) c.fn << "\" arg: " << c.arg.full << std::endl;

    std::cout << "--------------------------------------------------\n";
    FuckOptimizer optimizer(code); // compiler and optimizer in 1
    optimizer.compile(optLevel);
    for (const auto &c : code)
        std::cout << "\"" << (char) c.fn << "\" arg: " << c.arg.full << std::endl;

    std::cout << "--------------------------------------------------\n";

    for (const auto &c : code)
        std::cout << "\"" << (char) c.fn << "\" arg: " << c.arg.full << std::endl;

    std::string outfile;
    if(argc < 3)
    {
        outfile = argv[1];
        outfile = outfile.substr(0, outfile.rfind('.')) + ".bfc";
    }
    else
        outfile = argv[2];

    Instruction::save(outfile, code);

    return 0;
}
