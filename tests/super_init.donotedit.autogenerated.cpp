
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

template <typename _ceto_private_C1>struct Generic : public ceto::enable_shared_from_this_base_for_templates {

    _ceto_private_C1 x;

    explicit Generic(_ceto_private_C1 x) : x(std::move(x)) {}

    Generic() = delete;

};

struct GenericChild : public std::type_identity_t<decltype(Generic(std::declval<const int>()))> {

    explicit GenericChild(const int  x) : std::type_identity_t<decltype(Generic(std::declval<const int>()))> (std::move(x)) {
    }

    GenericChild() = delete;

};

template <typename _ceto_private_C2>struct GenericChild2 : public std::type_identity_t<decltype(Generic(std::declval<_ceto_private_C2>()))> {

    _ceto_private_C2 y;

    explicit GenericChild2(const _ceto_private_C2 p) : std::type_identity_t<decltype(Generic(std::declval<_ceto_private_C2>()))> (std::move(p)), y(p) {
    }

    GenericChild2() = delete;

};

    auto main() -> int {
        const auto f = std::make_shared<const decltype(Generic{5})>(5);
        const auto f2 = std::make_shared<const decltype(GenericChild{5})>(5);
        const auto f3 = std::make_shared<const decltype(GenericChild2{5})>(5);
        ((std::cout << ceto::mado(f)->x) << ceto::mado(f2)->x) << ceto::mado(f3)->x;
    }

