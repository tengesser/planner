// Author: T. Engesser
// (c) 2014

#include "EAStar.h"
#include <iostream>
#include <set>

Plan EAStar::extractPlan(const EANode * toNode,
                         const EANode * fromNode,
                         const int & goalDist)
{
  toNode->goalDist = goalDist;
  if (toNode->parent == NULL ||
      toNode->state == fromNode->state)
  {
    Plan emptyPlan;
    emptyPlan.ok = true;
    emptyPlan.stringRep = "";
    emptyPlan.depth = goalDist;
    return emptyPlan;
  }
  else
  {
    Plan result = extractPlan(toNode->parent, fromNode, goalDist + 1);
    result.mapping[toNode->state] = toNode->action;
    result.stringRep += gp.actions[toNode->action].name + "\n";
    return result;
  }
}


void EAStar::printSearchInfo()
{
 std::cout << exp << '\t' << crt << '\t' << omit
           << "\t-\t-\t!\t!\t" << witness.depth << std::endl;
  // std::cout << " no EA* information implemented" << std::endl;
}

Plan EAStar::search(const EPState * s)
{
  // reset witness
  witness = Plan();

  // determine real initial state for scenario
  EPState initState = 
    s == NULL ? EPState(newEpistemicStateCopyC(problem.initState))
              : *s;
  EPState agentState =
    initState.associatedLocal(agent == -1 ? 0 : agent);
  EANode initNode = (s == NULL && ps == PSIndividual && agent != -1) ?
      EANode(agentState)
    : EANode(initState);

  // initialize priority queue and created set for subproblem
  std::priority_queue<EANode*, std::vector<EANode*>, EAcompare> queue;
  std::set<EPState> created2;

  // if state is already existant
  std::map<EPState, EANode*>::iterator
    found = createdStates.find(initNode.state);
  if (found != createdStates.end())
  {
    // if solved, just return plan
    if (found->second->goalDist >= 0)
    {
      witness = extractPlan(found->second, &initNode, found->second->goalDist);
      return witness;
    }
    else // begin with its node
    {
      found->second->gvalue = 0;
      queue.push(found->second);
      created2.insert(found->second->state);
    }
  }
  else
  {
    queue.push(&initNode);
    createdStates[initState] = &initNode;
    created2.insert(initState);
  }

  // search loop
  while (!stop && !queue.empty())
  {
    EANode * current = queue.top();
    queue.pop();

    //if (witness.ok && (current->gvalue + current->hvalue >= witness.depth))
          // && (agent == -1 || s == NULL || 
          //     problem.actions[witness.mapping[initNode.state]]->agent 
          //       == agent)
    if (witness.ok) // HACK!
      return witness;

    // check if goal state
    if (current->state.eval(problem.goal))
      witness = extractPlan(current, &initNode);

    // check if expanded, but yet unsolved
    else if (current->children.size() > 0 && current->goalDist == -1)
      // re-queue its children
      for (std::vector<EANode*>::iterator
             it = current->children.begin();
             it != current->children.end(); ++it)
      {
        queue.push(*it);
        created2.insert((*it)->state);
        (*it)->gvalue = current->gvalue + 1;
      }

    // or expanded and solved
    else if (current->children.size() > 0 && current->goalDist >= 0)
      witness = extractPlan(current, &initNode, current->goalDist);

    // otherwise, expand node
    else
    {
      exp++;
      for (unsigned int a = 0; a < problem.actions.size(); ++a)
      {
        bool applicable, applicableAgent;
        EPState childState = h->convertState(
          (ps == PSIndividual) ?
            current->state.aProduct(*problem.actions[a], applicable)
          : current->state.product(*problem.actions[a], applicable,
                                    applicableAgent));

        if (ps == PSCollective ? applicableAgent : applicable) {
          if (created2.find(childState) == created2.end())
          {
            crt++;
            int h = this->h->h(childState);
            EANode * childNode = new EANode(childState, h,
                                            current, a);
            queue.push(childNode);
            current->children.push_back(childNode);
            created2.insert(childState);

            if (createdStates.find(childState) == createdStates.end())
              createdStates[childState] = childNode;
          }
          else
            omit++;
        }
      }
    }
  }
  return witness;
}
