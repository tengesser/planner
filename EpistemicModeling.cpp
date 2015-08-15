// Author: T. Engesser
// (c) 2014

#include "EpistemicModeling.h"
#include "FormulaManager.h"
#include <iostream>
#include <cstring>
#include <queue>

PlanningProblem::PlanningProblem(
                         const std::string & name,
                         const EpistemicState * initState,
                         const std::vector<EpistemicAction*> actions,
                         const int & goal) :
  name(name), initState(initState), actions(actions), goal(goal) {}


EPState::EPState(const EpistemicState * state) :
  state(state), count(new int(1)) {}

EPState::EPState(const EPState & source) :
  state(source.state), count(source.count) { *count += 1; }

EPState EPState::product(const EpistemicAction & action,
                         bool & ok, bool & oka) const
{
  return EPState(this->state->product(action, ok, oka));
}

int EPState::getSize() const
{
  return state->nWorlds;
}

PlanningProblem::PlanningProblem(const PlanningProblem & source,
                                 const EPState & current) :
    name(source.name), initState(current.state),
    actions(source.actions), goal(source.goal) { }

PlanningProblem::PlanningProblem(const PlanningProblem & source) :
    name(source.name), initState(source.initState),
    actions(source.actions), goal(source.goal) { }

EPState EPState::aProduct(const EpistemicAction & action, bool & ok) const
{
  bool dummy;
  EpistemicState * inducedState =
    new EpistemicState(*this->state, action.agent);
  EpistemicState * childState = inducedState->product(action, ok, dummy);
  delete inducedState;
  return EPState(childState);
}

EPState EPState::associatedLocal(const int & agent)
{
  return EPState(new EpistemicState(*this->state, agent));
}

EPState::EPState() :
  state(NULL), count(new int(1)) { }

EPState & EPState::operator=(const EPState & source)
{
    EPState maybeDelete(*this);
    *count -= 1;
    state = source.state;
    count = source.count;
    *count += 1;
    return *this;
}
    

EPState::~EPState()
{
  *count -= 1;
  /*if (*count == 0)
  {
    delete count;
    if (state != NULL)
      delete state;
  }*/
}

bool EPState::operator<(const EPState& other) const
{
  if (this->state->nAgents < other.state->nAgents)
    return true;
  if (this->state->nAgents > other.state->nAgents)
    return false;
  if (this->state->nPropositions < other.state->nPropositions)
    return true;
  if (this->state->nPropositions > other.state->nPropositions)
    return false;
  if (this->state->nWorlds < other.state->nWorlds)
    return true;
  if (this->state->nWorlds > other.state->nWorlds)
    return false;
  for (int world = 0; world < this->state->nWorlds; ++world)
  {
    if (this->state->val[world] < other.state->val[world])
      return true;
    if (this->state->val[world] > other.state->val[world])
      return false;
    if ((*this->state->designated)[world] <
        (*other.state->designated)[world])
      return true;
    if ((*this->state->designated)[world] >
        (*other.state->designated)[world])
      return false;
  }
  
  for (int agent = 0; agent < this->state->nAgents; ++agent)
  {
    for (int world = 0; world < this->state->nWorlds; ++world)
    {
      if (this->state->eqc[agent][world] < other.state->eqc[agent][world])
        return true;
      if (this->state->eqc[agent][world] > other.state->eqc[agent][world])
        return false;
    }
  }

  return false;
}

// syntactic bisimilarity check (imprecise)
bool EPState::operator==(const EPState& other) const
{
  if (*this < other) return false;
  if (other < *this) return false;
  return true;
}



EpistemicState::EpistemicState() :
  nAgents(0), nPropositions(0), nWorlds(0),
  eqc(NULL), val(NULL), parent(NULL), action(NULL),
  fromWorld(NULL), fromEvent(NULL), designated(NULL), induced(false) {}  

EpistemicState::EpistemicState(
               const int & nAgents,const int & nPropositions,
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
               const std::vector<bool> * designated) :
  nAgents(nAgents), nPropositions(nPropositions), nWorlds(nWorlds),
  eqc(eqc), val(val), parent(parent), action(action),
  fromWorld(fromWorld), fromEvent(fromEvent), designated(designated),
#if MEMOIZATION
  memo(new MAP<int, bool>[nWorlds]),
#endif
  induced(false) {}

EpistemicState::~EpistemicState()
{
  // only delete designated set for induced state
  if (induced)
    delete designated;
  // do nothing for empty epistemic state
  else if (val != NULL)
  {
    // else delete data
    // - equivalence classes
    for (int agent = 0; agent < nAgents; ++agent)
      delete[] eqc[agent];
    delete[] eqc;
    // - valuation functions
#if !USEBITSETS
    for (int world = 0; world < nWorlds; ++world)
      delete[] val[world];
#endif
    delete[] val;
    // - world history information
    delete fromWorld;
    delete fromEvent;
    // - set of designated workds
    delete designated;
    // memoization
#if MEMOIZATION
    delete[] memo;
#endif
    // remove predecessor information
    parent = NULL; // has to be manually deleted
    action = NULL; // has to be manually deleted
  }
}

