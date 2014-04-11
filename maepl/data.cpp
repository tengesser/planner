// Author: T. Engesser
// (c) 2014

#include "data.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#define GDBGPRNT 0

/*********************************************************************
*  formula generation                                                *
*********************************************************************/

Formula * newFormula()
{
  Formula * f = new Formula;
  return f;
}

Formula * newCond(Formula * s1, Formula * s2)
{
  Formula * f = new Formula;
  f->type = FCOND;
  f->whenf = s1;
  f->thenf = s2;
  return f;
}

Formula * Formula::setType(FType t)
{
  this->type = t;
  return this;
}

Formula * Formula::addSub(Formula * sub)
{ 
  subf.push_back(sub);
  return this;
}

Formula * Formula::setScope(stringmmap s)
{
  this->scope = s;
  return this;
}

Formula * Formula::setName(std::string s)
{
  this->name = s;
  return this;
}

Formula * Formula::setArgs(std::vector<std::string> a)
{
  this->args = a;
  return this;
}

bool deleteF( Formula * f ) { delete f; return true; }
Formula::~Formula()
{ 
  std::remove_if(subf.begin(), subf.end(), deleteF);
  deleteF(whenf);
  deleteF(thenf);
}

FImage neg(FImage img)
{
  if (img == ITRUE)
    return IFALSE;
  else if (img == ITRUE)
    return IFALSE;
  else
    return IBOTH;
}

FImage genImage(const Formula * f, const std::set<std::string> & canbe_t,
                                   const std::set<std::string> & canbe_f)
{
  switch (f->type)
  {
    case FATOM:
      if (canbe_t.find(f->groundedatom) != canbe_t.end())
      {
        if (canbe_f.find(f->groundedatom) != canbe_f.end())
          return IBOTH; // sowohl in true als auch in false list
        else
          return ITRUE; // nur in true liste
      }
      else
        return IFALSE; // TODO: semantics?
      break;
    case FNOT:
      return neg(genImage(f->subf[0], canbe_t, canbe_f));
      break;
    case FAND:
    {
      bool both = false;
      for (std::vector<Formula*>::const_iterator it = f->subf.begin(); 
             it != f->subf.end(); ++it)
      {
        if (genImage(*it, canbe_t, canbe_f) == IFALSE)
           return IFALSE;
        if (genImage(*it, canbe_t, canbe_f) == IBOTH)
           both = true; 
      }
      if (both)
        return IBOTH;
      else
        return ITRUE;
      break;
    }
    case FOR:
    {
      bool both = false;
      for (std::vector<Formula*>::const_iterator it = f->subf.begin(); 
             it != f->subf.end(); ++it)
      {
        if (genImage(*it, canbe_t, canbe_f) == ITRUE)
           return ITRUE;
        if (genImage(*it, canbe_t, canbe_f) == IBOTH);
           both = true; 
      }
      if (both)
        return IBOTH;
      else
        return IFALSE;
      break;
    }
    case FIMPLY:
      return genImage(newFormula()->setType(FNOT)->addSub(f->subf[0])
                                                 ->addSub(f->subf[1]),
                      canbe_t, canbe_f);
    case FKNOWS:
    case FCOMMK:
      return genImage(f->subf[0], canbe_t, canbe_f);
      break;
    case FTRUE:
      /* should only happen for outer formula */
      return ITRUE;
      break;
    case FFALSE:
      /* should only happen for outer formula */
      return IFALSE;
      break;
    case FEXISTS:
    case FFORALL:
    case FEQUALS:
    case FCOND:
    case LLIST:
      /* this should not happen at all */
      break;    
  }
  return IBOTH;
}

/*********************************************************************
*  partition generation                                              *
*********************************************************************/

Partition::Partition()
  : nClasses(0) { }
 

Partition * newPartition()
{
  Partition * p = new Partition;
  return p;
}

Partition * Partition::setClass(const std::vector<std::string> & c)
{
  for (std::vector<std::string>::const_iterator it =
         c.begin(); it != c.end(); ++it)
  {
    // no element may be asigned to more than one class
    if (pFun.find(*it) != pFun.end())
    {
      std::cerr << "ERROR: world/event \"" << *it
                << "\" cannot be assigned to multiple classes."
                << std::endl;
      return NULL;
    }
    pFun[*it] = nClasses;
  }
  nClasses += 1;
  return this;
}

