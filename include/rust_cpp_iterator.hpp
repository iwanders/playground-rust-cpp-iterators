#pragma once
#include <iostream>
#include <utility>

namespace rust
{
namespace detail
{

template <typename T>
struct Option
{
  using type = T;

  //  static constexpr Option<T> None = Option<T>{};
  T& get() && = delete;  // Calling get() on rvalue results in dangling reference.
  T& get() &
  {
    return v_;
  }

  template <typename F>
  Option<typename std::invoke_result<F, T>::type> map(F&& f)
  {
    using U = std::invoke_result<F, T>::type;
    if (populated_)
    {
      return Option<U>(f(v_));
    }
    else
    {
      return Option<U>();
    }
  }

  Option(const T& v) : v_{ v }, populated_{ true } {};
  Option(T&& v) : v_{ v }, populated_{ true } {};
  Option() : populated_{ false } {};

  bool populated_{ false };
  T v_;
};

template <typename SS, typename T>
SS& operator<<(SS& os, const Option<T>& opt)
{
  if (opt.populated_)
  {
    os << "Some(" << opt.v_ << ")";
  }
  else
  {
    os << "None";
  }
  return os;
}

template <typename T, typename NextFun>
struct Iterator
{
  using type = T;
  using function_type = NextFun;

  Option<T> next()
  {
    return f_();
  };

  template <typename F>
  auto map(F&& f) &&
  {
    using U = std::invoke_result<F, T>::type;
    // Here the return type is the return of f(f_()), in an option.
    auto old_f = std::move(f_);  // rip out the guts of the previous
    auto generator = [f, old_f]() mutable -> Option<U> { return old_f().map(f); };
    return make_iterator<U>(std::move(generator));
  }

  NextFun f_;
};

template <typename Z, typename RealNextFun>
static auto make_iterator(RealNextFun&& v)
{
  return Iterator<Z, RealNextFun>{ v };
};

}  // namespace detail

template <typename T>
using Option = detail::Option<T>;

template <typename C>
auto iter(const C& container)
{
  auto start = container.begin();
  auto end = container.end();
  return detail::make_iterator<typename C::value_type>(
      [start, end]() mutable
      {
        if (start != end)
        {
          auto v = *start;
          start++;
          return detail::Option(std::move(v));
        }
        else
        {
          return detail::Option<typename C::value_type>();
        }
      });
}

}  // namespace rust
