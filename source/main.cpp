#include "solver.h"

#include <fstream>
#include <stdexcept>

#include <string>
#include <limits.h>
#include <unistd.h>

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
        dimacsStream = std::ifstream{"../source/test-SAT.cnf"};
    }
    else
    {
        dimacsStream = std::ifstream{argv[1]};
    }

    if (!dimacsStream)
    {
        throw std::runtime_error{"Bad path to dimacs file"};
    }
    Solver s{dimacsStream};
    OptionalPartialValuation solution = s.solve();
    if (solution)
    {
        std::cout << solution.value() << std::endl;
    }
    else
    {
        std::cout << "UNSAT" << std::endl;
    }
    return 0;
}

std::string getexepath()
{
    char result[1000];
    ssize_t count = readlink( "/proc/self/exe", result, 1000);
    return std::string( result, (count > 0) ? count : 0 );
}