/*********************************************************************
*  grounding actions & events                                        *
*********************************************************************/

bool isTrue(const Formula * f)
{
  return f->type == FTRUE;
}

bool isFalse(const Formula * f)
{
  return f->type == FFALSE;
}

Formula * Formula::insertTF(const std::set<std::string> & trueconstants,
                            const std::set<std::string> & vars) const
{
  Formula * res = new Formula;
  switch (this->type)
  {
    case FTRUE:
      res->type = FTRUE;
      break;
    case FFALSE:
      res->type = FFALSE;
      break;
    case FATOM:
    {
      if (trueconstants.find(this->groundedatom) != trueconstants.end())
        res->type = FTRUE;
      else if (vars.find(this->groundedatom) == vars.end())
        res->type = FFALSE;
      else
      {
        res->type = FATOM;
        res->name = this->name;
        res->groundedatom = this->groundedatom;
        for (std::vector<std::string>::const_iterator
               it = this->args.begin(); it != this->args.end(); ++it)
          res->args.push_back(*it);
      }
      break;
    }
    case FNOT:
      res->type = FNOT;
      res->subf.push_back(this->subf[0]->insertTF(trueconstants,vars));
      break;
    case FAND:
      res->type = FAND;
      for (std::vector<Formula*>::const_iterator
             it = this->subf.begin(); it != this->subf.end(); ++it)
        res->subf.push_back((*it)->insertTF(trueconstants,vars));
      break;
    case FOR:
      res->type = FOR;
      for (std::vector<Formula*>::const_iterator
             it = this->subf.begin(); it != this->subf.end(); ++it)
        res->subf.push_back((*it)->insertTF(trueconstants,vars));
      break;
    case FIMPLY:
      res->type = FIMPLY;
      res->subf.push_back(this->subf[0]->insertTF(trueconstants,vars));
      res->subf.push_back(this->subf[1]->insertTF(trueconstants,vars));
      break;
    case FCOND: // TODO: conditional case
      res->type = FCOND;
      res->whenf = this->whenf->insertTF(trueconstants,vars);
      res->thenf = this->thenf->insertTF(trueconstants,vars);
      break;
    case FKNOWS:
      res->type = FKNOWS;
      res->name = this->name;
      res->subf.push_back(this->subf[0]->insertTF(trueconstants,vars));
      break;
    case FCOMMK:
      res->type = FCOMMK;
      res->subf.push_back(this->subf[0]->insertTF(trueconstants,vars));
      break;
    case FEQUALS:
    case FEXISTS:
    case FFORALL:
    case LLIST:
      /* this should not happen */
      break;
  }
  return res;
}

Formula * Formula::ground(const std::set<std::string> & trueconstants,
                          const std::set<std::string> & variables) const
{
  //std::cout << "BLA: " << toString(this) << std::endl;
  Formula * fi = this->insertTF(trueconstants, variables);
  //std::cout << "BLUB: " << toString(fi) << std::endl;
  stringmap dummysm;
  stringmmap dummysmm;
  Formula * res = fi->ground(dummysm, dummysmm);
  // std::cout << "GF: " << toString(res) << std::endl;
  return res;
}

