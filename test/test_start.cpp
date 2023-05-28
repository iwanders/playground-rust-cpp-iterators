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
#include <cmath>
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
    std::cout << "Start with the definition of an iterator" << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto it = rs::iter(a);

    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;

    std::cout << rs::Option<int>() << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << "Test the option a bit" << std::endl;
    {
      // This shouldn't compile, get on an rvalue is invalid.
      //  auto& z = rs::Option(3).get();
      // But this should:
      auto opt = rs::Option(3);
      auto& z = opt.get();
      auto x = std::move(opt).unwrap();
    }
  }

  {
    auto opt = rs::Option<int>();
    try
    {
      auto bad = std::move(opt).unwrap();
    }
    catch (const rs::panic_error& e)
    {
      std::cout << e.what() << std::endl;
    }
    std::cout << std::endl;
  }

  {
    auto opt = rs::Option(3);
    auto& [x] = opt;
    std::cout << "x: " << x << std::endl;
  }

  {
    auto opt = rs::Option(3);
    auto [x] = opt;
    std::cout << "x: " << x << std::endl;
  }

  {
    auto [x] = rs::Option(3);
    std::cout << "x: " << x << std::endl;
  }

  {
    auto opt = rs::Option(3);
    if (decltype(opt)::type v; opt.Some(v))
    {
      std::cout << "Opt was: " << v << std::endl;
    }
    if (double v; rs::Option(3.3).Some(v))
    {
      std::cout << "Opt was: " << v << std::endl;
    }
  }

  {
    auto opt = rs::Option<int>();
    if (int v; opt.Some(v))
    {
      std::cout << "Opt was: " << v << std::endl;
    }
    else
    {
      std::cout << "Opt was none. " << std::endl;
    }
  }

  {
    std::cout << "Check that we can map on an option" << std::endl;
    // Option map...
    auto opt1 = rs::Option(3);
    auto opt2 = opt1.map([](const auto& v) { return v * v; });
    std::cout << opt2 << std::endl;

    auto opt3 = opt2.map([](const auto& v) { return v + 0.5; });
    std::cout << opt3 << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << "Check we can make a mapped iterator" << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto it = rs::iter(a).map([](const auto& v) -> int { return v * v; });
    std::cout << type_string<decltype(it)::type>() << std::endl;
    std::cout << type_string<decltype(it)::function_type>() << std::endl;

    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << "Check if we can collect into an inferred type " << std::endl;

    const auto print_vec = [](const std::vector<int>& c)
    {
      for (auto& v : c)
      {
        std::cout << v << ", ";
      }
      std::cout << std::endl;
    };

    const std::vector<int> a{ 1, 2, 3, 4 };

    // Return type idiom, type inferred from conversion to the type on the left
    std::vector<int> and_back = rs::iter(a).collect();
    // print it.
    print_vec(and_back);

    // type infered from the conversion needed to make the argument work.
    print_vec(rs::iter(a).collect());
  }

  {
    std::cout << "Check if we can collect into an explicit type." << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto and_back = rs::iter(a).collect<std::vector<float>>();
    for (auto& v : and_back)
    {
      std::cout << v << ", ";
    }
    std::cout << std::endl;
  }

  {
    std::cout << "Check if we can chain maps and collects." << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto our_map_it = rs::iter(a)
                          .map([](const auto& v) { return static_cast<double>(v); })
                          .map([](const auto& v) { return v * v + 0.5; })
                          .map([](const auto& x) { return std::sqrt(x); });
    std::cout << "here be dragons: ";
    std::cout << type_string<decltype(our_map_it)::function_type>() << std::endl;
    auto and_back = std::move(our_map_it).collect<std::vector<float>>();
    for (auto& v : and_back)
    {
      std::cout << v << std::endl;
    }
    std::cout << std::endl;
  }

  {
    std::cout << "Check if sum works" << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto sum = rs::iter(a).map([](const auto& v) { return v * v; }).sum();
    std::cout << sum << std::endl;
  }

  {
    std::cout << "Check if range based for loop works." << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    for (const auto& v : rs::iter(a).map([](const auto& v) { return v * v; }))
    {
      std::cout << " " << v;
    }
    std::cout << std::endl;
  }

  {
    std::cout << "Check any value is odd." << std::endl;
    const std::vector<int> a{ 2, 4, 6 };
    const auto has_even = rs::iter(a).any([](const auto& v) { return v % 2 == 0; });
    std::cout << "has_even:" << has_even << std::endl;
    const auto has_odd = rs::iter(a).any([](const auto& v) { return v % 2 != 0; });
    std::cout << "has_odd:" << has_odd << std::endl;
    std::cout << std::endl;
  }

  return 0;
}
