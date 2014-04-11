// Author: T. Engesser
// (c) 2014

#ifndef EPISTEMICMODELING_H_
#define EPISTEMICMODELING_H_

#include "GlobalSettings.h"
#include "maepl/PlanningTask.h"

#if USEBITSETS
#include <boost/dynamic_bitset.hpp>
#endif

#include <vector>
#include <set>

class PlanningTask;

enum SearchMode { SMLinear, SMConditional };
enum PlanningScenario { PSNormal, PSCollective, PSIndividual };

// an epistemic action
struct EpistemicAction
{
  EpistemicAction(const int & nAgents, const int & nEvents,
                  const int & agent,
                  const int ** eqc, const int * prec,
#if USEBITSETS
                  const boost::dynamic_bitset<> * add_effects,
                  const boost::dynamic_bitset<> * del_effects,
#else
                  const std::vector<int> * add_effects,
                  const std::vector<int> * del_effects,
#endif
                  const std::vector<bool> * designated);
  ~EpistemicAction();
  /// number of agents
  const int nAgents;
  /// number of events in the event model
  const int nEvents;
  /// the executing agent
  const int agent;
  /// for each agent/event: assigned equivalence classes
  const int ** eqc;
  /// for each event: precondition formula
  const int * prec;
#if USEBITSETS
  /// for each event: add effects (as bitset)
  const boost::dynamic_bitset<> * add_effects;
  /// for each event: delete effects (as bitset)
  const boost::dynamic_bitset<> * del_effects;
#else
  /// for each event: add effects (vector of propositions)
  const std::vector<int> * add_effects;
  /// for each event: delete effects (vector of propositions)
  const std::vector<int> * del_effects;
#endif
  /// for each event: is world designated
  const std::vector<bool> * designated;

private:
  EpistemicAction(const EpistemicAction & source);
};

/// an epistemic state \f$(M,W_d)\f$ with \f$M = (W,R,V)\f$
struct EpistemicState
{
  EpistemicState();
  ~EpistemicState();
  EpistemicState(const int & nAgents, const int & nPropositions,
                 const int & nWorlds, const int ** eqc,
#if USEBITSETS
                 const boost::dynamic_bitset<> * val,
#else
                 const char ** val,
#endif
                 const EpistemicState * parent,
                 const EpistemicAction * action,
                 const std::vector<int> * fromWorld,
                 const std::vector<int> * fromEvent,
                 const std::vector<bool> * designated);

  /// associated local state of an epistemic state for an agent
  EpistemicState(const EpistemicState & source, const int & agent);

  /// copy of epistemic state with other designated set
  EpistemicState(const EpistemicState & source,
                 const std::set<int> & designated);

  /// product update of an epistemic state and an epistemic action
  EpistemicState * product(const EpistemicAction & action,
                           bool & ok, bool & oka) const;

  /// number of agents
  const int nAgents;
  /// number of propositions (world variables)
  const int nPropositions;
  /// number of worlds \f$|w|\f$
  const int nWorlds;
  /// for each agent/world: assigned equivalence class
  const int ** eqc;
  /// valuation function
#if USEBITSETS
  const boost::dynamic_bitset<> * val;
#else
  const char ** val;
#endif
  /// parent state, if existing
  const EpistemicState * parent;
  /// action that led from parent to current state
  const EpistemicAction * action;
  /// for each world: parent world
  const std::vector<int> * fromWorld;
  /// for each world: parent event
  const std::vector<int> * fromEvent;
  /// for each world: is world designated
  const std::vector<bool> * designated;

  /// memo table - true formulas per world
#if MEMOIZATION
  mutable MAP<int, bool> * memo;
#endif

private:
  EpistemicState(const EpistemicState & source);
  bool induced;
};

/// incomplete bisimulation contraction
EpistemicState * contract(const int & nAgents, const int & nPropositions,
                          int nWorlds, int ** eqc,
                          boost::dynamic_bitset<> * val,
                          const EpistemicState * parent,
                          const EpistemicAction * action,
                          std::vector<int> * fromWorld,
                          std::vector<int> * fromEvent,
                          std::vector<bool> * designated);

/// explicit copy of epistemic state
EpistemicState * newEpistemicStateCopy(const EpistemicState * source);
/// explicit copy of epistemic state, with contraction
EpistemicState * newEpistemicStateCopyC(const EpistemicState * source);


/// a smart pointer containing an epistemic state
class EPState
{
public:
  EPState(const EpistemicState * state);
  EPState(const EPState & source);
  EPState();
  ~EPState();

  EPState & operator=(const EPState & source);

  /// product update with global and agent applicability
  EPState product(const EpistemicAction & action,
                  bool & ok, bool & oka) const;
  /// associated local state and product update
  EPState aProduct(const EpistemicAction & action, bool & ok) const;
  /// the associated local state
  EPState associatedLocal(const int & agent);
  /// calculate abstract state
  EPState abstract(const boost::dynamic_bitset<> & delMask) const;
  /// calculate relaxed state
  EPState relax(const int & depth) const;
  /// calculate run-time distingshable splits
  std::vector<EPState> split() const;
  /// evaluate state with respect to formula
  bool eval(const int & phi) const;

  /// return number of worlds
  int getSize() const;
  
  /// state ordering
  bool operator<(const EPState& other) const;
  /// state equality
  bool operator==(const EPState& other) const;

  friend class PlanningProblem;
  friend class PrettyPrint;

private:
  /// the encapsulated state
  const EpistemicState * state;
  /// the instance counter
  int * count;
};

/// a planning problem
struct PlanningProblem
{ 
  PlanningProblem(const std::string & name,
                  const EpistemicState * initState,
                  const std::vector<EpistemicAction*> actions,
                  const int & goal);
  PlanningProblem(const PlanningProblem & source);
  PlanningProblem(const PlanningProblem & source,
                  const EPState & current);
  /// the problem name
  const std::string name;
  /// the initial epistemic state
  const EpistemicState * initState;
  /// the set of available epistemic actions
  const std::vector<EpistemicAction*> actions;
  /// the goal formula
  const int goal;
};

/// convert grounded problem to epistemic planning problem
PlanningProblem generatePP(const GroundedProblem & p);

/// evaluate state with respect to formula
bool evaluate(const EpistemicState & state, const int & phi);

/// evaluate state/world pair with respect to formula
bool evaluate(const EpistemicState & state, const int & world, 
                                            const int & phi);

//bool evaluate(const EpistemicState & state, int agent, int phi);

/// perform an abstraction
EpistemicState * absState(const EpistemicState & source2,
                          const boost::dynamic_bitset<> & delMask);

/// perform a relaxation
EpistemicState * relState(const EpistemicState & source2,
                          const int & depth);


#endif // EPISTEMICMODELING_H_