Formula * Formula::ground(const stringmap & parMapping,
                          const stringmmap & types) const
{
  Formula * result = new Formula;

  // for quantified formulae: ground scope first, then parameters
  if (type == FFORALL || type == FEXISTS)
  {
    result->type = (type == FFORALL) ? FAND : FOR;

    std::vector<elem> scopelist;
    for (stringmmap::const_iterator it = scope.begin();
           it != scope.end(); ++it)
      scopelist.push_back(*it);

    std::set< std::map<std::string, std::string> > instantiations
      = genInstantiations(scopelist, types);

    for (std::set< std::map<std::string, std::string> >::const_iterator
          it = instantiations.begin(); it != instantiations.end(); ++it)
    {
      stringmap newParMapping = parMapping; // TODO: ok?
      newParMapping.insert(it->begin(), it->end());
      // Formula * fnew = subf[0]->ground(*it, types);
      // result->subf.push_back(fnew->ground(parMapping, types));
      result->subf.push_back(subf[0]->ground(newParMapping, types));
      // delete fnew; // TODO
    }
  }
  
  else
  {
    // clone type
    result->type = type;

    // ground agent name
    if (name[0] == '?')
      result->name = parMapping.find(name)->second;
    else
      result->name = name;

    // ground atomic / equality arguments
    for (std::vector<std::string>::const_iterator
           it = args.begin(); it != args.end(); ++it)
    {
      if ((*it)[0] == '?')
      {
        std::map<std::string, std::string>::const_iterator
          found = parMapping.find(*it);
        if (found != parMapping.end())
          result->args.push_back(found->second);
        else
          result->args.push_back(*it); // TODO: ok?
      }
      else
        result->args.push_back(*it);
    }

    // ground subformulae
    for (std::vector<Formula*>::const_iterator
           it = subf.begin(); it != subf.end(); ++it)
      result->subf.push_back((*it)->ground(parMapping, types));

    // for equals formula directly evaluate result
    if (type == FEQUALS)
    {
      result->setType(FTRUE);
      for (std::vector<std::string>::const_iterator
             it = result->args.begin(); it != result->args.end(); ++it)
        if (*it != result->args[0])
          result->setType(FFALSE);
    }

    // set grounded atom string representation for grounded atoms
    if (type == FATOM)
    {
      std::ostringstream os;
      os << name;
      for (std::vector<std::string>::const_iterator
           it = result->args.begin(); it != result->args.end(); ++it)
        os << "-" << *it;
      result->groundedatom = os.str();
    }
  
    // for type predicate formula directly evaluate result
    // TODO: not for predicates shadowing type parameters
    if (type == FATOM && types.find(name) != types.end())
    {
      result->setType(FTRUE);
      for (std::vector<std::string>::const_iterator
             it = result->args.begin(); it != result->args.end(); ++it)
        if (   types.find(name)->second.find(*it) 
            == types.find(name)->second.end())
          result->setType(FFALSE);
    }
    
    // ground condition, effect for conditional formulae
    if (type == FCOND)
    {
      result->whenf = whenf->ground(parMapping, types);
      result->thenf = thenf->ground(parMapping, types);
    }

    // the result shouldn't need any scope
    // result->scope = scope;
  }

  // consolidate AND / OR / NOT / IMPLICATION formulae
  if (result->type == FAND)
  {
    std::vector<Formula*>::iterator new_end
      = remove_if(result->subf.begin(), result->subf.end(), isTrue);
    result->subf.erase(new_end, result->subf.end());
    for (std::vector<Formula*>::iterator it = result->subf.begin();
           it != result->subf.end(); ++it)
      if (isFalse(*it))
        result->type = FFALSE;
    if (result->subf.size() == 0)
      result->type = FTRUE;
    // TODO ...
    /*else if (result->subf.size() == 1)
    {
      Formula * toDelete = result;
      result = result->subf[0];
      delete toDelete;
    }*/
  }
  if (result->type == FOR)
  {
    std::vector<Formula*>::iterator new_end
      = remove_if(result->subf.begin(), result->subf.end(), isFalse);
    result->subf.erase(new_end, result->subf.end());
    for (std::vector<Formula*>::iterator it = result->subf.begin();
           it != result->subf.end(); ++it)
      if (isTrue(*it))
        result->type = FTRUE;
    if (result->subf.size() == 0)
      result->type = FFALSE;
    /*else if (result->subf.size() == 1)
    {
      Formula * toDelete = result;
      result = result->subf[0];
      delete toDelete;
    }*/
  }
  if (result->type == FNOT)
  {
    if (result->subf[0]->type == FTRUE)
      result->type = FFALSE;
    else if (result->subf[0]->type == FFALSE)
      result->type = FTRUE;
  }
  if (result->type == FIMPLY)
  {
    if (result->subf[0]->type == FFALSE)
      result->type = FTRUE;
    else if (result->subf[1]->type == FTRUE)
      result->type = FTRUE;
    else if (result->subf[0]->type == FTRUE &&
             result->subf[1]->type == FFALSE)
      result->type = FFALSE;
  } 
  if (result->type == FKNOWS || result->type == FCOMMK)
  {
    if (result->subf[0]->type == FTRUE)
      result->type = FTRUE;
    if (result->subf[0]->type == FFALSE)
      result->type = FFALSE;
  }

  return result;
}

