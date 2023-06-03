# Playground Rust C++ Iterators


> **Warning**
> Do not use this... it's just an experiment.


Some dabbling around exploring how Rust style iterators could work in C++ and learning to use C++20
concepts instead of SFINAE

## Iterators
```cpp
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto sum = rs::iter(a).map([](const int* v) { return *v * *v; }).sum();
```

```cpp
    const std::vector<int> a{ 1, 2, 3, 4 };
    for (const auto& v : rs::iter(a).map([](const int* v) { return *v * *v; }))
    {
      std::cout << " " << v;
    }
```
```cpp
    std::vector<int> a{ 1, 2, 3, 4 };
    for (const auto& [i, v] : rs::iter(a).enumerate())
    {
      std::cout << "i: " << i << " -> " << *v << std::endl;
      ;
    }
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
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto our_map_it = rs::iter(a)
                          .map([](const auto& v) { return static_cast<double>(*v); })
                          .map([](const auto& v) { return v * v + 0.5; })
                          .map([](const auto& x) { return std::sqrt(x); });
    auto and_back = our_map_it.collect<std::vector<float>>();
```

```cpp
    const auto has_even = rs::iter(a).any([](const int* v) { return *v % 2 == 0; });
    std::cout << "has_even:" << has_even << std::endl;
    const auto has_odd = rs::iter(a).any([](const int* v) { return *v % 2 == 0; });
    std::cout << "has_odd:" << has_odd << std::endl;
```

```cpp

    auto z =
        rs::drain(std::vector<int>{ 1, 2, 3, 4 }).map([](const int& v) { return v * v; }).collect<std::vector<float>>();
```

## Slices

```cpp
    std::vector<int> a{ 1, 2, 3, 4 };
    auto slice = rs::slice(a);
    // slice[2..]
    auto subslice = slice(2, {});
    ASSERT_EQ(subslice.len(), 2);
    ASSERT_EQ(subslice[0], 3);
    ASSERT_EQ(subslice[1], 4);
```

```cpp
    std::vector<int> a{ 1, 2, 3, 4 };
    auto slice = rs::slice(a);
    for (auto* x : slice.iter_mut())
    {
      *x = *x * *x;
    }
    std::cout << "s: " << slice << std::endl;
```

Example of using a slice method, like `starts_with`.
```cpp

    std::vector<char> a{ 'H', 'e', 'l', 'l', 'o' };
    auto slice_hello = rs::slice(a);

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
