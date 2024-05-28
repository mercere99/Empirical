/*
 *  This file is part of Empirical, https://github.com/devosoft/Empirical
 *  Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  date: 2024.
*/
/**
 *  @file
 *  @brief  Tools for all puzzle-analysis techniques.
*/

#ifndef EMP_GAMES_PUZZLE_HPP
#define EMP_GAMES_PUZZLE_HPP

#include <iostream>

#include "../tools/String.hpp"

namespace emp {

  struct PuzzleMove {
    enum MoveType { BLOCK_STATE, SET_STATE };

    MoveType type;
    size_t pos_id;
    uint8_t state;

    emp::String TypeString() const {
      if (type == BLOCK_STATE) return "BLOCKING";
      else return "SETTING";
    }

    emp::String ToString() const {
      return MakeString(TypeString(), " state ", state+1, " at position ", pos_id);
    }

    std::ostream & Print(std::ostream & os=std::cout) const {
      return os << ToString();
    }
  };

  struct PuzzleSolveFun {
    using move_set_t = emp::vector<PuzzleMove>;
    using solve_fun_t = move_set_t();

    std::function<solve_fun_t> solve_fun;
    String move_name;
    double difficulty;
  };

  struct PuzzleProfile {
    struct Slice {
      size_t level;  // How difficult is the easiest move at this point?
      size_t count;  // How many options are there for this move?

      String ToString() const { return MakeString(level, ":", count); }
    };

    enum FinalState { SOLVED, UNSOLVED, UNSOLVABLE };

    std::vector<Slice> slices; // Actual solving profile.
    FinalState final_state = UNSOLVED;

    bool IsSolved() const { return final_state == SOLVED; }
    bool IsUnsolved() const { return final_state == UNSOLVED; }
    bool IsUnsolvable() const { return final_state == UNSOLVABLE; }
    
    void AddMoves(size_t level, size_t count) {
      slices.push_back(Slice{level, count});
    }

    void SetSolved()     { final_state = SOLVED; }
    void SetUnsolved()   { final_state = UNSOLVED; }
    void SetUnsolvable() { final_state = UNSOLVABLE; }
    
    void Clear() { slices.resize(0); }

    String ToString() const {
      String out;
      for (auto & slice : slices) { out.Append(slice.ToString(), " "); }
      return out;
    }

    void Print(std::ostream & out=std::cout) const {
      out << ToString() << std::endl;
    }
  };
  

  /// @brief A generic analyzer for puzzles that have a set of positions, each in a set of values.
  template <size_t NUM_ROWS, size_t NUM_COLS, size_t NUM_STATES>
  struct GridPuzzleAnalyzer {
    static constexpr size_t NUM_CELLS = NUM_ROWS * NUM_COLS;
    static constexpr uint8_t UNKNOWN_STATE = NUM_STATES; // Lower values are actual states.

    // Which symbols are we using in this puzzle? (default to standard)
    using symbol_set_t = emp::array<char, NUM_STATES>;
    symbol_set_t symbols = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

    emp::array<uint8_t,NUM_CELLS> value;      // Known value for cells

    // Track the available options by state, across the whole puzzle.
    // E.g., symbol 5 is at bit_options[5] and has 81 bits indicating if each cell can be '5'.
    using grid_bits_t = BitSet<NUM_CELLS>;
    emp::array<grid_bits_t, NUM_STATES> bit_options;

    grid_bits_t is_set;


    // ===== Helper Functions =====

    uint8_t GetValue(size_t cell) const { return value[cell]; }

    size_t SymbolToState(char symbol) const {
      for (size_t i=0; i < NUM_STATES; ++i) {
        if (symbols[i] == symbol) return i;
      }
      return UNKNOWN_STATE;
    }

    /// Test if a cell is allowed to be a particular state.
    bool HasOption(size_t cell, uint8_t state) {
      emp_assert(cell < 81, cell);
      emp_assert(state >= 0 && state < NUM_STATES, state);
      return bit_options[state].Has(cell);
    }

    /// Return a currently valid option for provided cell; may not be correct solution
    uint8_t FindOption(size_t cell) {
      for (uint8_t state = 0; state < NUM_STATES; ++state) {
        if (HasOption(cell, state)) return state;
      }
      return UNKNOWN_STATE;
    }
    bool IsSet(size_t cell) const { return value[cell] != UNKNOWN_STATE; }
    bool IsSolved() const { return is_set.All(); }
    
    // Clear out the old solution info when starting a new solve attempt.
    void Clear() {
      value.fill(UNKNOWN_STATE);
      for (auto & val_options : bit_options) {
        val_options.SetAll();
      }
      is_set.Clear();
    }

    // Set the value of an individual cell
    // Return true/false based on whether progress was made toward solving the puzzle.
    virtual bool Set(size_t cell, uint8_t state) {
      emp_assert(cell < NUM_CELLS);   // Make sure cell is in a valid range.
      emp_assert(state < NUM_STATES); // Make sure state is in a valid range.

      if (value[cell] == state) return false;  // If state is already set, SKIP!

      emp_assert(HasOption(cell,state));     // Make sure state is allowed.
      is_set.Set(cell);
      value[cell] = state;                   // Store found value!

      // Clear this cell from all sets of options.
      for (size_t s=0; s < NUM_STATES; ++s) bit_options[s].Clear(cell);
      
      return true;
    }

    // Remove a symbol option from a particular cell.
    void Block(size_t cell, uint8_t state) {
      bit_options[state].Clear(cell);
    }

    // Operate on a "move" object.
    void Move(const PuzzleMove & move) {
      emp_assert(move.pos_id >= 0 && move.pos_id < NUM_CELLS, move.pos_id);
      emp_assert(move.state >= 0 && move.state < NUM_STATES, move.state);
      
      switch (move.type) {
      case PuzzleMove::SET_STATE:   Set(move.pos_id, move.state);   break;
      case PuzzleMove::BLOCK_STATE: Block(move.pos_id, move.state); break;
      default:
        emp_assert(false);   // One of the previous move options should have been triggered!
      }
    }
    
    // Operate on a set of "move" objects.
    void Move(const emp::vector<PuzzleMove> & moves) {
      for (const auto & move : moves) { Move(move); }
    }

    // Scan for contradictions or lack of options that would make puzzle unsolvable.
    bool IsUnsolvable() {
      // Identify which cells are either set or still have options.
      grid_bits_t has_options = is_set;
      for (uint8_t state = 0; state < NUM_STATES; ++state) {
        has_options |= bit_options[state];
      }

      return !has_options.All();
    }

  };
} // End namespace: emp

#endif