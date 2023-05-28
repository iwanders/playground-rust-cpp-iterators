/*
Copyright 2023 Ivor Wanders

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the author nor the names of contributors may be used to
  endorse or promote products derived from this software without specific
  prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>


namespace rust
{

using usize = std::size_t;

template <typename A>
struct FromIterator;

template <typename A>
struct FromIterator<std::vector<A>>
{
  template <typename It>
  static std::vector<A> from_iter(It&& it)
  {
    std::vector<A> c;
    auto [lower, upper] = it.size_hint();
    if (std::size_t limit; upper.Some(limit))
    {
      c.reserve(limit);
    }
    else
    {
      c.reserve(lower);
    }
    while (true)
    {
      auto z = it.next();
      if (z.is_some())
      {
        c.push_back(std::move(z).unwrap());
      }
      else
      {
        break;
      }
    }
    return c;
  }
};

struct panic_error : std::runtime_error
{
  panic_error(const char* s) : std::runtime_error(s){};
  panic_error(const std::string& s) : std::runtime_error(s){};
};

namespace detail
{

template <typename T>
struct Option
{
  using type = T;

  // We don't actually use get(), but... why doesn't the std::optional do this `&&` trick?
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

  T unwrap() &&
  {
    if (populated_)
    {
      return std::exchange(v_, T{});  // bah, now Option requires default constructibility.
    }
    throw panic_error("unwrap called on empty Option");
  }

  bool is_some() const
  {
    return populated_;
  }
  bool is_none() const
  {
    return !is_some();
  }

  bool Some(T& x) &
  {
    if (populated_)
    {
      x = v_;
      return true;
    }
    return false;
  }
  bool Some(T& x) &&
  {
    if (populated_)
    {
      x = std::move(v_);
      return true;
    }
    return false;
  }

  Option(const T& v) : v_{ v }, populated_{ true } {};
  Option(T&& v) : v_{ v }, populated_{ true } {};
  Option() : populated_{ false } {};

  bool populated_{ false };
  T v_;
};

template <std::size_t Index, typename T>
std::tuple_element_t<Index, Option<T>>& get(Option<T>& opt)
{
  if (opt.is_some())
  {
    return opt.get();
  }
  else
  {
    throw rust::panic_error("structured binding accessed empty optional");
  }
}
template <std::size_t Index, typename T>
std::tuple_element_t<Index, Option<T>> get(Option<T>&& opt)
{
  return std::forward<Option<T>>(opt).unwrap();
}

/// Make an Option printable.
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

/// Helper struct to allow return type conversion.
template <typename It>
struct Collector
{
  template <class Container>
  operator Container()
  {
    return FromIterator<Container>::from_iter(it_);
  }

  It it_;
};

template <typename T, typename IterPtr>
struct RangeIter
{
  T operator*()
  {
    return opt_.get();  // bah, bah, bah.
  }

  bool operator!=(const auto& other)
  {
    return opt_.is_some();
  }

  RangeIter<T, IterPtr>& operator++()
  {
    opt_ = (*it_).next();
    return *this;
  }
  RangeIter<T, IterPtr> operator++(int)
  {
    opt_ = (*it_).next();
    return *this;
  }

  static RangeIter<T, IterPtr> end()
  {
    return RangeIter<T, IterPtr>{ {}, nullptr };
  }
  static RangeIter<T, IterPtr> begin(IterPtr it)
  {
    auto range_it = RangeIter<T, IterPtr>{ {}, it };
    range_it++;
    return range_it;
  }

  Option<T> opt_;
  IterPtr it_;  //  iff nullptr, end iterator.
};

template <typename T, typename NextFun>
struct Iterator
{
  using type = T;
  using function_type = NextFun;

  Iterator(NextFun&& f, std::size_t size) : f_(f), size_(size){};

  Option<T> next()
  {
    return f_();
  };

  std::pair<usize, Option<usize>> size_hint() const
  {
    return { size_, Option<usize>() };
  }

  template <typename F>
  auto map(F&& f)
  {
    using U = std::invoke_result<F, T>::type;
    auto generator = [this, f]() mutable -> Option<U> { return this->next().map(f); };
    return make_iterator<U>(std::move(generator), size_);
  }

  template <typename F>
  bool any(F&& f) &&
  {
    using U = std::invoke_result<F, T>::type;
    static_assert(std::is_same<U, bool>::value, "return for any must be bool");
    auto value = next();
    while (value.is_some())
    {
      if (f(std::move(value).unwrap()))
      {
        return true;
      }
      value = next();
    }
    return false;
  }

  template <typename CollectType = void>
  auto collect() &&
  {
    if constexpr (std::is_same<CollectType, void>::value)
    {
      return Collector{ std::move(*this) };
    }
    else
    {
      return FromIterator<CollectType>::from_iter(std::move(*this));
    }
  }

  auto sum() &&
  {
    auto first = f_();
    if (first.is_some())
    {
      auto current = std::move(first).unwrap();
      auto i_next = next();
      while (i_next.is_some())
      {
        current = current + std::move(i_next).unwrap();  // should call the sum trait really.
        i_next = next();
      }
      return current;
    }
    else
    {
      return typename decltype(first)::type{};  // should be the zero value of a type... but alas.
    }
  }

  auto begin()
  {
    return RangeIter<T, decltype(this)>::begin(this);
  }
  auto end()
  {
    return RangeIter<T, decltype(this)>::end();
  }

private:
  template <typename Z, typename U>
  friend struct RangeIter;
  template <typename Z>
  friend struct FromIterator;

  NextFun f_;
  usize size_;
};

template <typename Z, typename RealNextFun>
static auto make_iterator(RealNextFun&& v, std::size_t size)
{
  return Iterator<Z, RealNextFun>{ std::forward<RealNextFun>(v), size };
};

}  // namespace detail

template <typename T>
using Option = detail::Option<T>;

template <typename C>
auto iter(const C& container)
{
  auto start = container.begin();
  auto end = container.end();
  const auto size = container.size();
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
      },
      size);
}

}  // namespace rust

// mixing for structured bindings on options.
namespace std
{
template <typename T>
struct tuple_size<rust::Option<T>> : std::integral_constant<std::size_t, 1>
{
};

template <typename T>
struct tuple_element<0, rust::Option<T>>
{
  using type = T;
};
}  // namespace std
