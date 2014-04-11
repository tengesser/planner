// Author: T. Engesser
// (c) 2014

#ifndef HEURISTIC_H
#define HEURISTIC_H

#include "../EpistemicModeling.h"
#include "../search/SearchAlgorithm.h"

class Heuristic
{
  public:
    Heuristic(const PlanningProblem & problem,
               const GroundedProblem & gp,
               const PlanningScenario & ps,
               const EPState (*convert)(EPState) = NULL,
               const Heuristic * hsub = NULL);
    /// the heuristic function
    virtual int h(const EPState & state) const;
    /// state conversion function (returns s or convert(s))
    EPState convertState(const EPState & s) const;
    /// output of search-specific information
    virtual void printHeurInfo() const;

  protected:
    /// the epistemic planning problem
    const PlanningProblem & problem;
    /// the problem description
    const GroundedProblem & gp;
    /// the cooperative planning scenario
    const PlanningScenario & ps;
    /// the heuristic-specific state-conversion function 
    const EPState (*convert)(EPState);
    /// the sub-heuristic
    const Heuristic * hsub;
};

#endif // HEURISTIC_H
