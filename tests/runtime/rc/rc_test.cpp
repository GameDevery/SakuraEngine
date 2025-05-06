#include "SkrTestFramework/framework.hpp"

#include <SkrCore/rc/rc.hpp>

static uint64_t base_count           = 0;
static uint64_t derived_count        = 0;
static uint64_t custom_deleter_count = 0;

struct TestCounterBase {
    SKR_RC_IMPL()
public:
    TestCounterBase()
    {
        ++base_count;
    }
    virtual ~TestCounterBase()
    {
        --base_count;
    }
};

struct TestCounterDerived : public TestCounterBase {
public:
    TestCounterDerived()
    {
        ++derived_count;
    }
    ~TestCounterDerived()
    {
        --derived_count;
    }
};

struct TestCustomDeleter {
    SKR_RC_IMPL()

    inline void skr_rc_delete() const
    {
        ++custom_deleter_count;
        SkrDelete(this);
    }
};

TEST_CASE("Test RC")
{
    using namespace skr;

    SUBCASE("ctor & dtor")
    {
        RC<TestCounterBase> empty_ctor{};
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(empty_ctor.get(), nullptr);
        REQUIRE_EQ(empty_ctor.ref_count(), 0);
        REQUIRE_EQ(empty_ctor.ref_count_weak(), 0);

        RC<TestCounterBase> null_ctor{ nullptr };
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(null_ctor.get(), nullptr);
        REQUIRE_EQ(null_ctor.ref_count(), 0);
        REQUIRE_EQ(null_ctor.ref_count_weak(), 0);

        {
            RC<TestCounterBase> pointer_ctor{ SkrNew<TestCounterBase>() };
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_NE(pointer_ctor.get(), nullptr);
            REQUIRE_EQ(pointer_ctor.ref_count(), 1);
            REQUIRE_EQ(pointer_ctor.ref_count_weak(), 0);
        }

        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
    }

    SUBCASE("copy & move")
    {
        RC<TestCounterBase>    base{ SkrNew<TestCounterBase>() };
        RC<TestCounterDerived> derived{ SkrNew<TestCounterDerived>() };

        RC<TestCounterBase> copy_base{ base };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(base.get(), copy_base.get());
        REQUIRE_EQ(base.ref_count(), 2);
        REQUIRE_EQ(copy_base.ref_count(), 2);
        REQUIRE_EQ(base.ref_count_weak(), 0);
        REQUIRE_EQ(copy_base.ref_count_weak(), 0);

        RC<TestCounterBase> copy_derived{ derived };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(derived.get(), copy_derived.get());
        REQUIRE_EQ(derived.ref_count(), 2);
        REQUIRE_EQ(copy_derived.ref_count(), 2);
        REQUIRE_EQ(derived.ref_count_weak(), 0);
        REQUIRE_EQ(copy_derived.ref_count_weak(), 0);

        RC<TestCounterBase> move_base{ std::move(base) };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(base.get(), nullptr);
        REQUIRE_EQ(move_base.get(), copy_base.get());
        REQUIRE_EQ(base.ref_count(), 0);
        REQUIRE_EQ(copy_base.ref_count(), 2);
        REQUIRE_EQ(move_base.ref_count(), 2);
        REQUIRE_EQ(base.ref_count_weak(), 0);
        REQUIRE_EQ(copy_base.ref_count_weak(), 0);

        RC<TestCounterBase> move_derived{ std::move(derived) };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(derived.get(), nullptr);
        REQUIRE_EQ(move_derived.get(), copy_derived.get());
        REQUIRE_EQ(derived.ref_count(), 0);
        REQUIRE_EQ(copy_derived.ref_count(), 2);
        REQUIRE_EQ(move_derived.ref_count(), 2);
        REQUIRE_EQ(derived.ref_count_weak(), 0);
        REQUIRE_EQ(copy_derived.ref_count_weak(), 0);

        copy_base    = nullptr;
        move_base    = nullptr;
        copy_derived = nullptr;
        move_derived = nullptr;

        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
    }

    SUBCASE("assign & move assign")
    {
        RC<TestCounterBase>    base{ SkrNew<TestCounterBase>() };
        RC<TestCounterDerived> derived{ SkrNew<TestCounterDerived>() };

        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(base.ref_count(), 1);
        REQUIRE_EQ(derived.ref_count(), 1);
        {
            // assign
            RC<TestCounterBase> assign{};
            assign = base;
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_EQ(base.ref_count(), 2);
            REQUIRE_EQ(derived.ref_count(), 1);
            REQUIRE_EQ(assign.ref_count(), 2);
            REQUIRE_EQ(assign.get(), base.get());

            // assign derived
            assign = derived;
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_EQ(base.ref_count(), 1);
            REQUIRE_EQ(derived.ref_count(), 2);
            REQUIRE_EQ(assign.ref_count(), 2);
            REQUIRE_EQ(assign.get(), derived.get());
        }

        {
            // move assign
            RC<TestCounterBase> move_assign{};
            auto                cached_base = base.get();
            move_assign                     = std::move(base);
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_EQ(base.ref_count(), 0);
            REQUIRE_EQ(derived.ref_count(), 1);
            REQUIRE_EQ(move_assign.ref_count(), 1);
            REQUIRE_EQ(move_assign.get(), cached_base);
            REQUIRE_EQ(base.get(), nullptr);

            // move assign derived
            auto cached_derived = derived.get();
            move_assign         = std::move(derived);
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_EQ(base.ref_count(), 0);
            REQUIRE_EQ(derived.ref_count(), 0);
            REQUIRE_EQ(move_assign.ref_count(), 1);
            REQUIRE_EQ(move_assign.get(), cached_derived);
            REQUIRE_EQ(derived.get(), nullptr);
        }
    }

    // [need't test] factory

    SUBCASE("test compare")
    {
        RC<TestCounterBase>    base1{ SkrNew<TestCounterBase>() };
        RC<TestCounterBase>    base2{ SkrNew<TestCounterBase>() };
        RC<TestCounterDerived> derived1{ SkrNew<TestCounterDerived>() };
        RC<TestCounterDerived> derived2{ SkrNew<TestCounterDerived>() };

        RC<TestCounterBase> copy_base1    = base1;
        RC<TestCounterBase> copy_derived1 = derived1;

        // eq
        REQUIRE_EQ(base1 == base1, true);
        REQUIRE_EQ(base1 == base2, false);
        REQUIRE_EQ(base1 == derived1, false);
        REQUIRE_EQ(base1 == copy_base1, true);
        REQUIRE_EQ(derived1 == copy_derived1, true);
        REQUIRE_EQ(derived2 == copy_derived1, false);

        // neq
        REQUIRE_EQ(base1 != base1, false);
        REQUIRE_EQ(base1 != base2, true);
        REQUIRE_EQ(base1 != derived1, true);
        REQUIRE_EQ(base1 != copy_base1, false);
        REQUIRE_EQ(derived1 != copy_derived1, false);
        REQUIRE_EQ(derived2 != copy_derived1, true);

        // nullptr
        REQUIRE_EQ(base1 == nullptr, false);
        REQUIRE_EQ(nullptr == base1, false);
        REQUIRE_EQ(base1 != nullptr, true);
        REQUIRE_EQ(nullptr != base1, true);

        // lt (just test for compile)
        REQUIRE_EQ(base1 < base1, false);
        REQUIRE_EQ(derived1 < copy_derived1, false);
        REQUIRE_EQ(copy_derived1 < derived1, false);

        // gt (just test for compile)
        REQUIRE_EQ(base1 > base1, false);
        REQUIRE_EQ(derived1 > copy_derived1, false);
        REQUIRE_EQ(copy_derived1 > derived1, false);

        // le (just test for compile)
        REQUIRE_EQ(base1 <= base1, true);
        REQUIRE_EQ(derived1 <= copy_derived1, true);
        REQUIRE_EQ(copy_derived1 <= derived1, true);

        // ge (just test for compile)
        REQUIRE_EQ(base1 >= base1, true);
        REQUIRE_EQ(derived1 >= copy_derived1, true);
        REQUIRE_EQ(copy_derived1 >= derived1, true);
    }

    // [need't test] getter
    // [need't test] count getter

    SUBCASE("test is empty")
    {
        RC<TestCounterBase> empty{};
        RC<TestCounterBase> something{ SkrNew<TestCounterBase>() };

        REQUIRE_EQ(empty.is_empty(), true);
        REQUIRE_EQ(something.is_empty(), false);
        REQUIRE_EQ(empty, false);
        REQUIRE_EQ(something, true);
    }

    SUBCASE("test ops")
    {
        RC<TestCounterBase> rc = SkrNew<TestCounterBase>();
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_NE(rc.get(), nullptr);

        rc.reset();
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(rc.get(), nullptr);

        rc.reset(SkrNew<TestCounterBase>());
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_NE(rc.get(), nullptr);

        RC<TestCounterBase> other;
        rc.swap(other);
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(rc.get(), nullptr);
        REQUIRE_NE(other.get(), nullptr);

        other.reset(SkrNew<TestCounterBase>());
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_NE(other.get(), nullptr);
    }

    // [need't test] pointer behaviour
    // [need't test] skr hash

    SUBCASE("test empty")
    {
        RC<TestCounterBase> empty{};
        RC<TestCounterBase> empty_copy{ empty };
        RC<TestCounterBase> empty_move{ std::move(empty) };
        RC<TestCounterBase> empty_assign, empty_move_assign;
        empty_assign      = empty;
        empty_move_assign = std::move(empty);

        empty.get();
        empty.ref_count();
        empty.ref_count_weak();
        empty.is_empty();
        empty.operator bool();
        empty.reset();
        empty.reset(nullptr);
        empty.swap(empty_copy);
    }
}

