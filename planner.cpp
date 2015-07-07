// Author: T. Engesser
// (c) 2014

#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "EpistemicModeling.h"
#include "PrettyPrint.h"
#include "maepl/maepl.h"
#include "search/SearchAlgorithm.h"
#include "search/EAStar.h"
#include "search/EAOStar.h"
#include "heuristics/Heuristic.h"
#include "heuristics/Abstraction.h"
#include "heuristics/Relaxation.h"
#include "heuristics/HStar.h"

enum HeuristicMode { HMAbs, HMRel, HMHStar, HMNone };

int main(int argc, char* argv[])
{
  // open argument files
  if (argc < 4)
  {
    std::cerr << "Not enough Arguments" << std::endl;
    std::cout << "usage: planner SEARCH_MODE PLANNING_SCENARIO HEURISTICS"
              << " [FILES]" << std::endl;
    std::cout << " with SEARCH_MODE in {linear,conditional}" << std::endl;
    std::cout << " with PLANNING_SCENARIO in "
              << "{normal,collective,individual}" << std::endl
              << " with HEURISTICS in "
              << "{none,abstraction,relaxation,exact}"
              << std::endl;
    return -1;
  }

  std::string cmp;

  // determine search mode
  SearchMode sm;
  cmp = argv[1];
  if (cmp == "linear")
    sm = SMLinear;
  else if (cmp == "conditional")
    sm = SMConditional;
  else
  {
    std::cerr << "Search Mode " << cmp
              << " not recognized." << std::endl;
    return -1;
  }
  // determine planning scenario
  PlanningScenario ps;
  cmp = argv[2];
  if (cmp == "normal")
    ps = PSNormal;
  else if (cmp == "collective")
    ps = PSCollective;
  else if (cmp == "individual")
    ps = PSIndividual;
  else
  {
    std::cerr << "Planning Scenario " << cmp
              << " not recognized." << std::endl;
    return -1;
  }
  // determine heuristic
  HeuristicMode hm;
  cmp = argv[3];
  if (cmp == "abstraction")
    hm = HMAbs;
  else if (cmp == "relaxation")
    hm = HMRel;
  else if (cmp == "exact")
    hm = HMHStar;
  else if (cmp == "none")
    hm = HMNone;
  else
  {
    std::cerr << "Heuristic " << cmp
              << " not recognized." << std::endl;
    return -1;
  }
 
  // now, parse stuff into planning task
  PlanningTask task;
  for (int i = 4; i < argc; ++i)
    task.parse(argv[i]);
  // task.debugPrint();
  
  // and ground task to problems
  std::vector<GroundedProblem> probs = task.ground();
  
  for (std::vector<GroundedProblem>::const_iterator
         it = probs.begin(); it != probs.end(); ++it)
  {
    PlanningProblem p = generatePP(*it);
    // std::cout << "GENERATED PLANNING PROBLEM" << std::endl;
    // std::cout << PrettyPrint::toString(*(p.initState), &*it, false)
    //          << std::endl;
    //
    // for (std::vector<EpistemicAction*>::const_iterator
    //       ita = p.actions.begin(); ita != p.actions.end(); ++ita)
    //   std::cout << PrettyPrint::toString(**ita) << std::endl;
    
    std::cout << "Problem " << it->name << ":" << std::endl;

    cmp = argv[3];

    SearchAlgorithm * planner;
    Heuristic * h = new Heuristic(p,*it,ps);
    switch (sm)
    {
      case (SMLinear):
      {
        if (hm == HMAbs)
          h = new Abstraction<EAStar>(p,*it,ps);
        else if (hm == HMRel)
          h = new Relaxation<EAStar>(p,*it,ps);
        else if (hm == HMHStar)
          h = new HStar<EAStar>(p,*it,ps);
        planner = new EAStar(p, *it, ps, h);
        break;
      }
      case (SMConditional):
      {
        if (hm == HMAbs)
          h = new Abstraction<EAOStar>(p,*it,ps);
        else if (hm == HMRel)
          h = new Relaxation<EAOStar>(p,*it,ps); // NULL,0,false
        else if (hm == HMHStar)
          h = new HStar<EAOStar>(p,*it,ps);
        planner = new EAOStar(p, *it, ps, h);
        break;
      }
    }

    std::cout << planner->search().stringRep << std::endl;
    std::cout << "\t#NExp\t#NCrt\t#NOmit\t#HHitS\t#HHitUS"
              << "\t#Wtot\t#Wmax\tPlanDepth" << std::endl;
    std::cout << "Search:\t"; planner->printSearchInfo();
    std::cout << "Heur:\t"; h->printHeurInfo();
  }
 
  return 0;
}
