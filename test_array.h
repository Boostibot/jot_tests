#pragma once

#include "tester_utils.h"

#include "jot/array.h"
#include "jot/defines.h"

namespace jot::tests
{

    struct TestStruct1 
    {
        i32 i1 = -51631;
        i32 i2 = 63464;
        u8 u1 = 144;
        f64 f1 = -5151.54684646;
        u8 u2 = 64;

        proc operator<=>(const TestStruct1&) const = default;
    };

    proc test_construction()
    {
        Array_<u8, 0> empty;
        constexpr Array_<u8, 0> empty_constexpr2 = {0};

        static_assert(empty.size == 0);
        static_assert(empty_constexpr2.size == 0);

        static_assert(empty.capacity == 0);
        static_assert(empty_constexpr2.capacity == 0);

        //Non empty array needs to initialized (compiles because unint is not constexpr)
        Array_<u8, 10> uninit1;
        Array_<TestStruct1, 10> uninit2;

        static_assert(uninit1.capacity == 10);
        static_assert(uninit2.capacity == 10);

        //C-like zero init
        constexpr Array_<u8, 10> non_empty1 = {0};
        constexpr Array_<TestStruct1, 10> non_empty2 = {TestStruct1()};

        static_assert(non_empty1.size == 10);
        static_assert(non_empty2.size == 10);

        hybrid_assert(non_empty1[0] == 0);
        hybrid_assert(non_empty1[3] == 0);
        hybrid_assert(non_empty1[9] == 0);
        hybrid_assert(non_empty2[0] == TestStruct1());
        hybrid_assert(non_empty2[1] == TestStruct1());
        hybrid_assert(non_empty2[7] == TestStruct1());

        //Deduction
        constexpr Array_ deduced1 = {1, 2, 3, 4, 5};
        constexpr Array_ deduced2 = {1.1, 2.1, 3.1, 4, 5.8};
        constexpr Array_ deduced3 = {1U, 2, 3, 4, 5};
        constexpr Array_ deduced4 = {true, true, false};
        constexpr Array_ deduced5 = {TestStruct1(), TestStruct1()};

        static_assert(deduced1.size == 5);
        static_assert(deduced2.size == 5);
        static_assert(deduced3.size == 5);
        static_assert(deduced4.size == 3);
        static_assert(deduced5.size == 2);

        static_assert(deduced3.capacity == 5);
        static_assert(deduced4.capacity == 3);
        static_assert(deduced5.capacity == 2);


        hybrid_assert(deduced1[0] == 1);
        hybrid_assert(deduced1[3] == 4);

        hybrid_assert(float_equal(deduced2[3], 4.0));
        hybrid_assert(float_equal(deduced2[4], 5.8));

        hybrid_assert(deduced4[2] == false);
    }

    proc test_destructuring()
    {
        {
            constexpr Array_ arr = {1, 2, 3};
            auto el1 = std::get<0>(arr);
            auto el2 = std::get<1>(arr);
            auto el3 = std::get<2>(arr);
            auto [a, b, c] = arr;
            static_assert(same<decltype(a), decltype(b), decltype(c), int>);
        }
        {
            constexpr Array_ arr = {1};
            auto [a] = arr;
            static_assert(same<decltype(a), int>);
        }
        {
            constexpr Array_ arr = {1.5, 1, 3, 7};
            auto [a, b, c, d] = arr;
            static_assert(same<decltype(a), decltype(b), decltype(c), decltype(d), double>);
        }
    }

