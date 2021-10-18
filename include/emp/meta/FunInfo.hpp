/**
 *  @note This file is part of Empirical, https://github.com/devosoft/Empirical
 *  @copyright Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  @date 2021
 *
 *  @file FunInfo.hpp
 *  @brief Wrap a function to provide more information about it.
 *  @note Status: ALPHA
 * 
 *  FunInfo will collect information about a provided function and facilitate
 *  manipulations.
 * 
 * 
 *  Developer Notes:
 *  - Need to setup BindAt to choose bind position(s).
 *    For example:  fun_info::BindAt<2,4>(my_fun, 12, "abc")
 */

#ifndef EMP_FUN_INFO_HPP
#define EMP_FUN_INFO_HPP

#include <functional>

#include "TypePack.hpp"

namespace emp {

  // A generic base class that expands anything with operator()
  template <typename T>
  struct FunInfo : public FunInfo< decltype(&T::operator()) > {};

  // Specialization for functions; redirect to function-object specialization.
  template <typename RETURN_T, typename... PARAM_Ts>
  struct FunInfo <RETURN_T(PARAM_Ts...)>
  : public FunInfo< std::function<RETURN_T(PARAM_Ts...)> > {};

  // Specialization for functions; redirect to function-object specialization.
  template <typename RETURN_T, typename... PARAM_Ts>
  struct FunInfo <RETURN_T(*)(PARAM_Ts...)>
  : public FunInfo< std::function<RETURN_T(PARAM_Ts...)> > {};


  // Specialization for function objects with AT LEAST ONE parameter...
  template <typename CLASS_T, typename RETURN_T, typename PARAM1_T, typename... PARAM_Ts>
  struct FunInfo <RETURN_T(CLASS_T::*)(PARAM1_T, PARAM_Ts...) const> {
  private:
  //   template <typename T> struct is_templated_converter : std::false_type{};
  //   template <typename T>
  //   struct is_templated_converter<emp::decoy_t<T, decltype(std::declval<T&>().template operator()<int>(0))>> : std::true_type{};

    /// Helper function to lock an argument at a designated position in a function.
    template <typename BOUND_T, typename... BEFORE_Ts, typename... AFTER_Ts>
    static auto BindAt_impl(CLASS_T fun, BOUND_T && bound,
                            TypePack<BEFORE_Ts...> before_tp, TypePack<AFTER_Ts...> after_tp) {
      // If the function needs a reference for the parameter, send the supplied value through.
      if constexpr (std::is_reference<PARAM1_T>()) {
        return [fun, &bound](BEFORE_Ts &&... before_args, AFTER_Ts &&... after_args) {
          return fun(std::forward<BEFORE_Ts>(before_args)...,
                     std::forward<BOUND_T>(bound),
                     std::forward<AFTER_Ts>(after_args)...);
        };
      }
      // Otherwise, a copy is fine.
      else {
        return [fun, bound](BEFORE_Ts &&... before_args, AFTER_Ts &&... after_args) {
          return fun(std::forward<BEFORE_Ts>(before_args)...,
                     bound,
                     std::forward<AFTER_Ts>(after_args)...);
        };
      }
    }


  public:
    using return_t = RETURN_T;
    using params_t = TypePack<PARAM1_T, PARAM_Ts...>;

    template <size_t ID>
    using arg_t = typename params_t::template get<ID>;

    static constexpr size_t NumArgs() { return 1 + sizeof...(PARAM_Ts); }

    /// Test if this function can be called with a particular set of arguments.
    template <typename ARG1, typename... ARG_Ts>
    static constexpr bool InvocableWith(ARG1 &&, ARG_Ts &&...) {
      return std::is_invocable<CLASS_T, ARG1, ARG_Ts...>();
    }

    /// Test if this function can be called with a particular set of argument TYPEs.
    template <typename... ARG_Ts>
    static constexpr bool InvocableWith() {
      return std::is_invocable<CLASS_T, ARG_Ts...>();
    }

    /// Change a function's return type using a converter function.
    template <typename FUN_T, typename CONVERTER_T>
    static auto ChangeReturnType(FUN_T fun, CONVERTER_T convert_fun)
    {
      return [fun=fun, c=convert_fun](PARAM1_T && arg1, PARAM_Ts &&... args) {
        return c( fun(std::forward<PARAM1_T>(arg1), std::forward<PARAM_Ts>(args)...) );
      };
    }

    /// Change a function's arguments using a fixed converter function.
    template <typename NEW_T, typename FUN_T, typename CONVERTER_T>
    static auto ChangeParameterTypes(FUN_T fun, CONVERTER_T convert_fun)
    {
      return [fun=fun, c=convert_fun](NEW_T arg1, decoy_t<NEW_T, PARAM_Ts>... args) {
        return fun(c(arg1), c(args)...);
      };
    }

