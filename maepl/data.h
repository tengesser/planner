// Author: T. Engesser
// (c) 2014

#ifndef DATA_H_
#define DATA_H_

#include <string>
#include <vector>
#include <set>
#include <map>

typedef std::pair< std::string, std::set<std::string> > elem;
typedef std::map< std::string, std::set<std::string> > stringmmap;
typedef std::map<std::string, std::string> stringmap;

/// formula types
enum FType { FATOM, FNOT, FAND, FOR, FIMPLY,
             FEQUALS, FEXISTS, FFORALL, FCOND,
             FKNOWS, FCOMMK, LLIST, FTRUE, FFALSE };

/// formula image: can formula be true, false or both
enum FImage { ITRUE, IFALSE, IBOTH };

/// formulae / literal lists
struct Formula
{
  /// type of the formla (negation, conjunction, knowledge, ..., literals)
  FType                     type;
  /// string representation of grounded atom
  std::string               groundedatom;
  /// relevant predicate or agent for an atomic or knowledge formula
  std::string               name;  
  /// variable/object arguments for an atomic formula or equality testing
  std::vector<std::string>  args;
  /// subformula(e) for a compound formula
  std::vector<Formula*>     subf;
  // precondition (gd) for conditional effect
  Formula*                  whenf;
  /// effect (literals) for conditional effect
  Formula*                  thenf;
  /// variable scopes for a quantified formula
  stringmmap                scope;

  /// set formula type
  Formula * setType(FType t);
  /// set predicate/agent name
  Formula * setName(std::string s);
  /// set variable/object arguments
  Formula * setArgs(std::vector<std::string> a);
  /// add a subformula
  Formula * addSub(Formula * sub);
  /// set variable scope
  Formula * setScope(stringmmap s);

  /// ground a formula - insert objects into atomic parameters
  Formula * ground(const stringmap & parMapping,
                   const stringmmap & types) const;

  /// ground a formula - insert true and false constants
  Formula * ground(const std::set<std::string> & trueconstants,
                   const std::set<std::string> & variables) const;

  Formula * insertTF(const std::set<std::string> & trueconstants,
                     const std::set<std::string> & variables) const;

  ~Formula();
};

/// a event which can or cannot occur during an action
struct Event
{
  /// event precondition
  Formula * prec;
  /// event action
  Formula * eff;

  /// ground an event by grounding its preconditions and effects
  Event ground(const stringmap & parMapping,
               const stringmmap & types) const;

  Event ground(const std::set<std::string> & trueconstants,
               const std::set<std::string> & variables) const;
};

/// a name partition inducing an equivalence relation
struct Partition
{
  /// number of equivalence classes
  int nClasses;
  /// equivalence class mapping
  std::map<std::string,int> pFun;

  /// constructs a partition with 0 classes
  Partition();
  /// add eqivalence class
  Partition * setClass(const std::vector<std::string> & c);
};

/// a grounded action
struct GroundedAction
{
  /// the action name
  std::string                             name;
  /// the exeuting agent
  std::string                             agent;
  /// events that can possible occur
  std::vector<Event>                      events;
  /// and their names
  std::vector<std::string>                event_names;
  /// the set of designated events
  std::set<std::string>                   design;
  /// observability partitions for specific agents
  std::map<std::string, Partition>        partitions;
  /// final ground an action - insert true and false constantss
  GroundedAction ground(const std::set<std::string> & trueconstants,
                        const std::set<std::string> & variables) const;
};

/// an action containing multiple possible events
struct Action
{
  /// the action name
  std::string                             name;
  /// typed parameter list
  std::vector<elem>                       parameters;
  /// the exeuting agent
  std::string                             agent;
  /// events that can possible occur
  std::map<std::string,Event>             events;
  /// designated mapping
  std::map<std::string,bool>              design;
  /// observability partitions for specific agents
  std::map<std::string, Partition>        partitions;
  /// default observability for all other agents
  Partition                               defPartition;
  /// ground an action - insert objects into atomic parameters
  GroundedAction ground(const stringmap & parMapping,
                        const stringmmap & types) const;
};

