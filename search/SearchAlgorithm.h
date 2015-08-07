// Author: T. Engesser
// (c) 2014

#ifndef SEARCHALGORITHM_H_
#define SEARCHALGORITHM_H_

#include "../EpistemicModeling.h"
#include "../FormulaManager.h"
#include "../heuristics/Heuristic.h"
#include <vector>
#include <algorithm>

class Heuristic;

/// Plan Structure
struct Plan
{
  /// plan validity flag
  bool ok;
  /// plan mapping mapping each state to the appropriate action
  std::map<EPState, int> mapping;
  /// a string representation
  std::string stringRep;
  /// the plan depth
  int depth;

  Plan () :
    ok(false), stringRep("no solution"), depth(-1) { }
};

/// Abstract Search Algorithm Class
class SearchAlgorithm
{
  
public:
  SearchAlgorithm(const PlanningProblem & problem,
                  const GroundedProblem & gp,
                  const PlanningScenario & ps,
                  const Heuristic * h,
                  const int & agent,
                  const bool & isH = false);
  /// the abstract search method
  virtual Plan search(const EPState * s = NULL) = 0;
  /// print search information (created, expanded nodes etc.)
  virtual void printSearchInfo() = 0;
  /// return a previously found plan
  Plan getWitness();
  /// set stop flag to abort search procedure
  void stopSearch();

protected:
  /// the epistemic planning problem
  PlanningProblem problem;
  /// the problem description
  GroundedProblem gp;
  /// the cooperative planning scenario
  const PlanningScenario ps;
  /// the heuristics
  const Heuristic * h;
  /// the planning agent
  int agent;

  /// the formula manager reference
  const FormulaManager & fm;
  /// a found plan
  Plan witness;
  /// the stop flag
  bool stop;
  /// the heuristic flag
  bool isH;
};

#endif // SEARCHALGORITHM_H_
