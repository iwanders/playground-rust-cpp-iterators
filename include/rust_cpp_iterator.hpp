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
#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Container c{1,2,3};
// iter(c) -> Iterator<Ref<T>>
// iter_mut(c) -> Iterator<RefMut<T>>
// drain(std::move(c)) -> Iterator<T>

namespace rust
{

inline namespace types
{
using usize = std::size_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;
}  // namespace types

struct panic_error : std::runtime_error
{
  inline panic_error(const char* s) : std::runtime_error(s){};
  inline panic_error(const std::string& s) : std::runtime_error(s){};
};

template <typename T>
struct Ref
{
  using type = T;
  Ref(const T* v) : v_(v){};

  const T& operator*() const
  {
    if (v_ == nullptr)
    {
      throw panic_error("accessing dangling reference, just c++ things");
    }
    return *v_;
  }

  const auto& deref() const
  {
    return *(*this);
  }

private:
  const T* v_;
};
template <typename T>
std::string to_string(const Ref<T>& v)
{
  using std::to_string;
  return to_string(*v);
  //  return "Ref(" + to_string(*v) + ")";
}
template <typename SS, typename T>
SS& operator<<(SS& os, const Ref<T>& ref)
{
  os << to_string(ref);
  return os;
}

template <typename T>
struct RefMut
{
  using type = T;
  RefMut(T* v) : v_(v){};
  T& operator*()
  {
    if (v_ == nullptr)
    {
      throw panic_error("accessing dangling reference, just c++ things");
    }
    return *v_;
  }

  auto& deref()
  {
    return *(*this);
  }

private:
  T* v_;
};
template <typename T>
std::string to_string(RefMut<T>& v)
{
  using std::to_string;
  return to_string(*v);
  //  return "RefMut(" + to_string(*v) + ")";
}
template <typename SS, typename T>
SS& operator<<(SS& os, RefMut<T>& ref)
{
  os << to_string(ref);
  return os;
}

template <typename T>
using RefWrapper = std::conditional_t<std::is_const_v<std::remove_reference_t<T>>, Ref<const std::remove_cvref_t<T>>,
                                      RefMut<std::remove_cvref_t<T>>>;

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

template <typename A, typename B>
concept Add = requires(A a, B b)
{
  a + b;
};

template <typename A>
concept DataSize = requires(A a)
{
  a.data();
  a.size();
};

template <typename A>
struct Borrow;

template <typename A>
concept Borrowable = requires(A a)
{
  Borrow<A>::borrow(a);
};

namespace detail
{

template <typename F, std::size_t... I>
auto static_for_impl(F&& a, std::index_sequence<I...>)
{
  (a(I), ...);
}
template <std::size_t N, typename F, typename Indices = std::make_index_sequence<N>>
auto static_for(F&& a)
{
  return static_for_impl(a, Indices{});
}
// rust::detail::static_for<3>([](const long unsigned int z) { std::cout << z << std::endl; });

template <typename T, std::size_t... I, typename... Args>
auto static_for_call_impl(std::index_sequence<I...>, Args&&... args)
{
  (T::template call<I>(args...), ...);
}
template <std::size_t N, typename T, typename Indices = std::make_index_sequence<N>, typename... Args>
auto static_for_call(Args&&... args)
{
  return static_for_call_impl<T>(Indices{}, args...);
}
/*
struct Callable {
  template <std::size_t N>
  static auto call(int z) {
    std::cout << N  << " z: " << z << std::endl;
  }
};
rust::detail::static_for_call<3, Callable>(5);
*/

template <char z, char... rest>
struct ascii_to_integer
{
  // chr(0x30) == '0'
  using type = std::integral_constant<std::size_t, z - 0x30>;
  static constexpr auto value = std::integral_constant<std::size_t, z - 0x30>{};
};
template <char... str>
constexpr auto operator"" _i()
{
  return ascii_to_integer<str...>::value;
}

template <typename... Types>
struct Tuple
{
  static constexpr usize length = sizeof...(Types);

  Tuple(Types... v) : v_(v...)
  {
  }

  template <typename T>
  auto& operator[](T z)
  {
    return std::get<T::type::value>(v_);
  }

  template <typename T>
  const auto& operator[](T z) const
  {
    return std::get<T::type::value>(v_);
  }

