// Author: T. Engesser
// (c) 2014

#include "Relaxation.h"

const EPState srel(EPState state)
{
  return state.relax(0); // TODO: parametrize depth
}

const PlanningProblem relaxProblem(const PlanningProblem & problem,
                                   const int & depth,
                                   const bool & removeDel)
{
  const std::string name = problem.name + "_REL";
  const EpistemicState * initState
    = relState(*problem.initState, depth);
  std::vector<EpistemicAction*> actions;
  for (std::vector<EpistemicAction*>::const_iterator
         it = problem.actions.begin(); it != problem.actions.end();
         ++it)
  {
    const int nAgents = (*it)->nAgents;
    const int nEvents = (*it)->nEvents;
    const int agent = (*it)->agent;
    int ** eqc = new int*[nAgents];
    for (int a = 0; a < nAgents; ++a)
    {
      eqc[a] = new int[nEvents];
      for (int e = 0; e < nEvents; ++e)
        eqc[a][e] = (*it)->eqc[a][e];
    }
    int * prec = new int[nEvents];
    boost::dynamic_bitset<> * add_effects
      = new boost::dynamic_bitset<>[nEvents];
    boost::dynamic_bitset<> * del_effects
      = new boost::dynamic_bitset<>[nEvents];
    boost::dynamic_bitset<> noEff((*it)->del_effects[0].size(), 0);
    for (int e = 0; e < nEvents; ++e)
    {
      prec[e] = (*it)->prec[e];
      add_effects[e] = (*it)->add_effects[e];
      del_effects[e] = removeDel ? noEff
                                 : (*it)->del_effects[e];
    }
    std::vector<bool> * designated
      = new std::vector<bool>;
    designated[0] = (*it)->designated[0];
    actions.push_back(
      new EpistemicAction(nAgents, nEvents, agent,
                          const_cast<const int**>(eqc),
                          prec, add_effects, del_effects,
                          designated));
  }
  const int goal = problem.goal;
  return PlanningProblem(name, initState, actions, goal);
}
