#include "solver.h"

#include <set>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iterator>


Clause Solver::findResponsibleLiterals(Clause& conflict)
{
    Clause reason(conflict);

    auto& stack = m_valuation.stack();

    while(m_valuation.stackSize() && m_valuation.back().hasReason())
    {
        reason = resolution(reason, m_formula[stack.back().reason], stack.back().lit);
        m_valuation.pop();
    }

    if (reason == conflict)
    {
        std::cout << "I'm not 100% sure about where to handle this, but it's UNSAT." << std::endl;
        return Clause();
    }

    return reason;
}

Clause Solver::resolution(Clause& reason, Clause& b, Literal& which) const
{
    if (reason.end() == std::find(reason.begin(), reason.end(), -which))
    {
        return reason;
    }
    std::set<Literal> literals(reason.begin(), reason.end());
    literals.insert(b.begin(), b.end());

    auto remove2 = std::find(literals.begin(), literals.end(), which);
    if (literals.end() == remove2)
    {
        throw std::runtime_error("Clause b doesnt contain literal which");
    }
    literals.erase(which);
    literals.erase(-which);
    return Clause(literals.begin(), literals.end());
}

bool Solver::learnClause(ClauseIndex conflict)
{
    if (conflict == -1)
    {
        throw std::runtime_error("Delete this: bug - conflict clause is null");
    }
    // Find the cut in the implication graph that led to the conflict
    auto reasonClause = findResponsibleLiterals(m_formula[conflict]);

    if (reasonClause.empty())
    {
        // empty clause => UNSAT
        return true;
    }
    m_formula.push_back(reasonClause);

    // Non-chronologically backtrack ("back jump")
    bool successful = m_valuation.backjump(reasonClause);
    return !successful;
}

// BUG: ako imamo na ulazu klauze sa duplikat literalima, ne sljaka algoritam, treba da se to obradi na ulazu
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
    m_formula.reserve(claCnt+1000);
    m_formula.resize(claCnt);
    ClauseIndex clauseIdx = 0;
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
            watchTwoLiterals(clauseIdx-1);
        }
    }
}

void Solver::watchTwoLiterals(ClauseIndex clauseIdx)
{
    if (m_formula[clauseIdx].size() == 0)
    {
        throw std::runtime_error("clause has 0 elements");
    }
    if (m_formula[clauseIdx].size() == 1)
    {
        // add to unitClauses which will be propagated in the begining
        // no need to watch this clause
        unitClauses.push(clauseIdx);
        unitLiterals.push(m_formula[clauseIdx][0]);
    }
    else
    {
        Literal watch1 = m_formula[clauseIdx][0];
        watchLit(watch1, clauseIdx);
        Literal watch2 = m_formula[clauseIdx][1];
        watchLit(watch2, clauseIdx);
    }
}

void Solver::watchLit(Literal lit, ClauseIndex clauseIdx)
{
    if (lit < 0)
    {
        m_valuation.values()[std::abs(lit)].negWatched.push_back(clauseIdx);
    }
    else
    {
        m_valuation.values()[std::abs(lit)].posWatched.push_back(clauseIdx);
    }
}

OptionalPartialValuation Solver::solve2()
{
    ClauseIndex conflict = -1;
    Literal l;

    while(true)
    {
        if (conflict != -1)
        {
            clearUnitProps();

            m_valuation.updateWeights(m_formula[conflict]);
            bool isUnsat = learnClause(conflict);
            if (isUnsat)
            {
                return {};
            }
            watchLearnedClause();

            // push learned clause to propagation
            pushUnitProp(m_valuation.isClauseUnit(m_formula.back()), m_formula.size()-1);

            conflict = -1;
        }
        // if there is unit prop literal, propagate it
        else if (!unitLiterals.empty())
        {
            conflict = unitProp();
        }
        // if there is an undefined literal, propagate it
        else if ((l = m_valuation.decideHeuristic()))
        {
            pushUnitProp(l, -1);
            conflict = unitProp();
        }
        // if no literal was decided, then it's a full valuation - SAT
        else
        {
            return m_valuation;
        }
    }
}

void Solver::clearUnitProps()
{
    while(!unitClauses.empty())
    {
        unitClauses.pop();
        unitLiterals.pop();
    }
}

void Solver::pushUnitProp(Literal lit, ClauseIndex ci)
{
    unitLiterals.push(lit);
    unitClauses.push(ci);
}

// watch-ujemo literale samo ako klauza ima bar 2 literala
// ako ima samo 1 literal, onda ce biti ubacena na pocetnom levelu, tako da je korektnost zagarantovana
void Solver::watchLearnedClause()
{
    Clause& learnedClause = m_formula.back();

    if (learnedClause.size() < 2)
    {
        return;
    }

    Literal toBePushed = m_valuation.isClauseUnit(m_formula.back());
    auto litPos = std::find(learnedClause.begin(), learnedClause.end(), toBePushed);
    std::swap(*litPos, learnedClause[0]);

    auto lit2Pos = std::max_element(learnedClause.begin(), learnedClause.end(),
             [&](Literal& l1, Literal& l2){
        return m_valuation.values()[std::abs(l1)].level < m_valuation.values()[std::abs(l2)].level;
    });
    std::swap(*lit2Pos, learnedClause[1]);

    watchTwoLiterals(m_formula.size()-1);
}

