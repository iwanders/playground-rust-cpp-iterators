#include <iostream>
#include <vector>

#include "rust_cpp_iterator.hpp"

template <typename T>
std::string type_string()
{
  return __PRETTY_FUNCTION__;
}

int main(int argc, char* argv[])
{
  namespace rs = rust;
  {
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto it = rs::iter(a);

    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;

    std::cout << rs::Option<int>() << std::endl;
  }

  {
    // This shouldn't compile.
    //  auto& z = rs::Option(3).get();
    // But this should:
    auto opt = rs::Option(3);
    auto& z = opt.get();
  }

  {
    // Option map...
    auto opt1 = rs::Option(3);
    auto opt2 = opt1.map([](const auto& v) { return v * v; });
    std::cout << opt2 << std::endl;

    auto opt3 = opt2.map([](const auto& v) { return v + 0.5; });
    std::cout << opt3 << std::endl;
  }

  {
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto it = rs::iter(a).map([](const auto& v) -> int { return v * v; });
    std::cout << type_string<decltype(it)::type>() << std::endl;
    std::cout << type_string<decltype(it)::function_type>() << std::endl;

    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
  }

  {
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto it = rs::iter(a);
    auto mapped_it = it.map([](const auto& v) -> int { return v * v; });
    std::cout << type_string<decltype(it)::type>() << std::endl;
    std::cout << type_string<decltype(it)::function_type>() << std::endl;

    std::cout << mapped_it.next() << std::endl;
    std::cout << mapped_it.next() << std::endl;
    std::cout << mapped_it.next() << std::endl;
    std::cout << mapped_it.next() << std::endl;
    std::cout << mapped_it.next() << std::endl;
  }

  {
    const std::vector<int> a{ 1, 2, 3, 4 };
    std::vector<int> and_back = rs::iter(a).collect();
    for (auto& v : and_back) {
      std::cout << v << std::endl;
    }
  }

  {
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto and_back = rs::iter(a).collect<std::vector<float>>();
    for (auto& v : and_back) {
      std::cout << v << std::endl;
    }
  }

  return 0;
}