  template <usize N>
  auto& get()
  {
    return std::get<N>(v_);
  }
  template <usize N>
  const auto& get() const
  {
    return std::get<N>(v_);
  }

private:
  std::tuple<Types...> v_;
};

using std::to_string;
std::string to_string(const std::string& s)
{
  return s;
}

struct TuplePrinter
{
  template <std::size_t N, typename T>
  static auto call(std::string& s, std::size_t length, T&& t)
  {
    //  using rust::to_string;
    s += to_string(t.template get<N>());
    if (N != (length - 1))
    {
      s += ", ";
    }
  }
};

template <typename... T>
std::string to_string(const Tuple<T...>& t)
{
  using TupleType = Tuple<T...>;
  using std::to_string;
  std::string s = "(";
  static_for_call<TupleType::length, TuplePrinter>(s, TupleType::length, t);
  s += ")";
  return s;
}
template <typename SS, typename... T>
SS& operator<<(SS& os, const Tuple<T...>& t)
{
  os << to_string(t);
  return os;
}

using std::to_string;

template <typename T>
struct Option
{
  using type = T;

  template <std::invocable<T> F>
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
      populated_ = false;
      return std::move(v_);
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
      populated_ = false;
      x = std::move(v_);
      return true;
    }
    return false;
  }

  auto operator<=>(const Option<T>& other) const
  {
    if (populated_ && other.populated_)
    {
      return v_ <=> other.v_;
    }
    else
    {
      return populated_ <=> other.populated_;
    }
  };

  auto operator==(const Option<T>& other) const
  {
    if (populated_ && other.populated_)
    {
      return v_ == other.v_;
    }
    else
    {
      return populated_ == other.populated_;
    }
  };

  Option<Ref<T>> as_ref() const
  {
    if (is_some())
    {
      return Option<Ref<T>>(&v_);
    }
    else
    {
      return Option<Ref<T>>();
    }
  }

  Option<RefMut<T>> as_mut()
  {
    if (is_some())
    {
      return Option<RefMut<T>>(&v_);
    }
    else
    {
      return Option<RefMut<T>>();
    }
  }

  template <typename... Args>
  Option(Args... v) : v_(v...), populated_{ true } {};
  Option(const T& v) : v_(v), populated_{ true } {};
  //  Option(T&& v) : v_{ v }, populated_{ true } {};
  Option() : populated_{ false } {};
  Option(const Option<T>& v) : populated_{ v.populated_ }, v_{ v.v_ } {};
  ~Option()
  {
    if (populated_)
    {
      std::destroy_at(&v_);
    }
  }

  Option<T>& operator=(Option<T>&& v)
  {
    populated_ = v.populated_;
    v_ = std::move(v.v_);
    v.populated_ = false;
    return *this;
  }

private:
  bool populated_{ false };

  // Hairy storage for the optional without allocation.
  union
  {
    char not_used_{ 0 };
    T v_;
  };
};

template <typename T>
std::string to_string(const Option<T>& opt)
{
  if (opt.is_some())
  {
    return std::string("Some(") + to_string(opt.as_ref().unwrap().deref()) + ")";
  }
  else
  {
    return "None";
  }
}

