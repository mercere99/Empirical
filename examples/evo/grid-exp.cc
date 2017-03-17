//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2016.
//  Released under the MIT Software license; see doc/LICENSE
// 
// Author: Steven Jorgensen
//
//  This file explores the template defined in evo::Population.h

#include <iostream>
#include <string>
#include <cmath>

#include "../../config/ArgManager.h"
#include "../../evo/NK.h"
#include "../../evo/World.h"
#include "../../tools/BitSet.h"
#include "../../tools/Random.h"
#include "../../evo/EvoStats.h"
#include "../../evo/StatsManager.h"
#include "../../evo/LineageTracker.h"

// Default Config Options for the Population
EMP_BUILD_CONFIG( NKConfig,
  GROUP(DEFAULT, "Default settings for NK model"),
  VALUE(K, int, 10, "Level of epistasis in the NK model"),
  VALUE(N, int, 50, "Number of bits in each organisms (must be > K)"), ALIAS(GENOME_SIZE),
  VALUE(SEED, int, 0, "Random number seed (0 for based on time)"),
  VALUE(POP_SIZE, int, 100, "Number of organisms in the popoulation."),
  VALUE(MAX_GENS, int, 2000, "How many generations should we process?"),
  VALUE(MUT_COUNT, double, 0.005, "How many bit positions should be randomized?"), ALIAS(NUM_MUTS),
  VALUE(TOUR_SIZE, int, 20, "How many organisms should be picked in each Tournament?"),
  VALUE(NAME, std::string, "Result-", "Name of file printed to"),
)

using BitOrg = emp::BitVector;


int main(int argc, char* argv[])
{
  NKConfig config;
  config.Read("Grid.cfg");

  auto args = emp::cl::ArgManager(argc, argv);
  if (args.ProcessConfigOptions(config, std::cout, "Grid.cfg", "NK-macros.h") == false) exit(0);
  if (args.TestUnknown() == false) exit(0);  // If there are leftover args, throw an error.

  config.Write("SetGrid.cfg");

  // K controls # of hills in the fitness landscape
  // N controls the length of the genomes
  const int K = config.K();
  const int N = config.N();
  const double MUTATION_RATE = config.MUT_COUNT();
  const int TOURNAMENT_SIZE = config.TOUR_SIZE();
  const int POP_SIZE = config.POP_SIZE();
  const int UD_COUNT = config.MAX_GENS();
  std::string prefix;
  prefix = config.NAME();
  emp::Random random(config.SEED());
  emp::evo::NKLandscape landscape(N, K, random);

}
