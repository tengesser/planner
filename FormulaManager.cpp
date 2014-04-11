// Author: T. Engesser
// (c) 2014

#include "FormulaManager.h"

#include <iostream>

// return the singleton instance
FormulaManager & FormulaManager::getInstance()
{
  static FormulaManager instance;
  return instance;
}

// T: type = -1
int FormulaManager::genT()
{
#if DUPLICATECHECK
  for (unsigned int i = 0; i < ftype.size(); ++i)
    if (ftype[i] == -1)
      return i;
#endif
  ftype.push_back(-1);
  farg1.push_back(-1);
  farg2.push_back(-1);
  return (ftype.size() - 1);
}

// F: type = -2
int FormulaManager::genF()
{
#if DUPLICATECHECK
  for (unsigned int i = 0; i < ftype.size(); ++i)
    if (ftype[i] == -2)
      return i;
#endif
  ftype.push_back(-2);
  farg1.push_back(-1);
  farg2.push_back(-1);
  return (ftype.size() - 1);
}

// proposition prop: type = 0, arg1 = prop
int FormulaManager::genProp(const int & prop)
{
#if DUPLICATECHECK
  for (unsigned int i = 0; i < ftype.size(); ++i)
    if (ftype[i] == 0 && farg1[i] == prop)
      return i;
#endif
  ftype.push_back(0);
  farg1.push_back(prop);
  farg2.push_back(-1);
  return (ftype.size() - 1);
}

// negation of phi: type = 1, arg1 = phi
int FormulaManager::genNeg(const int & phi)
{
  // double negations are senseless...
  if (ftype[phi] == 1)
    return farg1[phi];
#if DUPLICATECHECK
  for (unsigned int i = 0; i < ftype.size(); ++i)
    if (ftype[i] == 1 && farg1[i] == phi)
      return i;
#endif
  ftype.push_back(1);
  farg1.push_back(phi);
  farg2.push_back(-1);
  return (ftype.size() - 1);
}


// conjunction of formulae: type = 2, arg1 = no in conjunction list
int FormulaManager::genCon(const std::set<int> & formulae)
{
  if (formulae.size() == 0)
    return genT();
  if (formulae.size() == 1)
    return *formulae.begin();
#if DUPLICATECHECK
  int c = 0;
  for (unsigned int i = 0; i < ftype.size(); ++i)
    if (ftype[i] == 2 && cargs[c++] == formulae)
      return i;
#endif
  ftype.push_back(2);
  farg1.push_back(cargs.size());
  cargs.push_back(formulae);
  farg2.push_back(-1);
  return (ftype.size() - 1);
}

// knowledge of phi for agent a: type = 3, arg1 = a, arg2 = phi
int FormulaManager::genK(const int & a, const int & phi)
{
#if DUPLICATECHECK
  for (unsigned int i = 0; i < ftype.size(); ++i)
    if (ftype[i] == 3 && 
         (farg1[i] == a && farg2[i] == phi))
      return i;
#endif
  ftype.push_back(3);
  farg1.push_back(a);
  farg2.push_back(phi);
  return (ftype.size() - 1);
}

// common knowledge of phi: type = 4, arg1 = phi
int FormulaManager::genCK(const int & phi)
{
#if DUPLICATECHECK
  for (unsigned int i = 0; i < ftype.size(); ++i)
    if (ftype[i] == 4 && farg1[i] == phi)
      return i;
#endif
  ftype.push_back(4);
  farg1.push_back(phi);
  farg2.push_back(-1);
  return (ftype.size() - 1);
}

// disjunctions as negated conjunction of negations
int FormulaManager::genDis(const std::set<int> & formulae) {
  std::set<int> negFormulae;
  for (std::set<int>::const_iterator f = formulae.begin();
       f != formulae.end(); ++f)
    negFormulae.insert(genNeg(*f));
  return genNeg(genCon(negFormulae));
}

// phi -> psi = (not phi) or psi
int FormulaManager::genImp(const int & phi, const int & psi)
{
  std::set<int> formulae;
  formulae.insert(genNeg(phi));
  formulae.insert(psi);
  return genDis(formulae);
}

// phi <-> psi = (phi -> psi) and (psi -> phi)
int FormulaManager::genEqv(const int & phi, const int & psi)
{
std::set<int> formulae;
  formulae.insert(genImp(phi,psi));
  formulae.insert(genImp(psi,phi));
  return genCon(formulae);
}

// generate abstraction
int FormulaManager::genAbstraction(const int & phi,
                                   const std::set<int> & pDis)
{
  int result = phi;
  // std::cout << "GenAbs";
  for (std::set<int>::const_iterator it = pDis.begin();
         it != pDis.end(); ++it)
  {
    int phif = replace(phi, *it, false);
    int phit = replace(phi, *it, true);
    if (phif != phit) // TODO: compare w. true
    {
      //std::cout << ".";
      std::set<int> formulae;
      formulae.insert(phif);
      formulae.insert(phit);
      result = genDis(formulae);
    }
  }
  //std::cout << std::endl;
  return result;
}

// replace proposition p with t in phi
int FormulaManager::replace(const int & phi, const int & p,
                            const bool & t)
{
  switch (ftype[phi])
  {
    case -2:
    case -1:
      return phi;
      break;
    case 0: // proposition
    {
      if (farg1[phi] == p)
        return t ? genT() : genF();
      else
        return phi;
      break;
    }
    case 1: // negation
      return genNeg(replace(farg1[phi], p, t));
      break;
    case 2: // conjunction
    {
      std::set<int> formulae;
      for (std::set<int>::const_iterator it = cargs[farg1[phi]].begin();
             it != cargs[farg1[phi]].end(); ++it)
        formulae.insert(replace(*it, p, t));
      return genCon(formulae);
      break;
    }
    case 3: // knowledge
      return genK(farg1[phi], replace(farg2[phi], p, t));
      break;
    case 4: // common knowledge
      return genCK(replace(farg1[phi], p, t));
      break;    
    default:
      std::cerr << "Something's Wrong!" << std::endl;
      return -3;
      break;
  }
}