EpistemicAction:: ~EpistemicAction()
{
  // delete data
  // - equivalence classes
  for (int agent = 0; agent < nAgents; ++agent)
    delete [] eqc[agent];
  delete [] eqc;
  // - preconditions
  delete [] prec;
  // - add and delete effects
  delete [] add_effects; // TODO ??
  delete [] del_effects; // TODO ??
  // - set of designated worlds
  delete [] designated;
}

EpistemicAction::EpistemicAction(
                  const int & nAgents, const int & nEvents,
                  const int & agent,
                  const int ** eqc, const int * prec,
#if USEBITSETS
                  const boost::dynamic_bitset<> * add_effects,
                  const boost::dynamic_bitset<> * del_effects,
#else
                  const std::vector<int> * add_effects,
                  const std::vector<int> * del_effects,
#endif
                  const std::vector<bool> * designated) :
  nAgents(nAgents), nEvents(nEvents), agent(agent), eqc(eqc), prec(prec),
  add_effects(add_effects), del_effects(del_effects),
  designated(designated) {}

struct SimHeap
{
  SimHeap() : index(-1), left(NULL), right(NULL) {}
  int index;
  boost::dynamic_bitset<> bs;
  SimHeap * left;
  SimHeap * right;
};

void insert(SimHeap & s, int i, boost::dynamic_bitset<> bs)
{
  if (s.index == -1)
  {
    s.index = i;
    s.bs = bs;
    s.left = new SimHeap();
    s.right = new SimHeap();
  }
  else if (bs < s.bs)
    insert(*s.left, i, bs);
  else
    insert(*s.right, i, bs);
}

void sortIndices(SimHeap & s, std::vector<int> & result)
{
  if (s.index != -1)
  {
    sortIndices(*s.left, result);
    result.push_back(s.index);
    sortIndices(*s.left, result);
  }
}
    

/// product update of an epistemic state and an epistemic action
EpistemicState * EpistemicState::product(const EpistemicAction & action,
                                         bool & ok, bool & oka) const
{
  // trivial data: number of agents / propositions
  //               parent state / action
  const int nAgents = this->nAgents;
  const int nPropositions = this->nPropositions;
  const EpistemicState * parent = this;
  const EpistemicAction * act = &action;
  // create worlds for new state
  std::vector<int> * fromWorld = new std::vector<int>();
  std::vector<int> * fromEvent = new std::vector<int>();
  std::vector<bool> * designated = new std::vector<bool>();
  // is agent applicable for executing agent?
  oka = true;
  // determine world that could be true, as seen by the executing agent
  bool classDed[this->nWorlds]; // |eqc| < |worlds|
  // initialize with false
  for (int w = 0; w < this->nWorlds; ++w)
    classDed[w] = false;
  // and set true for appropriate classes
  for (int w = 0; w < this->nWorlds; ++w)
    if (this->designated[0][w])
      classDed[this->eqc[action.agent][w]] = true;
  for (int w = 0; w < this->nWorlds; ++w)
  {
    bool worldOk = false;
    for (int e = 0; e < action.nEvents; ++e)
    {
      if (evaluate(*this, w, action.prec[e]))
      {
        fromWorld->push_back(w);
        fromEvent->push_back(e);
        designated->push_back((*this->designated)[w] &&
                              (*action.designated)[e]);
        worldOk |= action.designated[0][e];
       }
    }
    if (this->designated[0][w] && !worldOk)
    {
      delete fromWorld;
      delete fromEvent;
      delete designated;
      ok = false;
      oka = false;
      return new EpistemicState();
    }
    if (classDed[this->eqc[action.agent][w]] && !worldOk)
      oka = false;
  }
  const int nWorlds = fromWorld->size();
  // calculate accessibility relation for each agent
  int ** eqc = new int*[nAgents];
  for (int a = 0; a < nAgents; ++a)
  {
    const int classbound = this->nWorlds * action.nEvents;
    int * cmap = new int[classbound];
    for (int i = 0; i < classbound; ++i)
      cmap[i] = -1;                                    
    eqc[a] = new int[nWorlds];
    int c = 0;
    for (int w = 0; w < nWorlds; ++w)
    {
      int tclass = this->eqc[a][(*fromWorld)[w]] * action.nEvents
                 + action.eqc[a][(*fromEvent)[w]];
      if (cmap[tclass] == -1)
        cmap[tclass] = c++;
      eqc[a][w] = cmap[tclass];
    }
    delete[] cmap;
  }
  // calculate valuation function for each world
#if USEBITSETS
  boost::dynamic_bitset<> * val =
    new boost::dynamic_bitset<>[nWorlds];
  for (int w = 0; w < nWorlds; ++w)
  {
    int e = (*fromEvent)[w];
    val[w]  =  parent->val[(*fromWorld)[w]];
    val[w] |=  action.add_effects[e];
    val[w] &= ~(action.del_effects[e]);
  }
#else
  char ** val = new char*[nWorlds];
  for (int w = 0; w < nWorlds; ++w)
  {
    val[w] = new char[nPropositions];
    // bool changed = false;
    memcpy((void*)val[w], (void*)(parent->val[(*fromWorld)[w]]), 
           nPropositions * sizeof(char));
    int e = (*fromEvent)[w];
    for (std::vector<int>::const_iterator it = 
         action.add_effects[e].begin();
         it != action.add_effects[e].end(); ++it)
    {
      // changed |= !val[w][*it];
      val[w][*it] = true;
    }
    for (std::vector<int>::const_iterator it =
         action.del_effects[e].begin();
         it != action.del_effects[e].end(); ++it)
    {
      // changed |= val[w][*it];
      val[w][*it] = false;
    }
    /* if (!changed) // Speicherhack
    {
      delete val[w];
      val[w] = const_cast<char *>(parent->val[(*fromWorld)[w]]);
    }*/
  }
#endif
  ok = true;
  // re-sort
#if USEBITSETS
#if CONTRACT
  return contract(nAgents, nPropositions, nWorlds, eqc, val,
                  parent, act, fromWorld, fromEvent, designated);
#else
  return new EpistemicState(nAgents, nPropositions, nWorlds, 
                            const_cast<const int **>(eqc), val,
                            parent, act, fromWorld, fromEvent,
                            designated);
#endif


#else
  return new EpistemicState(nAgents, nPropositions, nWorlds, 
                            const_cast<const int **>(eqc),
                            const_cast<const char **>(val),
                            parent, act, fromWorld, fromEvent,
                            designated);
#endif
}

