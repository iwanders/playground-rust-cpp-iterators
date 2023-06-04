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

template <typename T>
void print_vector(const std::vector<T>& c)
{
  for (const auto& v : c)
  {
    std::cout << v << ", ";
  }
  std::cout << std::endl;
};

#define ASSERT_EQ(a, b)                                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    const auto a_ = a;                                                                                                 \
    const auto b_ = b;                                                                                                 \
    if (!(a_ == b_))                                                                                                   \
    {                                                                                                                  \
      std::cerr << __FILE__ << ":" << __LINE__ << " test failed: a != b (a:" << a_ << ", b:" << b_ << ")"              \
                << std::endl;                                                                                          \
      std::exit(1);                                                                                                    \
    }                                                                                                                  \
  } while (0)

#define ASSERT_NE(a, b)                                                                                                \
  do                                                                                                                   \
  {                                                                                                                    \
    const auto a_ = a;                                                                                                 \
    const auto b_ = b;                                                                                                 \
    if (!(a_ != b_))                                                                                                   \
    {                                                                                                                  \
      std::cerr << __FILE__ << ":" << __LINE__ << " test failed: a == b (a:" << a_ << ", b:" << b_ << ")"              \
                << std::endl;                                                                                          \
      std::exit(1);                                                                                                    \
    }                                                                                                                  \
  } while (0)