void atomDependenciesPrec(const Formula * prec,
                          std::string aname, 
                          stringmmap & res)
{
  switch (prec->type)
  {
    case FATOM:
      res[prec->groundedatom].insert(aname);
    break;
    case FNOT:
    case FAND:
    case FOR:
    case FIMPLY:
    case FKNOWS:
    case FCOMMK:
      for (std::vector<Formula*>::const_iterator it = prec->subf.begin();
             it != prec->subf.end(); ++it)
        atomDependenciesPrec(*it, aname, res);
    break;
    case FTRUE:
    case FFALSE:
      /* nothing to do */
    break;
    default:
      /* this should not happen */
    break;
  }
}

void atomDependenciesEff(const Formula * eff,
                         std::string aname,
                         stringmmap & res)
{
  switch (eff->type)
  {
    case FATOM:
    case FNOT:
    case FAND:
    case FTRUE:
    case FFALSE:
    break;
    case FCOND:
      atomDependenciesPrec(eff->whenf, aname, res);
    break;
    default:
      /* this should not happen */
    break;
  }
}


void atomDependencies(const GroundedAction & action,
                      stringmmap & res)
{
  const std::vector<Event> & evs = action.events;
  for (std::vector<Event>::const_iterator
    it = evs.begin(); it != evs.end(); ++it)
  {
    atomDependenciesPrec(it->prec, action.name, res);
    atomDependenciesEff(it->eff, action.name, res);
  }
}

void effectsOn(const Formula * f, std::set<std::string> & trueEffs,
                                  std::set<std::string> & falseEffs,
                                  std::set<std::string> & changes,
                                  const bool & negated)
{
  //std::cout << toString(f) << std::endl;
  if (f->type == FATOM)
  {
    if (negated && falseEffs.find(f->groundedatom) == falseEffs.end())
    {
      falseEffs.insert(f->groundedatom);
      changes.insert(f->groundedatom);
      //std::cout << "False += " << f->groundedatom << std::endl;
    }
    else if (!negated)
    {
      if (   trueEffs.find(f->groundedatom) == trueEffs.end()
          && falseEffs.find(f->groundedatom) == falseEffs.end())
        falseEffs.insert(f->groundedatom);
      if (trueEffs.find(f->groundedatom) == trueEffs.end())
      {
        trueEffs.insert(f->groundedatom);
        changes.insert(f->groundedatom);
      }
      //std::cout << "True += " << f->groundedatom << std::endl;
    }
  }
  else if (f->type == FNOT)
  {
    effectsOn(f->subf[0], trueEffs, falseEffs, changes, !negated);
  }
  if (f->type == FAND)
  {
    for (std::vector<Formula*>::const_iterator
       it = f->subf.begin(); it != f->subf.end(); ++it)
      effectsOn(*it, trueEffs, falseEffs, changes, negated);
  }
  else if (f->type == FCOND)
  {
    FImage img = genImage(f->whenf, trueEffs, falseEffs);
    if (img == ITRUE || img == IBOTH)
      effectsOn(f->thenf, trueEffs, falseEffs, changes, negated);
  }
}

Event Event::ground(const std::set<std::string> & trueconstants,
                    const std::set<std::string> & variables) const
{
  Event result;
  result.prec = prec->ground(trueconstants, variables);
  result.eff = eff->ground(trueconstants, variables);
  return result;
}

Event Event::ground(const stringmap & parMapping,
                    const stringmmap & types) const
{
  Event result;
  result.prec = prec->ground(parMapping, types);
//std::cout << "    prec: " << toString(result.prec) << std::endl;
  result.eff = eff->ground(parMapping, types);
//std::cout << "    eff: " << toString(result.eff) << std::endl;
  return result;
}

GroundedAction GroundedAction::ground(
                   const std::set<std::string> & trueconstants,
                   const std::set<std::string> & variables) const
{
  GroundedAction result;
  result.name = this->name;
  result.agent = this->agent;
  result.design = this->design;
  result.partitions = this->partitions;
  result.event_names = this->event_names;
  for (std::vector<Event>::const_iterator
         it = this->events.begin(); it != this->events.end(); ++it)
    result.events.push_back(it->ground(trueconstants,variables));
  return result;
}

