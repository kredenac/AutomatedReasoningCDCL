#include "partial_valuation.h"

#include <algorithm>
#include <cstdlib>

PartialValuation::PartialValuation(unsigned nVars)
    : m_values(nVars+1, Tribool::Undefined),
    m_stack()
{
    m_lastDecidePos = 0;
    m_stack.reserve(nVars * c_stackSizeMultiplier);
}

// TODO enable O(1) jumping
// ..... [position of last decide], [NullLiteral], [decided literal], .......
void PartialValuation::push(Literal l, bool decide)
{
    /* Promenljivu od literala dobijamo sa std::abs, a za polaritet proveravamo znak */
    m_values[std::abs(l)] = l > 0 ? Tribool::True : Tribool::False;
    if (decide)
    {
        // store the position of last decided literal
//        m_stack.push_back(m_lastDecidePos);
        // putting a ramp so it can be know that the next element is decided
        m_stack.push_back(NullLiteral);
        m_lastDecidePos = m_stack.size();
    }
    m_stack.push_back(l);
}

// TODO make O(1) jump
Literal PartialValuation::backtrack()
{
    /* Proveravamo da nije prazan stek */
    if (m_stack.empty())
    {
        return NullLiteral;
    }


//    if (m_lastDecidePos == 0)
//    {
//        return NullLiteral;
//    }
//    // may not actually be smart enough to know that it doesn't need linear
//    // time to resize, but constant time. In that case making a custom allocator is needed
//    m_stack.resize(m_lastDecidePos);



    Literal lastDecide = NullLiteral, last = NullLiteral;
    do {
        /* Dohvatamo poslednje postavljeni literal i skidamo ga sa steka */
        last = m_stack.back();
        m_stack.pop_back();
        m_values[std::abs(last)] = Tribool::Undefined;

        /* Ako je on NullLiteral, tj. rampa vracamo lastDecide literal */
        if (NullLiteral == last)
        {
            break;
        }
        lastDecide = last;
    } while (m_stack.size());

    return last == NullLiteral ? lastDecide : NullLiteral;
}

bool PartialValuation::isClauseFalse(const Clause &c) const
{
    /* Za svaki literal klauze proveravamo da li se njegov suprotni nalazi u parc. val. */
    for (Literal l : c)
    {
        Tribool variableInClause = l > 0 ? Tribool::True : Tribool::False;
        Tribool variableInValuation = m_values[std::abs(l)];
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
        Tribool valueInValuation = m_values[std::abs(l)];
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
    auto it = std::find(m_values.cbegin()+1, m_values.cend(), Tribool::Undefined);
    return it != m_values.cend() ? it-m_values.cbegin() : NullLiteral;
}

void PartialValuation::reset(unsigned nVars)
{
    m_values.resize(nVars+1);
    std::fill(m_values.begin(), m_values.end(), Tribool::Undefined);

    m_stack.clear();
    m_stack.reserve(nVars * c_stackSizeMultiplier);
}


std::ostream &operator<<(std::ostream &out, const PartialValuation &pval)
{ 
    out << "[ ";
    for (std::size_t i = 1; i < pval.m_values.size(); ++i)
    {
        if (pval.m_values[i] == Tribool::True)
        {
            out << 'p' << i << ' ';
        }
        else if (pval.m_values[i] == Tribool::False)
        {
            out << "~p" << i << ' ';
        }
        else if (pval.m_values[i] == Tribool::Undefined)
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
