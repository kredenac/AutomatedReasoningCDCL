#include "partial_valuation.h"

#include <algorithm>
#include <cstdlib>

PartialValuation::PartialValuation(unsigned nVars)
    : m_values(nVars+1, c_defaultLiteralInfo),
    m_stack()
{
    m_stack.reserve(nVars * c_stackSizeMultiplier);

}

void PartialValuation::updateWeights(Clause& c)
{
    for (Literal l : c)
    {
        m_values[abs(l)].weight += 1;
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

void PartialValuation::push(Literal l, Clause* reason)
{
   push(l, false, reason);
}

void PartialValuation::push(Literal l, bool isDecided, Clause* reason)
{
    unsigned pos = std::abs(l);
    m_values[pos].value = l > 0 ? Tribool::True : Tribool::False;
    unsigned level = m_stack.empty() ? 1 : m_stack.back().level;

    // if decide literal
    if (isDecided)
    {
        level++;
    }
    m_values[pos].level = level;
    if (isDecided)
    {
        m_stack.emplace_back(l, level, isDecided);
        return;
    }
    m_stack.emplace_back(l, level, reason);
}

Literal PartialValuation::backtrack()
{
    if (m_stack.empty())
    {
        return NullLiteral;
    }


    do {
        Choise last = m_stack.back();
        m_stack.pop_back();
        unsigned pos = abs(last.lit);
        m_values[pos].value = Tribool::Undefined;
        m_values[pos].level = 0;

        if (last.isDecided)
        {
            return last.lit;
        }
    } while (m_stack.size());

    return NullLiteral;
}

bool PartialValuation::backtrack(Clause& reason)
{
    if (m_stack.empty())
    {
        return false;
    }

    // find 2nd most deepest level of variables in reason clause
    std::vector<unsigned> levels(reason.size());
    for (unsigned i = 0; i < reason.size(); i++)
    {
        levels[i] = m_values[abs(reason[i])].level;
    }

    if (reason.size() < 2)
    {
        throw "reason clause is smaller than 2!";
    }

    std::nth_element(levels.begin(), levels.end() - 2, levels.end());
    unsigned secondMostLvl = reason[reason.size() - 2];

    while(m_stack.back().level > secondMostLvl)
    {
        auto l = m_stack.back().lit;
        unsigned pos = abs(l);
        m_values[pos].value = Tribool::Undefined;
        m_values[pos].level = 0;
        m_stack.pop_back();
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
    /* Definisemo promenljive za broj nedefinisanih literala i poslednji nedefinisani literal */
    Literal undefinedLit = NullLiteral;
    int cntUndefined = 0;

    /* Za svaki literal proveravamo da li je nedefinisan */
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

                 /* Ako naidjemo na jos jedan nedefinisan literal - klauza nije jedinicna */
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
    // todo why does this compile?
    auto it = std::find(m_values.cbegin()+1, m_values.cend(), c_defaultLiteralInfo);
    return it != m_values.cend() ? it-m_values.cbegin() : NullLiteral;
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
