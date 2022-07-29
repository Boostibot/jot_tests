#pragma once

#include <exception>
#include <vector>
#include "tester_utils.h"

#include "jot/defer.h"
#include "jot/stack.h"
#include "jot/stack_settings.h"
#include "jot/defines.h"

namespace jot::stack_test
{
    struct Res_Stats
    {
        i64 own_constr = 0;
        i64 copy_constr = 0;
        i64 move_constr = 0;
        i64 destructed = 0;
        
        //just for info
        i64 copy_assign = 0;
        i64 move_assign = 0;
    };

    template<typename T>
    struct Test_Res
    {
        T val;
        Res_Stats* stats;

        constexpr Test_Res(T val, Res_Stats* stats) : val(val), stats(stats)          
            { stats->own_constr++; };
        constexpr Test_Res(Test_Res const& other) : val(other.val), stats(other.stats)  
            { stats->copy_constr++; };
        constexpr Test_Res(Test_Res && other) : val(std::move(other.val)), stats(other.stats)
            { stats->move_constr++; };

        constexpr Test_Res& operator =(Test_Res const& other) 
        { 
            this->val = other.val;
            this->stats = other.stats;
            stats->copy_assign++;
            return *this;
        }

        constexpr Test_Res& operator =(Test_Res && other) 
        { 
            this->val = std::move(other.val);
            this->stats = other.stats;
            stats->move_assign++;
            return *this;
        }

        constexpr ~Test_Res() { stats->destructed++; }

        constexpr operator T() const {return val;};
    };

    template<typename T>
    struct Res_Watch
    {
        Res_Stats stats;

        proc make(T val) 
            { return Test_Res<T>(val, &stats); }

        template<typename... Ts>
        proc make_arr(Ts... args) 
            { return Array<Test_Res<T>, sizeof...(args)>{make(args)...}; }

        proc total_constructed() 
            { return stats.move_constr + stats.copy_constr + stats.own_constr; }

        proc total_desctructed() 
            { return stats.destructed; }

        proc ok()
            { return total_constructed() == total_desctructed(); }
    };

    template<typename T>
    test test_push_pop(Array<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;

        {
            Stack<T, 2, Size, Alloc, Grow> stack;

            runtime_assert(stack.data == 0);
            runtime_assert(stack.size == 0);
            runtime_assert(stack.capacity == 0);

            runtime_assert(*push(&stack, vals[0]) == vals[0]);

            runtime_assert(stack.data == stack.static_data());
            runtime_assert(stack.size == 1);
            runtime_assert(stack.capacity == stack.static_capacity);

            runtime_assert(*push(&stack, vals[1]) == vals[1]);

            runtime_assert(stack.data == stack.static_data());
            runtime_assert(stack.size == 2);
            runtime_assert(stack.capacity == 2);

            runtime_assert(pop(&stack) == vals[1]);
            runtime_assert(pop(&stack) == vals[0]);

            runtime_assert(stack.size == 0);
            runtime_assert(stack.capacity == 2);

            runtime_assert(*push(&stack, vals[2]) == vals[2]);
            runtime_assert(*push(&stack, vals[1]) == vals[1]);
            runtime_assert(*push(&stack, vals[0]) == vals[0]);

            runtime_assert(stack.size == 3);
            runtime_assert(stack.capacity == 4);

            runtime_assert(stack[0] == vals[2]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[0]);
        }
    }

    template<typename T>
    test test_push_pop_constexpr(Array<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;

        {
            Stack<T, 0, Size, Alloc, Grow> stack;

            runtime_assert(stack.data == 0);
            runtime_assert(stack.size == 0);
            runtime_assert(stack.capacity == 0);

            runtime_assert(*push(&stack, vals[0]) == vals[0]);

            runtime_assert(stack.data != nullptr);
            runtime_assert(stack.size == 1);
            runtime_assert(stack.capacity == 8);

            runtime_assert(*push(&stack, vals[1]) == vals[1]);

            runtime_assert(stack.data == stack.static_data());
            runtime_assert(stack.size == 2);
            runtime_assert(stack.capacity == 2);

            runtime_assert(pop(&stack) == vals[1]);
            runtime_assert(pop(&stack) == vals[0]);

            runtime_assert(stack.size == 0);
            runtime_assert(stack.capacity == 8);

            runtime_assert(*push(&stack, vals[2]) == vals[2]);
            runtime_assert(*push(&stack, vals[1]) == vals[1]);
            runtime_assert(*push(&stack, vals[0]) == vals[0]);
            runtime_assert(*push(&stack, vals[0]) == vals[0]);
            runtime_assert(*push(&stack, vals[0]) == vals[0]);
            runtime_assert(*push(&stack, vals[0]) == vals[0]);
            runtime_assert(*push(&stack, vals[0]) == vals[0]);

            runtime_assert(stack.size == 7);
            runtime_assert(stack.capacity == 12);

            runtime_assert(stack[0] == vals[2]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[0]);
        }
    }