bool sortsBefore(const int & nAgents,
                 std::vector<bool> * designated, int ** eqc,
                 const int & w1, const int & w2)
{
  if ((*designated)[w1] < (*designated)[w2])
    return true;
  if ((*designated)[w1] > (*designated)[w2])
    return false;
  for (int a = 0; a < nAgents; ++a)
  {
    if (eqc[a][w1] < eqc[a][w2])
      return true;
    if (eqc[a][w1] > eqc[a][w2])
      return false;
  }
  return false;
}

bool worldsEqual(const int & nAgents,
                 std::vector<bool> * designated, int ** eqc,
                 const int & w1, const int & w2)
{
  return    !sortsBefore(nAgents, designated, eqc, w1, w2)
         && !sortsBefore(nAgents, designated, eqc, w2, w1);
}

EpistemicState * contract(const int & nAgents, const int & nPropositions,
                        int nWorlds, int ** eqc,
                        boost::dynamic_bitset<> * val,
                        const EpistemicState * parent,
                        const EpistemicAction * action,
                        std::vector<int> * fromWorld,
                        std::vector<int> * fromEvent,
                        std::vector<bool> * designated)
{
// 1st find unconnected (= unimportand) worlds - O(|A||W|log|W|)?
  // the result set of all connected worlds, initiate with designated
  std::set<int> connected;
  for (int w = 0; w < nWorlds; ++w)
    if ((*designated)[w])
      connected.insert(w);
  
  // O(|A||W|) - Make Mapping from EQC to connected worlds
  std::map< int, std::set<int> > * eqcmap
    = new std::map< int, std::set<int> >[nAgents];
  for (int a = 0; a < nAgents; ++a)
    for (int w = 0; w < nWorlds; ++w)
      eqcmap[a][eqc[a][w]].insert(w);
  
  // O(|W||A||RA|) = O(|A||W|log|W|)? search
  std::queue<int> toSearch;
  for (std::set<int>::iterator it = connected.begin();
         it != connected.end(); ++it)
    toSearch.push(*it);
  while (!toSearch.empty())
  {
    int w = toSearch.front();
    toSearch.pop();
    for (int a = 0; a < nAgents; ++a)
      for (std::set<int>::iterator
             it = eqcmap[a][eqc[a][w]].begin(); it != eqcmap[a][eqc[a][w]].end(); ++it)
        if (connected.find(*it) == connected.end())
        {
          connected.insert(*it);
          toSearch.push(*it);
        }
  }
  delete [] eqcmap;
  int nCWorlds = connected.size();
  // std::cout << nWorlds << " >>> " << nCWorlds;

// 2nd sort by bitset value
  // standard sort O(|W|log|W|)
  std::vector< std::pair< boost::dynamic_bitset<> , int > > sortWorlds;
  for (int w = 0; w < nWorlds; ++w)
    if (connected.find(w) != connected.end())
      sortWorlds.push_back(
        std::pair< boost::dynamic_bitset<>,int>(val[w], w));
  std::sort(sortWorlds.begin(), sortWorlds.end());
  // designated < nondesignated
  // low eqc < big eqc for A0, then A1, then A2, ...
  // in O(|W||W|)
  for (int w = 0; w < nCWorlds; ++w)
  {
    int wswap = w;
    while(   wswap > 0
          && sortWorlds[wswap].first == sortWorlds[wswap-1].first
          && sortsBefore(nAgents, designated, eqc,
                         sortWorlds[wswap].second,
                         sortWorlds[wswap-1].second))
    {
      std::swap(sortWorlds[wswap], sortWorlds[wswap-1]);
      wswap -= 1;
    }
  }
// 3nd remove val/eqc(A)/des equivalent worlds
  std::vector< std::pair< boost::dynamic_bitset<> , int > > finalWorlds;
  for (int w = 0; w < nCWorlds; ++w)
  {
    if (   finalWorlds.size() == 0
        || sortWorlds[w].first != finalWorlds.back().first
        || !worldsEqual(nAgents, designated, eqc,
                        sortWorlds[w].second,
                        finalWorlds.back().second))
      finalWorlds.push_back(sortWorlds[w]);
  }
  int nFinalWorlds = finalWorlds.size();
  // std::cout << " >>> " << nFinalWorlds << " worlds" << std::endl;
// 4nd remap equivalence classes: each agent 
  // new valuation function / equivalence relation / history information
  boost::dynamic_bitset<> * valNew
    = new boost::dynamic_bitset<>[nFinalWorlds];
  int ** eqcNew = new int*[nAgents];
  for (int a = 0; a < nAgents; ++a)
    eqcNew[a] = new int[nFinalWorlds];
  std::vector<int> * newFromWorld = 
    fromWorld == NULL ? NULL : new std::vector<int>(nFinalWorlds);
  std::vector<int> * newFromEvent =
    fromEvent == NULL ? NULL : new std::vector<int>(nFinalWorlds);
  std::vector<bool> * newDesignated = new std::vector<bool>(nFinalWorlds);
  // helper variables for re-mapping the eq. classes
  std::map<int,int> * neqcmap = new std::map<int,int>[nAgents];
  int * nexteqc = new int[nAgents];
  for (int a = 0; a < nAgents; ++a)
    nexteqc[a] = 0;
  // fill stuff
  for (int w = 0; w < nFinalWorlds; ++w)
  {
    valNew[w] = finalWorlds[w].first;
    int world = finalWorlds[w].second;
    if (fromWorld != NULL && fromWorld->size() > 0)
      (*newFromWorld)[w] = (*fromWorld)[world];
    if (fromEvent != NULL && fromEvent->size() > 0)
      (*newFromEvent)[w] = (*fromEvent)[world];
    (*newDesignated)[w] = (*designated)[world];
    for (int a = 0; a < nAgents; ++a)
    {
      int oldeqc = eqc[a][world];
      std::map<int,int>::iterator search = neqcmap[a].find(oldeqc);
      if (search == neqcmap[a].end())
      {
        eqcNew[a][w] = nexteqc[a];
        neqcmap[a][oldeqc] = nexteqc[a]++;
      }
      else
        eqcNew[a][w] = search->second;
    }
  }
  delete [] neqcmap;
  delete [] nexteqc;

  // delete old junk
  for (int a = 0; a < nAgents; ++a)
    delete [] eqc[a];
  delete [] eqc;
  delete [] val;
  if (fromWorld != NULL)
    delete fromWorld;
  if (fromEvent != NULL)
    delete fromEvent;
  delete designated;

  return new EpistemicState(nAgents, nPropositions, nFinalWorlds, 
                            const_cast<const int **>(eqcNew), valNew,
                            parent, action, newFromWorld, newFromEvent,
                            newDesignated);
}

