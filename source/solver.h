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
  
private:

    const std::string DimacsWrongFormat = "Wrong input format of DIMACS stream";
    /**
    * @brief hasConflict - provera da li je neka klauza u konfliktu sa tekucom valuacijom
    * @return true ako je neka klauza u konfliktu, false inace
    */
    bool hasConflict() const;

    /**
    * @brief hasUnitClause
    * @return
    */
    Literal hasUnitClause() const;

private:
    CNFFormula m_formula;
    PartialValuation m_valuation;
};

#endif // SOLVER_H
