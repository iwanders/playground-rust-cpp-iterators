#pragma once
#include <iostream>
#include <utility>
#include <string>
#include <exception>

namespace rust
{

template <typename A>
struct FromIterator;

template <typename A>
struct FromIterator<std::vector<A>> {
  template <typename F>
  static std::vector<A> from_iter(F&& f) {
    std::vector<A> c;
    while (true) {
      auto z = f();
      if (z.is_some()) {
        c.push_back(std::move(z).unwrap());
      } else {
        break;
      }
    }
    return c;
  }
};

struct panic_error : std::runtime_error {
  panic_error(const char* s) : std::runtime_error(s){};
  panic_error(const std::string& s) : std::runtime_error(s){};
};

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

  T unwrap() && {
    if (populated_) {
      return std::exchange(v_, T{}); // bah, now Option requires default constructibility.
    }
    throw panic_error("unwrap called on empty Option");
  }

  bool is_some() const {
    return populated_;
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

template <typename F>
struct Collector {

  template <class Container>
  operator Container () {
    return FromIterator<Container>::from_iter(f_);
  }

  F f_;
};

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

  template <typename F>
  auto map(F&& f) &
  {
    using U = std::invoke_result<F, T>::type;
    auto generator = [this, f]() mutable -> Option<U> { return this->next().map(f); };
    return make_iterator<U>(std::move(generator));
  }

  auto collect() && {
    auto old_f = std::move(f_);  // rip out the guts of the previous
    return Collector{[old_f]() mutable {return old_f();}};
  }

  template <typename R>
  R collect() {
    return FromIterator<R>::from_iter(f_);
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
