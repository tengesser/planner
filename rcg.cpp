// Author: T. Engesser
// (c) 2014

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sstream>
#include <set>

#include "EpistemicModeling.h"
#include "PrettyPrint.h"
#include "maepl/maepl.h"
#include "search/SearchAlgorithm.h"
#include "search/EAStar.h"
#include "search/EAOStar.h"
#include "heuristics/Heuristic.h"
#include "heuristics/Abstraction.h"
#include "heuristics/Relaxation.h"
#include "heuristics/HStar.h"

enum HeuristicMode { HMAbs, HMRel, HMHStar, HMNone };
typedef int formula;

struct comb {
  int i;
  int a;
  int b;
  int c;
  comb(const int & i, const int & a, const int & b, const int & c)
  : i(i), a(a), b(b), c(c) {
    if (a > b || b > c)
      std::cerr << "Invalid Combination!" << std::endl;
  }
};

struct deal {
  comb c1;
  comb c2;
  int x;
  deal(const comb & c1, const comb & c2, const int & x, bool & ok)
  : c1(c1), c2(c2), x(x) {
    ok = not(   c1.a == c2.a || c1.a == c2.b || c1.a == c2.c
             || c1.b == c2.a || c1.b == c2.b || c1.b == c2.c
             || c1.c == c2.a || c1.c == c2.b || c1.c == c2.c
             || c1.a == x || c1.b == x || c1.c == x
             || c2.a == x || c2.b == x || c2.c == x);
  }
};

std::string toString(const int & i)
{
  std::ostringstream oss;
  oss << i;
  return oss.str();
}

std::string toString(const comb & c) {
  std::ostringstream oss;
  oss << c.a << c.b << c.c;
  return oss.str();
}

std::string toString(const deal & d) {
  std::ostringstream oss;
  oss << toString(d.c1) << "-" << toString(d.c2) << "-" << d.x;
  return oss.str();
}

