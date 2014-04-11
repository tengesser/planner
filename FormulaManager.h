// Author: T. Engesser
// (c) 2014

#ifndef FORMULAMANAGER_H_
#define FORMULAMANAGER_H_

#include "GlobalSettings.h"
#include "PrettyPrint.h"
#include "EpistemicModeling.h"

/// formula manager for representing epistemic formulae as integers
class FormulaManager {

public:
  /// returns the singleton formula manager instance
  static FormulaManager & getInstance();

  /// generate proposition p_prop$
  int genProp(const int & prop);
  /// generate negation of phi$
  int genNeg(const int & phi);
  /// generate conjunction of formulae
  int genCon(const std::set<int> & formulae);
  /// generate the modality of agent a knowing phi 
  int genK(const int & a, const int & phi);
  /// generate the modality of phi being common knowledge
  int genCK(const int & phi);
    
  /// generate disjunction of phi and psi (syntactic sugar)
  int genDis(const std::set<int> & formulae);
  /// generate implication from phi to psi (syntactic sugar)
  int genImp(const int & phi, const int & psi);
  /// generate equivalence of phi and psi (syntactic sugar)
  int genEqv(const int & phi, const int & psi);

  /// existential abstraction for phi using only propositions p w. pOk[p]
  int genAbstraction(const int & phi, const std::set<int> & pDis);

private:
  /// replaces a proposition in phi with T or F
  int replace(const int & phi, const int & p, const bool & t);
  /// generate T (for abstractions only)
   int genT();
  /// generate F (for abstractions only)
   int genF();

  /// formula type (proposition, negation, conjunction or (cm.) knowledge)
  std::vector<int> ftype;
  /// arg1: prop. number, agent, subformula or entry of conjunction list
  std::vector<int> farg1;
  /// arg2: subformula
  std::vector<int> farg2;
  /// conjunction list
  std::vector< std::set<int> > cargs;

  // default constructor is private
  FormulaManager() {};
  // default destructor is private
  ~FormulaManager() {};
  // copy constructor is private
  FormulaManager(const FormulaManager &) {};

friend bool evaluate(const EpistemicState & state, const int & world, 
                                                   const int & phi); 
friend class PrettyPrint;
};

// TODO: Method for consolidating sub-conjunctions occuring multiple times

#endif // FORMULAMANAGER_H_