// TODO: remove events with F precondition?
GroundedAction Action::ground(const stringmap & parMapping,
                              const stringmmap & types) const
{
  GroundedAction result;
  
  std::set<std::string> agents = types.find("agent")->second;

  // first calculate name
  std::ostringstream ns;
  ns << name;
  for (std::vector<elem>::const_iterator it = parameters.begin();
         it != parameters.end(); ++it)
    ns << "-" << parMapping.find(it->first)->second;
  result.name = ns.str();
//std::cout << "Grounding " << result.name << std::endl;

  // and ground executing agent
  if (this->agent == "")
    result.agent = "";
  else if (this->agent[0] == '?')
    result.agent = parMapping.find(this->agent)->second;
  else
    result.agent = this->agent;

  // then ground associated events
  for (std::map<std::string,Event>::const_iterator it = events.begin();
         it != events.end(); ++it)
  {
//  std::cout << "  Grounding Event " << it->first << std::endl;
    result.event_names.push_back(it->first);
    result.events.push_back(it->second.ground(parMapping, types));
  }

  // the designated events remain the same
  for (std::vector<std::string>::iterator it = result.event_names.begin();
         it != result.event_names.end(); ++it)
    if (this->design.find(*it)->second)
      result.design.insert(*it);
  
  // set observability for each agent to default observability first
  for (std::set<std::string>::const_iterator it = agents.begin();
         it != agents.end(); ++it)
    result.partitions[*it] = defPartition;
  // and then, for specified agents to specified observability
  for (std::map<std::string, Partition>::const_iterator
         it = partitions.begin(); it != partitions.end(); ++it)
  {
    if (it->first[0] == '?')
      result.partitions[parMapping.find(it->first)->second] = it->second;
    else
      result.partitions[it->first] = it->second;
  }
/*
  std::cout << "  Observability Partitions:" << std::endl;
  for (std::set<std::string>::const_iterator it = agents.begin();
         it != agents.end(); ++it)
    std::cout << "    Agent " << *it << ": "
              << partToString(result.partitions[*it]) << std::endl;
*/

  return result;
}