int main(int argc, char* argv[])
{
  namespace rs = rust;

  {
    std::cout << "Start with the definition of an iterator" << std::endl;
    const std::vector<int> a{ 1, 2, 3 };
    auto it = rs::iter(a);

    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;
    std::cout << it.next() << std::endl;

    std::cout << rs::Option<int>() << std::endl;
    std::cout << std::endl;
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
    ASSERT_EQ(opt3, rs::Option(9.5));
  }

  {
    const auto text = rs::Option(std::string("Hello World!"));
    rs::Option<rs::usize> text_length = text.as_ref().map([](const auto& v) { return (*v).size(); });
    ASSERT_EQ(std::move(text_length).unwrap(), 12);
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

    const std::vector<int> a{ 1, 2, 3, 4 };

    const auto print_vec = [](const std::vector<int>& v) { print_vector(v); };
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
    auto and_back = rs::iter(a).collect<std::vector<float>>();
    std::cout << rs::slice(and_back) << std::endl;

    std::cout << std::endl;
  }

  {
    std::cout << "Check if we can chain maps and collects." << std::endl;
    const std::vector<int> a{ 1, 2, 3 };
    auto our_map_it = rs::iter(a)
                          .map([](const auto& v) { return static_cast<double>(*v); })
                          .map([](const auto& v) { return v * v + 0.5; });
    std::cout << "here be dragons: ";
    std::cout << type_string<decltype(our_map_it)::function_type>() << std::endl;
    auto and_back = std::move(our_map_it).collect<std::vector<float>>();
    std::cout << rs::slice(and_back) << std::endl;
    std::cout << std::endl;
  }

  {
    std::cout << "Check if sum works" << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    auto sum = rs::iter(a).map([](const auto& v) { return *v * *v; }).sum();
    ASSERT_EQ(sum, 1 + 4 + 9 + 16);
    std::cout << sum << std::endl;
  }

  {
    std::cout << "Check if range based for loop works." << std::endl;
    const std::vector<int> a{ 1, 2, 3, 4 };
    for (const auto& v : rs::iter(a).map([](const auto& v) { return *v * *v; }))
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
    const auto expected = std::vector<int>{ 1, 4, 9, 16 };
    ASSERT_EQ(rs::slice(a), rs::slice(expected));
    std::cout << std::endl;
  }

  {
    std::cout << "Check enumerate." << std::endl;
    std::vector<int> a{ 1, 2, 3 };
    for (const auto& [i, v] : rs::iter(a).enumerate())
    {
      std::cout << "i: " << i << " -> " << *v << std::endl;
    }
    std::cout << std::endl;
  }

  {
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
  }

  {
    using namespace rust::literals;
    std::vector<int> a{ 1, 2, 3, 4 };
    std::vector<int> b{ 10, 20, 30, 40 };
    // We can zip with a container (supports IntoIter).
    auto v = rust::iter(a).zip(b).map([](const auto& v) { return *v[0_i] + *v[1_i]; }).collect<std::vector<int>>();
    std::vector<int> expected{ 11, 22, 33, 44 };
    ASSERT_EQ(rust::slice(v), rust::slice(expected));

    // And of course, we can also zip with an iterator.
    auto x = rust::iter(a)
                 .copied()
                 .zip(rust::iter(a).map([](const auto& v) { return *v * 10; }))
                 .map([](const auto& v) { return v[0_i] + v[1_i]; })
                 .collect<std::vector<int>>();
    ASSERT_EQ(rust::slice(x), rust::slice(expected));
  }

  {
    std::cout << "Check any value is odd." << std::endl;
    const std::vector<int> a{ 2, 4, 6 };
    const auto has_even = rs::iter(a).any([](const auto& v) { return *v % 2 == 0; });
    std::cout << "has_even:" << has_even << std::endl;
    const auto has_odd = rs::iter(a).any([](const auto& v) { return *v % 2 != 0; });
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

    auto z = rs::drain(std::vector<int>{ 1, 2, 3, 4 })
                 .map([](const auto& v) { return v * v; })
                 .collect<std::vector<float>>();
    for (auto& x : z)
    {
      std::cout << " " << x << std::endl;
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
    for (auto& x : slice.iter_mut())
    {
      *x = *x * *x;
    }
    std::vector<int> expected{ 1, 4, 9, 16 };
    ASSERT_EQ(rs::slice(a), rs::slice(expected));
    print_vector(a);
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

  std::cout << type_string<rs::RefWrapper<int>>() << std::endl;
  std::cout << type_string<rs::RefWrapper<int&>>() << std::endl;
  std::cout << type_string<rs::RefWrapper<const int>>() << std::endl;
  std::cout << type_string<rs::RefWrapper<const int*>>() << std::endl;
  std::cout << type_string<rs::RefWrapper<const int* const>>() << std::endl;

  // Test our tuple, which is nice... printable, indexable, etc etc.
  {
    std::cout << "Tuple stuff " << std::endl;
    using namespace rust::prelude;
    const auto t = Tuple(3, 5.5);
    std::cout << t << std::endl;
    std::cout << "First: " << t[0_i] << std::endl;
    std::cout << "Second: " << t[1_i] << std::endl;

    auto t2 = Tuple(std::string("abc"), 1337.0);
    t2[0_i] = "Hello";
    std::cout << "First: " << t2[0_i] << std::endl;
    auto& [s, v] = t2;
    s = "Nope";
    std::cout << "s: " << s << ", "
              << "v:" << v << std::endl;
    std::cout << t2 << std::endl;

    std::cout << "end  Tuple stuff " << std::endl;

    // Test all possible permutations.
    {
      const Tuple<double, int> r_t = Tuple(3.3, 5);
      const std::tuple<double, int> s_t = std::make_tuple(3.3, 5);
      {
        const auto& [ra, rb] = r_t;
        const auto& [sa, sb] = s_t;
        //  std::cout << "ra: " << type_string<decltype(ra)>() << std::endl;
        //  std::cout << "sa: " << type_string<decltype(sa)>() << std::endl;
        ASSERT_EQ((std::is_same<decltype(ra), const double>::value), true);
        ASSERT_EQ((std::is_same<decltype(rb), const int>::value), true);
        ASSERT_EQ((std::is_same<decltype(sa), const double>::value), true);
        ASSERT_EQ((std::is_same<decltype(sb), const int>::value), true);

        ASSERT_EQ(r_t[0_i], 3.3);
        ASSERT_EQ(r_t[1_i], 5);
      }
      {
        const auto [ra, rb] = r_t;
        const auto [sa, sb] = s_t;
        //  std::cout << "ra: " << type_string<decltype(ra)>() << std::endl;
        //  std::cout << "sa: " << type_string<decltype(sa)>() << std::endl;

        ASSERT_EQ((std::is_same<decltype(ra), const double>::value), true);
        ASSERT_EQ((std::is_same<decltype(rb), const int>::value), true);
        ASSERT_EQ((std::is_same<decltype(sa), const double>::value), true);
        ASSERT_EQ((std::is_same<decltype(sb), const int>::value), true);
      }
    }

    {
      Tuple<double, int> r_t = Tuple(3.3, 5);
      std::tuple<double, int> s_t = std::make_tuple(3.3, 5);
      {
        auto& [ra, rb] = r_t;
        auto& [sa, sb] = s_t;
        std::cout << "ra: " << type_string<decltype(ra)>() << std::endl;
        std::cout << "sa: " << type_string<decltype(sa)>() << std::endl;
        ASSERT_EQ((std::is_same<decltype(ra), double>::value), true);
        ASSERT_EQ((std::is_same<decltype(rb), int>::value), true);
        ASSERT_EQ((std::is_same<decltype(sa), double>::value), true);
        ASSERT_EQ((std::is_same<decltype(sb), int>::value), true);

        ra = 7.5;
        sa = 7.5;
        ASSERT_EQ(std::get<0>(r_t), 7.5);
        ASSERT_EQ(r_t[0_i], 7.5);
        ASSERT_EQ(std::get<0>(s_t), 7.5);
        ASSERT_EQ(std::get<1>(s_t), 5);

        r_t[0_i] = 10.1;
        r_t[1_i] = 1;
        ASSERT_EQ(r_t[0_i], 10.1);
        ASSERT_EQ(r_t[1_i], 1);
      }

      {
        auto [ra, rb] = r_t;
        auto [sa, sb] = s_t;
        std::cout << "ra: " << type_string<decltype(ra)>() << std::endl;
        std::cout << "sa: " << type_string<decltype(sa)>() << std::endl;

        ASSERT_EQ((std::is_same<decltype(ra), double>::value), true);
        ASSERT_EQ((std::is_same<decltype(rb), int>::value), true);
        ASSERT_EQ((std::is_same<decltype(sa), double>::value), true);
        ASSERT_EQ((std::is_same<decltype(sb), int>::value), true);
        ra = 7.5;
        sa = 7.5;

        // Values should be unmodified from the previous block.
        ASSERT_EQ(r_t[0_i], 10.1);
        ASSERT_EQ(r_t[1_i], 1);
        ASSERT_EQ(std::get<0>(s_t), 7.5);
        ASSERT_EQ(std::get<1>(s_t), 5);
      }
    }
  }

  {
    using namespace rust::prelude;
    std::vector<u32> a{ 1, 2, 3, 4 };
    std::vector<u8> b{ 0x30, 0x30, 0x30, 0x30 };
    auto s = iter(a).map([](const auto& v) { return (*v) * 2; }).collect<std::vector<f32>>();
  }

  // Test our Vec, which implements the slice interface, not too sure if this is brilliant
  // but hey... at least it works.
  {
    using namespace rust::prelude;
    Vec<u8> a{ 0x61, 0x62, 0x63, 0x64 };
    Vec<char> b = a.iter().copied().map([](auto v) { return v - 0x20; }).collect();
    std::cout << "b:" << b << std::endl;

    // Vec<char> is collectable into a string.
    std::string v = b.iter().collect();
    std::cout << "v: " << v << std::endl;
    std::cout << "a:" << a << std::endl;

    // Since it supports all slice methods, we can also do starts_with.
    ASSERT_EQ(a.starts_with("abc"), true);

    // Last and first return Option<Ref<>> types;
    ASSERT_EQ(a.last(), Option<rust::Ref<u8>>(&a[3]));
    ASSERT_EQ(a.last().copied(), Option<u8>(0x64));
    ASSERT_EQ(a.first(), Option<rust::Ref<u8>>(&a[0]));
    ASSERT_EQ(a.first().copied(), Option<u8>(0x61));

    // We can also get a mut reference;
    //  a.first_mut().unwrap().deref() = 32;
    a.first_mut().map([](auto v) { *v = 32; });

    std::cout << "a:" << a << std::endl;
    ASSERT_EQ(a.first().copied(), Option<u8>(32));

    // Vec is convertible into std::vector<u8>&;
    const auto use_stdvec = [](std::vector<u8>& v) { v.front() = 33; };
    use_stdvec(a);
    ASSERT_EQ(a.first().copied(), Option<u8>(33));
    ASSERT_EQ(a[0], 33);
    ASSERT_EQ(a[3], 0x64);
    std::cout << a.first() << std::endl;

    // And into const vec
    const auto use_conststdvec = [](const std::vector<u8>& v) {};
    use_conststdvec(a);
  }

  return 0;
}