TEST_CASE("Test RCUnique")
{
    using namespace skr;

    SUBCASE("ctor & dtor")
    {
        RCUnique<TestCounterBase> empty_ctor{};
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(empty_ctor.get(), nullptr);
        REQUIRE_EQ(empty_ctor.ref_count(), 0);
        REQUIRE_EQ(empty_ctor.ref_count_weak(), 0);

        RCUnique<TestCounterBase> null_ctor{ nullptr };
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(null_ctor.get(), nullptr);
        REQUIRE_EQ(null_ctor.ref_count(), 0);
        REQUIRE_EQ(null_ctor.ref_count_weak(), 0);

        {
            RCUnique<TestCounterBase> pointer_ctor{ SkrNew<TestCounterBase>() };
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_NE(pointer_ctor.get(), nullptr);
            REQUIRE_EQ(pointer_ctor.ref_count(), 1);
            REQUIRE_EQ(pointer_ctor.ref_count_weak(), 0);
        }

        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
    }

    SUBCASE("copy & move")
    {
        RCUnique<TestCounterBase>    base{ SkrNew<TestCounterBase>() };
        RCUnique<TestCounterDerived> derived{ SkrNew<TestCounterDerived>() };

        auto                      cached_base = base.get();
        RCUnique<TestCounterBase> move_base{ std::move(base) };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(base.get(), nullptr);
        REQUIRE_EQ(base.ref_count(), 0);
        REQUIRE_EQ(move_base.ref_count(), 1);
        REQUIRE_EQ(move_base.get(), cached_base);

        auto                      cached_derived = derived.get();
        RCUnique<TestCounterBase> move_derived{ std::move(derived) };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(derived.get(), nullptr);
        REQUIRE_EQ(move_derived.ref_count(), 1);
        REQUIRE_EQ(move_derived.get(), cached_derived);

        move_base    = nullptr;
        move_derived = nullptr;

        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
    }

    SUBCASE("assign & move assign")
    {
        RCUnique<TestCounterBase>    base{ SkrNew<TestCounterBase>() };
        RCUnique<TestCounterDerived> derived{ SkrNew<TestCounterDerived>() };

        RCUnique<TestCounterBase> move_assign{};

        // move assign base
        auto cached_base = base.get();
        move_assign      = std::move(base);
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(base.get(), nullptr);
        REQUIRE_EQ(base.ref_count(), 0);
        REQUIRE_EQ(move_assign.ref_count(), 1);
        REQUIRE_EQ(move_assign.get(), cached_base);

        // move assign derived
        auto cached_derived = derived.get();
        move_assign         = std::move(derived);
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(derived.get(), nullptr);
        REQUIRE_EQ(derived.ref_count(), 0);
        REQUIRE_EQ(move_assign.ref_count(), 1);
        REQUIRE_EQ(move_assign.get(), cached_derived);
        REQUIRE_EQ(derived.get(), nullptr);

        move_assign = nullptr;
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(move_assign.get(), nullptr);
        REQUIRE_EQ(move_assign.ref_count(), 0);
    }

    SUBCASE("test compare")
    {
        RCUnique<TestCounterBase>    base1{ SkrNew<TestCounterBase>() };
        RCUnique<TestCounterBase>    base2{ SkrNew<TestCounterBase>() };
        RCUnique<TestCounterDerived> derived1{ SkrNew<TestCounterDerived>() };
        RCUnique<TestCounterDerived> derived2{ SkrNew<TestCounterDerived>() };

        // eq
        REQUIRE_EQ(base1 == base1, true);
        REQUIRE_EQ(base1 == base2, false);
        REQUIRE_EQ(base1 == derived1, false);

        // neq
        REQUIRE_EQ(base1 != base1, false);
        REQUIRE_EQ(base1 != base2, true);
        REQUIRE_EQ(base1 != derived1, true);

        // nullptr
        REQUIRE_EQ(base1 == nullptr, false);
        REQUIRE_EQ(nullptr == base1, false);
        REQUIRE_EQ(base1 != nullptr, true);
        REQUIRE_EQ(nullptr != base1, true);

        // lt (just test for compile)
        REQUIRE_EQ(base1 < base1, false);

        // gt (just test for compile)
        REQUIRE_EQ(base1 > base1, false);

        // le (just test for compile)
        REQUIRE_EQ(base1 <= base1, true);

        // ge (just test for compile)
        REQUIRE_EQ(base1 >= base1, true);
    }

    // [need't test] getter
    // [need't test] count getter

    SUBCASE("test is empty")
    {
        RCUnique<TestCounterBase> empty{};
        RCUnique<TestCounterBase> something{ SkrNew<TestCounterBase>() };

        REQUIRE_EQ(empty.is_empty(), true);
        REQUIRE_EQ(something.is_empty(), false);
        REQUIRE_EQ(empty, false);
        REQUIRE_EQ(something, true);
    }

    SUBCASE("test ops")
    {
        RCUnique<TestCounterBase> rc = SkrNew<TestCounterBase>();
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_NE(rc.get(), nullptr);

        rc.reset();
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(rc.get(), nullptr);

        rc.reset(SkrNew<TestCounterBase>());
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_NE(rc.get(), nullptr);

        RCUnique<TestCounterBase> other;
        rc.swap(other);
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(rc.get(), nullptr);
        REQUIRE_NE(other.get(), nullptr);

        other.reset(SkrNew<TestCounterBase>());
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_NE(other.get(), nullptr);

        auto* cached_other   = other.get();
        auto* released_other = other.release();
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_EQ(other.get(), nullptr);
        REQUIRE_EQ(released_other, cached_other);
        SkrDelete(released_other);
    }

    // [need't test] pointer behaviour
    // [need't test] skr hash

    SUBCASE("test empty")
    {
        RCUnique<TestCounterBase> empty{};
        RCUnique<TestCounterBase> empty_move{ std::move(empty) };
        RCUnique<TestCounterBase> empty_move_assign;
        empty_move_assign = std::move(empty);

        empty.get();
        empty.ref_count();
        empty.ref_count_weak();
        empty.is_empty();
        empty.operator bool();
        empty.reset();
        empty.reset(nullptr);
        empty.swap(empty_move);
        empty.release();
    }
}

TEST_CASE("Test RCWeak")
{
}

TEST_CASE("Test Custom Deleter")
{
}