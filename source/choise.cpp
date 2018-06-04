#include "choise.h"

bool Choise::hasReason() const
{
    return reason != nullptr;
}

bool LiteralInfo::operator == (const LiteralInfo& op2) const
{
    return op2.value == value && op2.level == level && op2.weight == op2.weight;
}
