#include "solver.h"

#include <set>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iterator>
#include <queue>

//Solver::Solver(const CNFFormula &formula)
//  : m_formula(formula), m_valuation(m_formula.size())
//{
//}

Clause Solver::findResponsibleLiterals(Clause& conflict)
{
    Clause reason(conflict);

    auto& stack = m_valuation.stack();


//    std::cout << "Current reason: ";
//    for (Literal var : reason) {
//        std::cout << var << " ";
//    }
//    std::cout << std::endl;


    while(m_valuation.stackSize() && m_valuation.back().hasReason())
    {
        // CHANGEME: NOTHING SHOULD BE DONE HERE
        reason = resolution(reason, m_formula[stack.back().reason], stack.back().lit);
        m_valuation.pop();

//        std::cout << "Current reason: ";
//        for (Literal var : reason) {
//            std::cout << var << " ";
//        }
//        std::cout << std::endl;

    }
//    unsigned i;

//    for (i = stack.size() - 1; stack[i].hasReason() && i > 0; i--)
//    {
//        // std::cout << stack[i].lit << " was set because of: " << *stack[i].reason << std::endl;
//        reason = resolution(reason, *stack[i].reason);
//        // std::cout << "reason is now = " << reason << std::endl;
//    }

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
        // if it comes here, it possibly learned too much, and broke the pointers to m_formula
        throw std::runtime_error("Clause b doesnt contain literal which");
    }
    literals.erase(which);
    literals.erase(-which);
    return Clause(literals.begin(), literals.end());
}


// TODO: nigde se ne koristi
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
// TODO: add watchers to clause when adding new clause
// CHANGEME: mora da se menja potpis fje
bool Solver::learnClause(ClauseIndex conflict)
{
    if (conflict == -1)
    {
        throw std::runtime_error("Delete this: bug - conflict clause is null");
    }
    // Find the cut in the implication graph that led to the conflict
    // CHANGEME: mora da promeni argument fje
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
//    m_learned.reserve(claCnt);
    m_formula.reserve(claCnt*2);
}

OptionalPartialValuation Solver::solve2()
{
    std::queue<ClauseIndex> unitClauses;
    std::queue<Literal> unitLiterals;
    ClauseIndex conflict = -1;
    Literal l;
    while(true)
    {
        // if there is a conflict, clear queued unitProps, learn a clause and backtrack
        if (conflict != -1)
        {
            // function: clear all unitProps
            while(!unitClauses.empty())
            {
                unitClauses.pop();
                unitLiterals.pop();
            }

            m_valuation.updateWeights(m_formula[conflict]);
            bool isUnsat = learnClause(conflict);
            if (isUnsat)
            {
                return {};
            }

            conflict = -1;
        }
        // if there is unit prop literal, process it
        else if (!unitLiterals.empty())
        {
            unitProp(unitClauses, unitLiterals);
        }
        // if there is an undefined literal, propagate it
        else if ((l = m_valuation.decideHeuristic()))
        {
            unitLiterals.push(l);
            unitClauses.push(-1);
            unitProp(unitClauses, unitLiterals);
        }
        // if no literal was decided, then it's a full valuation - SAT
        else
        {
            return m_valuation;
        }
    }
}


ClauseIndex Solver::unitProp(std::queue<ClauseIndex> &unitClauses, std::queue<Literal> &unitLiterals)
{
    Literal lit = unitLiterals.front();
    std::vector<ClauseIndex> &watchedClauses = lit > 0 ?
            m_valuation.values()[abs(lit)].negWatched
            : m_valuation.values()[abs(lit)].posWatched;
    // Clause === vector<int>
    Clause currClause;
    for (unsigned i = 0; i < watchedClauses.size(); ++i) {
        currClause = m_formula[watchedClauses[i]];
//        watch1 = m_valuation.values()[abs(currClause[0])];
//        watch2 = m_valuation.values()[abs(currClause[currClause.size()-1])];
//        if ((*currClause)[0] == true || (*currClause)[currClause->size()] == true)
//        {
//            continue;
//        }
        // if otherWatched literal is true, continue
        // find a literal that is true or undef
//        for (unsigned j = 1; j < currClause->size()-1; ++j) {

//        }
        // if such literal exists:
            // change current watched lit with that lit2
        // else if other watched lit is undef
            // UnitProp that other watched lit
        // else
            // CONFLICT

    }
    // for all clauses watched by l
        // if exists undefined literal lit2, different from other watched
            // change watchers l, lit2
        // else if watchedLit2 == true, clause is sat
        // else clause is unit, watchedLit2 can be unitProped
            // unitClauses.push(currClause);
            // unitLiteral.push(watchedLit2);
    // process all clauses watched by l

    // if there is a conflict return the conflict clause


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

OptionalPartialValuation Solver::solve()
{
    while (true)
    {
//        for (unsigned k = 0; k < m_valuation.stackSize(); ++k) {
//            std::cout << m_valuation.stack()[k].lit << " ";
//        }
//        std::cout << std::endl;

        Literal l;
        ClauseIndex conflict;
        ClauseIndex unitClause;
        if ( (conflict = hasConflict()) != -1 )
        {
//            std::cout << std::endl << "conflict in " << *conflict << std::endl;
            if (UseLearning)
            {
                m_valuation.updateWeights(m_formula[conflict]);
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
        else if ( (unitClause = hasUnitClause(l)) != -1 )
        {
            // unit prop with stored unitClause
            // std::cout << "unit prop:" << (l>0? "" : "~") << "p" << abs(l) << std::endl;
            if (m_formula[unitClause].empty())
            {
                throw std::runtime_error("unit props unit clause has 0 elements.");
            }
            m_valuation.push(l, unitClause);
        }
        else
        {
            // deciding a literal

            // todo heuristic here
            // l = m_valuation.firstUndefined();
            l = m_valuation.decideHeuristic();
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
    // first checking learned clauses
    for (unsigned i = 0; i < m_formula.size(); ++i)
    {
        if ( m_valuation.isClauseFalse(m_formula[i]))
        {
//            return const_cast<Clause*>(&m_formula[i]);
            return i;
        }
    }
//    for (const Clause& c : m_formula)
//    {
//        if (m_valuation.isClauseFalse(c))
//        {
//            return const_cast<Clause*>(&c);
//        }
//    }
    return -1;
}

ClauseIndex Solver::hasUnitClause(Literal & l) const
{
    for (unsigned i = 0; i < m_formula.size(); ++i)
    {
        if (( l = m_valuation.isClauseUnit(m_formula[i]) ))
        {
//            return const_cast<Clause*>(&m_formula[i]);
            return i;
        }
    }
//    for (const Clause &c : m_formula)
//    {
//        if ((l = m_valuation.isClauseUnit(c)))
//        {
//            return const_cast<Clause*>(&c);
//        }
//    }
    l = NullLiteral;
    return -1;
}
