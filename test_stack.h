#pragma once

#include "tester_utils.h"

#include "jot/defer.h"
#include "jot/stack.h"
#include "jot/stack_settings.h"
#include "jot/bitfield.h"
#include "jot/array.h"
#include "jot/defines.h"


namespace jot::stack_test
{
    struct Not_Copyable
    {
        jot::i32 val;

        constexpr Not_Copyable(jot::i32 val) noexcept : val(val) {};
        constexpr Not_Copyable(const Not_Copyable&) noexcept = default;
        constexpr Not_Copyable(Not_Copyable&&) noexcept = default;

        constexpr Not_Copyable& operator =(const Not_Copyable&) noexcept  = delete;
        constexpr Not_Copyable& operator =(Not_Copyable&&) noexcept  = default;

        constexpr bool operator==(const Not_Copyable&) const noexcept = default;

        constexpr operator jot::i32() const { return val; };
    };

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

        constexpr bool operator ==(const Test_Res& other) const 
            { return this->val == other.val; };
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
            { return Array_<Test_Res<T>, sizeof...(args)>{make(args)...}; }

        proc total_constructed() 
            { return stats.move_constr + stats.copy_constr + stats.own_constr; }

        proc total_desctructed() 
            { return stats.destructed; }

        proc ok()
            { return total_constructed() == total_desctructed(); }
    };

    template<typename T>
    proc test_push_pop(Array_<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;

        {
            Stack_<T, 2, Size, Alloc, Grow> stack;

            runtime_assert(stack.data == 0);
            runtime_assert(stack.size == 0);
            runtime_assert(stack.capacity == 0);

            runtime_assert(*push(&stack, vals[0]) == vals[0]);

            runtime_assert(stack.data == static_data(stack));
            runtime_assert(stack.size == 1);
            runtime_assert(stack.capacity == stack.static_capacity);

            runtime_assert(*push(&stack, vals[1]) == vals[1]);

            runtime_assert(stack.data == static_data(stack));
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
    proc test_push_pop_constexpr(Array_<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;

        {
            Stack_<T, 0, Size, Alloc, Grow> stack;

            runtime_assert(stack.data == 0);
            runtime_assert(stack.size == 0);
            runtime_assert(stack.capacity == 0);

            runtime_assert(*push(&stack, vals[0]) == vals[0]);

            runtime_assert(stack.data != nullptr);
            runtime_assert(stack.size == 1);
            runtime_assert(stack.capacity == 8);

            runtime_assert(*push(&stack, vals[1]) == vals[1]);

            runtime_assert(stack.data == static_data(stack));
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

    proc test_push_pop()
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
    proc test_copy(Array_<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack_<T, static_cap, Size, Alloc, Grow>;

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

            //runtime_assert(copy3.capacity > stack.capacity);
            runtime_assert(copy3.size == 9);

            //copying to less elems with bigger capacity
            copy3 = stack;

            runtime_assert(copy3.size == 5);


            //copying to more elems with bigger capacity
            pop(&copy3);
            pop(&copy3);
            pop(&copy3);

            //runtime_assert(copy3.capacity > stack.capacity);
            
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
    proc test_swap(Array_<T, 6> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack_<T, static_cap, Size, Alloc, Grow>;

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



    proc test_swap()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        constexpr let arr3 = to_array<Not_Copyable>({{1}, {2}, {3}, {6346}, {-422}, {12}});
        {
            let arr1 = w1.make_arr(10, 20, 30, 40, 50, 60);
            let arr2 = w2.make_arr(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);

            test_swap<0>(arr1);
            test_swap<3>(arr1);
            test_swap<4>(arr1);
            test_swap<0>(arr2);
            test_swap<3>(arr2);
            test_swap<4>(arr2);

            test_swap<3, Not_Copyable>(arr3);
            test_swap<4, Not_Copyable>(arr3);
        }

        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = []{
            test_swap<0, i32>({10, 20, 30, 40, 50, 60});
            test_swap<0, f64>({ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 });
            test_swap<0, Not_Copyable>(arr3);

            return true;
        }();
    }

    template<size_t static_cap, typename T>
    proc test_move(Array_<T, 6> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack_<T, static_cap, Size, Alloc, Grow>;

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


    proc test_move()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        constexpr let arr3 = to_array<Not_Copyable>({{1}, {2}, {3}, {6346}, {-422}, {12}});
        {
            let arr1 = w1.make_arr(10, 20, 30, 40, 50, 60);
            let arr2 = w2.make_arr(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);

            test_move<0>(arr1);
            test_move<3>(arr1);
            test_move<4>(arr1);
            test_move<0>(arr2);
            test_move<3>(arr2);
            test_move<4>(arr2);

            test_move<3, Not_Copyable>(arr3);
            test_move<7, Not_Copyable>(arr3);
        }

        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = [] {
            test_move<0, i32>({ 10, 20, 30, 40, 50, 60 });
            test_move<0, f64>({ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 });
            test_move<0, Not_Copyable>(arr3);

            return true;
        }();
    }


    proc test_copy()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        constexpr let arr3 = to_array<Not_Copyable>({{1}, {2}, {3}});

        test_copy<3>(w1.make_arr(10, 20, 30));
        test_copy<2>(w2.make_arr(1.0, 2.0, 3.0));
        test_copy<10>(arr3);
        test_copy<2>(arr3);

        test_copy<0>(w1.make_arr(10, 20, 30));
        test_copy<0>(w2.make_arr(1.0, 2.0, 3.0));

        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = []{
            test_copy<0, i32>({10, 20, 30});
            test_copy<0, f64>({1.0, 2.0, 3.0});
            test_copy<0, Not_Copyable>(arr3);

            return true;
        }();
    }

    template<size_t static_cap, typename T>
    proc test_reserve(Array_<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack_<T, static_cap, Size, Alloc, Grow>;

        static_assert(static_cap <= 6, "static capacity is assumed to be below 7");

        {
            Stack empty;
            reserve(&empty, 5);

            runtime_assert(empty.capacity >= 5);
            runtime_assert(empty.size == 0);

            reserve(&empty, 13);

            runtime_assert(empty.capacity >= 13);
            runtime_assert(empty.size == 0);
        }

        {
            Stack empty;
            reserve(&empty, 7);

            runtime_assert(empty.capacity >= 7);
            runtime_assert(empty.size == 0);


            reserve(&empty, 2);
            runtime_assert(empty.capacity >= 7);
            runtime_assert(empty.size == 0);
        }

        {
            Stack stack;
            push(&stack, vals[0]);
            push(&stack, vals[0]);
            push(&stack, vals[0]);
            runtime_assert(stack.capacity >= 3);
            runtime_assert(stack.size == 3);

            reserve(&stack, 7);
            runtime_assert(stack.capacity >= 7);
            runtime_assert(stack.size == 3);

            pop(&stack);

            reserve(&stack, 2);
            runtime_assert(stack.capacity >= 7);
            runtime_assert(stack.size == 2);
        }
    }

    template<size_t static_cap, typename T>
    proc test_resize(Array_<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack_<T, static_cap, Size, Alloc, Grow>;

        static_assert(static_cap <= 6, "static capacity is assumed to be below 7");
        //resizing empty stack
        {
            Stack empty;
            resize(&empty, 5, vals[0]);

            runtime_assert(empty.capacity == 6 || empty.capacity == empty.static_capacity);
            runtime_assert(empty.size == 5);
            runtime_assert(empty[0] == vals[0]);
            runtime_assert(empty[1] == vals[0]);
            runtime_assert(empty[2] == vals[0]);
            runtime_assert(empty[3] == vals[0]);
            runtime_assert(empty[4] == vals[0]);
        }

        {
            Stack stack;
            resize(&stack, 7, vals[0]);

            runtime_assert(stack.capacity == 12);
            runtime_assert(stack.size == 7);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[2] == vals[0]);
            runtime_assert(stack[4] == vals[0]);
            runtime_assert(stack[6] == vals[0]);

            //growing
            resize(&stack, 11, vals[1]);
            resize(&stack, 12, vals[2]);
            runtime_assert(stack.capacity == 12);
            runtime_assert(stack.size == 12);
            runtime_assert(stack[7] == vals[1]);
            runtime_assert(stack[9] == vals[1]);
            runtime_assert(stack[10] == vals[1]);
            runtime_assert(stack[11] == vals[2]);

            //shrinking
            resize(&stack, 11, vals[1]);
            runtime_assert(stack.capacity == 12);
            runtime_assert(stack.size == 11);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[6] == vals[0]);
            runtime_assert(stack[10] == vals[1]);

            push(&stack, vals[2]);

            resize(&stack, 7, vals[1]);
            runtime_assert(stack.capacity == 12);
            runtime_assert(stack.size == 7);
            runtime_assert(stack[1] == vals[0]);
            runtime_assert(stack[3] == vals[0]);
            runtime_assert(stack[5] == vals[0]);
            runtime_assert(stack[6] == vals[0]);
        }
    }

    proc test_reserve_resize()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        constexpr let arr3 = to_array<Not_Copyable>({{1}, {2}, {3}});
        {
            let arr1 = w1.make_arr(10, 20, 30);
            let arr2 = w2.make_arr(1.0, 2.0, 3.0);
            test_resize<5>(arr1);
            test_resize<1>(arr1);
            test_resize<3>(arr1);
            test_resize<0>(arr1);
            
            test_reserve<5>(arr1);
            test_reserve<1>(arr1);
            test_reserve<3>(arr1);
            test_reserve<0>(arr1);


            test_resize<5>(arr2);
            test_resize<1>(arr2);
            test_resize<3>(arr2);
            test_resize<0>(arr2);

            test_reserve<5>(arr2);
            test_reserve<1>(arr2);
            test_reserve<3>(arr2);
            test_reserve<0>(arr2);


            test_resize<5>(arr3);
            test_resize<1>(arr3);
            test_resize<3>(arr3);
            test_resize<0>(arr3);

            test_reserve<5>(arr3);
            test_reserve<1>(arr3);
            test_reserve<3>(arr3);
            test_reserve<0>(arr3);
        }
        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = []{
            test_reserve<0, i32>({10, 20, 30});
            test_reserve<0, f64>({1.0, 2.0, 3.0});
            test_reserve<0, Not_Copyable>(arr3);

            return true;
        }();
    }

    template<size_t static_cap, typename T>
    proc test_insert_remove(Array_<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack_<T, static_cap, Size, Alloc, Grow>;

        {
            Stack stack;
            resize(&stack, 5, vals[0]);

            insert(&stack, 2, vals[1]);
            runtime_assert(stack.capacity >= 6);
            runtime_assert(stack.size == 6);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[0]);
            runtime_assert(stack[2] == vals[1]);
            runtime_assert(stack[3] == vals[0]);
            runtime_assert(stack[5] == vals[0]);

            insert(&stack, 2, vals[2]);
            runtime_assert(stack.capacity >= 7);
            runtime_assert(stack.size == 7);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[0]);
            runtime_assert(stack[2] == vals[2]);
            runtime_assert(stack[3] == vals[1]);
            runtime_assert(stack[4] == vals[0]);
            runtime_assert(stack[6] == vals[0]);

            remove(&stack, 2);
            runtime_assert(stack.capacity >= 7);
            runtime_assert(stack.size == 6);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[0]);
            runtime_assert(stack[2] == vals[1]);
            runtime_assert(stack[3] == vals[0]);
            runtime_assert(stack[5] == vals[0]);

            remove(&stack, 0);
            runtime_assert(stack.capacity >= 7);
            runtime_assert(stack.size == 5);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[0]);
            runtime_assert(stack[4] == vals[0]);
        }

        {
            Stack empty;
            insert(&empty, 0, vals[0]);
            runtime_assert(empty.capacity >= 1);
            runtime_assert(empty.size == 1);
            runtime_assert(back(empty) == vals[0]);

            insert(&empty, 1, vals[1]);
            runtime_assert(empty.capacity >= 2);
            runtime_assert(empty.size == 2);
            runtime_assert(back(empty) == vals[1]);

            remove(&empty, 1);
            runtime_assert(empty.capacity >= 2);
            runtime_assert(empty.size == 1);
            runtime_assert(back(empty) == vals[0]);

            remove(&empty, 0);
            runtime_assert(empty.capacity >= 2);
            runtime_assert(empty.size == 0);
        }
    }

    proc test_insert_remove()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        constexpr let arr3 = to_array<Not_Copyable>({{1}, {2}, {3}});
        {
            let arr1 = w1.make_arr(10, 20, 30);
            let arr2 = w2.make_arr(1.0, 2.0, 3.0);

            test_insert_remove<5>(arr1);
            test_insert_remove<1>(arr1);
            test_insert_remove<3>(arr1);
            test_insert_remove<0>(arr1);

            test_insert_remove<5>(arr2);
            test_insert_remove<1>(arr2);
            test_insert_remove<3>(arr2);
            test_insert_remove<0>(arr2);


            test_insert_remove<5>(arr3);
            test_insert_remove<1>(arr3);
            test_insert_remove<3>(arr3);
            test_insert_remove<0>(arr3);
        }
        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        /*constexpr auto test_constexpr = []{
            test_insert_remove<0, i32>({10, 20, 30});
            test_insert_remove<0, f64>({1.0, 2.0, 3.0});
            test_insert_remove<0, Not_Copyable>(arr3);

            return true;
        }();*/
    }

    template<size_t static_cap, typename T>
    proc test_slicing_and_common_ops(Array_<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack_<T, static_cap, Size, Alloc, Grow>;


        {
            Stack stack;
            runtime_assert(stdr::begin(stack) == stdr::end(stack));
            runtime_assert(empty(stack));
            runtime_assert(is_empty(stack));
            runtime_assert(is_static_alloced(stack) == false);

            push(&stack, vals[0]);

            runtime_assert(empty(stack) == false);
            runtime_assert(is_empty(stack) == false);
            runtime_assert(is_static_alloced(stack) || static_cap == 0);

            push(&stack, vals[1]);
            push(&stack, vals[2]);

            let beg_it = stdr::begin(stack);
            let end_it = stdr::end(stack);

            runtime_assert(*beg_it == vals[0]);
            runtime_assert(*(end_it - 1) == vals[2]);

            runtime_assert(front(stack) == vals[0]);
            runtime_assert(back(stack) == vals[2]);

            static_assert(stdr::range<Stack>);

            runtime_assert(empty(stack) == false);
            runtime_assert(is_empty(stack) == false);

            Slice_<T> slice1 = {std::begin(stack), std::end(stack)};
            Slice_<T> slice2 = stack(BEGIN, END);
            Slice_<T> slice3 = stack;
            Slice_ slice4 = stack;

            for(auto i = 0; i < std::size(stack); i++)
            {
                runtime_assert(slice1[i] == stack[i]);
                runtime_assert(slice2[i] == stack[i]);
                runtime_assert(slice3[i] == stack[i]);
                runtime_assert(slice4[i] == stack[i]);
            }
        }
    }

    proc test_slicing_and_common_ops()
    {

        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        constexpr let arr3 = to_array<Not_Copyable>({{1}, {2}, {3}});
        {
            let arr1 = w1.make_arr(10, 20, 30);
            let arr2 = w2.make_arr(1.0, 2.0, 3.0);

            test_slicing_and_common_ops<5>(arr1);
            test_slicing_and_common_ops<1>(arr1);
            test_slicing_and_common_ops<3>(arr1);
            test_slicing_and_common_ops<0>(arr1);

            test_slicing_and_common_ops<5>(arr2);
            test_slicing_and_common_ops<1>(arr2);
            test_slicing_and_common_ops<3>(arr2);
            test_slicing_and_common_ops<0>(arr2);

            test_slicing_and_common_ops<5>(arr3);
            test_slicing_and_common_ops<1>(arr3);
            test_slicing_and_common_ops<3>(arr3);
            test_slicing_and_common_ops<0>(arr3);
        }
        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = []{
            test_slicing_and_common_ops<0, i32>({10, 20, 30});
            test_slicing_and_common_ops<0, f64>({1.0, 2.0, 3.0});
            test_slicing_and_common_ops<0, Not_Copyable>(arr3);

            return true;
        }();
    }


    template<size_t static_cap, typename T>
    proc test_splice(Array_<T, 3> vals)
    {
        using Grow = Def_Grow<2, 0, 6>;
        using Alloc = Def_Alloc<T>;
        using Size = size_t;
        using Stack = jot::Stack_<T, static_cap, Size, Alloc, Grow>;

        Array_ expaned = {vals[0], vals[1], vals[2], vals[1], vals[0], vals[1], vals[2]};
        Slice_ single = expaned(0, 1);

        using Tested = Slice_<T>;
        using Iter = stdr::iterator_t<Tested>;

        static_assert(std::forward_iterator<Iter>);
        static_assert(stdr::input_range<Slice_<T, Size>>);
        static_assert(stdr::forward_range<Slice_<T, Size>>);

        {
            Stack stack;
            splice(&stack, 0, 0, move(vals));

            runtime_assert(stack.size == 3);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[2]);

            Slice_<T> empty;
            splice(&stack, 3, 0, move(empty));
            runtime_assert(stack.size == 3);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[2]);

            splice(&stack, 3, 0, move(vals));
            runtime_assert(stack.size == 6);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[2]);
            runtime_assert(stack[3] == vals[0]);
            runtime_assert(stack[4] == vals[1]);
            runtime_assert(stack[5] == vals[2]);

            splice(&stack, 3, 2, move(empty));
            runtime_assert(stack.size == 4);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[2]);
            runtime_assert(stack[3] == vals[2]);

            splice(&stack, 2, 2, move(vals));
            runtime_assert(stack.size == 5);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[0]);
            runtime_assert(stack[3] == vals[1]);
            runtime_assert(stack[4] == vals[2]);

            splice(&stack, 2, 3);
            runtime_assert(stack.size == 2);

            splice(&stack, 0, 2, move(expaned));
            runtime_assert(stack.size == 7);
            runtime_assert(stack[4] == vals[0]);
            runtime_assert(stack[5] == vals[1]);
            runtime_assert(stack[6] == vals[2]);
        }


        {
            
            Stack stack;
            resize(&stack, 3, [&](size_t i){return vals[i];});
            runtime_assert(stack.size == 3);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[1]);
            runtime_assert(stack[2] == vals[2]);
            
            splice(&stack, 1, 0, move(single));
            runtime_assert(stack.size == 4);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[0]);
            runtime_assert(stack[2] == vals[1]);
            runtime_assert(stack[3] == vals[2]);
            
            splice(&stack, 3, 0, move(vals));
            runtime_assert(stack.size == 7);
            runtime_assert(stack[0] == vals[0]);
            runtime_assert(stack[1] == vals[0]);
            runtime_assert(stack[2] == vals[1]);
            runtime_assert(stack[3] == vals[0]);
            runtime_assert(stack[4] == vals[1]);
            runtime_assert(stack[5] == vals[2]);
            runtime_assert(stack[6] == vals[2]);
            
        }
    }

    proc test_splice()
    {
        Res_Watch<i32> w1;
        Res_Watch<f64> w2;

        constexpr let arr3 = to_array<Not_Copyable>({{1}, {2}, {3}});
        {
            let arr1 = w1.make_arr(10, 20, 30);
            let arr2 = w2.make_arr(1.0, 2.0, 3.0);

            test_splice<5>(arr1);
            test_splice<1>(arr1);
            test_splice<3>(arr1);
            test_splice<0>(arr1);

            test_splice<5>(arr2);
            test_splice<1>(arr2);
            test_splice<3>(arr2);
            test_splice<0>(arr2);

            test_splice<5>(arr3);
            test_splice<1>(arr3);
            test_splice<3>(arr3);
            test_splice<0>(arr3);
        }
        runtime_assert(w1.ok());
        runtime_assert(w2.ok());

        constexpr auto test_constexpr = []{
            test_splice<0, i32>({10, 20, 30});
            test_splice<0, f64>({1.0, 2.0, 3.0});
            test_splice<0, Not_Copyable>(std::move(arr3));

            return true;
        }();
    }


    proc test_stack()
    {
        test_push_pop();
        test_swap();
        test_copy();
        test_move();
        test_reserve_resize();
        test_insert_remove();
        test_slicing_and_common_ops();
        test_splice();
    }

    run_test(test_stack);
}


#include "jot/undefs.h"