const int ** eqccp(const int ** eqc, const int & nAgents,
                   const int & nWorlds)
{
  int ** result = new int*[nAgents];
  for (int a = 0; a < nAgents; ++a)
  {
    result[a] = new int[nWorlds];
    memcpy((void*)result[a], (void*)eqc[a], nWorlds * sizeof(int));
  }
  return const_cast<const int **>(result);
}

#if USEBITSETS
boost::dynamic_bitset<> * valcp(const boost::dynamic_bitset<> * val,
                                const int & nWorlds)
{
  boost::dynamic_bitset<> * result = new boost::dynamic_bitset<>[nWorlds];
  for (int w = 0; w < nWorlds; ++w)
    result[w] = boost::dynamic_bitset<>(val[w]);
  return result;
}
#else
char ** valcp(const char ** val, const int & nWorlds, const int & nProps)
{
  char ** result = new int*[nWorlds];
  for (int w = 0; w < nWorlds; ++w)
  {
    result[w] = new int[nProps];
    memcpy((void*)result[w], (void*)eqc[w], nWorlds * sizeof(char));
  }
  return result;
}
#endif

std::vector<bool> * stbv(const std::set<int> src, int nWorlds)
{
  std::vector<bool> * result = new std::vector<bool>(nWorlds);
  for (int w = 0; w < nWorlds; ++w)
    result[0][w] = false;
  for (std::set<int>::const_iterator
         it = src.begin(); it != src.end(); ++it)
    result[0][*it] = true;
  return result;
}

std::vector<int> * mbcpiv(const std::vector<int> * iv)
{
  return iv == NULL ? NULL : new std::vector<int>(*iv);
}
    

EpistemicState::EpistemicState(const EpistemicState & source,
                               const std::set<int> & designated)
  :
  nAgents(source.nAgents), nPropositions(source.nPropositions),
  nWorlds(source.nWorlds),
  eqc(eqccp(source.eqc, source.nAgents, source.nWorlds)),
#if USEBITSETS
  val(valcp(source.val, source.nWorlds)),
#else
  val(valcp(source.val, source.nWorlds, source.nPropositions)),
#endif
  parent(source.parent), action(source.action),
  fromWorld(mbcpiv(source.fromWorld)),
  fromEvent(mbcpiv(source.fromEvent)),
  designated(stbv(designated, source.nWorlds)),