    proc test_comparison()
    {
        constexpr Array_ int1 = {1, 2, 3, 4, 5};
        constexpr Array_ int2 = {1, 2, 3, 4, 5};
        constexpr Array_ int3 = {5, 2, 3, 4, 5};

        static_assert(int2 < int3 == true);
        static_assert(int2 > int3 == false);

        
        hybrid_assert(int1 == int1);
        hybrid_assert(int1 >= int1);
        hybrid_assert(int1 <= int1);
        hybrid_assert(int1 != int1 == false);

        //should not have strict comparisons
        hybrid_assert(int1 < int1 == false);
        hybrid_assert(int1 > int1 == false);

        hybrid_assert(int1 == int2);
        hybrid_assert(int1 != int2 == false);
        hybrid_assert(int1 < int2 == false);
        hybrid_assert(int2 == int1);
        hybrid_assert(int2 != int1 == false);
        hybrid_assert(int2 < int1 == false);
        hybrid_assert(int2 > int1 == false);

        hybrid_assert(int1 == int3 == false);
        hybrid_assert(int1 != int3);

        hybrid_assert(int2 == int3 == false);

        //Strict comaprison should work here
        hybrid_assert(int2 < int3 == true);
        hybrid_assert(int2 <= int3 == true);
        hybrid_assert(int2 != int3);

        constexpr Array_ struct1 = {TestStruct1(), TestStruct1()};
        constexpr Array_ struct2 = {TestStruct1(), TestStruct1()};
        constexpr Array_<TestStruct1, 2> struct3 = {TestStruct1(), {1}};

        hybrid_assert(struct1 == struct1);
        hybrid_assert(struct1 >= struct1);
        hybrid_assert(struct1 <= struct1);
        hybrid_assert(struct1 != struct1 == false);

        //should not have strict comparisons
        hybrid_assert(struct1 < struct1 == false);
        hybrid_assert(struct1 > struct1 == false);

        hybrid_assert(struct1 == struct3 == false);
        hybrid_assert(struct2 < struct3 == true);
        hybrid_assert(struct1 != struct3);
    }

    proc test_assignment()
    {
        Array_ deduced1 = {1, 2, 3, 4, 5};
        Array_ deduced2 = {1.1, 2.1, 3.1, 4, 5.8};
        Array_ deduced3 = {1U, 2, 3, 4, 5};
        Array_ deduced4 = {true, true, false};
        Array_ deduced5 = {TestStruct1(), TestStruct1()};

        decltype(deduced1) assigned1;
        decltype(deduced2) assigned2;
        decltype(deduced3) assigned3;
        decltype(deduced4) assigned4;
        decltype(deduced5) assigned5;

        assigned1 = deduced1;
        assigned2 = deduced2;
        assigned3 = deduced3;
        assigned4 = deduced4;
        assigned5 = deduced5;

        runtime_assert(assigned1 == deduced1);
        runtime_assert(assigned2 == deduced2);
        runtime_assert(assigned3 == deduced3);
        runtime_assert(assigned4 == deduced4);
        runtime_assert(assigned5 == deduced5);

        assigned1 = {1, 3, 4, 5, 6};
        runtime_assert(assigned1 == Array_{1, 3, 4, 5, 6});
    }

    proc test_acessors()
    {
        constexpr Array_ deduced1 = {1, 2, 3, 4, 5};
        constexpr Array_ deduced3 = {1U, 2, 3, 4, 5};
        constexpr Array_ deduced5 = {TestStruct1(), TestStruct1()};

        hybrid_assert(std::begin(deduced1) == deduced1.data);
        hybrid_assert(std::end(deduced1) == deduced1 + deduced1.size);
        hybrid_assert(std::cbegin(deduced1) == deduced1.data);
        hybrid_assert(std::cend(deduced1) == deduced1 + deduced1.size);
        hybrid_assert(std::size(deduced1) == deduced1.size);

        hybrid_assert(std::begin(deduced3) == deduced3.data);
        hybrid_assert(std::end(deduced3) == deduced3 + deduced3.size);
        hybrid_assert(std::cbegin(deduced3) == deduced3.data);
        hybrid_assert(std::cend(deduced3) == deduced3 + deduced3.size);
        hybrid_assert(std::size(deduced3) == deduced3.size);

        hybrid_assert(std::begin(deduced5) == deduced5.data);
        hybrid_assert(std::end(deduced5) == deduced5 + deduced5.size);
        hybrid_assert(std::cbegin(deduced5) == deduced5.data);
        hybrid_assert(std::cend(deduced5) == deduced5 + deduced5.size);
        hybrid_assert(std::size(deduced5) == deduced5.size);
    }

    proc test_array()
    {
        test_construction();
        test_comparison();
        test_assignment();
        test_acessors();
        test_destructuring();
    }

    run_test(test_array);
}

#include "jot/undefs.h"