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

// TODO faster way of finding unit clauses
// TODO clause needs to be more sophisticated than just a vector.

int main(int argc, char **argv)
{
    std::ifstream dimacsStream;
    if (2 != argc)
    {
        //std::cout << getexepath() << std::endl;
        //throw std::runtime_error{"Usage: ./3 dimacs.cnf"};
        //std::cout << "type in cnf file name" << std::endl;
        //std::string fname;
        //std::cin >> fname;
        std::vector<std::string> tests {"test-SAT.cnf", "test-UNSAT.cnf", "sudoku.cnf", "sat.cnf", "unsat.cnf"};
        dimacsStream = std::ifstream{"../source/" + tests[3]};
    }
    else
    {
        dimacsStream = std::ifstream{argv[1]};
    }

    if (!dimacsStream)
    {
        throw std::runtime_error{"Bad path to dimacs file"};
    }
    // std::cout << "sizeof long double = " << sizeof(long double) << std::endl; // some compilers print 16


    time_point startTime = high_resolution_clock::now();
    std::clock_t c_start = std::clock();

    Solver s{dimacsStream};
    s.UseLearning = true;
    OptionalPartialValuation solution = s.solve();

    std::clock_t c_end = std::clock();
    time_point finishTime = high_resolution_clock::now();

    if (solution)
    {
        std::cout << solution.value() << std::endl;
    }
    else
    {
        std::cout << "UNSAT" << std::endl;
    }

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finishTime - startTime).count();
    long double time_elapsed_ms = 1000.0 * (c_end-c_start) / CLOCKS_PER_SEC;
    std::cout << "CPU time used: " << time_elapsed_ms  << " ms" << std::endl;
    std::cout << "Total time elapsed = " << duration / 1000.0 << " ms" << std::endl;
    return 0;
}

std::string getexepath()
{
    char result[1000];
    ssize_t count = readlink( "/proc/self/exe", result, 1000);
    return std::string( result, (count > 0) ? count : 0 );
}
