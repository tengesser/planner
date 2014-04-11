// Author: T. Engesser
// (c) 2014

#ifndef PRETTYPRINT_H_
#define PRETTYPRINT_H_

#include <string>
#include "FormulaManager.h"
#include "EpistemicModeling.h"

class PrettyPrint
{
public:
  static std::string toString(const int & formula);
  static std::string toString(const EpistemicState & state,
                              const GroundedProblem * p,
                              const bool & problemInfo = true,
                              const bool & fullRelations = true,
                              const bool & fullValuation = true);
  static std::string toString(const EPState & state,
                              const GroundedProblem * p,
                              const bool & problemInfo = true,
                              const bool & fullRelations = true,
                              const bool & fullValuation = true);
  static std::string toString(const EpistemicAction & action);
};

#endif