    test test_push_pop()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;
        test_push_pop(w1.make_arr(10, 20, 30));
        test_push_pop(w2.make_arr(1.0, 2.0, 3.0));
        test_push_pop<i32>({ 10, 20, 30 });

        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = [] {
            test_push_pop_constexpr<i32>({ 10, 20, 30 });
            test_push_pop_constexpr<f64>({ 1.0, 2.0, 3.0 });

            return true;
        }();
    }

    template<size_t static_cap, typename T>
    test test_copy(Array<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack<T, static_cap, Size, Alloc, Grow>;

        {
            Stack stack;
            push(&stack, vals[0]);
            push(&stack, vals[1]);
            push(&stack, vals[2]);
            push(&stack, vals[2]);

            Stack copy = stack;
            runtime_assert(copy.size == 4);
            runtime_assert(copy.capacity >= 4);

            runtime_assert(copy[0] == vals[0]);
            runtime_assert(copy[1] == vals[1]);
            runtime_assert(copy[2] == vals[2]);
            runtime_assert(copy[3] == vals[2]);

            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[2]);
            runtime_assert(stack[3] == vals[2]);

            //not linked
            push(&stack, vals[1]);
            runtime_assert(stack.size == 5);
            runtime_assert(copy.size == 4);

            copy = stack;
            runtime_assert(copy.size == 5);

            runtime_assert(copy[0] == vals[0]);
            runtime_assert(copy[4] == vals[1]);

            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[4] == vals[1]);

            //from zero filling up
            Stack copy2;
            copy2 = stack;
            runtime_assert(copy2[0] == vals[0]);
            runtime_assert(copy2[3] == vals[2]);
            runtime_assert(copy2[4] == vals[1]);

            
            Stack copy3 = stack;
            push(&copy3, vals[0]);
            push(&copy3, vals[1]);
            push(&copy3, vals[0]);
            push(&copy3, vals[1]);

            runtime_assert(copy3.capacity > stack.capacity);
            runtime_assert(copy3.size == 9);

            //copying to less elems with bigger capacity
            copy3 = stack;

            runtime_assert(copy3.size == 5);


            //copying to more elems with bigger capacity
            pop(&copy3);
            pop(&copy3);
            pop(&copy3);

            runtime_assert(copy3.capacity > stack.capacity);
            
            copy3 = stack;

            runtime_assert(copy3.size == 5);
        }

        {
            //copying to zero elems
            Stack empty;
            Stack stack;
            push(&stack, vals[0]);
            push(&stack, vals[1]);
            push(&stack, vals[2]);
            push(&stack, vals[2]);
            runtime_assert(stack.size == 4);

            stack = empty;
            runtime_assert(stack.size == 0);
        }

        {
            //copy constructing empty
            Stack empty;
            Stack stack(empty);

            runtime_assert(stack.size == 0);
            runtime_assert(stack.capacity == stack.static_capacity);
        }
    }

    template<size_t static_cap, typename T>
    test test_swap(Array<T, 6> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack<T, static_cap, Size, Alloc, Grow>;

        {
            Stack stack1;
            push(&stack1, vals[0]);
            push(&stack1, vals[1]);
            push(&stack1, vals[2]);
            push(&stack1, vals[3]);
            
            {
                Stack stack2;
                push(&stack2, vals[3]);
                push(&stack2, vals[4]);
                push(&stack2, vals[5]);

                std::swap(stack1, stack2);

                runtime_assert(stack2.size == 4);
                runtime_assert(stack2.capacity >= 4);
                runtime_assert(stack2[0] == vals[0]);
                runtime_assert(stack2[1] == vals[1]);
                runtime_assert(stack2[2] == vals[2]);
                runtime_assert(stack2[3] == vals[3]);
            }

            runtime_assert(stack1.size == 3);
            runtime_assert(stack1.capacity >= 3);
            runtime_assert(stack1[0] == vals[3]);
            runtime_assert(stack1[1] == vals[4]);
            runtime_assert(stack1[1] == vals[4]);
        }

        {
            Stack stack1;
            push(&stack1, vals[0]);
            push(&stack1, vals[1]);
            push(&stack1, vals[2]);
            push(&stack1, vals[3]);

            {
                Stack stack2;
                std::swap(stack1, stack2);

                runtime_assert(stack2.size == 4);
                runtime_assert(stack2.capacity >= 4);

                runtime_assert(stack2[0] == vals[0]);
                runtime_assert(stack2[1] == vals[1]);
                runtime_assert(stack2[2] == vals[2]);
                runtime_assert(stack2[3] == vals[3]);
            }
            runtime_assert(stack1.size == 0);
        }

        {
            Stack stack1;
            Stack stack2;
            std::swap(stack1, stack2);

            runtime_assert(stack1.data == nullptr);
            runtime_assert(stack1.size == 0);
            runtime_assert(stack1.capacity >= 0);
            runtime_assert(stack2.data == nullptr);
            runtime_assert(stack2.size == 0);
            runtime_assert(stack2.capacity >= 0);
        }
    }

    test test_swap()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        {
            let arr1 = w1.make_arr(10, 20, 30, 40, 50, 60);
            let arr2 = w2.make_arr(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);

            test_swap<0>(arr1);
            test_swap<3>(arr1);
            test_swap<4>(arr1);
            test_swap<0>(arr2);
            test_swap<3>(arr2);
            test_swap<4>(arr2);
        }

        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = []{
            test_swap<0, i32>({10, 20, 30, 40, 50, 60});
            test_swap<0, f64>({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

            return true;
        }();
    }

    template<size_t static_cap, typename T>
    test test_move(Array<T, 6> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack<T, static_cap, Size, Alloc, Grow>;

        {
            Stack stack1;
            push(&stack1, vals[0]);
            push(&stack1, vals[1]);
            push(&stack1, vals[2]);
            push(&stack1, vals[3]);

            Stack stack2(std::move(stack1));

            runtime_assert(stack2.size == 4);
            runtime_assert(stack2.capacity >= 4);
            runtime_assert(stack2[0] == vals[0]);
            runtime_assert(stack2[1] == vals[1]);
            runtime_assert(stack2[2] == vals[2]);
            runtime_assert(stack2[3] == vals[3]);
        }

        {
            Stack stack1;
            Stack stack2(std::move(stack1));

            runtime_assert(stack1.data == nullptr);
            runtime_assert(stack1.size == 0);
            runtime_assert(stack1.capacity >= 0);
            runtime_assert(stack2.data == nullptr);
            runtime_assert(stack2.size == 0);
            runtime_assert(stack2.capacity >= 0);
        }

        {
            Stack stack1;
            push(&stack1, vals[0]);
            push(&stack1, vals[1]);
            push(&stack1, vals[2]);
            push(&stack1, vals[3]);

            {
                Stack stack2;
                push(&stack2, vals[3]);
                push(&stack2, vals[4]);
                push(&stack2, vals[5]);

                stack1 = std::move(stack2);

                runtime_assert(stack2.size == 4);
                runtime_assert(stack2.capacity >= 4);
                runtime_assert(stack2[0] == vals[0]);
                runtime_assert(stack2[1] == vals[1]);
                runtime_assert(stack2[2] == vals[2]);
                runtime_assert(stack2[3] == vals[3]);
            }

            runtime_assert(stack1.size == 3);
            runtime_assert(stack1.capacity >= 3);
            runtime_assert(stack1[0] == vals[3]);
            runtime_assert(stack1[1] == vals[4]);
            runtime_assert(stack1[1] == vals[4]);
        }

        {
            Stack stack1;
            push(&stack1, vals[0]);
            push(&stack1, vals[1]);
            push(&stack1, vals[2]);
            push(&stack1, vals[3]);

            {
                Stack stack2;
                stack1 = std::move(stack2);

                runtime_assert(stack2.size == 4);
                runtime_assert(stack2.capacity >= 4);

                runtime_assert(stack2[0] == vals[0]);
                runtime_assert(stack2[1] == vals[1]);
                runtime_assert(stack2[2] == vals[2]);
                runtime_assert(stack2[3] == vals[3]);
            }
            runtime_assert(stack1.size == 0);
        }

        {
            Stack stack1;
            Stack stack2;
            stack1 = std::move(stack2);

            runtime_assert(stack1.data == nullptr);
            runtime_assert(stack1.size == 0);
            runtime_assert(stack1.capacity >= 0);
            runtime_assert(stack2.data == nullptr);
            runtime_assert(stack2.size == 0);
            runtime_assert(stack2.capacity >= 0);
        }
    }


    test test_move()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        {
            let arr1 = w1.make_arr(10, 20, 30, 40, 50, 60);
            let arr2 = w2.make_arr(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);

            test_move<0>(arr1);
            test_move<3>(arr1);
            test_move<4>(arr1);
            test_move<0>(arr2);
            test_move<3>(arr2);
            test_move<4>(arr2);
        }

        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = [] {
            test_move<0, i32>({ 10, 20, 30, 40, 50, 60 });
            test_move<0, f64>({ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 });

            return true;
        }();
    }

    test test_copy()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        test_copy<3>(w1.make_arr(10, 20, 30));
        test_copy<2>(w2.make_arr(1.0, 2.0, 3.0));

        test_copy<0>(w1.make_arr(10, 20, 30));
        test_copy<0>(w2.make_arr(1.0, 2.0, 3.0));

        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = []{
            test_copy<0, i32>({10, 20, 30});
            test_copy<0, f64>({1.0, 2.0, 3.0});

            return true;
        }();
    }

    test test_stack()
    {
        test_push_pop();
        test_swap();
        test_copy();
        test_move();
    }

    run_test(test_stack);
}


#include "jot/undefs.h"