    /// Convert a function's arguments using a dynamic (tempalted) lambda function.
    template <typename NEW_T, typename FUN_T, typename CONVERTER_T>
    static auto ConvertParameterTypes(FUN_T fun, CONVERTER_T convert_lambda)
    {
      return [fun=fun, c=convert_lambda](NEW_T arg1, decoy_t<NEW_T, PARAM_Ts>... args) {
        return fun(c.template operator()<PARAM1_T>(arg1),
                   c.template operator()<PARAM_Ts>(args)...);
      };
    }


    /// Lock in a specified argument of a function.
    template <size_t POS, typename T>
    static auto BindAt(CLASS_T fun, T && bound) {
      using before_pack = typename params_t::template shrink<POS>;
      using after_pack = typename params_t::template popN<POS+1>;
      return BindAt_impl(fun, std::forward<T>(bound), before_pack(), after_pack());
    }

  };

  // Specialization for function objects with NO parameters...
  template <typename CLASS_T, typename RETURN_T>
  struct FunInfo <RETURN_T(CLASS_T::*)() const>
  {
    using return_t = RETURN_T;
    using params_t = TypePack<>;

    static constexpr size_t NumArgs() { return 0; }

    /// Test if this function can be called with a particular set of arguments.
    template <typename... ARG_Ts>
    static constexpr bool InvocableWith(ARG_Ts...) { return sizeof...(ARG_Ts) == 0; }

    /// Change a function's return type using a converter function.
    template <typename FUN_T, typename CONVERTER_T>
    static auto ChangeReturnType(FUN_T fun, CONVERTER_T convert_fun)
    {
      return [fun=fun, c=convert_fun]() {
        return c(fun());
      };
    }

    /// Change a function's arguments using a converter function.
    template <typename /*NEW_T*/, typename FUN_T, typename CONVERTER_T>
    static auto ChangeParameterTypes(FUN_T fun, CONVERTER_T /*convert_fun*/)
    {
      // No parameters, so no changes to make.
      return fun;
    }

    /// Convert a function's arguments using a dynamic (tempalted) lambda function.
    template <typename NEW_T, typename FUN_T, typename CONVERTER_T>
    static auto ConvertParameterTypes(FUN_T fun, CONVERTER_T convert_lambda)
    {
      // No parameters, so no conversions to make.
      return fun;
    }

  };


  // === Stand-alone helper functions ===

    /// Change a function's return type using a converter function.
  template <typename FUN_T, typename CONVERTER_T>
  static auto ChangeReturnType(FUN_T fun, CONVERTER_T convert_fun)
  {
    return FunInfo<FUN_T>::ChangeReturnType(fun, convert_fun);
  }

  /// Change a function's arguments using a simple converter function.
  template <typename NEW_T, typename FUN_T, typename CONVERTER_T>
  static auto ChangeParameterTypes(FUN_T fun, CONVERTER_T convert_fun)
  {
    return FunInfo<FUN_T>::template ChangeParameterTypes<NEW_T>(fun, convert_fun);
  }

  /// Convert a function's arguments using a templated lambda.
  /// @note: Will not work until C++20!!
  template <typename NEW_T, typename FUN_T, typename CONVERTER_T>
  static auto ConvertParameterTypes(FUN_T fun, CONVERTER_T convert_fun)
  {
    return FunInfo<FUN_T>::template ConvertParameterTypes<NEW_T>(fun, convert_fun);
  }

  /// Convert both return type AND parameter type.
  /// Convert a function's arguments using a templated lambda.
  template <typename NEW_T, typename FUN_T, typename R_CONVERTER_T, typename P_CONVERTER_T>
  static auto ChangeTypes(FUN_T fun, R_CONVERTER_T ret_convert_fun, P_CONVERTER_T param_convert_fun)
  {
    auto partial = FunInfo<FUN_T>::template ChangeParameterTypes<NEW_T>(fun, param_convert_fun);
    return FunInfo<decltype(partial)>::ChangeReturnType(partial, ret_convert_fun); 
  }

  /// Lock in a specified argument of a function.
  template <size_t POS, typename FUN_T, typename BOUND_T>
  auto BindAt(FUN_T fun, BOUND_T && bound) {
    return FunInfo<FUN_T>::template BindAt<POS>(fun, std::forward<BOUND_T>(bound));
  }

  /// Lock in the first argument of a function.
  template <typename FUN_T, typename BOUND_T>
  auto BindFirst(FUN_T fun, BOUND_T && bound) {
    return FunInfo<FUN_T>::template BindAt<0>(fun, std::forward<BOUND_T>(bound));
  }

}

#endif
