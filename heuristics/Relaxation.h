// Author: T. Engesser
// (c) 2014

#ifndef RELAXATION_H
#define RELAXATION_H

#include "Heuristic.h"
#include <vector>
#include <iostream>
#include "../PrettyPrint.h"

template <class S>
class Relaxation : public Heuristic
{
  public:
    Relaxation(const PlanningProblem & problem,
               const GroundedProblem & gp,
               const PlanningScenario & ps,
               const Heuristic * hsub = NULL,
               const int & depth = 0,
               const bool & removeDel = false);
    virtual int h(const EPState & state) const;
    virtual void printHeurInfo() const;
  private:
    /// the relaxed problem
    const PlanningProblem rProblem;
    /// the heuristic search
    S * planner;
    /// the cut depth
    int depth;
};

/// relex a planning problem
const PlanningProblem relaxProblem(const PlanningProblem & problem,
                                   const int & depth,
                                   const bool & removeDel);

/// calculate the relaxation of an epistemic state
const EPState srel(EPState state);

template <class S>
Relaxation<S>::Relaxation(const PlanningProblem & problem,
                          const GroundedProblem & gp,
                          const PlanningScenario & ps,
                          const Heuristic * hsub,
                          const int & depth,
                          const bool & removeDel)
  : Heuristic(problem, gp, ps, NULL,
      hsub != NULL ? hsub : new Heuristic(problem,gp,ps,&srel,NULL)),
    rProblem(relaxProblem(problem, depth,removeDel)),
    planner(new S(rProblem, gp, ps, this->hsub)), depth(depth) { }

template <class S>
int Relaxation<S>::h(const EPState & state) const
{
  EPState rState = state.relax(depth);
  return planner->search(&rState).depth;
}

template <class S>
void Relaxation<S>::printHeurInfo() const
{
  planner->printSearchInfo();
}



#endif // RELAXATION_H
