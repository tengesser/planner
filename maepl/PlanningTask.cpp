// Author: T. Engesser
// (c) 2014

#include "PlanningTask.h"

#include <iostream>
#include <algorithm>
#include <sstream>
#include <queue>

PlanningTask::PlanningTask()
{
  ivNameType = CHILD;
}

bool PlanningTask::parse(char* filename)
{
  return bparse(filename, this);
}

/*********************************************************************
*  generate new domains / problems                                   *
*********************************************************************/

bool PlanningTask::newDomain(std::string s)
{
  // set parse mode to domain parsing
  ivParseDom = true;

  // check if no domain with that name has been previously defined
  if (ivDomains.find(s) != ivDomains.end())
  {
    std::cerr << "ERROR: a domain with the name \"" << s
              << "\"has already been defined." << std::endl;
    return false;
  }

  // initialize domain and remember it for subsequent parsing
  ivDom = new Domain;
  ivDom->name = s;
  ivDomains[s] = ivDom;

  return true;
}

bool PlanningTask::newProblem(std::string s)
{
  // set parse mode to problem parsing
  ivParseDom = false;

  // check if no problem with that name has been previously defined 
  if (ivProblems.find(s) != ivProblems.end())
  {
    std::cerr << "ERROR: a problem with the name \"" << s
              << "\"has already been defined." << std::endl;
    return false;
  }

  // initialize problem and remember it for subsequent parsing
  ivProb = new Problem;
  ivProb->name = s;
  ivProblems[s] = ivProb;

  return true;
}

/*********************************************************************
*  retrieve domain specification                                     *
*********************************************************************/

bool PlanningTask::setSuperDomains()
{
  // retrieve names of domains to inherit from
  std::vector<std::string> superDomainNames = getNameList();
  if (superDomainNames.size() == 0);
    // no effects, no error
  else if (superDomainNames.size() > 1)
  {
    std::cerr << "ERROR: multiple inheritance not supported."
              << std::endl;
    return 0;
  }
  // or try to inherit
  else
  {
    // first, find domain by name
    std::map<std::string, Domain*>::iterator
      dit = ivDomains.find(superDomainNames[0]);
    // if it doesn't exist, return error
    if (dit == ivDomains.end())
    {
      std::cerr << "ERROR: domain \"" << superDomainNames[0]
                << "\" was previously not defined. "
                << "Inheritance not possible."<< std::endl;
      return false;
    }
    // else load types/objects/predicates/actions
    else
    {
      ivDom->types = dit->second->types;
      ivDom->objects = dit->second->objects;
      ivDom->pred_args = dit->second->pred_args;
      ivDom->pred_const = dit->second->pred_const;
      ivDom->actions = dit->second->actions;
    }
  }
  return true;
}

