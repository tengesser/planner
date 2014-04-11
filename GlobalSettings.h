// Author: T. Engesser
// (c) 2014

#ifndef GLOBALSETTINGS_H_
#define GLOBALSETTINGS_H_

// use state contractions
#define CONTRACT 1
// perform duplicatechecks to ensure (sub)formulae are generated just once
#define DUPLICATECHECK 1
// use bitsets istead of char arrays ("0" not supported anymore)
#define USEBITSETS 1
// check states for syntactic equivalence implying bisimilarity
#define WEAKBISIMILARITYCHECK 1 // requires use of bitsets
// memoize results of evaluated (sub)formulae for state-world pairs
#define MEMOIZATION 0
// use c++11 hash containers (set, map, multimap) (unused)
#define USEHASHCONTAINERS 0

// containers
#include <vector>
#include <map>
#include <set>

#if USEHASHCONTAINERS
#include <unordered_map>
#include <unordered_set>
#define MAP std::unordered_map
#define SET std::unordered_set
#define MMAP std::unordered_multimap
#else
#define MAP std::map
#define SET std::set
#define MMAP std::multimap
#endif

#endif // GLOBALSETTINGS_H_
