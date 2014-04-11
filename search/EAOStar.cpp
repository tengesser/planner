// Author: T. Engesser
// (c) 2014

#include "EAOStar.h"
#include <iostream>
#include <sstream>
#include <queue>

#define MIN(x,y) x < y ? x : y
#define MAX(x,y) x > y ? x : y

AncestorInfo::AncestorInfo(EAONode * const parent,
                           const int & split, const int & action) :
  parent(parent), split(split), action(action) { }

// lexicographical ordering for AncestorInfo
// does compare parent, split, action in that order
bool AncestorInfo::operator<(const AncestorInfo & other) const
{
  if (this->parent < other.parent)
    return true;
  if (this->parent > other.parent)
    return false;
  if (this->split < other.split)
    return true;
  if (this->split > other.split)
    return false;
  if (this->action < other.action)
    return true;
  if (this->action > other.action)
    return false;
  return false;
}

EAONode::EAONode(const EPState & state, const int & hvalue,
                 EAONode * parent, const int & parentSplit,
                 const int & parentAction) :
  state(state), gvalue (parent == NULL ? 0 : parent->gvalue + 1),
  hvalue(hvalue), solveState(SSSearch),
  maxGoalDist(-1), mhh(NULL)
{
  if (parent != NULL)
    parents.insert(AncestorInfo(parent, parentSplit, parentAction));
}

void registerWithHeap(EAONode * node, MinHeap & heap)
{
  // remove previous handle
  if (node->mhh != NULL)
    delete node->mhh;
  // insert node and create new hanlde
  node->mhh = new MHHandle(heap.push(node));
}

EAONode * takeFromHeap(MinHeap & heap)
{
  EAONode * result = heap.top();
  delete result->mhh;
  result->mhh = NULL;
  heap.pop();
  return result;
}

void cleanupHeap(MinHeap & heap)
{
  while (!heap.empty())
    takeFromHeap(heap);
}

bool EAOcompare::operator()(EAONode * const & n1,
                            EAONode * const & n2) const
{
  if (n1->hvalue == -1 && n2->hvalue != -1)
    return true;
  else if (n1->hvalue != -1 && n2->hvalue == -1)
    return false;
  else
    return n1->gvalue + n1->hvalue > n2->gvalue + n2->hvalue;
}

AncestorInfo appendParent(EAONode * parent, EAONode * toNode,
                          const int & parentSplit,
                          const int & parentAction,
                          bool refreshNode)
{
  AncestorInfo ai(parent, parentSplit, parentAction);
  std::set<AncestorInfo>::iterator found =
    toNode->parents.find(ai);
  if (found == toNode->parents.end())
  {
    // append ancestor information to node
    toNode->parents.insert(ai);
    // refresh g-Value - if in heap, resorting becomes necessary
    if (refreshNode)
      toNode->gvalue = parent->gvalue+1;
    else
      toNode->gvalue = MIN(toNode->gvalue, parent->gvalue + 1);
  }
  else // debug case
    std::cerr << "ERROR: PARENT CAN'T BE APPENDED MORE THAN ONCE"
              << std::endl;
  return ai;
}

bool EAOStar::propagateSolved(EAONode * toNode,
                              EAONode * fromNode)
{
  toNode->solveState = SSSolved;
  
  toNode->maxGoalDist = MAX(0, toNode->maxGoalDist);
  
  if (toNode == fromNode) 
  // = initial state reached, do not proceed further
  {
    witness = extractPlan(fromNode);
    return true;
  }
  
  bool result = false;
  for (std::set<AncestorInfo>::iterator it = toNode->parents.begin();
           it != toNode->parents.end(); ++it)
    result |= propagateSolved(it->parent, fromNode,
                              toNode->maxGoalDist + 1,
                              it->split, it->action);
  return result;
}

