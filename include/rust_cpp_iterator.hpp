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

  bool operator==(const Ref<T>& other) const
  {
    return v_ == other.v_;
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

  bool operator==(const RefMut<T>& other) const
  {
    return v_ == other.v_;
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

struct Unit
{
};
std::string to_string(Unit)
{
  return "()";
}
template <typename SS, typename T>
SS& operator<<(SS& os, Unit u)
{
  os << to_string(u);
  return os;
}

template <typename T>
using RefWrapper = std::conditional_t<std::is_const_v<std::remove_reference_t<T>>, Ref<const std::remove_cvref_t<T>>,
                                      RefMut<std::remove_cvref_t<T>>>;

template <typename A>
concept Dereferencable = requires(A a)
{
  *a;
};

template <typename T>
auto deref(T&& v) requires(!Dereferencable<T>)
{
  return v;
}

template <typename T>
auto deref(T&& v) requires Dereferencable<T>
{
  return deref(*v);
}

template <typename A>
struct FromIterator;

template <typename T>
concept Underscore = std::is_same_v<T, void> || std::is_same_v<T, Unit>;

template <Underscore A>
struct FromIterator<A>
{
  template <typename It>
  static A from_iter(It&& it)
  {
    auto z = it.next();
    while (z.is_some())
    {
      z = it.next();
    }
    if constexpr (!std::is_same_v<A, void>)
    {
      return A{};
    }
  }
};

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
        c.push_back(deref(std::move(z).unwrap()));
      }
      else
      {
        break;
      }
    }
    return c;
  }
};

