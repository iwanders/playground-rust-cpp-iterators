# Playground Rust C++ Iterators


> **Warning**
> Do not use this... it's just an experiment.


Some dabbling around exploring how Rust style iterators could work in C++.

```cpp
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto sum = rs::iter(a).map([](const auto& v) { return v * v; }).sum();
```

```cpp
    const std::vector<int> a{ 1, 2, 3, 4 };
    for (const auto& v : rs::iter(a).map([](const auto& v) { return v * v; }))
    {
      std::cout << " " << v;
    }
```

```cpp

    const std::vector<int> a{ 1, 2, 3, 4 };

    // Return type idiom, type inferred from conversion to the type on the left
    std::vector<int> and_back = rs::iter(a).collect();
```

```cpp
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto our_map_it = rs::iter(a)
                          .map([](const auto& v) { return static_cast<double>(v); })
                          .map([](const auto& v) { return v * v + 0.5; })
                          .map([](const auto& x) { return std::sqrt(x); });
    auto and_back = our_map_it.collect<std::vector<float>>();
```
