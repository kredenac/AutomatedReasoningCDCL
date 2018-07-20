#include "partial_valuation.h"

#include <algorithm>
#include <cstdlib>

PartialValuation::PartialValuation(unsigned nVars)
    : m_values(nVars+1, c_defaultLiteralInfo),
    m_stack()
{
    m_stack.reserve(nVars * c_stackSizeMultiplier);
}

void PartialValuation::ClearVariable(Literal l)
{
    unsigned pos = std::abs(l);
    m_values[pos].level = 0;
    m_values[pos].value = Tribool::Undefined;
}

void PartialValuation::pop()
{
    Choice c = m_stack.back();
    if (c.isDecided)
    {
        decides.erase(std::remove(decides.begin(), decides.end(), c.lit), decides.end());
    }

    ClearVariable(c.lit);
    m_stack.pop_back();
}

unsigned PartialValuation::stackSize() const
{
    return m_stack.size();
}

Choice& PartialValuation::back() const
{
    return const_cast<Choice&>(m_stack.back());
}

void PartialValuation::updateWeights(Clause& c)
{
    for (Literal l : c)
    {
        m_values[std::abs(l)].weight += 1;
    }
}

void PartialValuation::updateWeights()
{
    std::for_each(m_values.begin(), m_values.end(),
        [divideWeightsBy = divideWeightsBy] (LiteralInfo& f)
        {
            f.weight = f.weight / divideWeightsBy;
        }
    );
}

Literal PartialValuation::decideHeuristic()
{
    // Variable State Independent Decaying Sum
    unsigned n = m_values.size();
    unsigned candidatePos = NullLiteral;
    for (unsigned i = 1; i < n; i++)
    {
        if (m_values[i].value == Tribool::Undefined &&
                m_values[i].weight >= m_values[candidatePos].weight)
        {
            candidatePos = i;
        }
    }
    return candidatePos;
}

void PartialValuation::push(Literal l, ClauseIndex reason)
{
    push(l, false, reason);
}

void PartialValuation::push(Literal l, bool isDecided, ClauseIndex reason)
{
    unsigned pos = std::abs(l);
    m_values[pos].value = l > 0 ? Tribool::True : Tribool::False;
    unsigned level = m_stack.empty() ? 1 : m_stack.back().level;

    if (isDecided)
    {
        decides.push_back(l);
        level++;
    }
    m_values[pos].level = level;
    if (isDecided)
    {
        m_stack.emplace_back(l, level, true);
        return;
    }
    else if (reason != -1)
    {
        m_stack.emplace_back(l, level, reason);
    }
    else // doesn't have a reason and it's not decided => it's a negation of previously decided literal
    {
        // Uz ucenje klauza i backjumping nikada se ne ulazi ovde
        m_stack.emplace_back(l, level, false);
    }
}

Literal PartialValuation::backjump()
{
    if (m_stack.empty())
    {
        return NullLiteral;
    }


    do {
        Choice last = m_stack.back();
        pop();

        if (last.isDecided)
        {
            return last.lit;
        }
    } while (m_stack.size());

    return NullLiteral;
}

bool PartialValuation::backjump(Clause& reason)
{
    if (m_stack.empty())
    {
        return false;
    }

    // find 2nd most deepest level of variables in reason clause
    std::vector<unsigned> levels(reason.size());
    for (unsigned i = 0; i < reason.size(); i++)
    {
        levels[i] = m_values[std::abs(reason[i])].level;
    }

    unsigned backjumpToLevel;

    if (reason.size() < 2)
    {
        if (reason.size() == 0)
        {
            throw std::runtime_error("reason clause has 0 elements");
        }
        Literal l = reason.back();
        // if there's only one variable in learned clause, than it's only that decided literals fault
        backjumpToLevel = m_values[std::abs(l)].level - 1;
    }
    else
    {
        std::nth_element(levels.begin(), levels.end() - 2, levels.end());
        backjumpToLevel = levels[levels.size() - 2];
    }

    while(m_stack.size() && m_stack.back().level > backjumpToLevel)
    {
        pop();
    }
    return true;
}

bool PartialValuation::isClauseFalse(const Clause &c) const
{
    /* Za svaki literal klauze proveravamo da li se njegov suprotni nalazi u parc. val. */
    for (Literal l : c)
    {
        Tribool variableInClause = l > 0 ? Tribool::True : Tribool::False;
        Tribool variableInValuation = m_values[std::abs(l)].value;
        if (variableInClause == variableInValuation || variableInValuation == Tribool::Undefined)
        {
            return false;
        }
    }
    return true;
}

Literal PartialValuation::isClauseUnit(const Clause &c) const
{
    Literal undefinedLit = NullLiteral;
    int cntUndefined = 0;

    for (Literal l : c)
    {
        Tribool valueInClause = l > 0 ? Tribool::True : Tribool::False;
        Tribool valueInValuation = m_values[std::abs(l)].value;
        if (valueInClause != valueInValuation)
        {
            if (valueInValuation == Tribool::Undefined)
            {
                ++cntUndefined;
                undefinedLit = l;

                if (cntUndefined > 1)
                {
                    break;
                }
            }
        }
        else
        {
            return NullLiteral;
        }
    }
    return cntUndefined == 1 ? undefinedLit : NullLiteral;
}

Literal PartialValuation::firstUndefined() const
{
    auto it = std::find(m_values.cbegin()+1, m_values.cend(), c_defaultLiteralInfo);
    return it != m_values.cend() ? it-m_values.cbegin() : NullLiteral;
}

bool PartialValuation::isLiteralTrue(Literal lit)
{
    Tribool litInClause = lit > 0 ? Tribool::True : Tribool::False;
    Tribool litInValuation = m_values[std::abs(lit)].value;
    return litInClause == litInValuation;
}

bool PartialValuation::isLiteralUndefined(Literal lit)
{
    return m_values[std::abs(lit)].value == Tribool::Undefined;
}

int PartialValuation::posOfFirstNonFalseInClause(Clause &currClause, int startInd)
{
    for (unsigned j = startInd; j < currClause.size(); j++)
    {
        Literal l = currClause[j];
        Tribool variableInClause = l > 0 ? Tribool::True : Tribool::False;
        Tribool variableInValuation = values()[std::abs(l)].value;
        if (variableInClause == variableInValuation || variableInValuation == Tribool::Undefined)
        {
            return j;
        }
    }
    return -1;
}

void PartialValuation::reset(unsigned nVars)
{
    m_values.resize(nVars+1);
    std::fill(m_values.begin(), m_values.end(), c_defaultLiteralInfo);

    m_stack.clear();
    m_stack.reserve(nVars * c_stackSizeMultiplier);
}

std::ostream &operator<<(std::ostream &out, const PartialValuation &pval)
{ 
    out << "[ ";
    for (std::size_t i = 1; i < pval.m_values.size(); ++i)
    {
        if (pval.m_values[i].value == Tribool::True)
        {
            out << 'p' << i << ' ';
        }
        else if (pval.m_values[i].value == Tribool::False)
        {
            out << "~p" << i << ' ';
        }
        else if (pval.m_values[i].value == Tribool::Undefined)
        {
            out << 'u' << i << ' ';
        }
        else
        {
            throw std::logic_error{"Unknown value assigned to variable (not True, nor Flase, nor Undefined)"};
        }
    }
    return out << " ]";
}