#if MEMOIZATION
  memo(new MAP<int, bool>[nWorlds]),
#endif
  induced(false) {} 

std::vector<EPState> EPState::split() const
{
  if (this->count <= 0)
    std::cerr << "WUT?" << std::endl;
  std::vector<EPState> result;

  std::set<int> des;
  for (unsigned int w = 0; w < this->state->designated->size(); ++w)
    if (this->state->designated[0][w])
      des.insert(w);

  std::vector< std::set<int> > split;
  split.push_back(des);
 
  for (int a = 0; a < this->state->nAgents; ++a)
  {
    // new splitting of designated sets
    std::vector< std::set<int> > newSplit;
    // for each old designated subset: split further for agent a
    for (std::vector< std::set<int> >::const_iterator
           sit = split.begin(); sit != split.end(); ++sit)
    {
      // map for the splitting
      std::map< int, std::set<int> > splitmap;
      // associate elements to the agent's equivalence classes
      for (std::set<int>::const_iterator
             eit = sit->begin(); eit != sit->end(); ++eit)
        splitmap[this->state->eqc[a][*eit]].insert(*eit);
      // and use these associations as the new subsets
      for (std::map< int, std::set<int> >::const_iterator
             dsit = splitmap.begin(); dsit != splitmap.end(); ++dsit)
        newSplit.push_back(dsit->second);
    }
    split = newSplit;
  }

  //std::cout << "SPLITS" << std::endl;
  for (std::vector< std::set<int> >::const_iterator
         ds = split.begin(); ds != split.end(); ++ds)
  {
    EpistemicState * s = new EpistemicState(*this->state, *ds);
    result.push_back(EPState(s));
  //std::cout << PrettyPrint::toString(*s, NULL, false, true, true)
  //          << std::endl;
  }

  return result; 
}