void PlanningTask::setDomainTypes()
{
  // retrieve inherited types
  std::map<std::string, std::set<std::string> > inhTypes = ivDom->types;
  // and new types
  std::map<std::string, std::set<std::string> > newTypes = getTypedMap();

  // now determine all types used for domain
  std::map<std::string, std::set<std::string> > types = inhTypes;

  for (std::map< std::string, std::set<std::string> >::iterator
         it = newTypes.begin(); it != newTypes.end(); ++it)
  {
    std::map<std::string, std::set<std::string> >::iterator
      foundInherited = inhTypes.find(it->first);
    if (foundInherited != inhTypes.end())
      std::cerr << "WARNING: Extending inherited type. This may"
                << " have unintended implications." << std::endl;
    types[it->first].insert(it->second.begin(), it->second.end());
  }

  // if necessary, add object/agent type
  types["object"];
  types["agent"];

  // for each type
  std::set<std::string> newtypes;
  for (std::map< std::string, std::set<std::string> >::const_iterator
         it1 = types.begin(); it1 != types.end(); ++it1)
  {
    // warn against the usage of either, if appropriate
    if (it1->second.size() > 1)
    {
      std::cerr << "WARNING: usage of either keyword in type "
                << "declarations is highly discouraged because "
                << "of unclear PDDL semantics." << std::endl;
    }
    // and collect all supertypes
    for (std::set<std::string>::const_iterator it2 = it1->second.begin();
           it2 != it1->second.end(); ++it2)
      newtypes.insert(*it2);
  }
  // such that they can be added as normal types, if necessary
  for (std::set<std::string>::const_iterator it = newtypes.begin();
         it != newtypes.end(); ++it)
    types[*it];

  // add reflexiviy as well as global inheritance from object
  for (std::map< std::string, std::set<std::string> >::iterator
         it1 = types.begin(); it1 != types.end(); ++it1)
  {
    it1->second.insert(it1->first);  // a subtype a
    it1->second.insert("object");    // a subtype "object"
  }

  // and compute the transitive closure
  bool change;
  do {
    change = false;
    for (std::map< std::string, std::set<std::string> >::const_iterator
         it1 = types.begin(); it1 != types.end(); ++it1)
      for (std::set<std::string>::const_iterator
            it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
        for (std::map< std::string, std::set<std::string> >::iterator
              it3 = types.begin(); it3 != types.end(); ++it3)
          // (a subtype b) and (c subtype a)
          if (it3->second.find(it1->first) != it3->second.end())
          {
            // => (c subtype b)
            const unsigned int size_old = it3->second.size();
            it3->second.insert(*it2);
            change &= it3->second.size() != size_old;
          }
  } while (change);

  // finally, write the type hierarchy into domain information
  ivDom->types = types;
}

bool PlanningTask::setDomainObjects()
{
  // for each typed object
  for (std::vector<elem>::const_iterator it = ivTypedList.begin();
         it != ivTypedList.end(); ++it)
  {
    // check that no object with this name has been defined before
    if (ivDom->objects.find(it->first) != ivDom->objects.end())
    {
      std::cerr << "ERROR: multiple definition of object "
                << it->first << "." << std::endl;
      return false;
    }
    // and then add it with its respective type
    ivDom->objects[it->first] = it->second;
  }
  // finally check, that each object has a defined type
  bool typesOk = checkTypedList(true, false);

  // and discard tempory typedlist / return
  ivTypedList.clear();
  return typesOk;
}

bool PlanningTask::setDomainPred(std::string s)
{
  // check that predicate name is unique
  if (ivDom->pred_args.find(s) != ivDom->pred_args.end())
  {
    std::cerr << "ERROR: multiple definition of predicate\""
              << s << "\"." << std::endl;
    return false;
  }
  // or at least only shadows an existing type predicate (warn then)
  if (ivDom->types.find(s) != ivDom->types.end())
    std::cerr << "WARNING: predicate \"" << s
              << "\" shadows a type predicate." << std::endl;

  // check that parameter types exist and argument names are unique
  bool typesOk = checkTypedList(true, true);

  // save predicate
  ivDom->pred_args[s] = ivTypedList;
  ivDom->pred_const[s] = true;

  // and discard tempory typedlist / return
  ivTypedList.clear();
  return typesOk;
}

bool PlanningTask::setDomainAction(std::string s)
{
  // initialize action object with name & parameters
  Action act;
  act.parameters = ivTypedList;
  act.name = s;

  // and remember it for subsequent parsing
  // if action with same name was defined before (inherited?) - overwrite
  ivDom->actions[s] = act;
  ivAction = &ivDom->actions[s];

  // check that parameter types exist and argument names are unique
  bool typesOk = checkTypedList(true, true);

  // finally discard tempory typedlist / return
  ivTypedList.clear();
  return typesOk;
}

/*********************************************************************
*  for actions: peace by peace event insertion                       *
*********************************************************************/

bool PlanningTask::setActionAgent(std::string s)
{
  // TODO: check if s represents an agent
  // std::cout << "Executing agent: " << s << std::endl;
  ivAction->agent = s;
  return true;
}

bool PlanningTask::setDomainEvent(std::string s, bool d)
{
  // check that there exists no event with the same name in action
  if (ivAction->events.find(s) != ivAction->events.end())
  {
    std::cerr << "ERROR: an event with the name \"" << s
              << "\"has already been defined for the action \""
              << ivAction->name << "\"." << std::endl;
    return false;
  }
  // create event object 
  Event ev;
  ev.prec = ivGD;
  ev.eff = ivEff;
  // and add it to the current action's events
  ivAction->events[s] = ev;
  ivAction->design[s] = d;
  return true;
}

void PlanningTask::setEventGoalDescription(Formula * f)
{
  // just set temporary formula variable to memorize parsing result
  ivGD = f;
}

void PlanningTask::setEventEffect(Formula * f)
{
  // just set temporary formula variable to memorize parsing result
  ivEff = f;
}

bool PlanningTask::setObservability(const std::vector<std::string> & a,
                                      const Partition * p)
{
  // for error messages: are we parsing event or world observability?
  std::string thing = ivParseDom ? " event \"" : " world \"";
  std::string container = ivParseDom ? " action \"" : " problem \"";
  // and for which world / problem?
  const std::string mycontainer
    = ivParseDom ? ivAction->name : ivProb->name;

  // the set which ought to be partitioned
  const std::map<std::string,bool> & things
    = ivParseDom ? ivAction->design :
                   ivProb->world_design;
  
  // check if all elements in partition really exist in source set
  for (std::map<std::string,int>::const_iterator it =
         p->pFun.begin(); it != p->pFun.end(); ++it)
    if (things.find(it->first) == things.end())
    {
      std::cerr << "ERROR: There is no" << thing << it->first
                << "\" in" << container << mycontainer << "\""
                << std::endl;
      return false;
    }
  
  // check that no element was forgotten in the partition
  for (std::map<std::string,bool>::const_iterator it =
         things.begin(); it != things.end(); ++it)
    if (p->pFun.find(it->first) == p->pFun.end())
    {
      std::cerr << "ERROR: world/event \"" << it->first
                << "\" cannot be assigned to multiple classes."
                << std::endl;
      return false;
    }

  // special case: no agents given - this is an default partition
  if (a.size() == 0)
    (ivParseDom ? ivAction->defPartition : ivProb->defPartition) = *p;

  // otherwise assign partition to specified agents
  for(std::vector<std::string>::const_iterator it =
        a.begin(); it != a.end(); ++it)
    (ivParseDom ? ivAction->partitions[*it] : ivProb->partitions[*it])
       = *p;

  return true;
}

void PlanningTask::setDefaultObservability()
{
  // if no default observability relation has been set yet
  if ((ivParseDom?ivAction->defPartition:ivProb->defPartition).nClasses 
       == 0)
  // define minimum observability as default
  setObservability(std::vector<std::string>(),
                   genTemplatePartition("none"));
}

/*********************************************************************
*  retrieve problem specification                                    *
*********************************************************************/

bool PlanningTask::setProblemDomain(std::string s)
{
  // first, check if the domain to be used is defined
  std::map< std::string, Domain* >::iterator dom = ivDomains.find(s);
  if (dom == ivDomains.end())
  {
    std::cerr << "ERROR: Domain " << s << " not yet defined for problem "
              << ivProb->name << "." << std::endl;
    return false;
  }
  // if so, reference it in problem information and memorize it
  else
  {
    ivProb->domain = s;
    ivDom = dom->second;
    return true;
  }
}

bool PlanningTask::setProblemObjects()
{
  // for each object that is specified
  for (std::vector<elem>::const_iterator it = ivTypedList.begin();
         it != ivTypedList.end(); ++it)
  {
    // check that there is no object with the same name either in domain 
    if (ivDom->objects.find(it->first) != ivDom->objects.end())
    {
      std::cerr << "ERROR: redefinition of domain object "
                << it->first << "." << std::endl;
      return false;
    }
    // or in problem object specifications 
    if (ivProb->objects.find(it->first) != ivProb->objects.end())
    {
      std::cerr << "ERROR: multiple definition of object "
                << it->first << "." << std::endl;
      return false;
    }
    // and then add it with its respective type
    ivProb->objects[it->first] = it->second;
  }

  // finally check, that each object has a defined type
  bool typesOk = checkTypedList(true, false);

  // and discard tempory typedlist / return
  ivTypedList.clear();
  return typesOk;
}

void PlanningTask::setProblemInits(std::string n, bool d,
                                   const Formula * f)
{
  if (n != "")
    ivProb->world_design[n] = d;

  std::set< std::vector<std::string> > & inits_t
    = (n == "" ? ivProb->global_inits_t : ivProb->world_inits_t[n]);
  std::set< std::vector<std::string> > & inits_f
    = (n == "" ? ivProb->global_inits_f : ivProb->world_inits_f[n]);

  for (std::vector<Formula*>::const_iterator
         it = f->subf.begin(); it != f->subf.end(); ++it)
  {
    if ((*it)->type == FATOM)
    {
      std::vector<std::string> plist = (*it)->args;
      plist.insert(plist.begin(), (*it)->name);
      inits_t.insert(plist);
    }
    else if ((*it)->type == FNOT)
    {
      std::vector<std::string> plist = (*it)->subf[0]->args;
      plist.insert(plist.begin(), (*it)->subf[0]->name);
      inits_f.insert(plist);
    }
  }
}

void PlanningTask::setProblemGoal(Formula * f)
{
  ivProb->goal = f;
}

/*********************************************************************
*  parsing quantified formula with semantic checks                   *
*********************************************************************/

/// build quantified formula and remember its scope for semantic checks
Formula * PlanningTask::beginQuantifiedFormula(FType type,
                                               bool & warning)
{
  // retrieve argument list
  stringmmap vars = getTypedMap();

  // for action definitions, retrieve parameter names
  std::set<std::string> aparams;
  if (ivParseDom)
    for (std::vector<elem>::const_iterator
           it  = ivAction->parameters.begin();
           it != ivAction->parameters.end(); ++it)
      aparams.insert(it->first);

  warning = false;
  // check for name collisions
  for (stringmmap::const_iterator it1 = vars.begin(); it1 != vars.end();
       ++it1)
  {
    // with other quantifier bound variables
    for (std::vector<stringmmap>::const_iterator it2 =
           ivScopeStack.begin(); it2 != ivScopeStack.end(); ++it2)
      if (it2->find(it1->first) != it2->end())
      {
        std::cerr << "WARNING: bound variable " << it1->first
                  << " shadows another variable." << std::endl;
        warning = true;
      }
    // or action parameters
    if (aparams.find(it1->first) != aparams.end())
    {
      std::cerr << "WARNING: bound variable " << it1->first
                << " shadows an action parameter." << std::endl;
      warning = true;
    }
  }
  
  // memorize now bound variables for parsing of inner formula
  ivScopeStack.push_back(vars);

  // and return the formula structure
  return newFormula()->setType(type)->setScope(vars);
}

/// remove variables from scopestack
void PlanningTask::finishQuantifiedFormula()
{
  // un-memorize variables that are bound no more
  ivScopeStack.pop_back();
}

/*********************************************************************
*  parsing typed lists                                               *
*********************************************************************/

stringmmap PlanningTask::getTypedMap()
{
  stringmmap result =
    vToM< std::string, std::set<std::string> >(ivTypedList);
  ivTypedList.clear();
  return result;
}

std::vector<std::string> PlanningTask::getNameList()
{
  std::vector<std::string> result = ivNewChildren;
  ivNewChildren.clear();
  return result;
}

void PlanningTask::inputName(std::string s)
{
  if (ivNameType == CHILD)
    ivNewChildren.push_back(s);

  else if (ivNameType == PARENT)
    ivNewParents.insert(s);
}

void PlanningTask::setParseChild()
{
  for (std::vector<std::string>::const_iterator it
         = ivNewChildren.begin(); it != ivNewChildren.end(); ++it)
    ivTypedList.push_back(std::make_pair(*it, ivNewParents));
  ivNewChildren.clear();
  ivNewParents.clear();
  ivNameType = CHILD;
}

void PlanningTask::setParseParent()
{
  ivNameType = PARENT;
}

void PlanningTask::finishList()
{
  for (std::vector<std::string>::const_iterator it =
         ivNewChildren.begin(); it != ivNewChildren.end(); ++it)
    ivTypedList.push_back(std::make_pair(*it, std::set<std::string>()));
  ivNewChildren.clear();
}

/*********************************************************************
*  debugging / grounding stuff                                       *
*********************************************************************/

std::vector<GroundedProblem> PlanningTask::ground()
{
  std::vector<GroundedProblem> results;

  // turn each problem into corresponding PlanningProblem
  for (std::map<std::string, Problem*>::const_iterator
         p = ivProblems.begin(); p != ivProblems.end(); ++p)
  {
    Domain * dom = ivDomains.find(p->second->domain)->second;
    results.push_back(p->second->ground(p->first, *dom));
  }

  return results;
}

void PlanningTask::debugPrint() const
{
  for (std::map<std::string, Domain*>::const_iterator di = 
         ivDomains.begin(); di != ivDomains.end(); ++di)
  {
    // domain name
    std::cout << "domain " << di->first << ":\n########" << std::endl;
    const Domain * d = di->second;
    // :types
    std::cout << d->types.size() << " types:";
    std::cout << mmapToString(d->types, "\n  ") << std::endl;
    // :consts
    std::cout  << d->objects.size()
               << " domain specific (const) objects:";
    std::cout << mmapToString(d->objects, "\n  ") << std::endl;
    // :predicates
    std::cout << d->pred_args.size() << " predicates:" << std::endl;
    for (std::map< std::string, std::vector<elem> >::const_iterator
           it = d->pred_args.begin(); it != d->pred_args.end(); ++it)
    {
      bool cpred = d->pred_const.find(it->first)->second;
      std::cout << "  " << it->first;
      std::cout << (cpred ? " (const)" : "");
      std::cout << listToString(it->second, "\n    ") << std::endl;
    }
    // :action *
    std::cout << d->actions.size() << " actions:" << std::endl;
    for (std::map< std::string, Action >::const_iterator
           it = d->actions.begin(); it != d->actions.end(); ++it)
    {
      // :parameters
      std::cout << "  |" << it->first << "|" << std::endl;
      std::cout << "    parameters:";
      std::cout << listToString(it->second.parameters, "\n      ")
                << std::endl;
      // :events
      for (std::map< std::string, Event >::const_iterator
             ite = it->second.events.begin();
             ite != it->second.events.end(); ++ite)
      {
        std::cout << "    event " << ite->first;
        if (it->second.design.find(ite->first)->second)
          std::cout << " (designated)";
        // :precondition
        std::cout << std::endl;
        std::cout << "      prec: " << toString(ite->second.prec)
                  << std::endl;
        // :effect
        std::cout << "      eff:  " << toString(ite->second.eff)
                  << std::endl;
      }
      // :observability
      std::cout << "    observability:" << std::endl;
      std::cout << "      DEFAULT: "
                << partToString(it->second.defPartition) << std::endl;
      for (std::map<std::string, Partition>::const_iterator
             it2 = it->second.partitions.begin();
             it2 != it->second.partitions.end(); ++it2)
              std::cout << "      " << it2->first << ": "
                        << partToString(it2->second) << std::endl;
    }
  }
  for (std::map<std::string, Problem*>::const_iterator pi = 
         ivProblems.begin(); pi != ivProblems.end(); ++pi)
  {
    // problem name
    std::cout << "problem " << pi->first << ":\n########" << std::endl;
    const Problem * p = pi->second;
    // :domain
    std::cout << "of domain: " << p->domain << std::endl;
    // :objects
    std::cout << p->objects.size()
               << " problem specific objects:";
    std::cout << mmapToString(p->objects, "\n  ") << std::endl;
    // :init
    std::cout << p->global_inits_t.size()
              << " global initializations (true):";
    std::cout << initsToString(p->global_inits_t, "\n  ") << std::endl;
    std::cout << p->global_inits_f.size()
              << " global initializations (false):";
    std::cout << initsToString(p->global_inits_f, "\n  ") << std::endl;
    // :world *
    std::cout << p->world_design.size() 
              << " initial worlds:" << std::endl;
    for (std::map<std::string, bool>::const_iterator
           wi = p->world_design.begin();
           wi != p->world_design.end(); ++wi)
    {
      std::cout << "  " << wi->first 
                << (wi->second ? " (designated)" : "" ) << std::endl;
      const std::set< std::vector<std::string> > & inits_t
              = p->world_inits_t.find(wi->first)->second;
      const std::set< std::vector<std::string> > & inits_f
              = p->world_inits_f.find(wi->first)->second;
      std::cout << "    " << inits_t.size()
                << " world initializations (true):"
                << initsToString(inits_t, "\n      ") << std::endl;
      std::cout << "    " << inits_f.size()
                << " world initializations (false):"
                << initsToString(inits_f, "\n      ") << std::endl;
    }
    // :observability
    std::cout << "    observability:" << std::endl;
    std::cout << "      DEFAULT: "
              << partToString(p->defPartition) << std::endl;
      for (std::map<std::string, Partition>::const_iterator
             it = p->partitions.begin();
             it != p->partitions.end(); ++it)
              std::cout << "      " << it->first << ": "
                        << partToString(it->second) << std::endl;
    // :goal
    std::cout << "goal: "<< toString(p->goal) << std::endl;
  }
}

/*********************************************************************
*  helper / debug functions                                          *
*********************************************************************/

Partition * PlanningTask::genTemplatePartition(const std::string & s)
{
  const std::map<std::string,bool> & things
    = ivParseDom ? ivAction->design :
                   ivProb->world_design;
  Partition * result = new Partition;
  if (s == "none")
  {
    std::vector<std::string> onlyclass;
    for (std::map<std::string,bool>::const_iterator
           it = things.begin(); it != things.end(); ++it)
      onlyclass.push_back(it->first);
    result->setClass(onlyclass);
  }
  else if (s == "full")
  {
    for (std::map<std::string,bool>::const_iterator
           it = things.begin(); it != things.end(); ++it)
    {
      std::vector<std::string> singletonclass;
      singletonclass.push_back(it->first);
      result->setClass(singletonclass);
    }
  }
  else
  {
    std::cerr << "WARNING: \"" << s << "\" is no partition template."
              << " Ignoring statement." << std::endl;
  }
  return result;
}

bool PlanningTask::checkTypedList(bool types, bool unique)
{
  std::set<std::string> encountered;
  for (std::vector<elem>::iterator it1 = ivTypedList.begin();
         it1 != ivTypedList.end(); ++it1)
  {
    if (unique)
    {
      if (encountered.find(it1->first) != encountered.end())
      {
        std::cerr << "ERROR: multiple occurences of identifier \""
                  << it1->first << "\"." << std::endl;   
        return false;
      }
      encountered.insert(it1->first);
    } 
    if (types)
      for (std::set<std::string>::iterator it2 = it1->second.begin();
             it2 != it1->second.end(); ++it2)
        if (ivDom->types.find(*it2) == ivDom->types.end())
        {
          std::cerr << "ERROR: undefined type \"" << *it2 << "\""
                    << " for object \"" << it1->first << "\""
                    << std::endl;
          return false;
        } 
  }
  return true;
}

bool PlanningTask::checkAtom(Formula * atom)
{
  std::map< std::string, std::vector<elem> >::const_iterator
    customPredicate = ivDom->pred_args.find(atom->name);
  std::map< std::string, std::set<std::string> >::const_iterator
    typePredicate = ivDom->types.find(atom->name);
  // if atom uses a custom predicate
  if (customPredicate != ivDom->pred_args.end())
  {
    // immediately discard, if the number of arguments don't match
    if (atom->args.size() != customPredicate->second.size())
    {
      std::cerr << "ERROR: atomic formula " << toString(atom)
                << " has the wrong number of arguments ("
                << customPredicate->second.size() << " required but "
                << atom->args.size() << " found)." << std::endl;
       return false;
    }
    // otherwise check for each argument
    for (unsigned int i = 0; i < atom->args.size(); ++i)
    {
      if (!checkArgument(atom->args[i],
                         customPredicate->second[i].second))
      {
        std::cerr << " in atomic formula " << toString(atom)
                  << "." << std::endl;
        return false;
      }
    }
    return true;
  }
  // else if atom uses a non-shadowed type predicate
  else if (typePredicate != ivDom->types.end())
  {
    // check if the argument is 
    if (atom->args.size() != 1)
    {
      std::cerr << "ERROR: atomic formula " << toString(atom)
                << " has the wrong number of arguments (1 required but "
                << atom->args.size() << " found)." << std::endl;
      return false;
    }
    // TODO: is it still possible here for variable to be undefined?
    else if (!checkArgument(atom->args[0],
                            ivDom->types[typePredicate->first]))
    {
      std::cerr << "???" << std::endl;
      return false;
    }
    return true;
  }
  // else the atom is defitively invalid
  std::cerr << "ERROR: predicate with name " << atom->name
            << " is not defined." << std::endl;
  return false;
}


class WithName
{
public:
  static void setName(std::string n) { name = n; }
  static bool hasName(elem n) { return n.first == name; }
  static bool hasName2(stringmmap n) { return n.find(name) != n.end(); }
private:
  static std::string name;
};

std::string WithName::name = "";

// TODO: check if arguments have the right type
bool PlanningTask::checkArgument(std::string argument,
                                 std::set<std::string> types)
{
  WithName::setName(argument);
  std::vector<stringmmap>::iterator
    quantifiedvar = std::find_if(ivScopeStack.begin(),
                                 ivScopeStack.end(),
                                 &WithName::hasName2);
  std::vector<elem>::iterator
    actionparam = std::find_if(ivAction->parameters.begin(),
                               ivAction->parameters.end(),
                               &WithName::hasName);
  // std::map< std::string, std::set<std::string> >:: const_iterator
  //   domainobject = ivDom->objects.find(argument);
  // std::map< std::string, std::set<std::string> >:: const_iterator
  //   problemobject = ivProb->objects.find(argument);

  // argument can be either
  // - bound by exists/forall
  if (quantifiedvar != ivScopeStack.end())
  {
    // std::cout << "QUANTIFIEDVAR" << std::endl;
  }
  // - or an action parameter
  else if (ivParseDom &&
             actionparam != ivAction->parameters.end())
  {
    // std::cout << "ACTIONPARAM" << std::endl;
  }
  // - or a domain specific object
  else if (ivDom->objects.find(argument) != ivDom->objects.end())
  {
    // std::cout << "DOMAINOBJECT" << std::endl;
  }
  // - or a problem specific object
  else if (!ivParseDom &&
           ivProb->objects.find(argument) != ivProb->objects.end())
  {
    // std::cout << "PROBLEMOBJECT" << std::endl;
  }
  else
  {
    std::cerr << "ERROR: "
              << (argument[0] == '?' ? "variable " : "object ")
              << argument << " is not defined";
    return false;
  }
  return true; 
}

template <class Key, class Val>
std::map<Key, Val> vToM(const std::vector< std::pair<Key, Val> > & v)
{
  std::map<Key, Val> result;
  for (unsigned int i = 0; i < v.size(); ++i)
  {
    if (result.find(v[i].first) != result.end())
      std::cerr << "WARNING: multiple declaration of \""
                << v[i].first << "\"." << std::endl;
    result[v[i].first].insert(v[i].second.begin(), v[i].second.end());
  }
  return result;
}