GroundedProblem Problem::ground(std::string nm, Domain dom) const
{
  // first gather all objects
  stringmmap objs;
  objs.insert(dom.objects.begin(), dom.objects.end());
  objs.insert(this->objects.begin(), this->objects.end());
  
  // and determine the instances of each type
  stringmmap typeInstances = typedListToObjects(objs, dom.types);

  // these will become the proposition / action sets, constants
  std::set<std::string> predInstancesT; // atoms that can become true
  std::set<std::string> predInstancesF; // atoms that can become false
  stringmmap worldF;
  stringmmap worldT;
  std::set<std::string> globalF;
  std::set<std::string> globalT;
  std::map<std::string, GroundedAction> actions; // grounded actions
  std::set<std::string> used_actions;
  std::set<std::string> aqueue;
 
  // add global true initializations
  for (std::set< std::vector<std::string> >::const_iterator
         it  = this->global_inits_t.begin();
         it != this->global_inits_t.end(); ++it)
  {
    std::string prName = toPredName(*it);
    predInstancesT.insert(prName);
    globalT.insert(prName);
  }

  // add global false initializations
  for (std::set< std::vector<std::string> >::const_iterator
         it  = this->global_inits_f.begin();
         it != this->global_inits_f.end(); ++it)
  {
    std::string prName = toPredName(*it);
    predInstancesF.insert(prName);
    globalF.insert(prName);
  }

  // add true initializations from worlds
  for (std::map< std::string, std::set< std::vector<std::string> > >
          ::const_iterator it  = this->world_inits_t.begin();
                           it != this->world_inits_t.end(); ++it) 
     for (std::set< std::vector<std::string> >::const_iterator
            it2  = it->second.begin();
            it2 != it->second.end(); ++it2)
     {  
       std::string prName = toPredName(*it2);
       // wouldn't be necessary for these, which are in every world true
       if (    predInstancesT.find(prName)
            == predInstancesT.end()
            && predInstancesF.find(prName)
            == predInstancesF.end())
         predInstancesF.insert(prName); // TODO: check
       predInstancesT.insert(prName);
       worldT[it->first].insert(prName);
     }

  // add false initializations from worlds
  for (std::map< std::string, std::set< std::vector<std::string> > >
          ::const_iterator it  = this->world_inits_f.begin();
                           it != this->world_inits_f.end(); ++it) 
     for (std::set< std::vector<std::string> >::const_iterator
            it2  = it->second.begin();
            it2 != it->second.end(); ++it2)
     {
        std::string prName = toPredName(*it2);
        predInstancesF.insert(prName);
        worldF[it->first].insert(prName);
     }

  // debug print out true and false predicates
#if GDBGPRNT
  std::cout << "Atoms that can be false:" << std::endl;
  for (std::set<std::string>::const_iterator
         it  = predInstancesF.begin();
         it != predInstancesF.end(); ++it)
    std::cout << "  " << *it << std::endl;
  std::cout << "Atoms that can be true:" << std::endl;
  for (std::set<std::string>::const_iterator
         it  = predInstancesT.begin();
         it != predInstancesT.end(); ++it)
    std::cout << "  " << *it << std::endl;
#endif

  // mapping from atoms to actions that could become applyable
  stringmmap maybeApplyableAfter; 

  // for each action 
  for (std::map< std::string, Action >::iterator
         a = dom.actions.begin(); a != dom.actions.end(); ++a)
  {
    // std::cout << "Action " << a->first << "\n#####" << std::endl;
    
    // calculate all action instances
    std::set< std::map<std::string, std::string> > instantiations 
      = genInstantiations(a->second.parameters, typeInstances);

    for (std::set< std::map<std::string, std::string> >::const_iterator
           it = instantiations.begin();
           it != instantiations.end(); ++it)
    {
        GroundedAction action = 
          a->second.ground(*it, typeInstances);
        actions[action.name] = action;
        aqueue.insert(action.name);
        atomDependencies(action, maybeApplyableAfter);
    }
  }
#if GDBGPRNT
  std::cout << "Atom consequences:" << std::endl;
  std::cout << mmapToString(maybeApplyableAfter, "  \n") << std::endl;
#endif

  while (!aqueue.empty())
  {
    std::string aname = *aqueue.begin();
    aqueue.erase(aqueue.begin());

    const std::vector<Event> & evs = actions[aname].events;
    int i = 0;
    for (std::vector<Event>::const_iterator
           it = evs.begin(); it != evs.end(); ++it, ++i)
    {
#if GDBGPRNT
      std::cout << "Checking " << aname << "-" << i;
#endif
      FImage im = genImage(it->prec, predInstancesT, predInstancesF);
      if (im == ITRUE || im ==  IBOTH)
      {
        std::set<std::string> changes;
        effectsOn(it->eff, predInstancesT, predInstancesF, changes);
#if GDBGPRNT
        std::cout << " - applyable, adds " << changes.size();
#endif
        
        for (std::set<std::string>::const_iterator
               it2 = changes.begin(); it2 != changes.end(); ++it2)
        {
          std::set<std::string> refrActions = maybeApplyableAfter[*it2];
          aqueue.insert(refrActions.begin(), refrActions.end());
        }
#if GDBGPRNT
        std::cout << " -> " << aqueue.size() << " acts in queue.";
#endif
        if (used_actions.find(aname) == used_actions.end())
        {
#if GDBGPRNT
          std::cout << "\nAction usable: " << aname;
#endif
          used_actions.insert(aname);
        }
      }
#if GDBGPRNT
      std::cout << std::endl;
#endif
    }
  }

  /* std::cout << "GROUNDING RESULTS" << std::endl;
   std::cout << "Usable Actions:" << std::endl;
   for (std::set<std::string>::const_iterator
        it = used_actions.begin(); it != used_actions.end(); ++it)
     std::cout << "  " << *it << std::endl; */
  
  // determine propositions and true constants - everything else is F
  std::vector<std::string> vars;
  std::set<std::string> varsm;
  std::set<std::string> constsT;
  for (std::set<std::string>::const_iterator
         it = predInstancesT.begin(); it != predInstancesT.end(); ++it)
  {
    if (predInstancesF.find(*it) != predInstancesF.end())
    {
      vars.push_back(*it);
      varsm.insert(*it);
    }
    else
      constsT.insert(*it);
  }

  /*std::cout << "True Constants:" << std::endl;
  for (std::set<std::string>::const_iterator
        it = constsT.begin(); it != constsT.end(); ++it)
    std::cout << "  " << *it << std::endl;

  std::cout << "Propositions:" << std::endl;
  int i = 0;
  for (std::vector<std::string>::const_iterator
        it = vars.begin(); it != vars.end(); ++it, ++i)
    std::cout << "  p" << i << " = " << *it << std::endl;*/
           
  // generate grounded planning problem
  GroundedProblem result;
  // name
  result.name = nm;
  // agents
  for (std::set<std::string>::const_iterator
         it = typeInstances["agent"].begin();
         it != typeInstances["agent"].end(); ++it)
    result.agents.push_back(*it);
  // variables
  result.variables = vars;
  // and worlds
  for (std::map< std::string, std::set< std::vector<std::string> > >
        ::const_iterator it  = this->world_inits_t.begin();
                         it != this->world_inits_t.end(); ++it) 
  {
    // world name
    result.worlds.push_back(it->first);
    std::set<std::string> wtrue;

    // add to designated if neccessary
    if (this->world_design.find(it->first)->second)
      result.designated.insert(it->first);

    // and true propositions
    for (std::vector<std::string>::const_iterator
        it2 = vars.begin(); it2 != vars.end(); ++it2)
    {
      // case 1 - defined locally as true
      if (worldT[it->first].find(*it2) != worldT[it->first].end())
        wtrue.insert(*it2);
      // case 2 - defined globally as true, not reverted locally
      if (globalT.find(*it2) != globalT.end()
          && worldF.find(*it2) == worldF.end())
        wtrue.insert(*it2);
    }
    result.inits_t.push_back(wtrue);
  }
  // partitions
  for (std::vector<std::string>::const_iterator
         it = result.agents.begin(); it != result.agents.end(); ++it)
  {
    std::map<std::string, Partition>::const_iterator
      itp = this->partitions.find(*it);
    if (itp != this->partitions.end())
      result.partitions.push_back(itp->second);
    else
      result.partitions.push_back(this->defPartition);
  }

  for (std::set<std::string>::const_iterator
        it = used_actions.begin(); it != used_actions.end(); ++it)
  {
    // std::cout << "GF for " << *it << ":" << std::endl;
    result.actions.push_back(actions[*it].ground(constsT, varsm));
  }
  stringmap dummy;
  //std::cout << "FOO:" << toString(this->goal) << std::endl;
  result.goal = this->goal->ground(dummy, typeInstances);
  //std::cout << "BAR: " << toString(result.goal) << std::endl;
  result.goal = result.goal->ground(constsT, varsm);
  
  return result;
}

