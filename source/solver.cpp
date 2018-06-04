#include "solver.h"

#include <set>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iterator>

Solver::Solver(const CNFFormula &formula)
  : m_formula(formula), m_valuation(m_formula.size())
{
}

// TODO this is just a dummy
Clause Solver::findResponsibleLiterals(Clause& conflict) const
{

    return conflict;
}

Clause Solver::resolution(Clause& a, Clause& b)
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
    return Clause();
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

bool Solver::learnClause(Clause* conflict)
{
    if (conflict == nullptr)
    {
        throw std::runtime_error("Delete this: bug - conflict clause is null");
    }
    // 1. Find the cut in the implication graph that led to the conflict
    auto cutClause = findResponsibleLiterals(*conflict);

    // 2. Derive a new clause which is the negation of the assignments that led to the conflict
    auto newClause = negateClauseLiterals(cutClause);
    m_learned.push_back(newClause);

    // 3. Non-chronologically backtrack ("back jump") to the appropriate decision level, where the first-assigned variable involved in the conflict was assigned
    // u nekim slajdovima pise da je Assertion level drugi najveci...
    auto lit = m_valuation.backtrack(newClause);
    return lit != NullLiteral;
}

Solver::Solver(std::istream &dimacsStream)
{
    /* Citamo uvodne komentare, preskacemo prazne linije */
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

    /* Proveravamo da smo procitali liniju 'p cnf brPromenljivih brKlauza' */
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

    /* Citamo klauze linije po liniju preskacuci komentare i prazne linije */
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
            m_formula[clauseIdx++].pop_back(); /* izbacujemo nulu sa kraja linije */
        }
    }
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
            if (UseLearning)
            {
                bool hasLearned = learnClause(conflict);

                if (hasLearned)
                {
                    continue;
                }
                throw std::runtime_error("Solver::solve Tried to learn but didn't");
            }

            Literal decidedLiteral = m_valuation.backtrack();
            if (NullLiteral == decidedLiteral)
            {
                // no more backtracking, we've tried out all valuations
                return {};
            }

            // try with opposide literal value
            m_valuation.push(-decidedLiteral, false);
        }
        else if ((unitClause = hasUnitClause(l)))
        {
            // unit prop with stored unitClause
            m_valuation.push(l, unitClause);
        }
        else
        {
            // deciding a literal
            // todo heuristic here
            l = m_valuation.firstUndefined();
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
