#pragma once

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


#include "ceto.h"
//#include "ceto_private_boundscheck.donotedit.autogenerated.h"

#include <ranges>
;
     template<typename T> inline auto maybe_reserve( std::vector<T> &  vec,  auto &&  sized) -> void requires (requires () {    std::size(sized);
}) {
        (*ceto::mad(vec)).reserve(std::size(std::forward<decltype(sized)>(sized)));
    }

     template<typename T> inline auto maybe_reserve( std::vector<T> &  vec,  auto &&  unsized) -> void requires (!requires () {    std::size(unsized);
}) {
        ; // pass
    }


;

;
