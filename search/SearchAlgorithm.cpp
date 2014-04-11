// Author: T. Engesser
// (c) 2014

#include "SearchAlgorithm.h"

SearchAlgorithm::SearchAlgorithm(const PlanningProblem & problem,
                                 const GroundedProblem & gp,
                                 const PlanningScenario & ps,
                                 const Heuristic * h,
                                 const int & agent,
                                 const bool & isH) :
  problem(problem), gp(gp), ps(ps), h(h), agent(agent),
  fm(FormulaManager::getInstance()), stop(false), isH(isH) { }

Plan SearchAlgorithm::getWitness()
{
  return witness;
}

void SearchAlgorithm::stopSearch()
{
  stop = true;
}