/// domain information
struct Domain
{
  /// the domain name
  std::string                                    name;
  /// object types (at least agent <= object)
  std::map< std::string, std::set<std::string> > types;
  /// domain specific objects (consts)
  std::map< std::string, std::set<std::string> > objects;
  /// predicates: name -> arguments
  std::map< std::string, std::vector<elem> >     pred_args;
  /// predicates: name -> is constant?
  std::map< std::string, bool >                  pred_const;
  /// available actions
  std::map< std::string, Action >                actions;
};

/// a grounded problem
struct GroundedProblem
{
  /// problem name
  std::string                                            name;
  /// agent names, introduces an ordering on agents
  std::vector<std::string>                               agents;
  /// problem propositions, introduces an ordering on propositions
  std::vector<std::string>                               variables;
  /// world names, introduces an ordering on worlds
  std::vector<std::string>                               worlds;
  /// set of designated worlds
  std::set<std::string>                                  designated;
  /// true variable initializations for each world
  std::vector< std::set<std::string> >                   inits_t;
  /// agents observability partitions for specific agents
  std::vector< Partition >                               partitions;
  /// action set
  std::vector<GroundedAction>                            actions;
  /// goal formula
  Formula *                                              goal;
};

/// problem information
struct Problem
{
  /// problem name
  std::string                                            name;
  /// respective domain
  std::string                                            domain;
  /// problem specific objects
  std::map< std::string, std::set<std::string> >         objects;
  /// global true initializations
  std::set< std::vector<std::string> >                   global_inits_t;
  /// global false initializations
  std::set< std::vector<std::string> >                   global_inits_f;
  /// worlds: name -> local true initializations
  std::map< std::string,
            std::set< std::vector<std::string> > >       world_inits_t;
  /// worlds: name -> local false initializations
  std::map< std::string,
            std::set< std::vector<std::string> > >       world_inits_f;
  /// worlds: name -> is designated?
  std::map< std::string, bool >                          world_design;
  /// observability partitions for specific agents
  std::map<std::string, Partition>                       partitions;
  /// default observability for all other agents
  Partition                                              defPartition;
  /// goal formula
  Formula *                                              goal;

  /// ground the problem
  GroundedProblem ground(std::string name, Domain dom) const;
};

/// generate an initially unspecified formula
Formula * newFormula();
/// generate a conditional formula
Formula * newCond(Formula * s1, Formula * s2);
/// convert a formula to string
std::string toString(const Formula * f);

/// negate FImage
FImage neg(FImage img);

/// analyze formula for possible result
FImage genImage(const Formula * f, const std::set<std::string> & canbe_t,
                                   const std::set<std::string> & canbe_f);
/// add atoms that can be affected by formula to true/false list
void effectsOn(const Formula * f, std::set<std::string> & trueEffs,
                                  std::set<std::string> & falseEffs,
                                  std::set<std::string> & changes,
                                  const bool & negated = false);

/// generate a new, empty partition
Partition * newPartition();

/// create (type : objects) mapping from (object : types) mapping
stringmmap typedListToObjects(const stringmmap & typedList,
                              const stringmmap & typesList);

/// find atomic formulae which have an influence on the appl/effects
void atomDependencies(const GroundedAction & action,
                      stringmmap & res);
/// generate all instantiation mappings for given parameter & object lists
std::set< std::map<std::string, std::string> > genInstantiations(
      const std::vector<elem> & params, const stringmmap & typeInstances);
/// generate a printable string representation for typed map
std::string mmapToString(const stringmmap & map, std::string indent);
/// generate a printable string representation for typed list
std::string listToString(const std::vector<elem> & list,
                         std::string indent);
/// generate a printable string representation for partition
std::string partToString(const Partition & part);
/// generate a printable string representation for initialization maps
std::string initsToString(
                const std::set< std::vector<std::string> > & inits,
                std::string indent);
/// generate a printable string representation for an initl. literal
std::string initToString(const std::vector<std::string> & init);
std::string toPredName(const std::vector<std::string> & sv);

#endif // DATA_H_

