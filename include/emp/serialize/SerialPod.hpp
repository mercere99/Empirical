/**
 * @note This file is part of Empirical, https://github.com/devosoft/Empirical
 * @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 * @date 2023.
 *
 * @file SerialPod.hpp
 * @brief Tools to save and load data from classes.
 * @note Status: ALPHA
 *
 * The SerialPod object knows whether it is loading or saving and object, and provides a
 * simple mechanism to do so.
 * 
 * How to use:
 *  A standard class can use a SerialPod by adding a `Serialize(SerialPod & pod)` member function.
 *
 *  An object of an independent class MyClass with a fixed interface can have a stand-alone
 *  Serialize function in the form of `Serialize(SerialPod & pos, const MyClass & obj)`.
 *
 *  Any class can have a constructor that takes a `SerialPod` to allow reconstruction of const
 *  objects.  As long as such a constructor is provided, the Serialize function can be const.
 * 
 *  More complex classes (e.g., those that do memory management) will need to have seprate
 *  `SerialSave(SerialPod & pos) const` and `SerialLoad(SerialPod & pos)` functions.
 *
 * Example:
 *  Inside of MyClass we might have:
 * 
 *  void MyClass(SerialPod & pod) {
 *    pod(member_a, member_b, member_c);  // List all members to save/load
 *  }
 */

#ifndef EMP_SERIALIZE_SERIALPOD_HPP_INCLUDE
#define EMP_SERIALIZE_SERIALPOD_HPP_INCLUDE

#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "../base/Ptr.hpp"
#include "../base/vector.hpp"
#include "../meta/type_traits.hpp"

namespace emp {

  /// A SerialPod manages information about other classes for serialization.
  class SerialPod {
  private:
    using is_ptr = emp::Ptr<std::istream>;
    using os_ptr = emp::Ptr<std::ostream>;
    std::variant<is_ptr, os_ptr> stream_ptr = static_cast<is_ptr>(nullptr);
    bool own_ptr = false;

    void ClearData() {
      // If we own this pointer, delete it.
      if (own_ptr) {
        std::visit([](auto ptr) { ptr.Delete(); }, stream_ptr);
        own_ptr = false;
      }
      stream_ptr = static_cast<is_ptr>(nullptr);
    }

  public:
    SerialPod(std::ostream & os) : stream_ptr(emp::ToPtr(&os)) { ; }
    SerialPod(std::istream & is) : stream_ptr(emp::ToPtr(&is)) { ; }
    SerialPod(std::iostream & ios, bool is_save) {
      if (is_save) stream_ptr = emp::ToPtr<std::ostream>(&ios);
      else stream_ptr = emp::ToPtr<std::istream>(&ios);
    }

    /// Move constructor
    SerialPod(SerialPod && rhs) : stream_ptr(rhs.stream_ptr), own_ptr(rhs.own_ptr) {
      rhs.own_ptr = false;
    }

    /// Move operator
    SerialPod & operator=(SerialPod && rhs) {
      ClearData();
      stream_ptr = rhs.stream_ptr;
      own_ptr = rhs.own_ptr;
      rhs.own_ptr = false;
      return *this;
    }

    // Make sure these are never accidentally created or copied.
    SerialPod() = delete;
    SerialPod(const SerialPod &) = delete;

    ~SerialPod() { ClearData(); }

    bool IsLoad() const { return std::holds_alternative<is_ptr>(stream_ptr); }
    bool IsSave() const { return std::holds_alternative<os_ptr>(stream_ptr); }

    template <typename T>
    SerialPod & Load(T & in) {
      static_assert(!emp::is_ptr_type<std::decay<T>>(),
        "SerialPod cannot load or save pointers without more information.\n"
        "Use ManagePtr(value) for restoring pointers by first building the instance,\n"
        "or LinkPtr(value) to use the value of a pointer that is managed elsewhere." );
    }

    template <typename T>
    SerialPod & Save(T & in) {
      static_assert(!emp::is_ptr_type<std::decay<T>>(),
        "SerialPod cannot load or save pointers without more information.\n"
        "Use ManagePtr(value) for restoring pointers by first building the instance,\n"
        "or LinkPtr(value) to use the value of a pointer that is managed elsewhere." );
    }

    template <typename T>
    SerialPod & operator()(T & in) {
      if (IsLoad()) return Load(in);
      else return Save(in);
    }
  };

} // Close namespace emp

#endif // #ifndef EMP_IO_SERIALIZE_HPP_INCLUDE