int main(int argc, char* argv[])
{
  int nCombs = 0;
  std::vector<comb> combs;
  for (int a=0; a<5; ++a)
    for (int b=a+1; b<6; ++b)
      for (int c=b+1; c<7; ++c)
        combs.push_back(comb(nCombs++,a,b,c));
  if (nCombs != (int)combs.size()) std::cerr << "Somethings Wrong!" << std::endl;
  std::cout << "Number of valid combinations: " << nCombs << std::endl;
  for (int i=0; i<nCombs; ++i) std::cout << i << ":" << toString(combs[i]) << ", ";
  std::cout << std::endl;

  std::vector<deal> deals;
  for (int i=0; i<nCombs; ++i)
    for (int j=0; j<nCombs; ++j)
      for (int x=0; x<7; ++x) {
        bool ok;
        deal d(combs[i],combs[j],x,ok);
        if (ok)
          deals.push_back(d);
  }
  int nDeals = deals.size();
  std::cout << "Number of valid deals: " << nDeals << std::endl;
  for (int i=0; i<nDeals; ++i) std::cout << toString(deals[i]) << ", "; std::cout << std::endl;

  int nAgents = 3;
  int nPropositions = 2*nCombs+1+3*7;
  int nWorlds = nDeals;
  int ** eqc = new int*[nAgents];
  for (int a=0; a<nAgents; ++a)
    eqc[a] = new int[nWorlds];
  for (int w=0; w<nWorlds; ++w) {
    eqc[0][w] = deals[w].c1.i;
    eqc[1][w] = deals[w].c2.i;
    eqc[2][w] = 0;
  }
  boost::dynamic_bitset<> * val = new boost::dynamic_bitset<>[nWorlds];
  for (int w=0; w<nWorlds; ++w) {
    val[w] = boost::dynamic_bitset<>(nPropositions);
    for (int p=0; p<nPropositions; ++p)
      val[w][p] = false;
    val[w][deals[w].c1.i]        = true; // comb of agent 1
    val[w][nCombs+deals[w].c2.i] = true; // comb of agent 2
    val[w][2*nCombs]             = true; // first move
    val[w][2*nCombs+1+deals[w].c1.a] = true;
    val[w][2*nCombs+1+deals[w].c1.b] = true;
    val[w][2*nCombs+1+deals[w].c1.c] = true;
    val[w][2*nCombs+1+7+deals[w].c2.a] = true;
    val[w][2*nCombs+1+7+deals[w].c2.b] = true;
    val[w][2*nCombs+1+7+deals[w].c2.c] = true;
    val[w][2*nCombs+1+14+deals[w].x] = true;
  }
  std::vector<bool> * designated = new std::vector<bool>(nWorlds);
  for (int w=0; w<nWorlds; ++w)
    designated->push_back(false);
  (*designated)[0] = true;

  EpistemicState initState(nAgents, nPropositions, nWorlds, const_cast<const int **>(eqc), val,
                           NULL, NULL, NULL, NULL, designated);

  FormulaManager * fm = &FormulaManager::getInstance();
  std::vector<formula> agent1HasComb,
                       agent1HasCard, agent2HasCard, agent3HasCard,
                       agent1knows2sCards, agent2knows1sCards, agent3knowsForeignCards;
  std::set<formula> agent1knows2sCardsSet, agent2knows1sCardsSet, agent3knowsForeignCardsSet;
  formula firstmove = fm->genProp(2*nCombs);
  // formulae agent 1 has combination X (for 5-action preconditions)
  for (int c=0; c<nCombs; ++c) {
    agent1HasComb.push_back(fm->genProp(c));
  }
  // formulae agent 1/2/3 has card X & agent 1/2 knows all of the other cards & agent 3 knows some of ot.
  for (int x=0; x<7; ++x) {
    agent1HasCard.push_back(fm->genProp(2*nCombs+1+ 0+x));
    agent2HasCard.push_back(fm->genProp(2*nCombs+1+ 7+x));
    agent3HasCard.push_back(fm->genProp(2*nCombs+1+14+x));
    agent1knows2sCardsSet.insert(fm->genImp(agent2HasCard[x], fm->genK(0,agent2HasCard[x])));
    agent2knows1sCardsSet.insert(fm->genImp(agent1HasCard[x], fm->genK(1,agent1HasCard[x])));
    agent3knowsForeignCardsSet.insert(fm->genK(2, agent1HasCard[x]));
    agent3knowsForeignCardsSet.insert(fm->genK(2, agent2HasCard[x]));
  }
  formula agent1knowing = fm->genCon(agent1knows2sCardsSet);
  formula agent2knowing = fm->genCon(agent2knows1sCardsSet);
  formula agent3ignorant = fm->genNeg(fm->genDis(agent3knowsForeignCardsSet));

  std::set<formula> goalL;
  goalL.insert(agent1knowing);
  goalL.insert(agent2knowing);
  goalL.insert(agent3ignorant);
  formula goal = fm->genCon(goalL);



  std::vector<GroundedAction> gactions;
  std::vector<EpistemicAction*> actions;
  // 5-actions (sol: 0:012 9:034 14:056 20:135 29:246)
  // ass 1 + ass 2: 31628
  std::cout << std::endl << "generating 5-actions!" << std::endl;
  for (int c1=   0; c1<1; ++c1) // assertion 1: own combination included
   for (int c2=c1+1; c2<32; ++c2) {
    std::cout << "announce5-" << c1 << "-" << c2 << "-X-X-X, " << std::flush;
    for (int c3=c2+1; c3<33; ++c3)
     for (int c4=c3+1; c4<34; ++c4)
      for (int c5=c4+1; c5<35; ++c5) {
        // assertion 2: all cards included
        std::set<int> cd;
        cd.insert(combs[c1].a); cd.insert(combs[c1].b); cd.insert(combs[c1].c); 
        cd.insert(combs[c2].a); cd.insert(combs[c2].b); cd.insert(combs[c2].c); 
        cd.insert(combs[c3].a); cd.insert(combs[c3].b); cd.insert(combs[c3].c); 
        cd.insert(combs[c4].a); cd.insert(combs[c4].b); cd.insert(combs[c4].c); 
        cd.insert(combs[c5].a); cd.insert(combs[c5].b); cd.insert(combs[c5].c);
        if (cd.size() != 7) continue;
        int nEvents = 1;
        int agent = 0;
        int ** eqc = new int*[3];
        for (int a=0; a<nAgents; ++a) {
          eqc[a] = new int[nEvents];
          eqc[a][0] = 0;
        }
        int * prec = new int[1];
        std::set<formula> preclistc, preclist;
        preclistc.insert(agent1HasComb[c1]);
        preclistc.insert(agent1HasComb[c2]);
        preclistc.insert(agent1HasComb[c3]);
        preclistc.insert(agent1HasComb[c4]);
        preclistc.insert(agent1HasComb[c5]);
        preclist.insert(firstmove);
        preclist.insert(fm->genDis(preclistc));
        prec[0] = fm->genCon(preclist);
        boost::dynamic_bitset<> * add_effects, * del_effects;
        std::vector<bool> * edesig = new std::vector<bool>;
        add_effects = new boost::dynamic_bitset<>(nPropositions);
        del_effects = new boost::dynamic_bitset<>(nPropositions);
        (*del_effects)[2*nCombs] = 1;
        // std::cout << add_effects[0] << std::endl;
        edesig->push_back(true);
        GroundedAction gaction;
        gaction.name = "a1: i have "+toString(combs[c1])+" or "+toString(combs[c2])+" or "
                                    +toString(combs[c3])+" or "+toString(combs[c4])+" or "
                                    +toString(combs[c5])+"!";
        gaction.agent = "agent1";
        gactions.push_back(gaction);
        actions.push_back(
          new EpistemicAction(nAgents, nEvents, agent, const_cast<const int**>(eqc),
                              prec, add_effects, del_effects, edesig));
    }
  }
  std::cout << actions.size() << " 5-actions!" << std::endl;
  // 1-actions (sol: 345 125 024)
  std::cout << "generating 1-actions!" << std::endl;
  for (int x = 0; x<7; ++x) {
        int nEvents = 1;
        int agent = 1;
        int ** eqc = new int*[3];
        for (int a=0; a<nAgents; ++a) {
          eqc[a] = new int[nEvents];
          eqc[a][0] = 0;
        }
        int * prec = new int[1];
        std::set<formula> preclist;
        preclist.insert(fm->genNeg(firstmove));
        //preclist.insert(agent3ignorant);
        preclist.insert(agent3HasCard[x]);
        prec[0] = fm->genCon(preclist);
        boost::dynamic_bitset<> * add_effects, * del_effects;
        std::vector<bool> * edesig = new std::vector<bool>;
        add_effects = new boost::dynamic_bitset<>(nPropositions);
        del_effects = new boost::dynamic_bitset<>(nPropositions);
        // std::cout << add_effects[0] << std::endl;
        edesig->push_back(true);
        GroundedAction gaction;
        gaction.name = "a2: 3 has "+ toString(x) +"!";
        gaction.agent = "agent2";
        gactions.push_back(gaction);
        actions.push_back(
          new EpistemicAction(nAgents, nEvents, agent, const_cast<const int**>(eqc),
                              prec, add_effects, del_effects, edesig));
  }
  std::cout << " -> finished. Overall Number of actions: " << actions.size() << std::endl;
  if (actions.size() != gactions.size()) std::cerr << "Somethings Wrong!" << std::endl;

  PlanningProblem problem ("rcg-problem", &initState, actions, goal);
  GroundedProblem gp;
  gp.name = "rcg-problem";
  gp.agents.push_back("agent1");
  gp.agents.push_back("agent2");
  gp.agents.push_back("agent3");
  for (int d=0; d<nDeals; ++d)
    gp.worlds.push_back("w"+toString(deals[d]));
  for (int c=0; c<nCombs; ++c)
    gp.variables.push_back("1-has-comb-"+toString(combs[c]));
  for (int c=0; c<nCombs; ++c)
    gp.variables.push_back("2-has-comb-"+toString(combs[c]));
  gp.variables.push_back("is-first-move");
  for (int x=0; x<7; ++x)
    gp.variables.push_back("1-has-card-"+toString(x));
  for (int x=0; x<7; ++x)
    gp.variables.push_back("2-has-card-"+toString(x));
  for (int x=0; x<7; ++x)
    gp.variables.push_back("3-has-card-"+toString(x));
  gp.actions = gactions;

  std::cout << PrettyPrint::toString(goal) << std::endl;
  std::cout << PrettyPrint::toString(initState, &gp) << std::endl;
  std::cout << PrettyPrint::toString(*actions[0]) << std::endl;
  std::cout << PrettyPrint::toString(*actions[1]) << std::endl;


  Heuristic * h = new Heuristic(problem,gp,PSIndividual);
  SearchAlgorithm * planner = new EAOStar(problem, gp, PSIndividual, h);
  std::cout << planner->search().stringRep << std::endl;
  std::cout << "\t#NExp\t#NCrt\t#NOmit\t#HHitS\t#HHitUS"
            << "\t#Wtot\t#Wmax\tPlanDepth" << std::endl;
  std::cout << "Search:\t"; planner->printSearchInfo();
 
  return 0;
}
