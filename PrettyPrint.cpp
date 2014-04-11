// Author: T. Engesser
// (c) 2014

#include "PrettyPrint.h"
#include <sstream>

std::string PrettyPrint::toString(const EPState & state,
                              const GroundedProblem * p,
                              const bool & problemInfo,
                              const bool & fullRelations,
                              const bool & fullValuation)
{
    return toString(*state.state, p, problemInfo, fullRelations, fullValuation);
}

// formulae -> string
std::string PrettyPrint::toString(const int & phi)
{
  const FormulaManager & fm = FormulaManager::getInstance();
  std::ostringstream os;

  switch (fm.ftype[phi])
  {
    case -2: // F
      os << "F";
      break;
    case -1: // T
      os << "T";
      break;
    case 0: // proposition
      os << "p" << fm.farg1[phi];
      break;
    case 1: // negation
      os << "n" << toString(fm.farg1[phi]);
      break;
    case 2: // conjunction
    {
      os << "(";
      const std::set<int> & subf = fm.cargs[fm.farg1[phi]];
      for (std::set<int>::const_iterator it = subf.begin();
           it != subf.end(); ++it)
        os << toString(*it) << "&";
      os << (subf.empty() ? ")" : "\b)");
      break;
    }
    case 3: // knowledge
      os << "k" << fm.farg1[phi] << ":" << toString(fm.farg2[phi]);
      break;
    case 4: // kommon knowledge
      os << "kc:" << toString(fm.farg1[phi]);
      break;
    default:
      os << "???";
      break;
  }
  return os.str();
}

// epistemic states -> string
std::string PrettyPrint::toString(const EpistemicState & state,
                                  const GroundedProblem * prob,
                                  const bool & problemInfo,
                                  const bool & fullRelations,
                                  const bool & fullValuation)
{
  std::ostringstream os;

  // number of agents/propositions/worlds
  os << "Number of Agents: " << state.nAgents << std::endl;
  os << "Number of Propositions: " << state.nPropositions << std::endl;
  os << "Number of Worlds: " << state.nWorlds << std::endl;

  // relations
  os << "Equivalence Relations:" << std::endl;
  for (int a = 0; a < state.nAgents; ++a) {
    os << "  for Agent " << a;
    if (problemInfo) os << "=" << prob->agents[a];
    os << (fullRelations ? ": {" : ": ");
    std::ostringstream os2;
    int c = 0;
    while (true) {
      os2 << "{";
      bool found = false;
      for (int w = 0; w < state.nWorlds; ++w)
        if (state.eqc[a][w] == c)
        {
          os2 << w << ", ";
          found = true;
        }
      if (found)
      {
        os2 << "\b\b}, ";
        ++c;
      }
      else
        break;
    }
    os2 << "\b\b\b}";
    if (fullRelations)
      os << os2.str() << std::endl;
    else
      os << c << " classes" << std::endl;
  }

  // valuation
  os << "Valuation funtcion: " << std::endl;
  int nTrue = 0;
  int nFalse = 0;
  for (int w = 0; w < state.nWorlds; ++w)
  {
    if (fullValuation)
      os << "World " << w << ": ";
    for (int p = 0; p < state.nPropositions; ++p)
    {
      std::string var = problemInfo ? prob->variables[p] : "";
      switch (state.val[w][p])
      {
        case 0:
          ++nFalse;
          break;
        case 1:
          ++nTrue;
          if (fullValuation)
          {
            if (problemInfo) os << var << " ";
              else  os << "p" << p << " ";
          }
          break;
#if !USEBITSETS
        case 2:
          if (fullValuation)
          {
            if (problemInfo) os << var << " ";
              else  os << "p" << p << " ";
          }
          break;
#endif
      }
    }
    if (fullValuation)
      os << std::endl;
  }
#if LAZYVALUATION
  os << nTrue << " true, " << nFalse << " false, "
     << state.nWorlds * state.nPropositions - nTrue - nFalse
     << " unevaluated" << std::endl;
#endif

  // designated states
  os << "Designated States: {";
  for (int w = 0; w < state.nWorlds; ++w)
  {
    if ((*state.designated)[w])
      os << w << ", ";
  }
  os << "\b\b}" << std::endl;
  return os.str();
}

// epistemic action -> string
std::string PrettyPrint::toString(const EpistemicAction & action)
{
  std::ostringstream os;
  
  // number of agents/events
  os << "Number of Agents: " << action.nAgents << std::endl;
  os << "Number of Events: " << action.nEvents << std::endl;
  os << "Executing Agent: " << action.agent << std::endl;
  // relations
  os << "Equivalence Classes: ";
  for (int a = 0; a < action.nAgents; ++a) {
    os << std::endl << "  for Agent " << a << " {";
    int c = 0;
    while (true) {
      os << "{";
      bool found = false;
      for (int e = 0; e < action.nEvents; ++e)
        if (action.eqc[a][e] == c)
        {
          os << e << ", ";
          found = true;
        }
      if (found)
      {
        os << "\b\b}, ";
        ++c;
      }
      else
        break;
    }
    os << "\b\b}";
  }

  // events: preconditions, add/delete effects
  os << std::endl << "Events:" << std::endl;
  for (int e = 0; e < action.nEvents; ++e)
  {
    os << "  Event " << e << ": " << std::endl;
    os << "    prec : " << action.prec[e] << " = " 
       << toString(action.prec[e]) << std::endl;
#if USEBITSETS
    os << "add/del_effects: to implement" << std::endl; // TODO
#else
    os << "    add_effects: {";
    for (std::vector<int>::const_iterator
          ae  = action.add_effects[e].begin();
          ae != action.add_effects[e].end(); ++ae)
      os << "p" << *ae << ", ";
    os << "\b\b}" << std::endl;
    os << "    del_effects: {";
    for (std::vector<int>::const_iterator
          de  = action.del_effects[e].begin();
          de != action.del_effects[e].end(); ++de)
      os << "p" << *de << ", ";
    os << "\b\b}" << std::endl;
#endif
  }

  // designated events
  os << "Designated Events: {";
  for (int e = 0; e < action.nEvents; ++e)
  {
    if ((*action.designated)[e])
      os << e << ", ";
  }
  os << "\b\b}" << std::endl;
  return os.str();
}
