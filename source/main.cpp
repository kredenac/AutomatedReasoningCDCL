#include "solver.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <limits.h>
#include <unistd.h>
#include <chrono>
#include <ctime>

using std::chrono::high_resolution_clock;
using time_point = std::chrono::high_resolution_clock::time_point;

void testWithTimer(std::string fileName, bool useLearning);

int main(int argc, char **argv)
{
    std::ifstream dimacsStream;
    if (2 != argc)
    {
        std::vector<std::string> tests {"plsWrk.cnf", "test-SAT.cnf", "test-UNSAT.cnf", "sat.cnf", "unsat.cnf", "sudoku.cnf"};
        std::vector<bool> expected {true, true, false, true, false, true};
        // run tests
        std::cout << "----------------------------------------------------------\n";
        std::cout << "     Running tests with two-watched-literals:\n";
        std::cout << "----------------------------------------------------------\n";
        for (unsigned i = 0; i < tests.size() - 1; i++)
        {
            std::string str = tests[i];
            dimacsStream = std::ifstream{"../source/" + str};

            Solver t{dimacsStream};
            t.UseLearning = true;
            OptionalPartialValuation solution = t.solve2();
            dimacsStream.close();

            if (solution.operator bool() == expected[i])
            {
                std::cout << "test #" << i << "passed" << std::endl;
            }
            else
            {
                std::cout << "test #" << i << "failed" << std::endl;
            }
        }
        std::cout << "\n\n";
        const unsigned runWhich = tests.size()-1;
        dimacsStream = std::ifstream{"../source/" + tests[runWhich]};
    }
    else
    {
        dimacsStream = std::ifstream{argv[1]};
    }

    if (!dimacsStream)
    {
        throw std::runtime_error{"Bad path to dimacs file"};
    }

    testWithTimer("sudoku.cnf", true);
    testWithTimer("sudoku.cnf", false);

    return 0;
}

void testWithTimer(std::string fileName, bool useLearning)
{
    std::string fName = fileName + (useLearning ? " with " : " without ");
    std::ifstream dimacsStream = std::ifstream{"../source/" + fileName};

    std::cout << "----------------------------------------------------------\n";
    std::cout << "     Running " << fName << "two-watched-literals:\n";
    std::cout << "----------------------------------------------------------\n";

    time_point startTime = high_resolution_clock::now();
    std::clock_t c_start = std::clock();

    Solver s{dimacsStream};
    s.UseLearning = true;
    OptionalPartialValuation solution = useLearning ? s.solve2() : s.solve();

    std::clock_t c_end = std::clock();
    time_point finishTime = high_resolution_clock::now();
    dimacsStream.close();

    if (solution)
    {
        std::cout << "SAT" << std::endl;
//        std::cout << solution.value() << std::endl;
    }
    else
    {
        std::cout << "UNSAT" << std::endl;
    }

//    std::cout << s.getInfo() << std::endl;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finishTime - startTime).count();
    long double time_elapsed_ms = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
    std::cout << "CPU time used: " << time_elapsed_ms  << " ms" << std::endl;
    std::cout << "Total time elapsed = " << duration / 1000.0 << " ms" << std::endl;
    std::cout << "\n\n";
}
