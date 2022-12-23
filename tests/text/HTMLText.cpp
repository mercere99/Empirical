/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2022
 *
 *  @file Text.cpp
 */

#include <iostream>
#include <sstream>

#include "third-party/Catch/single_include/catch2/catch.hpp"

#include "emp/text/HTMLText.hpp"

TEST_CASE("Testing HTMLText", "[text]") {
  emp::HTMLText text("Test Text");
  REQUIRE(text.GetSize() == 9);
  REQUIRE(text.GetText() == "Test Text");
  REQUIRE(text.ToString() == "Test Text");

  // Try adding style.
  text.Bold(5,9);
  REQUIRE(text.GetSize() == 9);
  REQUIRE(text.GetText() == "Test Text");
  REQUIRE(text.ToString() == "Test <b>Text</b>");

  // Try appending.
  text << " and more Text.";
  REQUIRE(text.GetSize() == 24);
  REQUIRE(text.GetText() == "Test Text and more Text.");
  REQUIRE(text.ToString() == "Test <b>Text</b> and more Text.");

  // Try changing letters.
  text[21] = 's';
  text[23] = 's';
  REQUIRE(text.GetSize() == 24);
  REQUIRE(text.GetText() == "Test Text and more Tests");
  REQUIRE(text.ToString() == "Test <b>Text</b> and more Tests");

  // Try making a change that involves style.
  text[19] = text[5];
  REQUIRE(text.GetSize() == 24);
  REQUIRE(text.GetText() == "Test Text and more Tests");
  REQUIRE(text.ToString() == "Test <b>Text</b> and more <b>T</b>ests");

  // Try erasing the text.
  text.Resize(0);
  REQUIRE(text.GetSize() == 0);
  REQUIRE(text.GetText() == "");
  REQUIRE(text.ToString() == "");
  REQUIRE(text.GetStyles().size() == 0);

  text << "This is <i><b>Pre-</i>formatted</b> text.";
  // REQUIRE(text.GetSize() == 27);
  REQUIRE(text.GetText() == "This is Pre-formatted text.");
  REQUIRE(text.ToString() == "This is <b><i>Pre-</i>formatted</b> text.");
}