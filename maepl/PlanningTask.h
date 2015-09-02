// Author: T. Engesser
// (c) 2014

#ifndef PLANNINGTASK_H_
#define PLANNINGTASK_H_

#include <string>
#include <vector>
#include <set>
#include <map>

#include "maepl.h"
#include "data.h"

#include "../EpistemicModeling.h"

enum NameType { CHILD, PARENT };

class PlanningProblem;


/// class for parsing domains / problems and grounding them
class PlanningTask
{
public:
  PlanningTask();
  /// parse a flie;
  bool parse(const char * filename);
  /// perform grounding
  std::vector<GroundedProblem> ground();
  /// for debugging: print out parsed information             
  void debugPrint() const;
  
 
  /// push a new domain onto domain stack
  bool newDomain(std::string s);
  /// push a new problem onto problem stack
  bool newProblem(std::string s);

  /// set superdomain
  bool setSuperDomains();

  /// retrieve domain types 
  void setDomainTypes();
  /// retrieve domain specific (const) objects
  bool setDomainObjects();
  /// retrieve domain predicates
  bool setDomainPred(std::string s); 
  /// retrieve domain actions
  bool setDomainAction(std::string s);

  /// set executing agent
  bool setActionAgent(std::string s);
  /// setup new named designated or nondesignated event 
  bool setDomainEvent(std::string s, bool d);
  /// for event: set precondition
  void setEventGoalDescription(Formula * f);
  /// for event: set effect
  void setEventEffect(Formula * f);

  /// set domain to be used by problem
  bool setProblemDomain(std::string s);
  /// retrieve problem specific objects
  bool setProblemObjects();
  /// retrieve global or world initialization literals
  void setProblemInits(std::string n, bool d, const Formula * f);
  /// set goal formula
  void setProblemGoal(Formula * f);

  /// for agents: setup events / worlds observability partitions
  bool setObservability(const std::vector<std::string> & a,
                        const Partition * p);
  /// set default observability to "none", if it has not been set already
  void setDefaultObservability();
  /// generate events / worlds partition by template
  Partition * genTemplatePartition(const std::string & s);

  /// build quantified formula and remember its scope for semantic checks
  Formula * beginQuantifiedFormula(FType type, bool & warning);
  /// remove variables from scopestack
  void finishQuantifiedFormula();

  /// return parsed typed list as map
  stringmmap getTypedMap();
  /// return parsed untyped list
  std::vector<std::string> getNameList();
  /// for parsing (typed) list: input element or type
  void inputName(std::string s);
  /// for parsing (typed) lists: parse elements
  void setParseChild();
  /// for parsing (typed) lists: parse types
  void setParseParent();
  /// for parsing (typed) list: finish with untyped rest list
  void finishList();

  /// perform atom validity check (types of vars/consts)
  bool checkAtom(Formula * atom);

private:
  
  /// perform type validity check for typed lists
  bool checkTypedList(bool types, bool unique);
  /// perform check for argument names // TODO: & type for validity
  bool checkArgument(std::string argument, std::set<std::string> types);

  /// domain stack
  std::map<std::string, Domain*>  ivDomains;
  /// problem stack
  std::map<std::string, Problem*> ivProblems;
 
  // parsing state variables
  bool          ivParseDom; // am I parsing domaim or problem
  NameType      ivNameType; // am I receiving names or types
  Domain *      ivDom;      // the domain I am parsing or I just parsed
  Problem *     ivProb;     // the problem I am parsing or I just parsed
  Action *      ivAction;   // the action I am parsing or I just parsed
  Formula *     ivGD;       // the last goal description I parsed
  Formula *     ivEff;      // the last effect I parsed
  std::vector<stringmmap> ivScopeStack;

  // lists for parsing (typed) names/variables
  std::vector<elem>        ivTypedList;   // the last typed list I parsed
  std::vector<std::string> ivNewChildren; // the last untyped sequence..
  std::set<std::string>    ivNewParents;  // the last either sequence..
};

/// convert typed list to typed map
template <class Key, class Val>
std::map<Key, Val> vToM(const std::vector< std::pair<Key, Val> > & v);

#endif