EpistemicState * newEpistemicStateCopy(const EpistemicState * source)
{
  const int nAgents = source->nAgents;
  const int nPropositions = source->nPropositions;
  const int nWorlds = source->nWorlds;

  int ** eqc = new int*[nAgents];
  for (int a = 0; a < nAgents; ++a)
  {
    eqc[a] = new int[nWorlds];
    for (int w = 0; w < nWorlds; ++w)
      eqc[a][w] = source->eqc[a][w];
  }   
#if USEBITSETS
  boost::dynamic_bitset<> * val = 
    new boost::dynamic_bitset<>[nWorlds];
  for (int w = 0; w < nWorlds; ++w)
    val[w] = source->val[w];
#else
  char ** val = new char*[nWorlds]
  for (int w = 0; w < nWorlds; ++w)
  {
    val[w] = new char[nPropositions];
    for (int p = 0; p < nPropositions; ++p)
      val[w][p] = source->val[w][p];
#endif
  const EpistemicState * parent = source->parent;
  const EpistemicAction * action = source->action;
  std::vector<int> * fromWorld = NULL;
  if (source->fromWorld != NULL)
  {
    fromWorld = new std::vector<int>();
    fromWorld[0] = source->fromWorld[0];
  }
  std::vector<int> * fromEvent = NULL;
  if (source->fromEvent != NULL)
  {
    fromEvent = new std::vector<int>();
    fromWorld[0] = source->fromEvent[0];
  }
  std::vector<bool> * designated = new std::vector<bool>();
  designated[0] = source->designated[0];
  
  return new EpistemicState(nAgents, nPropositions, nWorlds,
                            const_cast<const int **>(eqc),
#if USEBITSETS
                            val,
#else
                            const_cast<const char **>(val),
#endif
                            parent, action, fromWorld, fromEvent,
                            designated);

}

EpistemicState * newEpistemicStateCopyC(const EpistemicState * source2)
{
  EpistemicState * source = newEpistemicStateCopy(source2);
  return contract(source->nAgents, source->nPropositions,
                  source->nWorlds, const_cast<int**>(source->eqc),
                  const_cast<boost::dynamic_bitset<>*>(source->val),
                  source->parent, source->action, 
                  const_cast<std::vector<int>*>(source->fromWorld),
                  const_cast<std::vector<int>*>(source->fromEvent),
                  const_cast<std::vector<bool>*>(source->designated));
}


bool evaluate(const EpistemicState & state, const int & phi)
{
  for (int world = 0; world < state.nWorlds; ++world)
    if (state.designated[0][world] && !evaluate(state, world, phi))
      return false;
  return true;
}

bool EPState::eval(const int & phi) const
{
  return evaluate(*(this->state), phi);
}

FormulaManager & fm = FormulaManager::getInstance();

bool evaluate(const EpistemicState & state, const int & world, 
                                            const int & phi)
{
#if MEMOIZATION
  // first, check memotable
  MAP<int,bool>::const_iterator it = state.memo[world].find(phi);
  if (it != state.memo[world].end())
    return it->second;
#endif
 
  bool result = true;
  switch (fm.ftype[phi])
  {
    case -2:
      result = false;
      break;
    case -1:
      result = true;
      break;
    case 0:
      result = state.val[world][fm.farg1[phi]];
      break;
    case 1:
      result = !evaluate(state, world, fm.farg1[phi]);
      break;
    case 2:
      for (std::set<int>::const_iterator
             it  = fm.cargs[fm.farg1[phi]].begin();
             it != fm.cargs[fm.farg1[phi]].end(); ++it)
        if (!evaluate(state, world, *it))
        {
          result = false;
          break;
        }
      break;
    case 3:
      for (int oworld = 0; oworld < state.nWorlds; ++oworld)
        if (   state.eqc[fm.farg1[phi]][oworld]
            == state.eqc[fm.farg1[phi]][world]
            && !evaluate(state, oworld, fm.farg2[phi]))
        {
          result = false;
          break;
        }
      break;
    default: // TODO: test CK
      std::set<int> candidates;
      std::queue<int> toCheck;
      for (int world = 0; world < state.nWorlds; ++world)
      {
        if ((*state.designated)[world])
          toCheck.push(world);
        else
          candidates.insert(world);
      }
      result = true;
      while (result && !toCheck.empty())
      {
        int w = toCheck.front();
        toCheck.pop();
        std::set<int>::iterator pos = candidates.find(w);
        if (pos != candidates.end())
           candidates.erase(pos);
        result &= evaluate(state, w, fm.farg1[phi]);
        if (result)
        for (std::set<int>::const_iterator it = candidates.begin();
               it != candidates.end(); ++it)
        {
          for (int agent = 0; agent < state.nAgents; ++agent)
            if (state.eqc[agent][*it] == state.eqc[agent][w])
              toCheck.push(*it);
        }
      }
      break;
  }
  // finally write memo table and return result
#if MEMOIZATION
#pragma omp critical
  state.memo[world][phi] = result;
#endif
  return result;
}

const std::vector<bool> * inducedDedWorlds(const EpistemicState & source,
                                           const int & agent)
{
  std::vector<bool> * result = new std::vector<bool>;
  // determine world that could be true, as seen by the executing agent
  bool classDed[source.nWorlds]; // |eqc| < |worlds|
  // initialize with false
  for (int w = 0; w < source.nWorlds; ++w)
    classDed[w] = false;
  // and set true for appropriate classes
  for (int w = 0; w < source.nWorlds; ++w)
    if (source.designated[0][w])
      classDed[source.eqc[agent][w]] = true;
  for (int w = 0; w < source.nWorlds; ++w)
    result->push_back(classDed[source.eqc[agent][w]]);
  return result;
}

EpistemicState::EpistemicState(const EpistemicState & source,
                               const int & agent) :
  nAgents(source.nAgents), nPropositions(source.nPropositions),
  nWorlds(source.nWorlds), eqc(source.eqc), val(source.val),
  parent(source.parent), action(source.action),
  fromWorld(source.fromWorld), fromEvent(source.fromEvent),
  designated(inducedDedWorlds(source, agent)),
#if MEMOIZATION
  memo(source.memo), 
#endif
  induced(true) { }

boost::dynamic_bitset<> * abstractVal(
  const boost::dynamic_bitset<> * val,
  const boost::dynamic_bitset<> & delMask,
  int nWorlds)
{
  boost::dynamic_bitset<> * result = new boost::dynamic_bitset<>[nWorlds];
  for (int w = 0; w < nWorlds; ++w)
    result[w] = val[w] & delMask;
  return result;
}

EpistemicState * relState(const EpistemicState & source,
                          const int & depth)
{
  const int nAgents = source.nAgents;
  const int nPropositions = source.nPropositions;
  // determine interesting worlds
  bool * interesting = new bool[source.nWorlds];
  std::set<int> worlds;
  std::queue<int> newWorlds;
  for (int w = 0; w < source.nWorlds; ++w)
  {
    interesting[w] = false;
    if (source.designated[0][w])
      newWorlds.push(w);
  }
  
  for (int d = -1; d < depth && !newWorlds.empty(); ++d)
  {
    std::set<int> newNewWorlds;
    while (!newWorlds.empty())
    {
      int w = newWorlds.front(); newWorlds.pop();
      if (!interesting[w])
      {
        interesting[w] = true;
        worlds.insert(w);
        for (int w2 = 0; w2 < source.nWorlds; ++w2)
          if (source.eqc[w] == source.eqc[w2])
            newNewWorlds.insert(w);
      }
    }
    for (std::set<int>::const_iterator it = newNewWorlds.begin();
           it != newNewWorlds.end(); ++it)
      if (!interesting[*it])
        newWorlds.push(*it);
  }
  delete [] interesting;

  const int nWorlds = worlds.size();
  int ** eqc = new int*[nAgents];
  for (int a = 0; a < nAgents; ++a)
  {
    eqc[a] = new int[nWorlds];
    int w = 0;
    for (std::set<int>::const_iterator it = worlds.begin();
           it != worlds.end(); ++it, ++w)
      eqc[a][w] = source.eqc[a][*it];
  }
  boost::dynamic_bitset<> * val =
    new boost::dynamic_bitset<>[nWorlds];
  std::vector<int> * fromWorld = NULL;
  std::vector<int> * fromEvent = NULL;
  if (source.fromWorld != NULL)
    fromWorld = new std::vector<int>;
  if (source.fromEvent != NULL)
    fromEvent = new std::vector<int>;
  std::vector<bool> * designated = new std::vector<bool>;
  int w = 0;
  for (std::set<int>::const_iterator it = worlds.begin();
         it != worlds.end(); ++it, ++w)
  {
    val[w] = source.val[*it];
    if (fromWorld != NULL)
      fromWorld->push_back(source.fromWorld[0][*it]);
    if (fromEvent != NULL)
      fromEvent->push_back(source.fromEvent[0][*it]);
    designated->push_back(source.designated[0][*it]);
  }

  return contract(nAgents, nPropositions, nWorlds,
                  const_cast<int**>(eqc), val,
                  source.parent, source.action,
                  fromWorld, fromEvent, designated);
}

EpistemicState * absState(const EpistemicState & source2,
                          const boost::dynamic_bitset<> & delMask)
{
  EpistemicState * source = newEpistemicStateCopy(&source2);
  return contract(source->nAgents, source->nPropositions,
                  source->nWorlds, const_cast<int**>(source->eqc),
                  abstractVal(source->val, delMask, source->nWorlds),
                  source->parent, source->action, 
                  const_cast<std::vector<int>*>(source->fromWorld),
                  const_cast<std::vector<int>*>(source->fromEvent),
                  const_cast<std::vector<bool>*>(source->designated));
}

EPState EPState::abstract(const boost::dynamic_bitset<> & delMask) const
{
  return EPState(absState(*this->state, delMask));
}

EPState EPState::relax(const int & depth) const
{
  return EPState(relState(*this->state, depth));
}

int convertGF(const Formula * f, const std::map<std::string, int> & vars,
              const std::map<std::string, int> & agents)
{
  int result = -1;
  switch (f->type)
  { 
    case FATOM:
      result = FormulaManager::getInstance()
                 .genProp(vars.find(f->groundedatom)->second);
      break;
    case FNOT:
      result = FormulaManager::getInstance()
                 .genNeg(convertGF(f->subf[0], vars, agents));
      break;
    case FAND:
    {
      std::set<int> subf;
      for (std::vector<Formula*>::const_iterator
             it = f->subf.begin(); it != f->subf.end(); ++it)
        subf.insert(convertGF(*it, vars, agents));
      result = FormulaManager::getInstance().genCon(subf);
      break;
    }
    case FOR:
    {
      std::set<int> subf;
      for (std::vector<Formula*>::const_iterator
             it = f->subf.begin(); it != f->subf.end(); ++it)
        subf.insert(convertGF(*it, vars, agents));
      result = FormulaManager::getInstance().genDis(subf);
      break;
    }
    case FIMPLY:
      result = FormulaManager::getInstance()
                 .genImp(convertGF(f->subf[0], vars, agents),
                         convertGF(f->subf[1], vars, agents));
      break;
    case FKNOWS:
      result = FormulaManager::getInstance()
                 .genK(agents.find(f->name)->second,
                       convertGF(f->subf[0], vars, agents));
      break;
    case FTRUE: // HACK
    {
      int p0 = FormulaManager::getInstance().genProp(0);
      std::set<int> subf;
      subf.insert(p0);
      subf.insert(FormulaManager::getInstance().genNeg(p0));
      result = FormulaManager::getInstance().genDis(subf);
      break;
    }
    case FFALSE: // HACK
    {
      int p0 = FormulaManager::getInstance().genProp(0);
      std::set<int> subf;
      subf.insert(p0);
      subf.insert(FormulaManager::getInstance().genNeg(p0));
      result = FormulaManager::getInstance().genCon(subf);
      break;
    }
    case FCOMMK:
      result = FormulaManager::getInstance()
                 .genCK(convertGF(f->subf[1], vars, agents));
      std::cerr << "WARNING: KC HAS NOT BEEN TESTED YET!"
                << std::endl;
      break;
    case FCOND:
    case LLIST:
      std::cerr << "ERROR: THESE DON'T MAKE ANY SENSE HERE!"
                << std::endl;
      break;
    case FFORALL:
    case FEXISTS:
    case FEQUALS:
      std::cerr << "ERROR: THESE SHOULD HAVE BEEN GROUNDED OUT!"
                << std::endl;
      break;
  }
  return result;
}

void convertOF(const Formula * f, const std::map<std::string, int> & vars,
#if USEBITSETS
               boost::dynamic_bitset<> & add_effs,
               boost::dynamic_bitset<> & del_effs)
#else
               std::vector<int> & add_effs, std::vector<int> & del_effs)