ClauseIndex Solver::unitProp()
{
    Literal lit = unitLiterals.front();
    if (!m_valuation.isLiteralUndefined(lit))
    {
        if (m_valuation.values()[std::abs(lit)].value != (lit>0?Tribool::True : Tribool::False))
        {
            // BUGPOSSIBLE: za unit propove proverava dal dolazi do konflikta
            // FIXME: trebalo bi da sljaka, ali nisam 100% siguran da je lepo obradjeno
            //           treba da se odradi: kada se nauci jedinicna klauza
            //           koja je protivrecna sa prethodno ubacenom jedinicnom klauzom
            return unitClauses.front();
        }

        unitLiterals.pop();
        unitClauses.pop();
        return -1;
    }
    std::vector<ClauseIndex> &watchedClauses = lit > 0 ?
            m_valuation.values()[std::abs(lit)].negWatched
            : m_valuation.values()[std::abs(lit)].posWatched;

    ClauseIndex conflict;
    if ((conflict = updateWatchedClauses(watchedClauses, lit)) != -1)
    {
        return conflict;
    }

    if (unitClauses.front() != -1)
        // if explain clause exists it's a unitProp
        m_valuation.push(unitLiterals.front(), unitClauses.front());
    else
        // else it's a decideProp
        m_valuation.push(unitLiterals.front(), true);
    unitLiterals.pop();
    unitClauses.pop();

    return -1;
}

void Solver::changeWatchedLiteral(std::vector<ClauseIndex> &watchedClauses,
            ClauseIndex currClauseInd, int currLitInd, int otherLitInd)
{
    auto& currClause = m_formula[watchedClauses[currClauseInd]];
    if (currClause[otherLitInd] > 0)
    {
        m_valuation.values()[std::abs(currClause[otherLitInd])].posWatched.push_back(watchedClauses[currClauseInd]);
    }
    else
    {
        m_valuation.values()[std::abs(currClause[otherLitInd])].negWatched.push_back(watchedClauses[currClauseInd]);
    }
    watchedClauses[currClauseInd] = watchedClauses[watchedClauses.size()-1];
    watchedClauses.pop_back();
    std::swap(currClause[currLitInd], currClause[otherLitInd]);
}

ClauseIndex Solver::updateWatchedClauses(std::vector<ClauseIndex> &watchedClauses, Literal lit)
{
    unsigned i = 0;
    int nonFalseLitInd;
    while (i < watchedClauses.size())
    {
        Clause &currClause = m_formula[watchedClauses[i]];
        if (std::abs(lit) == std::abs(currClause[1]))
        {
            std::swap(currClause[0], currClause[1]);
        }
        Literal watch2 = currClause[1];

        if (m_valuation.isLiteralTrue(watch2))
        {
            i++;
        }
        else if ((nonFalseLitInd = m_valuation.posOfFirstNonFalseInClause(currClause, 2)) != -1)
        {
            changeWatchedLiteral(watchedClauses, i, 0, nonFalseLitInd);
        }
        else if (m_valuation.isLiteralUndefined(watch2))
        {
            // UnitProp that other watched lit
            pushUnitProp(watch2, watchedClauses[i]);
            i++;
        }
        else
        {
            // CONFLICT
            m_valuation.push(unitLiterals.front(), unitClauses.front());
            return watchedClauses[i];
        }
    }
    return -1;
}

OptionalPartialValuation Solver::solve()
{
    while (true)
    {
        Literal l;
        ClauseIndex conflict;
        ClauseIndex unitClause;
        if ( (conflict = hasConflict()) != -1 )
        {
            if (UseLearning)
            {
                m_valuation.updateWeights(m_formula[conflict]);
                bool isUnsat = learnClause(conflict);
                if (isUnsat)
                {
                    return {};
                }
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
        else if ( (unitClause = hasUnitClause(l)) != -1 )
        {
            // unit prop with stored unitClause
            if (m_formula[unitClause].empty())
            {
                throw std::runtime_error("unit props unit clause has 0 elements.");
            }
            m_valuation.push(l, unitClause);
        }
        else
        {
            // deciding a literal

            // l = m_valuation.firstUndefined();
            l = m_valuation.decideHeuristic();
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

std::string Solver::getInfo() const
{
    return "stack size = " + std::to_string(m_valuation.stackSize()) +
            "\ndecides = " + std::to_string(m_valuation.decides.size()) +
//            "\nlearned clauses = " + std::to_string(m_learned.size()) +
            "\nunit propagations = " + "unknown" +
            "\nrestarts = " + std::to_string(0);
}

ClauseIndex Solver::hasConflict() const
{
    for (unsigned i = 0; i < m_formula.size(); ++i)
    {
        if ( m_valuation.isClauseFalse(m_formula[i]))
        {
            return i;
        }
    }
    return -1;
}

ClauseIndex Solver::hasUnitClause(Literal & l) const
{
    for (unsigned i = 0; i < m_formula.size(); ++i)
    {
        if (( l = m_valuation.isClauseUnit(m_formula[i]) ))
        {
            return i;
        }
    }
    l = NullLiteral;
    return -1;
}

void Solver::printAllWatchedClauses()
{
    for (unsigned i = 1; i < m_valuation.values().size(); ++i)
    {
        std::cout << "\n";
        LiteralInfo &litInf = m_valuation.values()[i];
        std::cout << "Lit: " << i << std::endl;
        std::cout << "  Pos Watched:" << std::endl;
        for (ClauseIndex cl : litInf.posWatched)
        {
            std::cout << "    " << m_formula[cl] << std::endl;
        }
        std::cout << "  Neg Watched:" << std::endl;
        for (ClauseIndex cl : litInf.negWatched)
        {
            std::cout << "    " << m_formula[cl] << std::endl;
        }
    }
    std::cout << "\n\n";
}
