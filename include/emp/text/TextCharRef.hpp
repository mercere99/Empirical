/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2023.
 *
 *  @file TextCharRef.hpp
 *  @brief Handles a single, potentially formatted character from emp::Text.
 *  @note Status: ALPHA
 * 
 */

#ifndef EMP_TOOLS_TEXT_CHAR_REF_HPP_INCLUDE
#define EMP_TOOLS_TEXT_CHAR_REF_HPP_INCLUDE

#include <type_traits>  // std::conditional

#include "../base/assert.hpp"
#include "../base/notify.hpp"
#include "../tools/String.hpp"

namespace emp {

  class Text;

  // An individual proxy character from Text that is format aware.
  template <bool IS_CONST=false>
  class TextCharRef {
  private:
    using text_t = typename std::conditional<IS_CONST, const Text, Text>::type;
    text_t & text_ref;
    size_t pos;
  public:
    TextCharRef(text_t & _ref, size_t _pos) : text_ref(_ref), pos(_pos) { }
    TextCharRef(const TextCharRef<false> & in) : text_ref(in.text_ref), pos(in.pos) { }
    TextCharRef(const TextCharRef<true> & in) : text_ref(in.text_ref), pos(in.pos) { }
    ~TextCharRef() = default;

    // Set this character equal (with same inputs) as in parameter; don't change reference.
    TextCharRef & operator=(const TextCharRef<false> & in) { text_ref.Set(pos, in); return *this; }
    TextCharRef & operator=(const TextCharRef<true> & in) {
      static_assert(!IS_CONST, "Cannot assign a const TextCharRef to a mutatble version.");
      text_ref.Set(pos, in);
      return *this;
    }

    // Set just this character; don't change style.
    TextCharRef & operator=(char in) { text_ref.Set(pos, in); return *this; }

    // Convert to a normal C++ char.
    char AsChar() const { return text_ref.GetChar(pos); }
    operator char() const { return AsChar(); }

    // Comparison operators
    auto operator<=>(const TextCharRef & in) const {
      return text_ref.GetChar(pos) <=> in.text_ref.GetChar(in.pos);
    }
    auto operator<=>(char in) const {
      return text_ref.GetChar(pos) <=> in;
    }

    text_t & GetText() const { return text_ref; }
    size_t GetPos() const { return pos; }
    emp::vector<String> GetStyles() const { return text_ref.GetStyles(pos); }

    bool HasStyle(const String & style) const { return text_ref.HasStyle(style, pos); }
    bool IsBold()        { return HasStyle("bold"); }
    bool IsCode()        { return HasStyle("code"); }
    bool IsItalic()      { return HasStyle("italic"); }
    bool IsStrike()      { return HasStyle("strike"); }
    bool IsSubscript()   { return HasStyle("subscript"); }
    bool IsSuperscript() { return HasStyle("superscript"); }
    bool IsUnderline()   { return HasStyle("underline"); }

    TextCharRef & SetStyle(const String & style) {
      text_ref.SetStyle(style, pos);
      return *this;      
    }
    TextCharRef & Bold()        { return SetStyle("bold"); }
    TextCharRef & Code()        { return SetStyle("code"); }
    TextCharRef & Italic()      { return SetStyle("italic"); }
    TextCharRef & Strike()      { return SetStyle("strike"); }
    TextCharRef & Subscript()   { return SetStyle("subscript"); }
    TextCharRef & Superscript() { return SetStyle("superscript"); }
    TextCharRef & Underline()   { return SetStyle("underline"); }
  };
}

#endif