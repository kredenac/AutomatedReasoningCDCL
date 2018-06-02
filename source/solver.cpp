#include "solver.h"

#include <string>
#include <sstream>
#include <stdexcept>
#include <iterator>

Solver::Solver(const CNFFormula &formula)
  : m_formula(formula), m_valuation(m_formula.size())
{
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
    throw std::runtime_error{"Pogresan format ulaza iz DIMACS stream-a"};
  }
  std::istringstream parser{line.substr(firstNonSpaceIdx+1, std::string::npos)};
  std::string tmp;
  if (!(parser >> tmp) || tmp != "cnf")
  {
    throw std::runtime_error{"Pogresan format ulaza iz DIMACS stream-a"};
  }
  unsigned varCnt, claCnt;
  if (!(parser >> varCnt >> claCnt))
  {
    throw std::runtime_error{"Pogresan format ulaza iz DIMACS stream-a"};
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
    /* Proverimo da li ima bar 1 konflikt */
    Literal l;
    if (hasConflict())
    {
      /* Radimo backtracking i dobijamo literal koji smo nekad ranije postavili decide pravilom */
      Literal decidedLiteral = m_valuation.backtrack();
      if (NullLiteral == decidedLiteral)
      {
        /* Ne mozemo vise da radimo backtrack, iscrpeli smo sve valuacije */
        return {};
      }
      
      /* Postavljamo suprotnu vrednost literala i nastavljamo */
      m_valuation.push(-decidedLiteral);
    }
    else if ((l = hasUnitClause()))
    {
      /* Propagacija jedinicne klauze */
      m_valuation.push(l);
    }
    else 
    {
      /* Decide pravilo */
      if ((l = m_valuation.firstUndefined()))
      {
        m_valuation.push(l, true);
      }
      else
      {
        /* Ne mozemo da primenimo decide jer imamo punu valuaciju -> formula je zadovoljena */
        return m_valuation;
      }
    }
  }
}

bool Solver::hasConflict() const
{
  for (const Clause &c : m_formula)
  {
    if (m_valuation.isClauseFalse(c))
    {
      return true;
    }
  }
  return false;
}

Literal Solver::hasUnitClause() const
{
  Literal l;
  for (const Clause &c : m_formula)
  {
    if ((l = m_valuation.isClauseUnit(c)))
    {
      return l;
    }
  }
  
  return 0;
}
