// Author: T. Engesser
// (c) 2014

#ifndef EASTAR_H
#define EASTAR_H

#include "SearchAlgorithm.h"
#include <queue>
#include <stack>
#include <vector>

struct EANode {
  /// the epistemic state
  const EPState state;
  /// the node's g-value (path length to node)
  int gvalue;
  /// the node's heursitic value
  const int hvalue;
  /// parent node, if existant
  const EANode* parent;
  /// action performed from parent state, if existant
  const int action;
  /// children list
  std::vector<EANode*> children;
  /// goal distance, or -1 if unsolved
  mutable int goalDist;

  EANode(const EPState state) :
    state(state), gvalue(0), hvalue(0), parent(NULL), action(-1),
    goalDist(-1) {}
  EANode(const EPState state, const int & hvalue,
         const EANode * parent, const int & action) :
    state(state), gvalue(parent->gvalue + 1), hvalue(hvalue),
    parent(parent), action(action), goalDist(-1) { }
};

class EAcompare { // simple comparison function
  public:
    bool operator()(const EANode * n1, const EANode * n2) 
    {
      if (n1->hvalue == -1 && n2->hvalue != -1)
        return true;
      else if (n1->hvalue != -1 && n2->hvalue == -1)
        return false;
      else
        return n1->gvalue + n1->hvalue > n2->gvalue + n2->hvalue;
    }
};

class EAStar : public SearchAlgorithm
{
public:
  EAStar(const PlanningProblem & problem,
         const GroundedProblem & gp,
         const PlanningScenario & ps,
         const Heuristic * h,
         const int & agent = -1,
         const bool & isH = false) :
    SearchAlgorithm(problem, gp, ps, h, agent, isH), exp(0), crt(1), omit(0) { }
  virtual Plan search(const EPState * s = NULL);
  virtual void printSearchInfo();
  // returns a full plan the first time it is used
  //   and maybe partial plans to already solved states thereafter

private:
  Plan extractPlan(const EANode * toNode,
                   const EANode * fromNode = NULL,
                   const int & goalDist = 0);
  std::map<EPState, EANode*> createdStates;
  int exp, crt, omit;
};



#endif
