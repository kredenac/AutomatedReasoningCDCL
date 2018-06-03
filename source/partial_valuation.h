#ifndef PARTIALVALUATION_H
#define PARTIALVALUATION_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>

/**
 * @brief The Tribool enum - koristimo da kodiramo 3 vrednosti za promenljivu u parcijalnoj valuaciji.
 * 
 * @details Za razliku od C-a u C++-u je moguce kontrolisati tip enumeracije. Konkretno nama 
 * trebaju 3 vrednosti zbog cega nam je 1 bajt i vise nego dovoljan. 
 */
enum class Tribool: int8_t
{
    False,
    True,
    Undefined
};

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
    PartialValuation(unsigned nVars = 0);

    /**
    * @brief push - na osnovu literala postavlja vrednost promenljive u parcijalnoj valuaciji.
    * @param l - literal nas osnovu koga se postavlja vrednost
    * @param decide - ukoliko je 'decide' flag true oznacava se da je u pitanju decide literal
    */
    void push(Literal l, bool decide = false);

    /**
    * @brief backtrack - skida literale sa steka sve do prvog decide literala na koji naidje
    * @return poslednji decide literal ili NullLiteral ukoliko takvog nema
    */
    Literal backtrack();

    /**
     * @brief backtrack to when the first-assigned variable involved in the conflict was assigned
     * @param cut - clause of variables involved in the conflict
     * @return first decided literal that is also in the given clause
     */
    Literal backtrack(Clause& cut);

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

    friend std::ostream& operator<<(std::ostream &out, const PartialValuation &pval);
private:
    /**
     * @brief c_stackSizeMultiplier - for each decided literal there will be
     * a ramp, so possibly 2x number of literals in stack
     */
    const unsigned c_stackSizeMultiplier = 2;

    /**
    * @brief m_values - vrednost promenljivih u valuaciji
    */
    std::vector<Tribool> m_values;

    /**
    * @brief m_stack - stek na kome cuvamo istoriju postavljanja vrednosti promenljivih zbog vracanja unazad
    */
    std::vector<Literal> m_stack;
};

#endif // PARTIALVALUATION_H
