#ifndef CHOISE_H
#define CHOISE_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <iterator>

using Literal = int;
using Clause = std::vector<Literal>;
using ClauseIndex = int;

/**
 * Template for printing out vectors into streams
 */
template <typename T>
std::ostream& operator << (std::ostream& out, const std::vector<T>& v) {
    if ( !v.empty() )
    {
        out << '[';
        std::copy (v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
        out << "\b\b]";
    }
    return out;
}

/**
 * @brief The Tribool enum - koristimo da kodiramo 3 vrednosti za promenljivu u parcijalnoj valuaciji.
 *
 * @details Za razliku od C-a u C++-u je moguce kontrolisati tip enumeracije. Konkretno nama
 * trebaju 3 vrednosti zbog cega nam je 1 bajt i vise nego dovoljan.
 */
enum class Tribool: int8_t
{
    False,
    True,
    Undefined
};

class LiteralInfo
{
public:
    LiteralInfo(Tribool val, unsigned lvl) : value(val), level(lvl), weight(0)
    {
    }

    LiteralInfo() : value(Tribool::Undefined), level(0), weight(0)
    {
    }

    bool operator == (const LiteralInfo& op2) const;

    std::vector<ClauseIndex> posWatched;
    std::vector<ClauseIndex> negWatched;
    Tribool value;
    unsigned level;
    float weight;
};

const LiteralInfo c_defaultLiteralInfo = LiteralInfo();

class Choice
{
public:
    /**
     * @brief Choice constructor of decided literal or its negation afterwards
     * @param lit
     * @param level
     * @param isDecided
     */
    Choice(Literal lit, unsigned level, bool isDecided) : lit(lit), level(level), reason(-1), isDecided(isDecided)
    {
//        if ((reason == nullptr || reason->empty()) && isDecided == false)
//        {
//            std::cout << "reason is empty" << std::endl;
//        }
    }

    /**
     * @brief Choice constructor of unit propagated literal
     * @param l
     * @param level
     * @param reason clause
     */
    Choice(Literal l, unsigned level, ClauseIndex reason) : lit(l), level(level), reason(reason), isDecided(false)
    {
//        if (reason->empty() && isDecided == false)
//        {
//            std::cout << "reason is empty" << std::endl;
//        }
    }

    bool hasReason() const;

    Literal lit;
    unsigned level;
    ClauseIndex reason;
    bool isDecided;
};

#endif // CHOISE_H
