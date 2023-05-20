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
  Option<T> next()
  {
    return f_();
  };

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
