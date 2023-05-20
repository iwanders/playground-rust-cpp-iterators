#include <iostream>
#include <vector>

#include "rust_cpp_iterator.hpp"

template <typename T>
std::string type(T&& z)
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
  return 0;
}
