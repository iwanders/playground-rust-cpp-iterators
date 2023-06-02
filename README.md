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

```cpp
    std::vector<char> a{ 'H', 'e', 'l', 'l', 'o' };
    auto slice_hello = rs::slice(a);
    std::cout << "slice_hello: " << slice_hello << std::endl;
    const auto str = "Hell";
    auto slice_hell = rs::slice(str);
    std::cout << "slice_hell: " << slice_hell << std::endl;
    ASSERT_EQ(slice_hello.starts_with(slice_hell), true);
    {
      std::array<char, 3> arr_hel = { 'H', 'e', 'l' };
      ASSERT_EQ(slice_hello.starts_with(rs::slice(arr_hel)), true);
    }
    {
      std::string str_hel = "Hel";
      ASSERT_EQ(slice_hello({}, 3).starts_with(rs::slice(str_hel)), true);
    }
```
