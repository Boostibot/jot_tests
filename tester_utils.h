#pragma once

#include <algorithm>
#include <compare>
#include <cstdio>
#include <array>

#include "jot/utils.h"
#include "jot/defines.h"

#define CONCAT(a, b) IN__CONCAT_(a, b)
#define IN__CONCAT_(a, b) a ## b

namespace jot::tests 
{
    constexpr bool LOG_SUCESS = false;

    #define runtime_assert_(ret, ...) \
        if(!(__VA_ARGS__)) { \
            if(!std::is_constant_evaluated()) \
                printf("FAILED: %s() : %d : " #__VA_ARGS__ "\n", __func__, __LINE__);  \
            return ret; \
        }  \
        else if(jot::tests::LOG_SUCESS && std::is_constant_evaluated() == false) { \
            printf("SUCCESS: %s() : " #__VA_ARGS__ "\n", __func__); \
        }\

    #define runtime_assert(...) runtime_assert_(, __VA_ARGS__);

    //#define hybrid_assert(...) runtime_assert(__VA_ARGS__)
    #define hybrid_assert(...) static_assert(__VA_ARGS__); runtime_assert(__VA_ARGS__)
    #define run_test(...) static bool CONCAT(CONCAT(test_result_, __LINE__), __VA_ARGS__) = jot::tests::run_test_(__VA_ARGS__, #__VA_ARGS__);

    auto run_test_(mut&& test_fn, const char* name) 
    {
        printf("RUNNING TEST: %s()\n", name);
        test_fn(); return true;
    }

    //The i know what i am doing assignment operator
    template <typename To, typename From>
    proc& operator <<=(To& assign_to, const From& assign_from)
    {
        return (assign_to = cast(To) assign_from);
    }

    proc equal_n(mut&& first, mut&& second, let count)
    {
        return std::equal(first, first + count, second);
    }

    proc float_equal(let f1, let f2, f64 epsilon = 0.0001)
    {
        mut diff = f2 - f1;
        if(diff < 0)
            diff *= -1;

        return diff < epsilon;
    }


}

#include "jot/undefs.h"