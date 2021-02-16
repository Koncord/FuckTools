//
// Created by koncord on 28.01.18.
//

#include <iostream>
#include <fstream>

#include "cistring.hpp"
#include "fucktranslator.hpp"
#include "fuckoptimizer.hpp"

int main(int argc, char **argv) {
    auto _error = [](const char *msg) {
        std::cerr << "fuckc: " << msg << "\nexecution interrupted." << std::endl;
        exit(1);
    };

    if (argc < 2)
        _error("no input file");


    FuckOptimizer::OptLevel optLevel = FuckOptimizer::OptLevel::none;

    if (argc >= 4) {
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


    std::cout << "-----------------[NOT OPTIMIZED]-----------------\n";
    for (const auto &c : code)
        std::cout << "\"" << (char) c.fn << "\" arg: " << c.arg.full << std::endl;

    std::cout << "-------------[APPLIED OPTIMIZATIONS]-------------\n";
    FuckOptimizer optimizer(code); // compiler and optimizer in 1
    optimizer.compile(optLevel);

    std::cout << "-------------------[OPTIMIZED]-------------------\n";

    for (const auto &c : code)
        std::cout << "\"" << (char) c.fn << "\" arg: " << c.arg.full << std::endl;

    std::string outfile;
    if (argc < 3) {
        outfile = argv[1];
        outfile = outfile.substr(0, outfile.rfind('.')) + ".bfc";
    } else
        outfile = argv[2];

    Instruction::save(outfile, code);

    return 0;
}
