// Author: T. Engesser
// (c) 2014

#ifndef ABSTRACTION_H
#define ABSTRACTION_H

#include "Heuristic.h"
#include <vector>
#include <iostream>
#include "../PrettyPrint.h"

boost::dynamic_bitset<> genDelMask (const GroundedProblem & gp);
std::set<int> genPAbs(const GroundedProblem & gp);
PlanningProblem abstractProblem(const PlanningProblem & prob,
                             const std::set<int> & pAbs,
                             const boost::dynamic_bitset<> & delMask);

template <class S>
class Abstraction : public Heuristic
{
  public:
    Abstraction(const PlanningProblem & problem,
                const GroundedProblem & gp,
                const PlanningScenario & ps,
                const Heuristic * hsub = NULL);
    virtual int h(const EPState & state) const;
    virtual void printHeurInfo() const;
  private:
    /// the propositions to abstract
    const std::set<int> pAbs;
    /// the delete bitset
    const boost::dynamic_bitset<> delMask;
    /// the abstract planning problem
    const PlanningProblem aProblem;
    /// the heuristic search
    S * planner;
};

template <class S>
Abstraction<S>::Abstraction(const PlanningProblem & problem,
                            const GroundedProblem & gp,
                            const PlanningScenario & ps,
                            const Heuristic * hsub)
  : Heuristic(problem,gp,ps,NULL,
      hsub != NULL ? hsub : new Heuristic(problem,gp,ps)),
    pAbs(genPAbs(gp)), delMask(genDelMask(gp)),
    aProblem(abstractProblem(problem, pAbs, delMask))
{
/*std::cout << aProblem.name << std::endl;
  std::cout << "Goal:" << std::endl;
  std::cout << PrettyPrint::toString(problem.goal) << " => " << std::endl;
  std::cout << PrettyPrint::toString(aProblem.goal) << std::endl;
  for (unsigned int p = 0; p < gp.variables.size(); ++p)
    std::cout << p << ": " << gp.variables[p] << std::endl;
  for (unsigned int a = 0; a < problem.actions.size(); ++a)
  {
    std::cout << "Action " << gp.actions[a].name << std::endl;
    for (int e = 0; e < problem.actions[a]->nEvents; ++e)
    {
      std::cout << "Prec[" << e << "]" << std::endl;
      std::cout << PrettyPrint::toString(problem.actions[a]->prec[e])
                << " => " << std::endl;
      std::cout << PrettyPrint::toString(aProblem.actions[a]->prec[e])
                << std::endl;
    }
  }*/
  planner = new S(aProblem, gp, ps, this->hsub);
  std::cerr << "WARNING: ABSTRACTION HEURISTIC NOT "
            << "FUNCTIONAL YET!" << std::endl;
}

template <class S>
void Abstraction<S>::printHeurInfo() const
{
  planner->printSearchInfo();
}


template <class S>
int Abstraction<S>::h(const EPState & state) const
{
  EPState aState = state.abstract(delMask);
  int result = planner->search(&aState).depth;
  // std::cout << result << std::endl;
  return result;
}

#endif // ABSTRACTION_H