bool EAOStar::propagateSolved(EAONode * toNode,
                              EAONode * fromNode,
                              const int & pathLength,
                              const int & fromState,
                              const int & fromAction)
{
  int maxGoalDist = -1;
  bool solvedbyagent = true;
  bool changedAndSolved = false;
  
  // first, update node information if necessary
  if (   !toNode->solvedChildren[fromState]  // substate not solved before
      || pathLength < toNode->solvedLength[fromState]     // shorter solve
      || (   toNode->parents.empty()                      // by pl. agent
          && problem.actions[fromAction]->agent == agent))
  {
    toNode->solvedChildren[fromState] = true;
    toNode->solvedActions[fromState] = fromAction;
    toNode->solvedLength[fromState] = pathLength;

    // check if every substate is solved, determinize max ss pathlength
    for (unsigned int s = 0; s < toNode->splitStates.size(); ++s)
    {
      if (!toNode->solvedChildren[s])
        return false;
      solvedbyagent &= (toNode->solvedActions[s] == -2) ||
        problem.actions[toNode->solvedActions[s]]->agent == agent;
      //if (maxPathLength < toNode->maxPathLength || toNode->maxPathLength == -1)
        
      maxGoalDist = MAX(maxGoalDist, toNode->solvedLength[s]);
    }

    solvedbyagent |= (agent == -1);
    toNode->maxGoalDist = maxGoalDist;
    changedAndSolved = true;
  }
  return changedAndSolved ? propagateSolved(toNode, fromNode) : false;
}

void propagateUnsolvable(EAONode * toNode)
{
  // debug assertion
  if (toNode->solveState == SSSolved)
    std::cerr << "propagateUS debug assertion" << std::endl;

  // mark node itself unsolvable
  toNode->solveState = SSUnsolvable;

  // further propagate unsolvable info
  std::set<EAONode*> parents;
  for (std::set<AncestorInfo>::iterator it = toNode->parents.begin();
         it != toNode->parents.end(); ++it)
    parents.insert(it->parent);
  for (std::set<EAONode*>::iterator it = parents.begin();
         it != parents.end(); ++it)
  {
    // if parent already marked unsolvable, further propagation not necessary
    if ((*it)->solveState == SSUnsolvable)
      continue;
    // else check if parent also has to be marked unsolvable
    bool propBack = false;
    for (unsigned int s = 0;
         !propBack && s < (*it)->splitStates.size(); ++s)
    {
      bool splitUnsolvable = !(*it)->solvedChildren[s];
      for (std::map<int, EAONode*>::iterator
             cit = (*it)->children[s].begin();
             splitUnsolvable && cit != (*it)->children[s].end(); ++cit)
        splitUnsolvable &= (cit->second->solveState == SSUnsolvable);
      propBack |= splitUnsolvable;
    }
    if (propBack)
        propagateUnsolvable(*it);
  }
}

EAONode * 
EAOStar::queueNewOrExistingNode(const EPState & state, EAONode * parent,
                                const int & pSplit, const int & pAction,
                                std::map<EPState,EAONode*> & createdGlobal,
                                std::map<EPState,EAONode*> & createdLocal,
                                MinHeap & heap)
{
  EAONode * node;
  std::map<EPState, EAONode*>::const_iterator
    found = createdGlobal.find(state);
  // if state hasn't been seen before - create and enqueue
  if (found == createdGlobal.end())
  {
    int h = this->h->h(state);
    node = new EAONode(state, h, parent, pSplit, pAction);
    registerWithHeap(node, heap);
    createdGlobal[state] = node;
    createdLocal[state] = node;
    worlds += state.getSize();
    wmax = MAX(wmax, state.getSize());
  }
  // else register parent with found node and maybe heap
  else
  {
    bool fromOldSearch = createdLocal.find(state) == createdLocal.end();
    // register node with parent
    node = found->second;
    appendParent(parent, found->second, pSplit, pAction, fromOldSearch);
    // if wasnt created this search: just add to heap
    if (fromOldSearch)
    {
      registerWithHeap(node, heap);
      createdLocal[state] = node;
    }
    // if was created and its still in heap, update heap position
    else if (found->second->mhh != NULL)
      heap.update(found->second->mhh[0]);
    // if it's not any more, (un)solved maybe has to be propagated in search
  }
  return node;
}

