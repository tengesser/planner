// Author: T. Engesser
// (c) 2014

#ifndef EAOSTAR_H
#define EAOSTAR_H

#include "SearchAlgorithm.h"
#include <boost/heap/fibonacci_heap.hpp>
#include <map>

struct EAONode;

class EAOcompare
{
  public:
    bool operator()(EAONode * const & n1, EAONode * const & n2) const;
};

typedef boost::heap::fibonacci_heap< EAONode *, 
        boost::heap::compare<EAOcompare> > MinHeap;
typedef boost::heap::fibonacci_heap< EAONode *,
        boost::heap::compare<EAOcompare> >::handle_type MHHandle;

struct AncestorInfo
{
  EAONode * const parent;
  int const split;
  int const action;

  AncestorInfo(EAONode * const parent,
               const int & split, const int & action);
  bool operator<(const AncestorInfo & other) const;
};

enum SolveState { SSUnsolvable, SSSearch, SSSolved }; 

struct EAONode
{
  /// the epistemic state which is to be solved
  const EPState state;
  /// the node's g-value (shortest path from initial node)
  int gvalue;
  /// the node's heuristic value
  int hvalue;
  /// node ancestry information - direct parents with their split/actions
  std::set<AncestorInfo> parents;
  /// to solve a state, for each splitState a subsolution has to be found 
  std::vector<EPState> splitStates;
  /// the states reachable from a splitState, if one is solved so is the splitState
  std::map<int, EAONode*> * children;
  /// marks if a split state is solved
  bool * solvedChildren;
  /// the action using which the split state is solved 
  int * solvedActions;
  /// the length of the solve
  int * solvedLength;
  /// the overall node solve state (yet undetermined, solved, unsolvable)
  SolveState solveState;
  /// max(solvedLength)
  int maxGoalDist;
  /// a handle referencing the node in a minheap for updating purposes
  MHHandle * mhh;

  EAONode(const EPState & state, const int & hvalue, EAONode * parent,
          const int & parentSplit, const int & parentAction);
};

class EAOStar : public SearchAlgorithm
{
public:
  EAOStar(const PlanningProblem & problem,
          const GroundedProblem & gp,
          const PlanningScenario & ps,
          const Heuristic * h,
          const int & agent = -1,
	  const bool & isH = false) :
    SearchAlgorithm(problem, gp, ps, h, agent, isH),
    expanded(0), ommited(0), perfect(0),
    seenbefore(0), worlds(0), wmax(0) { }
  virtual Plan search(const EPState * s = NULL);
  virtual void printSearchInfo();

private:
  Plan extractPlan(EAONode * fromNode);
  Plan extractPlan(EAONode * fromNode, std::string indent);
  bool propagateSolved(EAONode * toNode, EAONode * fromNode);
  bool propagateSolved(EAONode * toNode, EAONode * fromNode, 
                       const int & pathLength, const int & fromState,
                       const int & fromAction);
  EAONode * queueNewOrExistingNode(const EPState & state,
                                   EAONode * parent, const int & pSplit,
                                   const int & pAction,
                                   std::map<EPState,EAONode*> & createdGlobal,
                                   std::map<EPState,EAONode*> & createdLocal,
                                   MinHeap & heap);
  /// index created states -> node in tree
  std::map<EPState, EAONode*> created;
  // search information
  int expanded;
  int ommited;
  int perfect;
  int seenbefore;
  int worlds;
  int wmax;
};


#endif // EAOSTAR_H

