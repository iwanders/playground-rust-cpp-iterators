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
#include <compare>
#include <iostream>
#include <vector>

#include "rust_cpp_iterator.hpp"

template <typename T>
std::string type_string()
{
  return __PRETTY_FUNCTION__;
}

#define ASSERT_EQ(a, b)                                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!(a == b))                                                                                                     \
    {                                                                                                                  \
      std::cerr << __FILE__ << ":" << __LINE__ << " test failed: a != b (a:" << a << ", b:" << b << ")" << std::endl;  \
      std::exit(1);                                                                                                    \
    }                                                                                                                  \
  } while (0)

#define ASSERT_NE(a, b)                                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!(a != b))                                                                                                     \
    {                                                                                                                  \
      std::cerr << __FILE__ << ":" << __LINE__ << " test failed: a == b (a:" << a << ", b:" << b << ")" << std::endl;  \
      std::exit(1);                                                                                                    \
    }                                                                                                                  \
  } while (0)

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
    auto it = rs::iter(a).map([](const auto& v) -> int { return *v * *v; });
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
    std::vector<int> and_back = rs::iter(a).copied().collect();
    // print it.
    print_vec(and_back);

    // type infered from the conversion needed to make the argument work.
    print_vec(rs::iter(a).copied().collect());
  }

  {
    std::cout << "Check if we can collect into an explicit type." << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto and_back = rs::iter(a).copied().collect<std::vector<float>>();
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
                          .map([](const auto& v) { return static_cast<double>(*v); })
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
    auto sum = rs::iter(a).map([](const int* v) { return *v * *v; }).sum();
    std::cout << sum << std::endl;
  }

  {
    std::cout << "Check if range based for loop works." << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    for (const auto& v : rs::iter(a).map([](const int* v) { return *v * *v; }))
    {
      std::cout << " " << v;
    }
    std::cout << std::endl;
  }

  {
    std::cout << "Check if range based for reference works." << std::endl;
    std::vector<int> a{ 1, 2, 3, 4 };
    for (auto& v : rs::iter_mut(a))
    {
      *v = *v * *v;
      std::cout << " " << *v;
    }
    std::cout << std::endl;
  }

  {
    std::cout << "Check enumerate." << std::endl;
    std::vector<int> a{ 1, 2, 3, 4 };
    for (const auto& [i, v] : rs::iter(a).enumerate())
    {
      std::cout << "i: " << i << " -> " << *v << std::endl;
      ;
    }
    std::cout << std::endl;
  }

  {
    std::cout << "Check any value is odd." << std::endl;
    const std::vector<int> a{ 2, 4, 6 };
    const auto has_even = rs::iter(a).any([](const int* v) { return *v % 2 == 0; });
    std::cout << "has_even:" << has_even << std::endl;
    const auto has_odd = rs::iter(a).any([](const int* v) { return *v % 2 == 0; });
    std::cout << "has_odd:" << has_odd << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << "Drain should yield values." << std::endl;
    {
      std::vector<int> _test = rs::drain(std::vector<int>{ 1, 2, 3, 4 }).collect<std::vector<int>>();
      std::vector<int> _test2 = rs::drain(std::vector<int>{ 1, 2, 3, 4 }).collect();
    }

    auto z = rs::drain(std::vector<int>{ 1, 2, 3, 4 });
    //  std::cout << type_string<decltype(z)::function_type>() << std::endl;
    std::cout << "z.next()" << z.next() << std::endl;
    std::cout << "z.next()" << z.next() << std::endl;
    std::cout << "z.next()" << z.next() << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << "Drain into map." << std::endl;

    auto z =
        rs::drain(std::vector<int>{ 1, 2, 3, 4 }).map([](const int& v) { return v * v; }).collect<std::vector<float>>();
    for (auto& x : z)
    {
      std::cout << " " << x << std::endl;
      ;
    }
    std::cout << std::endl;
  }

  {
    struct S
    {
    };  // clearly not supporting Add
    const std::vector<S> a{ S{}, S{} };
    //  auto sum = rs::iter(a).sum();
  }

  {
    std::cout << "Check if range based for reference works." << std::endl;
    std::vector<int> a{ 1, 2, 3, 4 };
    auto slice = rs::slice(a);
    std::cout << "Slice len: " << slice.len() << std::endl;
    std::cout << "Slice[2]: " << slice[2] << std::endl;
    std::cout << std::endl;

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
  }

  {
    std::cout << "Check if iter over slice works" << std::endl;
    std::vector<int> a{ 1, 2, 3, 4 };
    auto slice = rs::slice(a);
    for (const auto& x : slice.iter())
    {
      std::cout << " " << *x;
    }
    std::cout << std::endl;
  }

  {
    std::cout << "Non const iter!" << std::endl;
    std::vector<int> a{ 1, 2, 3, 4 };
    auto slice = rs::slice(a);
    for (auto* x : slice.iter_mut())
    {
      *x = *x * *x;
    }
    std::cout << "s: " << slice << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << "Check if sorting slice works" << std::endl;
    std::vector<int> a{ 1, 4, 2, 3 };
    auto slice = rs::slice(a);
    std::cout << "s: " << slice << std::endl;
    slice.sort();
    std::cout << "s: " << slice << std::endl;

    std::cout << std::endl;
  }
  {
    std::cout << "Check if slice comparison slice works" << std::endl;
    std::vector<int> a{ 1, 4, 2, 3 };
    auto slice_a = rs::slice(a);
    slice_a.sort();
    std::vector<int> b{ 1, 2, 3, 4 };
    auto slice_b = rs::slice(b);
    ASSERT_EQ(slice_a, slice_b);
    ASSERT_NE(slice_a, slice_b({}, 3));
    ASSERT_EQ(slice_a({}, 3), slice_b({}, 3));

    auto first_half = std::vector<int>{ 1, 2 };
    auto second_half = std::vector<int>{ 3, 4 };
    auto slice_first = rs::slice(first_half);
    auto slice_second = rs::slice(second_half);
    ASSERT_EQ(slice_a.starts_with(slice_first), true);
    ASSERT_EQ(slice_a.starts_with(slice_second), false);
    ASSERT_EQ(slice_a(2, {}).starts_with(slice_second), true);
    std::cout << std::endl;
  }

  {
    std::cout << "Check if slice comparison slice works" << std::endl;
    std::vector<char> a{ 'H', 'e', 'l', 'l', 'o' };
    auto slice_hello = rs::slice(a);
    std::cout << "slice_hello: " << slice_hello << std::endl;

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
      std::string str_hel{ "Hel" };
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
    // This is problematic, because we can't see the difference between a c array of chars and a
    // string literal, they are both a fixed length char array. With a string literal it also
    // contains the nullbyte at the end, so doing a .starts_with("abc") has a 4 long char array of
    // {'a', 'b', 'c', 0}.
    // Since C arrays are more rare than string literals, we chose to remove the nullbyte at the end
    // of a string literal.
    {
      auto z = rs::slice("Hel");  // not actually const char*, instead char[4].
      std::cout << "z has null byte: " << z << std::endl;

      // Explicit C array initialisation
      const char foo[3] = { 'H', 'e', 'l' };
      ASSERT_EQ(rs::slice(foo).len(), 2);

      // So this fails, since it compaires "Hel\0"
      ASSERT_EQ(slice_hello.starts_with("Hel"), true);
      // Or this fails, since it compares "He"
      //  ASSERT_EQ(slice_hello.starts_with(foo), true);
    }
  }

  return 0;
}