Plan EAOStar::extractPlan(EAONode* fromNode)
{
  if (this->isH)
  {
    Plan dummy;
    dummy.depth = fromNode->maxGoalDist;
    dummy.ok = true;
    return dummy;
  }
  else
    return extractPlan(fromNode, "");
}


Plan EAOStar::extractPlan(EAONode * fromNode, std::string indent)
{
  Plan result;
  std::ostringstream os;

  int nSplits = fromNode->splitStates.size();
  if (nSplits == 0)
    os << indent << "---\n";      

  std::set<EAONode *> exploredSub;
  for (int s = 0; s < nSplits; ++s)
  {
    int action = fromNode->solvedActions[s];
    if (action == -2) // split is already goal state
      os << indent << "." << std::endl;
    else
    {
      EAONode * subNode = fromNode->children[s][action];
      // get subplan
      Plan sub = extractPlan(subNode, indent + " ");
      result.mapping.insert(sub.mapping.begin(), sub.mapping.end());
      // add state-action mapping
      int agent = problem.actions[action]->agent;
      EPState aState = fromNode->splitStates[s].
                                 associatedLocal(agent);
      result.mapping[aState] = action;
      // generate plan string
      os << indent << gp.actions[action].name << "\n";
      os << sub.stringRep;
    }
  }
  // set depth / ok flag / string representation
  result.depth = fromNode->maxGoalDist;
  result.ok = true;
  result.stringRep = os.str();
  
  return result;
}
           
void EAOStar::printSearchInfo()
{
  std::cout << expanded << "\t" << created.size() << "\t"
            << ommited << "\t" << perfect << "\t" << seenbefore << "\t"
            << worlds << "\t" << wmax << "\t" << witness.depth
            << std::endl;
}