/// Make an Option printable.
template <typename SS, typename T>
SS& operator<<(SS& os, const Option<T>& opt)
{
  os << to_string(opt);
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
  T& operator*()
  {
    return opt_.as_mut().unwrap().deref();
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

  Tuple<usize, Option<usize>> size_hint() const
  {
    return Tuple<usize, Option<usize>>(size_, Option<usize>());
  }

  template <std::invocable<T> F>
  auto map(F&& f)
  {
    using U = std::invoke_result<F, T>::type;
    auto generator = [this, f]() mutable -> Option<U> { return this->next().map(f); };
    return make_iterator<U>(std::move(generator), size_);
  }

  auto copied()
  {
    return map([](const auto& v) { return *v; });
    //  using U = std::invoke_result<F, T>::type;
    //  auto generator = [this, f]() mutable -> Option<U> { return this->next().map(f); };
    //  return make_iterator<U>(std::move(generator), size_);
  }

  template <std::predicate<T> F>
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

  auto enumerate() &&
  {
    std::size_t i = 0;
    auto f = std::move(f_);
    using U = std::tuple<usize, T>;
    auto generator = [this, f, i]() mutable -> Option<U>
    {
      auto v = f();
      if (v.is_some())
      {
        auto res = Option<U>(i, std::move(v).unwrap());
        i++;
        return res;
      }
      else
      {
        return Option<U>();
      }
    };
    return make_iterator<U>(std::move(generator), size_);
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

  auto sum() && requires Add<T, T>
  {
    auto first = f_();
    if (first.is_some())
    {
      auto current = std::move(first).unwrap();
      auto i_next = next();
      while (i_next.is_some())
      {
        std::cout << "Next: " << i_next << std::endl;
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

template <typename Z, typename RawIter>
static auto make_iterator(RawIter&& start_, RawIter&& end_, usize size)
{
  auto start = start_;
  auto end = end_;
  using Wrapper = RefWrapper<decltype(*start)>;
  return detail::make_iterator<Wrapper>(
      [start, end]() mutable
      {
        if (start != end)
        {
          auto v = Wrapper(std::addressof(*start));
          start++;
          return detail::Option(v);
        }
        else
        {
          return detail::Option<Wrapper>();
        }
      },
      size);
}

template <typename T>
struct Slice;

template <typename T>
struct Slice
{
  using type = T;

  static Slice<T> from_raw_parts(T* data, usize len)
  {
    Slice<T> res;
    res.start_ = data;
    res.len_ = len;
    return res;
  }

  T& operator[](usize index)
  {
    return start_[index];
  }

  const T& operator[](usize index) const
  {
    return start_[index];
  }

  /// Take a subslice
  template <typename A = std::initializer_list<int>, typename B = std::initializer_list<int>>
  Slice<T> operator()(A a, B b) const
  {
    using std::to_string;
    usize start = 0;
    usize end = len();
    if constexpr (!std::is_same<A, std::initializer_list<int>>::value)
    {
      start = a;
    }
    if constexpr (!std::is_same<B, std::initializer_list<int>>::value)
    {
      end = b;
    }
    if (start > end)
    {
      throw panic_error("slice index  starts at " + to_string(start) + " but ends at " + to_string(end));
    }
    if (end > len())
    {
      throw panic_error("range end index " + to_string(end) + " out of range for slice of length " + to_string(len_));
    }
    Slice<T> res;
    res.start_ = start_ + start;
    res.len_ = end - start;
    return res;
  }

  std::size_t len() const
  {
    return len_;
  }

  auto iter() const
  {
    auto start = start_;
    auto end = start_ + len_;

    using Wrapper = RefWrapper<const T>;
    return detail::make_iterator<Wrapper>(
        [start, end]() mutable
        {
          if (start != end)
          {
            auto v = Wrapper(start);
            start++;
            return detail::Option(std::move(v));
          }
          else
          {
            return detail::Option<Wrapper>();
          }
        },
        len_);
  }

  auto iter_mut() const
  {
    auto start = start_;
    auto end = start_ + len_;
    using Wrapper = RefWrapper<T>;
    return detail::make_iterator<Wrapper>(
        [start, end]() mutable
        {
          if (start != end)
          {
            auto v = Wrapper(start);
            start++;
            return detail::Option(std::move(v));
          }
          else
          {
            return detail::Option<Wrapper>();
          }
        },
        len_);
  }

  void sort() requires std::totally_ordered<T>
  {
    std::ranges::stable_sort(start_, start_ + len_);
  }

  template <typename T2>
  bool operator==(const Slice<T2>& other) const requires std::equality_comparable_with<T, T2>
  {
    if (len() != other.len())
    {
      return false;
    }
    for (usize i = 0; i < len(); i++)
    {
      if (!((*this)[i] == other[i]))
      {
        return false;
      }
    }
    return true;
  }

  template <Borrowable BorrowableType>
  bool starts_with(const BorrowableType& original)
      const requires std::equality_comparable_with<T, typename Borrow<BorrowableType>::type>
  {
    const auto needle = Borrow<BorrowableType>::borrow(original);
    const auto n = needle.len();
    return len() >= n && needle == (*this)({}, n);
  }

  const T* as_ptr() const
  {
    return start_;
  }

private:
  T* start_;
  std::size_t len_;
};

template <typename T>
std::string to_string(const Slice<T>& slice)
{
  std::string s = "[";
  for (const auto& v : slice.iter())
  {
    s += " " + to_string(*v);
  }
  s += "]";
  return s;
}

/// Make an Slice printable.
template <typename SS, typename T>
SS& operator<<(SS& os, const Slice<T>& slice)
{
  os << to_string(slice);
  return os;
}

}  // namespace detail

template <typename... T>
using Tuple = detail::Tuple<T...>;

template <typename T>
using Option = detail::Option<T>;

template <typename T>
using Slice = detail::Slice<T>;

template <typename C>
auto slice(C& container) requires rust::DataSize<C>
{
  using SliceType = std::conditional_t<std::is_const_v<C>, const typename C::value_type, typename C::value_type>;
  return detail::Slice<SliceType>::from_raw_parts(container.data(), container.size());
}

template <typename C>
auto slice(C&& container) requires Borrowable<C>
{
  return Borrow<C>::borrow(container);
}

template <typename C>
auto iter(const C& container)
{
  auto start = container.cbegin();
  auto end = container.cend();
  const auto size = container.size();
  return detail::make_iterator<const typename C::value_type*>(start, end, size);
}

template <typename C>
auto iter_mut(C& container)
{
  auto start = container.begin();
  auto end = container.end();
  const auto size = container.size();
  return detail::make_iterator<typename C::value_type*>(start, end, size);
}

template <typename C>
auto drain(C&& container)
{
  auto start = Option<typename C::iterator>();
  auto end = Option<typename C::iterator>();
  const auto size = container.size();
  auto f = [container, start, end]() mutable -> Option<typename C::value_type>
  {
    if (!start.is_some())
    {
      start = Option<typename C::iterator>(container.begin());
    }
    if (!end.is_some())
    {
      end = Option<typename C::iterator>(container.end());
    }

    auto& start_it = start.as_mut().unwrap().deref();
    auto& end_it = end.as_mut().unwrap().deref();
    if (start_it != end_it)
    {
      auto v = std::move(*start_it);
      auto res = Option<typename C::value_type>(std::move(v));
      start_it++;
      return res;
    }
    else
    {
      return Option<typename C::value_type>();
    }
  };
  return detail::make_iterator<typename C::value_type>(std::move(f), size);
}

template <class T>
concept ConstCharString = std::is_same_v < std::remove_cvref_t<T>,
const char* > ;

template <ConstCharString T>
struct Borrow<T>
{
  using type = const char;
  static detail::Slice<const char> borrow(const char* s)
  {
    for (usize i = 0; i < std::numeric_limits<usize>::max(); i++)
    {
      if (s[i] == 0)
      {
        return detail::Slice<const char>::from_raw_parts(s, i);
      }
    }
    throw panic_error("could not find end of string for borrow");
  }
};

template <rust::DataSize T>
struct Borrow<T>
{
  using type = const typename std::remove_cvref_t<T>::value_type;
  static detail::Slice<type> borrow(const T& s)
  {
    return detail::Slice<type>::from_raw_parts(s.data(), s.size());
  }
};

template <typename T>
struct Borrow<rust::Slice<T>>
{
  using type = const T;
  static rust::Slice<type> borrow(const rust::Slice<T>& s)
  {
    return detail::Slice<type>::from_raw_parts(s.as_ptr(), s.len());
    ;
  }
};

template <typename T>
concept IsArray = std::is_array_v<std::remove_cvref_t<T>>;

template <IsArray T>
struct Borrow<T>
{
  using type = const std::remove_extent_t<std::remove_cvref_t<T>>;
  static constexpr std::size_t N = std::extent_v<std::remove_cvref_t<T>>;

  // For char[N], we pop the null byte at the end.
  static rust::Slice<type> borrow(const type* s) requires std::is_same<type, const char>::value
  {
    return detail::Slice<type>::from_raw_parts(s, N - 1);
  }
  static rust::Slice<type> borrow(const type* s) requires(!std::is_same<type, const char>::value)
  {
    return detail::Slice<type>::from_raw_parts(s, N);
  }
};

namespace prelude
{
// This approximates the rust std prelude.

using rust::Option;
using rust::Slice;
using rust::Tuple;

using rust::drain;
using rust::iter;
using rust::iter_mut;
using rust::slice;

using namespace rust::types;

// The index type for our tuple.
using detail::operator""_i;
}  // namespace prelude

}  // namespace rust

namespace std
{
template <typename... T>
struct tuple_size<rust::Tuple<T...>> : std::integral_constant<std::size_t, sizeof...(T)>
{
};

template <std::size_t N, typename... T>
struct tuple_element<N, rust::Tuple<T...>>
{
  using type = std::tuple_element<N, std::tuple<T...>>::type;
};

template <std::size_t N, typename... T>
struct tuple_element<N, const rust::Tuple<T...>>
{
  using type = const std::tuple_element<N, std::tuple<T...>>::type;
};

template <std::size_t N, typename... T>
auto& get(rust::Tuple<T...>& t)
{
  return t.template get<N>();
}
template <std::size_t N, typename... T>
auto const& get(const rust::Tuple<T...>& t)
{
  return t.template get<N>();
}
//  template <std::size_t N, typename... T>
//  auto const&& get(const rust::Tuple<T...>&& t)
//  {
//  return t.template get<N>();
//  }
//  template <std::size_t N, typename... T>
//  auto&& get(rust::Tuple<T...>&& t)
//  {
//  return t.template get<N>();
//  }
}  // namespace std
