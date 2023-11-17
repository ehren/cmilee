
#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <cassert>
#include <compare> // for <=>
#include <thread>
#include <optional>

//#include <concepts>
//#include <ranges>
//#include <numeric>


#include "ceto.h"

template <typename _ceto_private_C1, typename _ceto_private_C2, typename _ceto_private_C3>struct Foo : public ceto::enable_shared_from_this_base_for_templates {

    _ceto_private_C1 a;

    _ceto_private_C2 b;

    _ceto_private_C3 c;

        template <typename T1>
auto bar(const T1& x) const -> void {
            ((std::cout << "bar: ") << x) << std::endl;
        }

    explicit Foo(_ceto_private_C1 a, _ceto_private_C2 b, _ceto_private_C3 c) : a(std::move(a)), b(std::move(b)), c(std::move(c)) {}

    Foo() = delete;

};

    auto main() -> int {
        const auto f = std::make_shared<const decltype(Foo{1, 2, 3})>(1, 2, 3);
        const auto f2 = std::make_shared<const decltype(Foo{"a", 2, nullptr})>("a", 2, nullptr);
        const auto f5 = std::make_shared<const decltype(Foo{1, 2, std::make_shared<const decltype(Foo{1, std::make_shared<const decltype(Foo{1, 2, std::make_shared<const decltype(Foo{1, 2, 3001})>(1, 2, 3001)})>(1, 2, std::make_shared<const decltype(Foo{1, 2, 3001})>(1, 2, 3001)), std::make_shared<const decltype(Foo{1, 2, 3})>(1, 2, 3)})>(1, std::make_shared<const decltype(Foo{1, 2, std::make_shared<const decltype(Foo{1, 2, 3001})>(1, 2, 3001)})>(1, 2, std::make_shared<const decltype(Foo{1, 2, 3001})>(1, 2, 3001)), std::make_shared<const decltype(Foo{1, 2, 3})>(1, 2, 3))})>(1, 2, std::make_shared<const decltype(Foo{1, std::make_shared<const decltype(Foo{1, 2, std::make_shared<const decltype(Foo{1, 2, 3001})>(1, 2, 3001)})>(1, 2, std::make_shared<const decltype(Foo{1, 2, 3001})>(1, 2, 3001)), std::make_shared<const decltype(Foo{1, 2, 3})>(1, 2, 3)})>(1, std::make_shared<const decltype(Foo{1, 2, std::make_shared<const decltype(Foo{1, 2, 3001})>(1, 2, 3001)})>(1, 2, std::make_shared<const decltype(Foo{1, 2, 3001})>(1, 2, 3001)), std::make_shared<const decltype(Foo{1, 2, 3})>(1, 2, 3)));
        (std::cout << ceto::mado(f)->a) << ceto::mado(f2)->a;
    }

