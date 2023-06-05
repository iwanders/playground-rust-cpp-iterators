# Playground Rust C++ Iterators and more...


> **Warning**
> 
> Do not use this... it's just an experiment


An exploration how a C++ version of the [Rust standard library](https://doc.rust-lang.org/std/index.html#the-rust-standard-library) could work.
Mostly to use the C++20 concepts for the first time. Started creating `Iterators`, but expanded this into a few more types:
- Option
- Iterators
- Slices
- Tuple
- Vec
- Ref / RefMut: Used by iterators.

Traits:
- FromIter (used by `collect`)
- Borrow (used by `starts_with`)

None of these are feature complete, that would take a lot of work, they have just a few methods each to
see how it could work and what the semantics would be.

Examples are sort of ordered, but sometimes they use types / concepts introduced later.

## Option
An iterator's `.next()` returns an [`Option`](https://doc.rust-lang.org/std/option/enum.Option.html#), so we need that first.

```cpp
std::cout << "Option intro" << std::endl;
auto empty = rs::Option<int>();
std::cout << "empty: " << empty << std::endl;
// empty: None
ASSERT_EQ(empty.is_some(), false);
ASSERT_EQ(empty.is_none(), true);
auto value = rs::Option(3);
std::cout << "value: " << value << std::endl;
// value: Some(3)
ASSERT_EQ(value.is_some(), true);
std::cout << "Option<Ref<int>>: " << value.as_ref() << std::endl;
// Option<Ref<int>>: Some(3)
std::cout << "Ref<int>: " << value.as_ref().unwrap() << std::endl;
// Ref<int>: 3
std::cout << "int&: " << value.as_ref().unwrap().deref() << std::endl;
// int&: 3
std::cout << "unwrap into int: " << std::move(value).unwrap() << std::endl;
// unwrap into int: 3
```

Map on an `Option`:
```cpp
auto opt2 = rs::Option(3).map([](const auto& v) { return v * v; });
std::cout << opt2 << std::endl;
// Some(9)
ASSERT_EQ(opt2, rs::Option(9));

auto opt3 = rs::Option(3).map([](const auto& v) { return v + 0.5; });
std::cout << opt3 << std::endl;
// Some(3.500000)
ASSERT_EQ(opt3, rs::Option(3.5));
```
Or, [reproduce the map example](https://doc.rust-lang.org/std/option/enum.Option.html#method.map)
```cpp
auto maybe_some_string = rs::Option(std::string("Hello, World!"));
auto maybe_some_len = maybe_some_string.map([](auto v){return v.size();});
ASSERT_EQ(maybe_some_len, rs::Option<std::size_t>(13));
rs::Option<rs::Ref<std::string>> x;
ASSERT_EQ(x.map([](auto v){return (*v).size();}), rs::Option<std::size_t>());
```
Notice the difference in the last assert, Rust would have automatic dereferencing on `v`, but in
c++ we need `*v` to make it compile.


Apply [`and_then`](https://doc.rust-lang.org/std/option/enum.Option.html#method.and_then):

```cpp
const auto sq_then_to_string = [](rust::u32 x) -> rust::Option<std::string> {
  rust::u64 z = x;
  if ((z * z) > std::numeric_limits<rust::u32>::max()) { // checked_mul(x)...
    return rust::Option<std::string>();
  }
  return rust::Option<std::string>(std::to_string(static_cast<rust::u32>(z * z)));
};
ASSERT_EQ(rs::Option(2).and_then(sq_then_to_string), rust::Option(std::to_string(4)));
ASSERT_EQ(rs::Option(1000000).and_then(sq_then_to_string), rust::Option<std::string>());
ASSERT_EQ(rs::Option<rust::u32>().and_then(sq_then_to_string), rust::Option<std::string>());
```

## Iterators

```cpp
std::cout << "Start with the definition of an iterator" << std::endl;
// Start with the definition of an iterator
const std::vector<int> a{ 1, 2, 3 };
auto it = rs::iter(a);

std::cout << it.next() << std::endl;
// Some(1)
std::cout << it.next() << std::endl;
// Some(2)
std::cout << it.next() << std::endl;
// Some(3)
std::cout << it.next() << std::endl;
// None

std::cout << rs::Option<int>() << std::endl;
// None
```

Create an iterator from a vector, map it and sum it.
```cpp
const std::vector<int> a{ 1, 2, 3, 4 };
auto sum = rs::iter(a).map([](const auto& v) { return *v * *v; }).sum();
ASSERT_EQ(sum, 1 + 4 + 9 + 16);
```

Or a range based for loop over the iterator.
```cpp
const std::vector<int> a{ 1, 2, 3, 4 };
for (const auto& v : rs::iter(a).map([](const auto& v) { return *v * *v; }))
{
  std::cout << " " << v;
}
// 1 4 9 16
```

Enumerate on an iterator returns a tuple of values;
```cpp
std::vector<int> a{ 1, 2, 3};
for (const auto& [i, v] : rs::iter(a).enumerate())
{
  std::cout << "i: " << i << " -> " << *v << std::endl;
}
// i: 0 -> 1
// i: 1 -> 2
// i: 2 -> 3
```

Collect works and support implicit types:
```cpp
const std::vector<int> a{ 1, 2, 3, 4 };

// Return type idiom, type inferred from conversion to the type on the left
std::vector<int> and_back = rs::iter(a).copied().collect();
```

Or explicit types:
```cpp
const std::vector<int> a{ 1, 2, 3, 4 };
auto and_back = rs::iter(a).copied().collect<std::vector<float>>();
std::cout << rs::slice(and_back) << std::endl;
// [1.000000, 2.000000, 3.000000, 4.000000]
```

map operations can be chained.
```cpp
const std::vector<int> a{ 1, 2, 3 };
auto our_map_it = rs::iter(a)
                      .map([](const auto& v) { return static_cast<double>(*v); })
                      .map([](const auto& v) { return v * v + 0.5; });
auto and_back = our_map_it.collect<std::vector<float>>();
std::cout << rs::slice(and_back) << std::endl;
// [1.500000, 4.500000, 9.500000]
```

Support for terminators like `any()`;
```cpp
const std::vector<int> a{ 2, 4, 6 };
const auto has_even = rs::iter(a).any([](const auto& v) { return *v % 2 == 0; });
std::cout << "has_even:" << has_even << std::endl;
// has_even:1
const auto has_odd = rs::iter(a).any([](const auto& v) { return *v % 2 != 0; });
std::cout << "has_odd:" << has_odd << std::endl;
// has_odd:0
```

Zip returns a tuple, which can be destructured with a structured binding.
```cpp
  std::vector<int> a{ 1, 2, 3, 4 };
  std::vector<int> b{ 10, 20, 30, 40 };
  auto v = rust::iter(a)
               .zip(rust::iter(b))
               .map(
                   [](const auto& v)
                   {
                     const auto& [l, r] = v;
                     return *l + *r;
                   })
               .collect<std::vector<int>>();
  std::vector<int> expected{ 11, 22, 33, 44 };
  ASSERT_EQ(rust::slice(v), rust::slice(expected));
```

Or zip with any container directly, as long as they support `IntoIter`, so `iter(container).zip(container)`, using the `Tuple` indexing showed later.
```cpp
using namespace rust::literals;
std::vector<int> a{ 1, 2, 3, 4 };
std::vector<int> b{ 10, 20, 30, 40 };
// We can zip with a container (supports IntoIter).
auto v = rust::iter(a).zip(b).map([](const auto& v) { return *v[0_i] + *v[1_i]; }).collect<std::vector<int>>();
std::vector<int> expected{ 11, 22, 33, 44 };
ASSERT_EQ(rust::slice(v), rust::slice(expected));
```

Or zip with anything that acts like an iterator, for example the result of `map()`.
```cpp
using namespace rust::literals;
auto x = rust::iter(a)
             .copied() // This ensures we get a copy by value instead of a Ref.
             .zip(rust::iter(a).map([](const auto& v) { return *v * 10; }))
             .map([](const auto& v) { return v[0_i] + v[1_i]; })
             .collect<std::vector<int>>();
std::vector<int> expected{ 11, 22, 33, 44 };
ASSERT_EQ(rust::slice(x), rust::slice(expected));
```

Collecting into `Unit` or `void` runs the iterator to completion:
```cpp
std::cout << "Map on iter without return" << std::endl;
using namespace rust::prelude;
Vec<u8> a{ 0x61, 0x62, 0x63, 0x64 };
{
  int v = 0;
  auto it = a.iter().map([&v](const auto& x) { v += *x; });
  std::cout << type_string<decltype(it)::type>() << std::endl;
  ASSERT_EQ(v, 0);  // nothing happened yet, nothing is yield from the iterator.
  std::move(it).collect<Unit>();
  ASSERT_EQ(v, 0x61 + 0x62 + 0x63 + 0x64);
}
{
  int v = 0;
  auto it = a.iter().map([&v](const auto& x) { v += *x; });
  std::cout << type_string<decltype(it)::type>() << std::endl;
  ASSERT_EQ(v, 0);  // nothing happened yet, nothing is yield from the iterator.
  std::move(it).collect<void>();
  ASSERT_EQ(v, 0x61 + 0x62 + 0x63 + 0x64);
}
```


## Slices

Slices can be constructed from anything that has `.data()` and `.size()`, but also with the `Borrow` trait.

Slices support equality, printing and slices can be... sliced, just like in Rust:
```cpp
std::vector<int> a{ 1, 2, 3, 4 };
auto slice = rs::slice(a);
std::cout << slice << std::endl;
// [1, 2, 3, 4]
std::cout << "Slice len: " << slice.len() << std::endl;
// Slice len: 4
std::cout << "Slice[2]: " << slice[2] << std::endl;
// Slice[2]: 3

{
  // slice[2..]
  auto subslice = slice(2, {});
  ASSERT_EQ(subslice.len(), 2);
  ASSERT_EQ(subslice[0], 3);
  ASSERT_EQ(subslice[1], 4);
}
{
  // slice[..2]
  auto subslice = slice({}, 2);
  ASSERT_EQ(subslice.len(), 2);
  ASSERT_EQ(subslice[0], 1);
  ASSERT_EQ(subslice[1], 2);
}
{
  // slice[1..3]
  auto subslice = slice(1, 3);
  ASSERT_EQ(subslice.len(), 2);
  ASSERT_EQ(subslice[0], 2);
  ASSERT_EQ(subslice[1], 3);
}
```

They can be changed into an (mutable) iterator;
```cpp
std::vector<int> a{ 1, 2, 3, 4 };
auto slice = rs::slice(a);
for (auto& x : slice.iter_mut())
{
  *x = *x * *x;
}
std::vector<int> expected{ 1, 4, 9, 16 };
ASSERT_EQ(rs::slice(a), rs::slice(expected));
```

Support `sort()` or print, lets use an `std::array` for this one:
```cpp
std::array<int, 4> a{ 1, 4, 2, 3 };
auto slice = rs::slice(a);
std::cout << "s: " << slice << std::endl;
//s: [1, 4, 2, 3]
slice.sort();
std::cout << "s: " << slice << std::endl;
//s: [1, 2, 3, 4]
```

Example of using a slice method, like `starts_with()`, which works with any `Borrowable` as argument.
Of course, the slice itself can also be constructed from any container that has a contiguous values
in memory. The code for `starts_with` is pretty boring, but it makes for a great showcase of the
convenience, we use a string here, but this would work for any value that is `std::equality_comparable_with` the result of the `Borrow`.
```cpp

// Definition of starts_with is:

template <Borrowable BorrowableType>
bool starts_with(const BorrowableType& original)
    const requires std::equality_comparable_with<T, typename Borrow<BorrowableType>::type>
{
  const auto needle = Borrow<BorrowableType>::borrow(original);
  const auto n = needle.len();
  return len() >= n && needle == (*this)({}, n);
}

// So we can pass anything that is borrowable, and the borrowed result has a type that is
// comparable with our current type.

// Create the vector to test with.
std::vector<char> a{ 'H', 'e', 'l', 'l', 'o' };
auto slice_hello = rs::slice(a);

// starts_with with std::vector
{
  std::vector<char> vec_hel{ 'H', 'e', 'l' };
  ASSERT_EQ(slice_hello.starts_with(rs::slice(vec_hel)), true);
  ASSERT_EQ(slice_hello.starts_with(vec_hel), true);
}
// starts_with with std::array
{
  std::array<char, 3> arr_hel{ 'H', 'e', 'l' };
  ASSERT_EQ(slice_hello.starts_with(rs::slice(arr_hel)), true);
  ASSERT_EQ(slice_hello.starts_with(arr_hel), true);
}

// starts_with with std::string
{
  std::string str_hel{"Hel"};
  ASSERT_EQ(slice_hello.starts_with(rs::slice(str_hel)), true);
  ASSERT_EQ(slice_hello.starts_with(str_hel), true);
}

// starts_with with with c string.
{
  const char* hel = "Hel";
  ASSERT_EQ(slice_hello.starts_with(rs::slice(hel)), true);
  ASSERT_EQ(slice_hello.starts_with(hel), true);
}

// starts_with c array :|
{
  auto z = rs::slice("Hel");  // not actually const char*, instead char[4].
  // We strip the nullbyte for char arrays of known lengths, that way this works:
  ASSERT_EQ(slice_hello.starts_with("Hel"), true);
  // but obviously, for this we pay the price:
  const char foo[3] = { 'H', 'e', 'l' };
  ASSERT_EQ(rs::slice(foo).len(), 2);
  // There is no way to distinguish between a string literal and a const char[N].
}
```

## Tuple

Tuples are used by the `zip()` method, and for `size_hint()`... so we might as well explore what a nice tuple would look like, with `operator[]` with a compile time constant 'string' literal; `t[0_i]` indexing instead of `std::get<0>(t)`.
```cpp
using namespace rust::prelude;
const auto t = Tuple(3, 5.5);
std::cout << t << std::endl;
// (3, 5.500000)
std::cout << "First: " << t[0_i] << std::endl;
// First: 3
std::cout << "Second: " << t[1_i] << std::endl;
// Second: 5.5



auto t2 = Tuple(std::string("abc"), 1337.0);
t2[0_i] = "Hello";
std::cout << "First: " << t2[0_i] << std::endl;
// First: Hello
auto& [s, v] = t2;
s = "Nope";
std::cout << "s: " << s << ", "
          << "v:" << v << std::endl;
// s: Nope, v:1337

std::cout << t2 << std::endl;
// (Nope, 1337.000000)

// All structured bindings work same as std::tuple
```

## Vec

`Vec<T>` is a thin wrapper around `std::vector<T>` and doesn't support anything really, but it does incorporate the `SliceInterface`, so all slice methods can be
invoked on it without conversion. We implement `FromIterator` such that `collect()` works nicely.

And we make it convertible to a `std::vector<T>&` such that other functions can interact with `Vec<T>` as if it is a normal `std::vector`.

```cpp
using namespace rust::prelude;
Vec<u8> a{ 0x61, 0x62, 0x63, 0x64 };
Vec<char> b = a.iter().copied().map([](auto v) { return v - 0x20; }).collect();
std::cout << "b:" <<  b << std::endl;
// b:[65, 66, 67, 68]

// Vec<char> is collectable into a string.
std::string v = b.iter().collect();
std::cout << "v: " << v << std::endl;
// v: ABCD
std::cout << "a:" << a << std::endl;
// a:[97, 98, 99, 100]

// Since it supports all slice methods, we can also do starts_with.
ASSERT_EQ(a.starts_with("abc"), true);

// Last and first return Option<Ref<>> types;
ASSERT_EQ(a.last(), Option<rust::Ref<u8>>(&a[3]));
ASSERT_EQ(a.last().copied(), Option<u8>(0x64));
ASSERT_EQ(a.first(), Option<rust::Ref<u8>>(&a[0]));
ASSERT_EQ(a.first().copied(), Option<u8>(0x61));

// We can also get a mut reference;
//  a.first_mut().unwrap().deref() = 32;
// or, the safer;
a.first_mut().map([](auto v) { *v = 32; });
std::cout << "a:" << a << std::endl;
// a:[32, 98, 99, 100]
ASSERT_EQ(a.first().copied(), Option<u8>(32));

// Vec is convertible into std::vector<u8>&;
const auto use_stdvec = [](std::vector<u8>& v) { v.front() = 33; };
use_stdvec(a);
ASSERT_EQ(a.first().copied(), Option<u8>(33));
ASSERT_EQ(a[0], 33);
std::cout << a.first() << std::endl;
// Some(33)

// And into const vec
const auto use_conststdvec = [](const std::vector<u8>& v) {};
use_conststdvec(a);
```


