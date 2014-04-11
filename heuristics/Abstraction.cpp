// Author: T. Engesser
// (c) 2014

#include "Abstraction.h"

std::set<int> genPAbs(const GroundedProblem & gp)
{
  std::set<int> result;
  for (unsigned int i = 0; i < gp.variables.size(); ++i)
    if (gp.variables[i].find("_abs_") != std::string::npos)
    {
      // std::cout << i << "," << gp.variables[i] << std::endl;
      result.insert(i);
    }
  return result;
}

boost::dynamic_bitset<> genDelMask (const GroundedProblem & gp)
{
  boost::dynamic_bitset<> result(gp.variables.size());
  for (unsigned int i = 0; i < gp.variables.size(); ++i)
      result[i] = gp.variables[i].find("_abs_") == std::string::npos;
  return result;
}

EpistemicAction * absAction(const EpistemicAction * source,
                            const boost::dynamic_bitset<> & delMask,
                            const std::set<int> & pAbs)
{
  int nAgents = source->nAgents;
  int nEvents = source->nEvents;
  int agent = source->agent;
  int ** eqc = new int*[nAgents];
  for (int a = 0; a < nAgents; ++a)
  {
    eqc[a] = new int[nEvents];
    for (int e = 0; e < nEvents; ++e)
      eqc[a][e] = source->eqc[a][e];
  }
  int * prec = new int[nEvents];
  boost::dynamic_bitset<> * add_effects = 
    new boost::dynamic_bitset<>[nEvents];
  boost::dynamic_bitset<> * del_effects =
    new boost::dynamic_bitset<>[nEvents];
  for (int e = 0; e < nEvents; ++e)
  {
    prec[e] = FormulaManager::getInstance().
                genAbstraction(source->prec[e], pAbs);
    add_effects[e] = source->add_effects[e] & delMask; // ok?
    del_effects[e] = source->del_effects[e] & delMask; // ok?
  }
  std::vector<bool> * designated = new std::vector<bool>[1];
  designated[0] = source->designated[0];

  return new EpistemicAction(nAgents, nEvents, agent, 
                             const_cast<const int**>(eqc), prec,
                             add_effects, del_effects, designated);
}


PlanningProblem abstractProblem(const PlanningProblem & prob,
                                const std::set<int> & pAbs,
                                const boost::dynamic_bitset<> & delMask)
{
  std::vector<EpistemicAction*> actions;
  for (std::vector<EpistemicAction*>::const_iterator
         it = prob.actions.begin(); it != prob.actions.end(); ++it)
    actions.push_back(absAction(*it, delMask, pAbs));
  const EpistemicState * initState = absState(*prob.initState, delMask);
  return PlanningProblem(prob.name + "_ABSTRACT", initState,
                         actions, FormulaManager::getInstance().
                                    genAbstraction(prob.goal, pAbs));
                         

}
