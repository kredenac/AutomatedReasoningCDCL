#ifndef CHOISE_H
#define CHOISE_H

#include <vector>
#include <cstdint>

using Literal = int;
using Clause = std::vector<Literal>;


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

    bool operator == (const LiteralInfo& op2) const
    {
        return op2.value == value && op2.level == level && op2.weight == op2.weight;
    }

    Tribool value;
    unsigned level;
    float weight;
};

const LiteralInfo c_defaultLiteralInfo = LiteralInfo();

class Choise
{
public:
    /**
     * @brief Choise constructor of decided literal or its negation afterwards
     * @param lit
     */
    Choise(Literal lit, unsigned level, bool isDecided) : lit(lit), level(level), reason(nullptr), isDecided(isDecided)
    {

    }

    /**
     * @brief Choise constructor of unit propagated literal
     * @param l
     * @param reason clause
     */
    Choise(Literal l, unsigned level, Clause* reason) : lit(l), level(level), reason(reason), isDecided(false)
    {

    }
    Literal lit;
    unsigned level;
    Clause* reason;
    bool isDecided;
};

#endif // CHOISE_H
