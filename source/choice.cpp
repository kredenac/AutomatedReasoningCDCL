#include "choice.h"

bool Choice::hasReason() const
{
    return reason != -1;
}

bool LiteralInfo::operator == (const LiteralInfo& op2) const
{
    return op2.value == value && op2.level == level && op2.weight == op2.weight;
}
