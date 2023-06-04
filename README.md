# Playground Rust C++ Iterators and more...


> **Warning**
> 
> Do not use this... it's just an experiment


Some dabbling around exploring how Rust style iterators could work in C++ and learning to use C++20
concepts instead of SFINAE.

And a bit more to prototype what a more-Rust-like standard library for C++ would look like.
- Slices
- Tuple
- Vec

## Iterators
```cpp
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto sum = rs::iter(a).map([](const auto& v) { return *v * *v; }).sum();
    ASSERT_EQ(sum, 1 + 4 + 9 + 16);
```

```cpp
    const std::vector<int> a{ 1, 2, 3, 4 };
    for (const auto& v : rs::iter(a).map([](const int* v) { return *v * *v; }))
    {
      std::cout << " " << v;
    }
    // 1 4 9 16
```

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

```cpp

    const std::vector<int> a{ 1, 2, 3, 4 };

    // Return type idiom, type inferred from conversion to the type on the left
    std::vector<int> and_back = rs::iter(a).copied().collect();
```
```cpp
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto and_back = rs::iter(a).copied().collect<std::vector<float>>();
```

```cpp
    const std::vector<int> a{ 1, 2, 3 };
    auto our_map_it = rs::iter(a)
                          .map([](const auto& v) { return static_cast<double>(*v); })
                          .map([](const auto& v) { return v * v + 0.5; });
    auto and_back = our_map_it.collect<std::vector<float>>();
    std::cout << rs::slice(and_back) << std::endl;
    // [1.500000, 4.500000, 9.500000]
```

```cpp
    const std::vector<int> a{ 2, 4, 6 };
    const auto has_even = rs::iter(a).any([](const auto& v) { return *v % 2 == 0; });
    std::cout << "has_even:" << has_even << std::endl;
    const auto has_odd = rs::iter(a).any([](const auto& v) { return *v % 2 != 0; });
    std::cout << "has_odd:" << has_odd << std::endl;
    // has_even:1
    // has_odd:0
```

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

Or zip with a container that supports `IntoIter`
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
                 .copied()
                 .zip(rust::iter(a).map([](const auto& v) { return *v * 10; }))
                 .map([](const auto& v) { return v[0_i] + v[1_i]; })
                 .collect<std::vector<int>>();
    ASSERT_EQ(rust::slice(x), rust::slice(expected));
```


## Slices

```cpp
    std::vector<int> a{ 1, 2, 3, 4 };
    auto slice = rs::slice(a);

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

```cpp
    std::vector<int> a{ 1, 4, 2, 3 };
    auto slice = rs::slice(a);
    std::cout << "s: " << slice << std::endl;
    slice.sort();
    std::cout << "s: " << slice << std::endl;
    //s: [1, 4, 2, 3]
    //s: [1, 2, 3, 4]

```

Example of using a slice method, like `starts_with`.
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

Tuples are used by the `zip()` method, and for `size_hint()`... so we might as well explore what a nice tuple would look like, with `t[0_1]` indexing instead of `std::get<0>(t)`. And lets make it printable, because developers print stuff.
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
invoked on it without conversion. And we make it convertible to a `std::vector<T>&`
such that other functions can interact with `Vec<T>` as if it is a normal `std::vector`.

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
    a.first_mut().unwrap().deref() = 32;
    std::cout << "a:" << a << std::endl;
    // a:[32, 98, 99, 100]
    ASSERT_EQ(a.first().copied(), Option<u8>(32));

    // Vec is convertible into std::vector<u8>&;
    const auto use_stdvec = [](std::vector<u8>& v) { v.front() = 33; };
    use_stdvec(a);
    ASSERT_EQ(a.first().copied(), Option<u8>(33));
    std::cout << a.first() << std::endl;
    // Some(33)

    // And into const vec
    const auto use_conststdvec = [](const std::vector<u8>& v) {};
    use_conststdvec(a);
```