/*********************************************************************
*  helper stuff                                                      *
*********************************************************************/

std::set< std::map<std::string, std::string> > genInstantiations(
       const std::vector<elem> & params, const stringmmap & typeInstances)
{
  int nParams = params.size();

  // create all parameter domains
  std::vector<std::string> domP[nParams];
  for (int iP = 0; iP < nParams; ++iP)
  {
    if (params[iP].second.size() == 0)
      std::copy(typeInstances.find("object")->second.begin(),
                typeInstances.find("object")->second.end(),
                std::back_inserter(domP[iP]));
    for (std::set<std::string>::const_iterator
           it = params[iP].second.begin();
           it != params[iP].second.end(); ++it)
    {
      std::copy(typeInstances.find(*it)->second.begin(),
                typeInstances.find(*it)->second.end(),
                std::back_inserter(domP[iP]));
    }
/*
    std::cout << "Dom " << params[iP].first << ":";
    for (std::vector<std::string>::const_iterator
           it = domP[iP].begin(); it != domP[iP].end(); ++it)
      std::cout << " " << *it;
    std::cout << std::endl;
*/
  }

  // and create all combinations
  std::set< std::map<std::string, std::string> > result;
  std::map<std::string, std::string> paramMapping;
  unsigned int iV[nParams];
  for (int iP = 0; iP < nParams; ++iP)
    iV[iP] = - 1;
  int iP = 0;
  while (iP >= 0)
  {
    if (iP == nParams)
    {
      result.insert(paramMapping);
      iP -= 1;         
    }
    else
    {
      iV[iP] += 1;
      if (iV[iP] < domP[iP].size())
      {
        paramMapping[params[iP].first] =
          domP[iP][iV[iP]];
        iP += 1;
      }
      else
      {
        iV[iP] = -1;
        iP -= 1;
      }
    }
  }
  return result;
}

