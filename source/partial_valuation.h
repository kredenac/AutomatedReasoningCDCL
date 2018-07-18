#ifndef PARTIALVALUATION_H
#define PARTIALVALUATION_H

#include "choice.h"

#include <vector>
#include <iostream>
#include <algorithm>


/**
 * Za literal cemo koristiti int vrednosti, pri cemu je najmanji indeks literala 1 tj. p1.
 * Ovo je zgodno iz 3 razloga, prvi je vezan za DIMACS format gde se promenljive predstavljaju
 * kao oznacene celobrojne vrednosti, drugi se odnosi na polaritet literala, odnosno na to
 * da li je literal na primer p1 ili ~p1 (1 koristimo za p1, a -1 za ~p1), a treci na postojanje
 * specijalne vrednosti koja ce igrati ulogu "rampe" u DPLL algoritmu (to je broj 0).
 */
using Literal = int;
using Clause = std::vector<Literal>;
using CNFFormula = std::vector<Clause>;

/**
 * S obzirom na to da cemo za literale koristiti oznacene celobrojne vrednosti 0 je specijalna vrednost.
 */
const Literal NullLiteral = 0;

/**
 * Deklaracija klase i operatora za ispis u stream.
 */
class PartialValuation;
std::ostream& operator<<(std::ostream &out, const PartialValuation &pval);

/**
 * @brief The PartialValuation class - koristi se da predstavi parcijalnu valuaciju u kojoj promenljive
 * mogu biti tacne, netacne ili nedefinisane.
 */
class PartialValuation
{
public:
    // used for debugging
    std::vector<Literal> decides;
    void getValues(Clause * c) const
    {
        for (unsigned i=0; i < c->size(); i++)
        {
            auto tribool = m_values[abs(c->at(i))].value;
            if (tribool == Tribool::True)
            {
                std::cout << "T";
            }
            else if (tribool == Tribool::False)
            {
                std::cout << "F";
            }
            else
            {
                std::cout << "U";
            }

            std::cout << ", ";
        }
    }

    //
    PartialValuation(unsigned nVars = 0);

    unsigned stackSize() const;
    /**
     * @brief updateWeights - increases the weights of literals in a given clause
     * @param c - learnt clause
     */
    void updateWeights(Clause& c);

    /**
     * @brief updateWeights - lowers the value of all weights
     */
    void updateWeights();

    /**
     * @brief decideHeuristic - heuristic based on VSIDS
     * @return decided literal
     */
    Literal decideHeuristic();
    /**
    * @brief push - set value of variable in valuation based on literal
    * @param l - literal
    * @param reason - pointer to clause which is a reason for unit prop.
    * default nullptr when it's a decided literal
    */
    void push(Literal l, ClauseIndex reason);

    void push(Literal l, bool isDecided, ClauseIndex reason = -1);

    /**
     * @brief pop the top of the stack of valuation
     */
    void pop();

    /**
     * @brief back - gets the choice from top of the stack
     * @return the choice on top of the stack
     */
    Choice& back() const;
    /**
    * @brief backtrack - skida literale sa steka sve do prvog decide literala na koji naidje
    * @return poslednji decide literal ili NullLiteral ukoliko takvog nema
    */
    Literal backjump();

    /**
     * @brief backtrack to when the 2nd most level variable involved in the conflict was assigned
     * @param cut - clause of variables involved in the conflict
     * @return if backtracking succeeded or not (case of failure means UNSAT)
     */
    bool backjump(Clause& learned);

    /**
    * @brief isClauseFalse - proverava da li je klauza netacna u tekucoj parcijalnoj valuaciji.
    *
    * @details Klauza je netacna u tekucoj parcijalnoj valuaciji ako za svaki literal klauze
    * vazi da je u parcijalnoj valuaciji njemu suprotan literal.
    * @param c - klauza koja se proverava
    * @return true ako je klauza netacna, false inace
    */
    bool isClauseFalse(const Clause &c) const;

    /**
    * @brief isClauseUnit - proverava da li je klauza jedinicna zbog propagacije jedinicnog literala.
    *
    * @details Klauza je jedinicna ako za svaki literal klauze osim jednog, parcijalna valuacija
    * sadrzi njemu suprotan literal. Ovaj jedan je nedefinisan.
    * @param c - klauza koja se proverava
    * @return literal koji je jedini nedefinisan u tekucoj klauzi
    */
    Literal isClauseUnit(const Clause &c) const;

    /**
    * @brief firstUndefined - trazi prvi nedefinisani literal u valuaciji
    *
    * @details Sluzi za izbor literala za decide pravilo. Ovde bi mogla da se ubaci neka heuristika
    * za biranje najpodesnijeg literala, trenutno se uzima od svih nedefinisanih onaj sa najmanjim
    * indeksom.
    * @return nedefinisani literal sa najmanjim indeksom
    */
    Literal firstUndefined() const;

    /**
    * @brief reset - postavlja parcijalnu valuaciju u pocetnu poziciju
    *
    * @details Sve promenljive se postavljaju na Tribool::Undefined, a stek se prazni.
    * @param nVars - broj promenljivih
    */
    void reset(unsigned nVars);

    std::vector<Choice>& stack()
    {
        return m_stack;
    }

    std::vector<LiteralInfo>& values()
    {
        return m_values;
    }

    friend std::ostream& operator<<(std::ostream &out, const PartialValuation &pval);
private:

    /**
     * @brief ClearVariable - clears info of variable which the given literal is referring to
     * @param l - literal whose variable shall be cleared
     */
    void ClearVariable(Literal l);

    /**
     * @brief divideWeightsBy how much to divide weigts when calling their update
     */
    const float divideWeightsBy = 2.0f;

    // TODO: mi ne koristimo rampu, tako da nam ovo ne treba
    /**
     * @brief c_stackSizeMultiplier - for each decided literal there will be
     * a ramp, so possibly 2x number of literals in stack
     */
    const unsigned c_stackSizeMultiplier = 2;

    /**
    * @brief m_values - vector of variable values and their levels
    */
    std::vector<LiteralInfo> m_values;

    /**
    * @brief m_stack - holds the history of selected literals
    */
    std::vector<Choice> m_stack;

};

#endif // PARTIALVALUATION_H
