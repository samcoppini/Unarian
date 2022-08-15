//
//  Copyright 2022 Sam Coppini
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "interpreter.hpp"
#include "optimizer.hpp"
#include "parser.hpp"

#include "CLI/CLI.hpp"

#include <iostream>
#include <fstream>

void printResult(const std::optional<unacpp::BigInt> &result) {
    if (result == std::nullopt) {
        std::cout << "-\n";
    }
    else {
        std::cout << *result << '\n';
    }
}

void runInterpreter(const unacpp::BytecodeModule &bytecode, bool readInput) {
    if (readInput) {
        while (std::cin) {
            unacpp::BigInt num;
            if (std::cin >> num) {
                printResult(unacpp::getResult(bytecode, num));
            }
        }
    }
    else {
        printResult(unacpp::getResult(bytecode, 0));
    }
}

int main(int argc, char **argv) {
    std::string filename;
    std::string expr = "main";
    bool readInput = false;
    bool debugMode = false;
    bool outputBytecode = false;

    CLI::App app{"An interpreter for Unarian"};

    app.add_option("file", filename, "The unarian file to interpret.")
       ->check(CLI::ExistingFile);

    app.add_flag("-g,--debug", debugMode, "Enables debug printing with the ! command.");

    app.add_option("-e,--expr", expr, "The expression to evaluate.");

    app.add_flag("-i,--input", readInput, "Uses input from stdin as input to the evaluated expression.");

    app.add_flag("-b,--bytecode", outputBytecode, "Outputs the bytecode generated from the unarian file.");

    CLI11_PARSE(app, argc, argv);

    std::ifstream file{filename};
    if (!file.is_open() && !filename.empty()) {
        std::cerr << "Unable to open " << filename << '\n';
        return 1;
    }

    std::string fileContents;

    if (!filename.empty()) {
        std::stringstream fileStream;
        fileStream << file.rdbuf();
        fileContents = fileStream.str();
    }

    unacpp::Parser parser{fileContents, expr};
    auto fileParseResult = parser.getParseResult();
    if (std::holds_alternative<unacpp::ParseErrors>(fileParseResult)) {
        auto errors = std::get<unacpp::ParseErrors>(fileParseResult);
        for (auto &error: errors) {
            std::cerr << "On line " << error.pos.line << ", column " << error.pos.col
                    << ": " << error.message << '\n';
        }
        return 2;
    }

    const auto &programs = std::get<unacpp::ProgramMap>(fileParseResult);

    auto programName = parser.getExpressionName();
    auto optimizedPrograms = unacpp::optimizePrograms(programs, programName);

    auto bytecode = unacpp::generateBytecode(optimizedPrograms, programName, debugMode);
    if (outputBytecode) {
        std::cout << unacpp::bytecodeToString(bytecode);
        return 0;
    }

    runInterpreter(bytecode, readInput);
}