Plan EAOStar::search(const EPState * state)
{
  // reset witness
  witness = Plan();
  
  if (state == NULL)
  std::cout << "BEGIN EAO* SEARCH!" << std::endl;

  // calculate global & associated local state
  EPState initState = 
    state == NULL ? EPState(newEpistemicStateCopyC(problem.initState))
                  : *state;
  EPState agentState = initState.associatedLocal(agent == -1 ? 0 : agent);
  // and a singleton node containing the initial state
  EAONode * iNode = (state == NULL && ps == PSIndividual && agent != -1) ?
    new EAONode(agentState, h->h(agentState), NULL, -1, -1) :
    new EAONode(initState, h->h(initState), NULL, -1, -1);
    
  EAONode * initNode = iNode;
  
  // create MinHeap and local visited set (to avoid multiple node gen)
  MinHeap heap;
  std::map<EPState,EAONode*> createdLocal;  
    
  // first, check if state was considered in previous search
  std::map<EPState, EAONode*>::iterator
    found = created.find(initNode->state);
  if (found != created.end())
  {
    // return appropritate plan
    if (found->second->solveState == SSUnsolvable)
    {
      perfect += 1;
      witness = Plan();
      return witness;
    }
    else if (found->second->solveState == SSSolved)
    {
      perfect += 1;
      witness = extractPlan(found->second);
      return witness;
    }
    // or change init node pointer to existing node
    else
    {
      seenbefore += 1;
      initNode = found->second;
      initNode->gvalue = 0;
    }
  }
  else
  {
    worlds += initState.getSize();
    wmax = MAX(wmax, initState.getSize());
  }

  // add init node to heap and visited maps
  registerWithHeap(initNode, heap);
  createdLocal[initNode->state] = initNode;
  created[initNode->state] = initNode;

  while (!heap.empty())
  {
    // abortion option
    if (stop)
    {
      stop = false;
      return witness;
    }

    // take node from queue
    EAONode * current = takeFromHeap(heap);
    
    // return if no more feasible solutions
    if (current->hvalue == -1)
      return witness;
    // return if no more better solutions
    if (witness.ok && (current->gvalue + current->hvalue
                                       >= witness.depth))
      return witness;
    
    // case 1: previously expanded, yet unsolved (from old search)
    if (   !current->splitStates.empty()
        &&  current->solveState == SSSearch)
    {
      // re-queue all new child nodes (solved, unsolved & unsolvable)
      for (unsigned int s = 0; s < current->splitStates.size(); ++s)
        for (std::map<int,EAONode*>::iterator
               it  = current->children[s].begin();
               it != current->children[s].end(); ++it)
        {
          if (   createdLocal.find(it->second->state)
              == createdLocal.end())
          {
            // for first time heap inserts, g value has to be set
            it->second->gvalue = current->gvalue + 1;
            registerWithHeap(it->second, heap);
            createdLocal[it->second->state] = it->second;
          }
        }
    }
    
    // case 2: previously expanded, and solved (from old search)
    else if (   !current->splitStates.empty()
             && current->solveState == SSSolved)
      propagateSolved(current, initNode);
    
    // case 3: previously expanded, deteremined unsolvable
    else if (   !current->splitStates.empty()
             && current->solveState == SSUnsolvable)
      propagateUnsolvable(current);
       
    // case 4: nonexpanded goal state
    else if (current->state.eval(problem.goal))
      propagateSolved(current, initNode);

    // case 5: nonexpanded node
    else
    {
      // perform split
      expanded += 1;
      current->splitStates = current->state.split();
      int nSplits = current->splitStates.size();
      current->children = new std::map<int, EAONode*>[nSplits];
      current->solvedChildren = new bool[nSplits];
      current->solvedActions = new int[nSplits];
      current->solvedLength = new int[nSplits];
      // and initialize solved information
      for (int s = 0; s < nSplits; ++s)
      {
        current->solvedChildren[s] = false;
        current->solvedActions[s] = -1;
        current->solvedLength[s] = -1;
      }

      std::vector<EAONode*> prevUnsolvable;
      // for each split node
      for (int s = 0; s < nSplits; ++s)
      {
        // check first, if goal state is already reached
        if (current->splitStates[s].eval(problem.goal))
          propagateSolved(current, initNode, 0, s, -2);
        else
        {
          // otherwise create successor nodes for each applicable action
          for (unsigned int a = 0; a < problem.actions.size(); ++a)
          {
            bool applicable = false;
            bool applicableAgent = false;
            EPState childState = h->convertState(
              (ps == PSIndividual) ?
                current->splitStates[s].aProduct(*problem.actions[a], applicable)
              : current->splitStates[s].product(*problem.actions[a],
                                                applicable, applicableAgent));
            // if applicable (according to setting)
            if (ps == PSCollective ? applicableAgent : applicable)
            { 
              EAONode * newN = queueNewOrExistingNode(childState, current,
                                                      s, a, created,
                                                      createdLocal, heap);
              current->children[s][a] = newN;
              if (newN->solveState == SSSolved)
                propagateSolved(newN, initNode);
              if (newN->solveState == SSUnsolvable)
                prevUnsolvable.push_back(newN);
            }
          }
        }
        // if split state is not solved and has no solvable children
        if (   !current->solvedChildren[s]
            &&  current->children[s].size() == 0)
          propagateUnsolvable(current);
      }
      // propagate unsolvable from unsolvable child nodes
      for (std::vector<EAONode*>::iterator it = prevUnsolvable.begin();
             it != prevUnsolvable.end(); ++it)
         propagateUnsolvable(*it);
      
    }
  }
  // if there is no more node in the heap
  // finally output a witness plan, that could contain
  // - no solution (if the task is unsolvable for the agent)
  // - a solution with another agent having to take the first action
  return witness;
}

