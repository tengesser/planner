// Author: T. Engesser
// (c) 2014

#include "Heuristic.h"
#include <iostream>

Heuristic::Heuristic(const PlanningProblem & problem,
                       const GroundedProblem & gp,
                       const PlanningScenario & ps,
                       const EPState (*convert)(EPState),
                       const Heuristic * hsub)
                         : problem(problem), gp(gp),
                           ps(ps), convert(convert), hsub(hsub) { }

void Heuristic::printHeurInfo() const
{
  std::cout << " no heuristics" << std::endl;
}
                           
EPState Heuristic::convertState(const EPState & s) const
{
  if (convert == NULL)
    return s;
  else
    return convert(s);
}
                           
int Heuristic::h(const EPState & state) const
{ 
  return 0;
}