template <>
struct FromIterator<std::string>
{
  template <typename It>
  static std::string from_iter(It&& it)
  {
    std::string s;
    auto [lower, upper] = it.size_hint();
    if (std::size_t limit; upper.Some(limit))
    {
      s.reserve(limit);
    }
    else
    {
      s.reserve(lower);
    }
    while (true)
    {
      auto z = it.next();
      if (z.is_some())
      {
        auto c = deref(std::move(z).unwrap());
        static_assert(std::is_same_v<decltype(c), char>, "may only collect string from char");
        s.push_back(c);
      }
      else
      {
        break;
      }
    }
    return s;
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

template <typename A>
concept HasNext = requires(A a)
{
  a.next();
};

template <class A>
struct IntoIterator;

template <HasNext A>
struct IntoIterator<A>
{
  static auto into_iter(A& c)
  {
    return c;
  }
};

template <typename A>
concept Iterable = requires(A a)
{
  IntoIterator<A>::into_iter(a);
};

template <typename T>
auto into_iter(T& a)
{
  return IntoIterator<std::remove_reference_t<T>>::into_iter(a);
}

namespace detail
{

using std::to_string;

template <typename T>
std::string type_string()
{
  return __PRETTY_FUNCTION__;
}

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

// Rolling our own Tuple to make it nice and printable etc.
// using https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2726r0.html

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

template <typename T>
using TypeOrUnit = typename std::conditional_t<std::is_same_v<T, void>, Unit, T>;

template <typename T>
struct Option
{
  using type = T;
  using NoneType = Option<T>;

  template <std::invocable<T> F>
  Option<TypeOrUnit<typename std::invoke_result_t<F, T>>> map(F&& f)
  {
    using U = TypeOrUnit<typename std::invoke_result_t<F, T>>;
    if (populated_)
    {
      if constexpr (std::is_same_v<U, Unit>)
      {
        f(v_);
        return Option<U>(Unit{});
      }
      else
      {
        return Option<U>(f(v_));
      }
    }
    else
    {
      return Option<U>();
    }
  }

  template <std::invocable<T> F>
  std::invoke_result_t<F, T> and_then(F&& f) &&
  {
    if (is_some())
    {
      return f(std::move(*this).unwrap());
    }
    return std::invoke_result_t<F, T>();
  }

  auto copied() &&
  {
    return map([](const auto& v) { return *v; });
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
  Option(Args... v) : v_(v...), populated_{ true }
  {
    std::construct_at(&v_, std::forward<Args>(v)...);
  };
  Option(const T& v) : populated_{ true }
  {
    std::construct_at(&v_, v);
  };
  Option(T&& v) : v_(v), populated_{ true }
  {
    std::construct_at(&v_, std::move(v));
  };
  Option() : populated_{ false } {};

  // Is this a copy constructor, or constructing an Option holding an Option!?
  Option(const Option<T>& v) : populated_{ v.populated_ }
  {
    // call the constructor for v_.
    if (v.is_some())
    {
      std::construct_at(&v_, v.as_ref().unwrap().deref());
    }
  };
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
    std::construct_at(&v_, std::move(v.v_));
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

  struct ReturnTypeCollect
  {
  };

  Iterator(NextFun&& f, std::size_t size) : f_(f), size_(size){};

  Option<T> next()
  {
    return f_();
  };

  Tuple<usize, Option<usize>> size_hint() const
  {
    return Tuple<usize, Option<usize>>(size_, Option<usize>());
  }

  // [[nodiscard("map is not consumed")]]  doesn't work? :<
  template <std::invocable<T> F>
  auto map(F&& f)
  {
    using U = TypeOrUnit<typename std::invoke_result_t<F, T>>;
    auto generator = [this, f]() mutable -> Option<U> { return this->next().map(f); };
    return make_iterator<U>(std::move(generator), size_);
  }

  template <Iterable It>
  auto zip(It&& f) &&
  {
    auto our_it = std::move(f_);
    auto other_it = into_iter(f);
    bool finished = false;

    const auto [our_lowest, our_highest] = size_hint();
    const auto [other_lowest, other_highest] = other_it.size_hint();
    const auto new_lowest = std::min<usize>(our_lowest, other_lowest);

    // Get the types, both return an option, which we can get the type of.
    using L = T;
    using R = decltype(other_it)::type;
    using U = Tuple<L, R>;
    auto generator = [our_it, other_it, finished]() mutable -> Option<U>
    {
      if (finished)
      {
        return Option<U>();
      }
      auto l = our_it();
      auto r = other_it.next();
      if (l.is_none() || r.is_none())
      {
        finished = true;
        return Option<U>();
      }
      auto l_r = U(std::move(l).unwrap(), std::move(r).unwrap());
      return Option<U>(std::move(l_r));
    };

    return make_iterator<U>(std::move(generator), new_lowest);
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

  template <typename CollectType = ReturnTypeCollect>
  auto collect() &&
  {
    if constexpr (std::is_same<CollectType, ReturnTypeCollect>::value)
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

template <typename Child, typename Z>
struct SliceInterface;

template <typename T>
struct Slice : SliceInterface<Slice<T>, T>
{
  using type = T;

  static Slice<T> from_raw_parts(T* data, usize len)
  {
    Slice<T> res;
    res.begin_ = data;
    res.len_ = len;
    return res;
  }

  T* _begin() const
  {
    return begin_;
  }
  usize _len() const
  {
    return len_;
  }

private:
  T* begin_;
  std::size_t len_;
};

template <typename Child, typename T>
struct SliceInterface
{
  SliceInterface() : child_(*static_cast<Child*>(this))
  {
  }

  usize len() const
  {
    return child_._len();
  }

  T& operator[](usize index)
  {
    if (index >= len())
    {
      throw panic_error("out of bounds access");
    }
    return begin()[index];
  }

  const T& operator[](usize index) const
  {
    if (index >= len())
    {
      throw panic_error("out of bounds access");
    }
    return begin()[index];
  }

  const T& get_unchecked(usize index) const
  {
    return (*this)[index];
  }

  T& get_unchecked_mut(usize index)
  {
    return (*this)[index];
  }

  Option<Ref<T>> last() const
  {
    if (len() > 0)
    {
      return Option<Ref<T>>(Ref<T>(&get_unchecked(len() - 1)));
    }
    return Option<Ref<T>>();
  }

  Option<Ref<T>> first() const
  {
    if (len() > 0)
    {
      return Option<Ref<T>>(Ref<T>(&get_unchecked(0)));
    }
    return Option<Ref<T>>();
  }

  Option<RefMut<T>> first_mut()
  {
    if (len() > 0)
    {
      return Option<RefMut<T>>(RefMut<T>(&get_unchecked_mut(0)));
    }
    return Option<RefMut<T>>();
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
      throw panic_error("range end index " + to_string(end) + " out of range for slice of length " + to_string(len()));
    }

    return Slice<T>::from_raw_parts(begin() + start, end - start);
  }

  auto iter() const
  {
    auto start = begin();
    auto end = begin() + len();

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
        len());
  }

  auto iter_mut() const
  {
    auto start = begin();
    auto end = begin() + len();
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
        len());
  }

  void sort() requires std::totally_ordered<T>
  {
    std::ranges::stable_sort(begin(), begin() + len());
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
    return begin();
  }

protected:
  T* begin() const
  {
    return child_._begin();
  }

private:
  Child& child_;
};

template <typename T>
std::string to_string(const Slice<T>& slice)
{
  std::string s = "[";
  for (const auto& [i, v] : slice.iter().enumerate())
  {
    s += to_string(*v);
    if (i != slice.len() - 1)
    {
      s += ", ";
    }
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

template <typename T>
struct Vec : SliceInterface<Vec<T>, T>
{
  Vec(std::initializer_list<T> v) : v_(v){};
  Vec(const std::vector<T>& v) : v_(v){};
  Vec(std::vector<T>&& v) : v_(v){};

  const T* _begin() const
  {
    return v_.data();
  }
  T* _begin()
  {
    return v_.data();
  }
  usize _len() const
  {
    return v_.size();
  }

  // We could implement the full vector interface here, but mehh, we can make our vector act
  // like it is a std::vector by doing the following;
  operator std::vector<T>&()
  {
    return v_;
  }
  operator const std::vector<T>&() const
  {
    return v_;
  }

private:
  std::vector<T> v_;
};

template <typename T>
std::string to_string(const Vec<T>& v)
{
  std::string s = "[";
  for (const auto& [index, value] : v.iter().enumerate())
  {
    s += to_string(value);
    if (index != v.len() - 1)
    {
      s += ", ";
    }
  }
  s += "]";
  return s;
}

/// Make an Slice printable.
template <typename SS, typename T>
SS& operator<<(SS& os, const Vec<T>& v)
{
  os << to_string(v);
  return os;
}

}  // namespace detail

template <typename... T>
using Tuple = detail::Tuple<T...>;

template <typename T>
using Option = detail::Option<T>;

template <typename T>
using Slice = detail::Slice<T>;

template <typename T>
using Vec = detail::Vec<T>;

template <typename C>
auto slice(C& container) requires rust::DataSize<C>
{
  using SliceType = std::conditional_t<std::is_const_v<C>, const typename C::value_type, typename C::value_type>;
  return detail::Slice<SliceType>::from_raw_parts(container.data(), container.size());
}

template <typename C>
auto slice(const C& container) requires Borrowable<C>
{
  return Borrow<C>::borrow(container);
}

template <DataSize A>
struct IntoIterator<A>
{
  static auto into_iter(const A& c)
  {
    return slice(c).iter();
  }
};

template <typename C>
auto iter(const C& container)
{
  auto start = container.cbegin();
  auto end = container.cend();
  const auto size = container.size();
  return detail::make_iterator<Ref<typename C::value_type>>(start, end, size);
}

template <typename C>
auto iter_mut(C& container)
{
  auto start = container.begin();
  auto end = container.end();
  const auto size = container.size();
  return detail::make_iterator<RefMut<typename C::value_type>>(start, end, size);
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

template <typename A>
struct FromIterator<Vec<A>>
{
  template <typename It>
  static Vec<A> from_iter(It&& it)
  {
    std::vector<A> c = FromIterator<std::vector<A>>::from_iter(it);
    return Vec<A>(std::move(c));
  }
};

namespace prelude
{
// This approximates the rust std prelude.

using rust::Option;
using rust::Slice;
using rust::Tuple;
using rust::Unit;
using rust::Vec;

using rust::drain;
using rust::iter;
using rust::iter_mut;
using rust::slice;

using namespace rust::types;

// The index type for our tuple.
using detail::operator""_i;
}  // namespace prelude

namespace literals
{
using detail::operator""_i;
}  // namespace literals
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