std::string toString(const Formula * f)
{
  std::ostringstream os;
  switch (f->type) {
    case FTRUE:
      os << "T";
    break;
    case FFALSE:
      os << "F";
    break;
    case FATOM:
    {
     if (f->groundedatom == "")
     {
       os << "(" << f->name;
       for (std::vector<std::string>::const_iterator it = f->args.begin();
       it != f->args.end(); ++it)
         os << " " << *it;
       os << ")";
     }
     else
       os << f->groundedatom;
    }
    break;
    case FNOT:
      os << "(not " << toString(f->subf[0]) << ")";
    break;
    case FAND:
      os << "(and";
      for (std::vector<Formula*>::const_iterator it = f->subf.begin();
         it != f->subf.end(); ++it)
        os << " " << toString(*it);
      os << ")";
    break;
    case FOR:
      os << "(or";
      for (std::vector<Formula*>::const_iterator it = f->subf.begin();
         it != f->subf.end(); ++it)
        os << " " << toString(*it);
      os << ")";
    break;
    case FIMPLY:
      os << "(imply " << toString(f->subf[0]) << " "
         << toString(f->subf[1]) << ")";
    break;
    case FEQUALS:
      os << "(= ";
      for(std::vector<std::string>::const_iterator it =
            f->args.begin(); it != f->args.end(); ++it)
        os << " " << *it;
      os << ")";
    break;
    case FEXISTS:
      os << "(exists (" << mmapToString(f->scope, " ")
         << ") " << toString(f->subf[0]) << ")";
    break;
    case FFORALL:
      os << "(forall (" << mmapToString(f->scope, " ")
         << ") " << toString(f->subf[0]) << ")";
    break;
    case FCOND:
      os << "(when " << toString(f->whenf) << " "
         << toString(f->thenf) << ")";
    break;
    case FKNOWS:
      os << "(knows " << f->name << " " << toString(f->subf[0]) << ")";
    break;
    case FCOMMK:
      os << "(common-knowledge " << toString(f->subf[0]) << ")";
    break;
    case LLIST:
      os << "(INITLIST";
      for (std::vector<Formula*>::const_iterator it = f->subf.begin();
         it != f->subf.end(); ++it)
        os << std::endl << " " << toString(*it);
      os << ")";
    break;
  }
  return os.str();
}



std::string mmapToString(const stringmmap & map, std::string indent)
{
  std::ostringstream os;
  for (stringmmap::const_iterator it = map.begin();
         it != map.end(); ++it)
  {
    os << indent << it->first << ": ";
    for (std::set<std::string>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); ++it2)
      os << *it2 << " ";
  os << "\b";
  }
  return os.str();
}

stringmmap typedListToObjects(const stringmmap & typedList,
                              const stringmmap & typesList)
{ 
  stringmmap result;

  // for each type: initialize empty object list
  for (stringmmap::const_iterator it = typesList.begin();
          it != typesList.end(); ++it)
     result[it->first];

  // iterate over each object
  for (stringmmap::const_iterator it = typedList.begin();
         it != typedList.end(); ++it)
    // and insert it to all of their respective classes
    for (std::set<std::string>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); ++it2)
      // and superclasses
      for (std::set<std::string>::iterator 
               it3  = typesList.find(*it2)->second.begin();
               it3 != typesList.find(*it2)->second.end(); ++it3)
          result[*it3].insert(it->first);

  return result;
}

std::string toPredName(const std::vector<std::string> & sv)
{
  std::ostringstream os;
  for (std::vector<std::string>::const_iterator it = sv.begin();
         it != sv.end(); ++it)
    os << * it << (it + 1 != sv.end() ? "-" : "");
  return os.str();
}

std::string listToString(const std::vector<elem> & list,
                         std::string indent)
{
  std::ostringstream os;
  for (std::vector<elem>::const_iterator it = list.begin();
         it != list.end(); ++it)
  {
    os << indent << it->first << ": ";
    for (std::set<std::string>::const_iterator it2 = it->second.begin();
           it2 != it->second.end(); ++it2)
      os << *it2 << " ";
    os << "\b";
  }
  return os.str();
}

std::string partToString(const Partition & part)
{
  std::ostringstream os;
  os << "{";
  for (std::map<std::string,int>::const_iterator
         it = part.pFun.begin(); it != part.pFun.end(); ++it)
    os << it->first << " -> " << it->second << ", ";
  os << "\b\b}";
  return os.str();
}

std::string initsToString(
                const std::set< std::vector<std::string> > & inits,
                std::string indent)
{
  std::ostringstream os;
  for (std::set< std::vector<std::string> >::const_iterator
         it = inits.begin(); it != inits.end(); ++it)
    os << indent << initToString(*it);
  return os.str();
}

std::string initToString(const std::vector<std::string> & i)
{
  std::ostringstream os;
  os << "(";
  for (std::vector<std::string>::const_iterator
         it = i.begin(); it != i.end(); ++it)
    os << *it << " ";  
  os << "\b)";
  return os.str();
}