#endif
{
  switch (f->type)
  { 
    case FATOM:
#if USEBITSETS
      add_effs[vars.find(f->groundedatom)->second] = 1;
#else
      add_effs.push_back(vars.find(f->groundedatom)->second);
#endif
      break;
    case FNOT:
      convertOF(f->subf[0], vars, del_effs, add_effs);
      break;
    case FAND:
      for (std::vector<Formula*>::const_iterator
             it = f->subf.begin(); it != f->subf.end(); ++it)
        convertOF(*it, vars, add_effs, del_effs);
      break;
    case FTRUE:  // ok?
    case FFALSE: // ok?
      break;
    case FCOND:
      std::cerr << "ERROR: WHEN EFFS HAS NOT BEEN FULLY IMPLEMENTED YET!"
                 << std::endl;
      break;
    case FOR:
    case FIMPLY:
    case FKNOWS:
    case FFORALL:
    case FEXISTS:
    case FEQUALS:
    case FCOMMK:
    case LLIST: // ok?
      std::cerr << "ERROR: THESE DON'T MAKE ANY SENSE HERE!!!"
                << std::endl;
    break;
  }
}

PlanningProblem generatePP(const GroundedProblem & p)
{
  // the name
  std::string name = p.name;

  // the initial state
  const int nAgents = p.agents.size();
  const int nPropositions = p.variables.size();
  const int nWorlds = p.worlds.size();

  // agent and var mapping
  std::map<std::string, int> agents;
  for (int agent = 0; agent < nAgents; ++agent)
    agents[p.agents[agent]] = agent;
  std::map<std::string, int> vars;
  for (int var = 0; var < nPropositions; ++var)
    vars[p.variables[var]] = var;
    
#if USEBITSETS
  boost::dynamic_bitset<> * val = new boost::dynamic_bitset<>[nWorlds];
#else
  char ** val = new char*[nWorlds];
#endif
  // retrieve valuation function
  for (int world = 0; world < nWorlds; ++world)
  {
#if USEBITSETS
    val[world] = boost::dynamic_bitset<>(nPropositions);
#else
    val[world] = new char[nPropositions];
#endif
    for (int prop = 0; prop < nPropositions; ++prop)
    {
      if (   p.inits_t[world].find(p.variables[prop])
          != p.inits_t[world].end())
        val[world][prop] = true;
      else
        val[world][prop] = false;
    }
  }
  // and the equivalence classes
  int ** eqc = new int*[nAgents];
  for (int agent = 0; agent < nAgents; ++agent)
  {
    eqc[agent] = new int[nWorlds];
    for (int world = 0; world < nWorlds; ++world)
      eqc[agent][world] = p.partitions[agent].pFun
                           .find(p.worlds[world])->second;
  }
  // as well as the designated set
  std::vector<bool> * designated  = new std::vector<bool>;
  for (int world = 0; world < nWorlds; ++world)
    designated->push_back(   p.designated.find(p.worlds[world])
                          != p.designated.end());

  

  // -> Initial State
  EpistemicState * initState =
    new EpistemicState(nAgents, nPropositions, nWorlds, 
                       const_cast<const int**>(eqc),
#if USEBITSETS
                       val,
#else
                       const_cast<const char**>(val),
#endif
                       NULL, NULL, NULL, NULL, designated);

  // the action set
  std::vector<EpistemicAction*> actions;
  for (std::vector<GroundedAction>::const_iterator
         a = p.actions.begin(); a != p.actions.end(); ++a)
  {
    // number of events
    const int nEvents = a->events.size();
    int * prec = new int[nEvents];
#if USEBITSETS
    boost::dynamic_bitset<> * add_effects
      = new boost::dynamic_bitset<>[nEvents];
    boost::dynamic_bitset<> * del_effects
      = new boost::dynamic_bitset<>[nEvents];
#else
    std::vector<int> * add_effects = new std::vector<int>[nEvents];
    std::vector<int> * del_effects = new std::vector<int>[nEvents];
#endif
    const int agent = agents[a->agent];

    // events
    for (int event = 0; event < nEvents; ++event)
    {
      prec[event] = convertGF(a->events[event].prec, vars, agents);
#if USEBITSETS
    add_effects[event] = boost::dynamic_bitset<>(nPropositions);
    del_effects[event] = boost::dynamic_bitset<>(nPropositions);
#endif
      convertOF(a->events[event].eff, vars,
                add_effects[event], del_effects[event]);
      /*std::cout << "  Event " << event << " - " << a->event_names[event]
                << std::endl;
      std::cout << "    GF: " << toString(a->events[event].prec)
                << std::endl;
      std::cout << "       = " << PrettyPrint::toString(prec[event])
                << std::endl;*/
    }
    // equivalence classes
    int ** eqc = new int*[nAgents];
    for (int agent = 0; agent < nAgents; ++agent)
    {
      eqc[agent] = new int[nEvents];
      for (int event = 0; event < nEvents; ++event)
        eqc[agent][event] = a->partitions.find(p.agents[agent])->second
                              .pFun.find(a->event_names[event])->second;
    }
    // designated set
    std::vector<bool> * designated = new std::vector<bool>;
    for (int event = 0; event < nEvents; ++event)
      designated->push_back(   a->design.find(a->event_names[event])
                            != a->design.end());

    EpistemicAction * ea = new EpistemicAction(
                         nAgents, nEvents, agent,
                         const_cast<const int**>(eqc),
                         prec, add_effects, del_effects, designated);
    // std::cout << "Action " << actions.size() << ": "
    //           << a->name << std::endl;
    actions.push_back(ea);
  }
  // goal formula
  const int goal = convertGF(p.goal, vars, agents);

  return PlanningProblem(name, initState, actions, goal);
}



