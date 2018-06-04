#include "solver.h"

#include <set>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iterator>

//Solver::Solver(const CNFFormula &formula)
//  : m_formula(formula), m_valuation(m_formula.size())
//{
//}

Clause Solver::findResponsibleLiterals(Clause& conflict)
{
    Clause reason(conflict);

    auto stack = m_valuation.stack();

    unsigned i;

    for (i = stack.size() - 1; stack[i].hasReason() && i > 0; i--)
    {
        // std::cout << stack[i].lit << " was set because of: " << *stack[i].reason << std::endl;
        reason = resolution(reason, *stack[i].reason);
        // std::cout << "reason is now = " << reason << std::endl;
    }


    if (i == 0)
    {
        // TODO I think that it is unsatisfiable?
    }

    if (reason == conflict)
    {
        std::cout << "I'm not 100% sure about where to handle this, but it's UNSAT." << std::endl;
        return Clause();
    }

    return reason;
}

Clause Solver::resolution(Clause& a, Clause& b) const
{
    std::set<Literal> literals(a.begin(), a.end());
    literals.insert(b.begin(), b.end());
    for (Literal i : a)
    {
        for (Literal j : b)
        {
            if (i == -j)
            {
                literals.erase(i);
                literals.erase(-i);
                return Clause(literals.begin(), literals.end());
            }
        }
    }
    return a;
}

Clause Solver::negateClauseLiterals(Clause& cut) const
{
    Clause c;
    for(Literal l : cut)
    {
        c.push_back(-l);
    }

    return c;
}

// TODO why is 1st element on stack lvl 2?
bool Solver::learnClause(Clause* conflict)
{
    if (conflict == nullptr)
    {
        throw std::runtime_error("Delete this: bug - conflict clause is null");
    }
    // Find the cut in the implication graph that led to the conflict
    auto reasonClause = findResponsibleLiterals(*conflict);

    if (reasonClause.empty())
    {
        // empty clause => UNSAT
        return true;
    }
    m_learned.push_back(reasonClause);

    // Non-chronologically backtrack ("back jump")

    bool successful = m_valuation.backjump(reasonClause);
    return !successful;
}

Solver::Solver(std::istream &dimacsStream)
{
    // skip comments and empty lines
    std::string line;
    std::size_t firstNonSpaceIdx;
    while (std::getline(dimacsStream, line))
    {
        firstNonSpaceIdx = line.find_first_not_of(" \t\r\n");
        if (firstNonSpaceIdx != std::string::npos && line[firstNonSpaceIdx] != 'c')
        {
            break;
        }
    }

    // problem line
    if (line[firstNonSpaceIdx] != 'p')
    {
        throw std::runtime_error{DimacsWrongFormat};
    }
    std::istringstream parser{line.substr(firstNonSpaceIdx+1, std::string::npos)};
    std::string tmp;
    if (!(parser >> tmp) || tmp != "cnf")
    {
        throw std::runtime_error{DimacsWrongFormat};
    }
    unsigned varCnt, claCnt;
    if (!(parser >> varCnt >> claCnt))
    {
        throw std::runtime_error{DimacsWrongFormat};
    }

    // read clauses whilst ignoring comments and empty lines
    m_valuation.reset(varCnt);
    m_formula.resize(claCnt);
    int clauseIdx = 0;
    while (std::getline(dimacsStream, line))
    {
        firstNonSpaceIdx = line.find_first_not_of(" \t\r\n");
        if (firstNonSpaceIdx != std::string::npos && line[firstNonSpaceIdx] != 'c')
        {
            parser.clear();
            parser.str(line);
            std::copy(std::istream_iterator<int>{parser}, {}, std::back_inserter(m_formula[clauseIdx]));
            // remove trailing 0
            m_formula[clauseIdx++].pop_back();
        }
    }
    // FIXME this doesn't guarantee that it will not be reallocated
    m_learned.reserve(claCnt);
}

OptionalPartialValuation Solver::solve()
{
    while (true)
    {
        Literal l;
        Clause* conflict;
        Clause* unitClause;
        if ((conflict = hasConflict()))
        {
//            std::cout << std::endl << "conflict in " << *conflict << std::endl;
            if (UseLearning)
            {
                bool isUnsat = learnClause(conflict);
                if (isUnsat)
                {
                    return {};
                }
//                std::cout << "num of learned clauses = " << m_learned.size() << std::endl;
//                std::cout << "last learned = " << m_learned.back() << std::endl;
//                std::cout << "decides = " << m_valuation.decides << std::endl;
//                std::cout << "values = ";
//                m_valuation.getValues(&m_learned.back());
//                std::cout << std::endl;
            }
            else
            {
                Literal decidedLiteral = m_valuation.backjump();
                if (NullLiteral == decidedLiteral)
                {
                    // no more backtracking, we've tried out all valuations
                    return {};
                }

                // try with opposide literal value
                m_valuation.push(-decidedLiteral, false);
            }
        }
        else if ((unitClause = hasUnitClause(l)))
        {
            // unit prop with stored unitClause
            // std::cout << "unit prop:" << (l>0? "" : "~") << "p" << abs(l) << std::endl;
            if (unitClause->empty())
            {
                throw std::runtime_error("unit props unit clause has 0 elements.");
            }
            m_valuation.push(l, unitClause);
        }
        else
        {
            // deciding a literal

            // todo heuristic here
            l = m_valuation.firstUndefined();
//            std::cout << "decide = " << l << std::endl;
            if (l)
            {
                m_valuation.push(l, true);
            }
            else
            {
                // if no literal was decided, then it's a full valuation - SAT
                return m_valuation;
            }
        }
    }
}

Clause* Solver::hasConflict() const
{
    // first checking learned clauses
    for (const Clause& c : m_learned)
    {
        if (m_valuation.isClauseFalse(c))
        {
            return const_cast<Clause*>(&c);
        }
    }
    for (const Clause& c : m_formula)
    {
        if (m_valuation.isClauseFalse(c))
        {   
            return const_cast<Clause*>(&c);
        }
    }
    return nullptr;
}

Clause* Solver::hasUnitClause(Literal & l) const
{
    // first checking learned clauses
    for (const Clause &c : m_learned)
    {
        if ((l = m_valuation.isClauseUnit(c)))
        {
            return const_cast<Clause*>(&c);
        }
    }
    for (const Clause &c : m_formula)
    {
        if ((l = m_valuation.isClauseUnit(c)))
        {
            return const_cast<Clause*>(&c);
        }
    }
    l = NullLiteral;
    return nullptr;
}
