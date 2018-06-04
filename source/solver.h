#ifndef SOLVER_H
#define SOLVER_H

#include "partial_valuation.h"

#include <iostream>
#include <experimental/optional>

using OptionalPartialValuation = std::experimental::optional<PartialValuation>;

class Solver
{
public:
    /**
    * @brief Solver - konstruktor od CNF formule
    * @param formula - CNF za koji proveravamo zadovoljivost
    */
    Solver(const CNFFormula &formula);

    /**
    * @brief Solver - konstruktor od C++ stream-a iz koga se cita CNF u DIMACS formatu
    * @param dimacsStream - ulazni stream
    */
    Solver(std::istream &dimacsStream);

    /**
    * @brief solve - DPLL algoritam za proveru zadovoljivosti
    * @return parcijalnu valuaciju ili nista
    */
    OptionalPartialValuation solve();

    /**
     * @brief UseLearning whether learning clauses should be used
     */
    bool UseLearning;
private:

    Clause resolution(Clause& a, Clause& b);

    const std::string DimacsWrongFormat = "Wrong input format of DIMACS stream";

    /**
     * @brief checks if there is a conflict with the current valuation
     * @return conflicting clause if it exists, nullptr otherwise
     */
    Clause* hasConflict() const;

    /**
    * @brief hasUnitClause
    * @param l - literal will be asigned if it's a unit clause, nullLiteral if it isn't
    * @return a unit clause, nullptr if there isn't one
    */
    Clause* hasUnitClause(Literal& l) const;

    /**
     * @brief LearnClause - lears a new clause by inferring from a conflicing clause
     * @param conflict - first clause that wasn't satisfiable with a current valuation
     * @return whether or not a new clause was learnt
     */
    bool learnClause(Clause* conflict);

    /**
     * @brief newClauseFromConflicting - constructs a learned clause from a conflicting one
     * @param conflict - clause that led to conflict
     * @return returns a learned clause that should be added to the formula
     */
    Clause negateClauseLiterals(Clause& conflict) const;

    Clause findResponsibleLiterals(Clause& conflict) const;
private:
    CNFFormula m_learned;
    CNFFormula m_formula;
    PartialValuation m_valuation;
};

#endif // SOLVER_H
