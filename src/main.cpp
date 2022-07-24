#include "interpreter.hpp"
#include "parser.hpp"

#include "CLI/CLI.hpp"

#include <iostream>
#include <fstream>

int main(int argc, char **argv) {
    std::string filename;
    std::string expr = "main";
    bool readInput = false;
    bool debugMode = false;

    CLI::App app{"An interpreter for Unarian"};

    app.add_option("file", filename, "The unarian file to interpret.")
       ->check(CLI::ExistingFile);

    app.add_flag("-g,--debug", debugMode, "Enables debug printing with the ! command.");

    app.add_option("-e,--expr", expr, "The expression to evaluate.");

    app.add_flag("-i,--input", readInput, "Uses input from stdin as input to the evaluated expression.");

    CLI11_PARSE(app, argc, argv);

    std::ifstream file{filename};
    if (!file.is_open()) {
        std::cerr << "Unable to open " << filename << '\n';
        return 1;
    }

    std::string fileContents;

    if (!filename.empty()) {
        std::stringstream fileStream;
        fileStream << file.rdbuf();
        fileContents = fileStream.str();
    }

    auto fileParseResult = unacpp::parseFile(fileContents);
    if (std::holds_alternative<unacpp::ParseErrors>(fileParseResult)) {
        auto errors = std::get<unacpp::ParseErrors>(fileParseResult);
        for (auto &error: errors) {
            std::cerr << "On line " << error.pos.line << ", column " << error.pos.col
                    << ": " << error.message << '\n';
        }
        return 2;
    }

    auto exprParseResult = unacpp::parseExpression(expr);
    if (std::holds_alternative<unacpp::ParseErrors>(exprParseResult)) {
        auto errors = std::get<unacpp::ParseErrors>(fileParseResult);
        for (auto &error: errors) {
            std::cerr << "On line " << error.pos.line << ", column " << error.pos.col
                    << ": " << error.message << '\n';
        }
        return 3;
    }

    const auto &programs = std::get<unacpp::ProgramMap>(fileParseResult);
    const auto &program = std::get<unacpp::Program>(exprParseResult);

    auto printResult = [] (unacpp::RunResult result) {
        auto *num = std::get_if<std::optional<unacpp::Counter>>(&result);
        if (num != nullptr) {
            if (*num == std::nullopt) {
                std::cout << "-\n";
            }
            else {
                (*num)->output();
            }
        }
        else {
            auto error = std::get<unacpp::RuntimeError>(result);
            std::cout << error.error << '\n';
        }
    };

    if (readInput) {
        while (std::cin) {
            uint64_t num;
            if (std::cin >> num) {
                printResult(unacpp::getResult(programs, program, unacpp::Counter{num}, debugMode));
            }
        }
    }
    else {
        printResult(unacpp::getResult(programs, program, unacpp::Counter{}, debugMode));
    }
}
