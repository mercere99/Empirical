//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2020.
//  Released under the MIT Software license; see doc/LICENSE

#include <string>

#define CATCH_CONFIG_MAIN

#include "third-party/Catch/single_include/catch2/catch.hpp"

#include "emp/matching/MatchDepository.hpp"
#include "emp/matching/matchbin_regulators.hpp"
#include "emp/matching/matchbin_metrics.hpp"
#include "emp/matching/regulators/PlusCountdownRegulator.hpp"
#include "emp/matching/selectors_static/RankedSelector.hpp"

TEST_CASE("MatchDepository Put, Get, GetSize, Clear", "[tools]") {

  emp::MatchDepository<
    std::string,
    emp::AbsDiffMetric,
    emp::statics::RankedSelector<>,
    emp::AdditiveCountdownRegulator<>
  > depo;

  REQUIRE( depo.GetSize() == 0 );

  depo.Put( "zero", 0 );
  REQUIRE( depo.GetSize() == 1 );
  REQUIRE( depo.GetVal(0) == "zero" );

  depo.Put( "two", 2 );
  REQUIRE( depo.GetSize() == 2 );
  REQUIRE( depo.GetVal(0) == "zero" );
  REQUIRE( depo.GetVal(1) == "two" );

  depo.Clear();

  REQUIRE( depo.GetSize() == 0 );

  depo.Put( "hundred", 100 );
  REQUIRE( depo.GetSize() == 1 );
  REQUIRE( depo.GetVal(0) == "hundred" );

}

TEST_CASE("MatchDepository MatchRaw", "[tools]") {

  emp::MatchDepository<
    std::string,
    emp::AbsDiffMetric,
    emp::statics::RankedSelector<>,
    emp::AdditiveCountdownRegulator<>,
    true,
    true
  > depo;

  REQUIRE( depo.GetSize() == 0 );

  depo.Put( "zero", 0 );

  depo.Put( "two", 2 );

  depo.Put( "hundred", 100 );

  {
  const auto res = depo.MatchRaw( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRaw( 90 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "hundred" );
  }

  // do some regulation
  depo.SetRegulator(0, 100.0);
  depo.SetRegulator(1, -100.0);
  depo.SetRegulator(2, 400.0);

  {
  const auto res = depo.MatchRaw( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRaw( 90 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "hundred" );
  }

  // do it again to check caching
  {
  const auto res = depo.MatchRaw( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRaw( 90 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "hundred" );
  }

}

TEST_CASE("MatchDepository MatchRegulated", "[tools]") {

  emp::MatchDepository<
    std::string,
    emp::AbsDiffMetric,
    emp::statics::RankedSelector<>,
    emp::PlusCountdownRegulator<
      std::deci, // Slope
      std::ratio<0>, // MaxUpreg
      std::deci, // ClampLeeway
      2 // countdown
    >,
    true,
    true
  > depo;

  REQUIRE( depo.GetSize() == 0 );

  depo.Put( "zero", 0 );

  depo.Put( "two", 2 );

  depo.Put( "hundred", 100 );

  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRegulated( 90 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "hundred" );
  }

  // do some regulation (+ values downregulate )
  depo.SetRegulator(2, 400.0);

  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRegulated( 90 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "two" );
  }

  // do it again to check caching
  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRegulated( 90 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "two" );
  }

  // upregulate back to baseline
  depo.SetRegulator(2, 0.0);

  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRegulated( 90 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "hundred" );
  }


  // do it again to check caching
  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRegulated( 90 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "hundred" );
  }


}

TEST_CASE("MatchDepository MatchRegulated pathological case", "[tools]") {

  emp::MatchDepository<
    std::string,
    emp::AbsDiffMetric,
    emp::statics::RankedSelector<>,
    emp::PlusCountdownRegulator<
      std::deci, // Slope
      std::ratio<0>, // MaxUpreg
      std::deci, // ClampLeeway
      2 // countdown
    >,
    true,
    true
  > depo;

  REQUIRE( depo.GetSize() == 0 );

  depo.Put( "zero", 0 );

  depo.Put( "one", 1 );

  depo.Put( "two", 2 );

  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  {
  const auto res = depo.MatchRegulated( 1 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "one" );
  }

  {
  const auto res = depo.MatchRegulated( 2 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "two" );
  }

  // downregulate one
  depo.SetRegulator(1, 400000000.0);

  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "zero" );
  }

  // downregulate zero
  depo.SetRegulator(0, 400000000.0);

  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "two" );
  }

  // upregulate one
  depo.SetRegulator(1, 0.0);

  {
  const auto res = depo.MatchRegulated( 0 );
  REQUIRE( res.size() == 1 );
  REQUIRE( depo.GetVal( res.front() ) == "one" );
  }


}
