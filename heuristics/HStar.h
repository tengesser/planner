// Author: T. Engesser
// (c) 2014

#ifndef HSTAR_H
#define HSTAR_H

#include "Heuristic.h"
#include <iostream>

template <class S>
class HStar : public Heuristic
{
  public:
    HStar(const PlanningProblem & problem,
               const GroundedProblem & gp,
               const PlanningScenario & ps,
               const Heuristic * hsub = NULL);
    virtual int h(const EPState & state) const;
    virtual void printHeurInfo() const;
  private:
    /// the heuristic search
    S * planner;
};

template <class S>
HStar<S>::HStar(const PlanningProblem & problem,
                const GroundedProblem & gp,
                const PlanningScenario & ps,
                const Heuristic * hsub)
  : Heuristic(problem,gp,ps,NULL,
        hsub != NULL ? hsub : new Heuristic(problem,gp,ps)),
      planner(new S(problem, gp, ps, this->hsub)) { }

template <class S>
int HStar<S>::h(const EPState & state) const
{
  return planner->search(&state).depth;
}

template <class S>
void HStar<S>::printHeurInfo() const
{
  planner->printSearchInfo();
}



#endif // HSTAR_H
