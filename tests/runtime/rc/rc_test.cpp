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

    inline void skr_rc_delete()
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
    using namespace skr;

    SUBCASE("test ctor")
    {
        RCWeak<TestCounterBase> empty_ctor{};
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE(empty_ctor.lock().is_empty());
        REQUIRE_EQ(empty_ctor.ref_count_weak(), 0);

        RCWeak<TestCounterBase> null_ctor{ nullptr };
        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE(null_ctor.lock().is_empty());
        REQUIRE_EQ(null_ctor.ref_count_weak(), 0);

        // from raw ptr
        {
            auto                    rc_unique = RCUnique<TestCounterBase>::New();
            RCWeak<TestCounterBase> rc_weak{ rc_unique.get() };
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_FALSE(rc_weak.lock().is_empty());
            REQUIRE_EQ(rc_weak.ref_count_weak(), 2);
            REQUIRE_EQ(rc_unique.ref_count_weak(), 2);
            REQUIRE_EQ(rc_unique.ref_count(), 1);
            if (auto locked = rc_weak.lock())
            {
                REQUIRE_EQ(base_count, 1);
                REQUIRE_EQ(derived_count, 0);
                REQUIRE_EQ(locked.get(), rc_unique.get());
            }
            else
            {
                SKR_UNREACHABLE_CODE()
            }
        }

        // from raw ptr derived
        {
            auto                    rc_unique = RCUnique<TestCounterDerived>::New();
            RCWeak<TestCounterBase> rc_weak{ rc_unique.get() };
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_FALSE(rc_weak.lock().is_empty());
            REQUIRE_EQ(rc_weak.ref_count_weak(), 2);
            REQUIRE_EQ(rc_unique.ref_count_weak(), 2);
            REQUIRE_EQ(rc_unique.ref_count(), 1);
            if (auto locked = rc_weak.lock())
            {
                REQUIRE_EQ(base_count, 1);
                REQUIRE_EQ(derived_count, 1);
                REQUIRE_EQ(locked.get(), rc_unique.get());
            }
            else
            {
                SKR_UNREACHABLE_CODE()
            }
        }

        // from rc
        {
            auto rc      = RC<TestCounterBase>::New();
            auto rc_weak = RCWeak<TestCounterBase>(rc);
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_FALSE(rc_weak.lock().is_empty());
            REQUIRE_EQ(rc_weak.ref_count_weak(), 2);
            REQUIRE_EQ(rc.ref_count_weak(), 2);
            REQUIRE_EQ(rc.ref_count(), 1);
            if (auto locked = rc_weak.lock())
            {
                REQUIRE_EQ(base_count, 1);
                REQUIRE_EQ(derived_count, 0);
                REQUIRE_EQ(locked.get(), rc.get());
            }
            else
            {
                SKR_UNREACHABLE_CODE()
            }
        }

        // from rc derived
        {
            auto rc      = RC<TestCounterDerived>::New();
            auto rc_weak = RCWeak<TestCounterBase>(rc);
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_FALSE(rc_weak.lock().is_empty());
            REQUIRE_EQ(rc_weak.ref_count_weak(), 2);
            REQUIRE_EQ(rc.ref_count_weak(), 2);
            REQUIRE_EQ(rc.ref_count(), 1);
            if (auto locked = rc_weak.lock())
            {
                REQUIRE_EQ(base_count, 1);
                REQUIRE_EQ(derived_count, 1);
                REQUIRE_EQ(locked.get(), rc.get());
            }
            else
            {
                SKR_UNREACHABLE_CODE()
            }
        }

        // from unique
        {
            auto rc_unique = RCUnique<TestCounterBase>::New();
            auto rc_weak   = RCWeak<TestCounterBase>(rc_unique);
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_FALSE(rc_weak.lock().is_empty());
            REQUIRE_EQ(rc_weak.ref_count_weak(), 2);
            REQUIRE_EQ(rc_unique.ref_count_weak(), 2);
            REQUIRE_EQ(rc_unique.ref_count(), 1);
            if (auto locked = rc_weak.lock())
            {
                REQUIRE_EQ(base_count, 1);
                REQUIRE_EQ(derived_count, 0);
                REQUIRE_EQ(locked.get(), rc_unique.get());
            }
            else
            {
                SKR_UNREACHABLE_CODE()
            }
        }

        // from unique derived
        {
            auto rc_unique = RCUnique<TestCounterDerived>::New();
            auto rc_weak   = RCWeak<TestCounterBase>(rc_unique);
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_FALSE(rc_weak.lock().is_empty());
            REQUIRE_EQ(rc_weak.ref_count_weak(), 2);
            REQUIRE_EQ(rc_unique.ref_count_weak(), 2);
            REQUIRE_EQ(rc_unique.ref_count(), 1);
            if (auto locked = rc_weak.lock())
            {
                REQUIRE_EQ(base_count, 1);
                REQUIRE_EQ(derived_count, 1);
                REQUIRE_EQ(locked.get(), rc_unique.get());
            }
            else
            {
                SKR_UNREACHABLE_CODE()
            }
        }

        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
    }

    SUBCASE("copy & move")
    {
        auto base    = RCUnique<TestCounterBase>::New();
        auto derived = RCUnique<TestCounterDerived>::New();

        RCWeak<TestCounterBase>    base_weak{ base };
        RCWeak<TestCounterDerived> derived_weak{ derived };

        RCWeak<TestCounterBase> copy_base{ base_weak };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(base_weak.ref_count_weak(), 3);
        REQUIRE_EQ(copy_base.ref_count_weak(), 3);
        REQUIRE_EQ(copy_base.lock().get(), base_weak.lock().get());

        RCWeak<TestCounterBase> copy_derived{ derived_weak };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(derived_weak.ref_count_weak(), 3);
        REQUIRE_EQ(copy_derived.ref_count_weak(), 3);
        REQUIRE_EQ(copy_derived.lock().get(), derived_weak.lock().get());

        RCWeak<TestCounterBase> move_base{ std::move(base_weak) };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(base.ref_count_weak(), 3);
        REQUIRE_EQ(move_base.ref_count_weak(), 3);
        REQUIRE_EQ(move_base.lock().get(), base.get());

        RCWeak<TestCounterBase> move_derived{ std::move(derived_weak) };
        REQUIRE_EQ(base_count, 2);
        REQUIRE_EQ(derived_count, 1);
        REQUIRE_EQ(base.ref_count_weak(), 3);
        REQUIRE_EQ(move_derived.ref_count_weak(), 3);
        REQUIRE_EQ(move_derived.lock().get(), derived.get());

        base         = nullptr;
        derived      = nullptr;
        base_weak    = nullptr;
        derived_weak = nullptr;
        copy_base    = nullptr;
        copy_derived = nullptr;
        move_base    = nullptr;
        move_derived = nullptr;

        REQUIRE_EQ(base_count, 0);
        REQUIRE_EQ(derived_count, 0);
    }

    SUBCASE("assign & move assign")
    {
        auto base    = RCUnique<TestCounterBase>::New();
        auto derived = RC<TestCounterDerived>::New();

        RCWeak<TestCounterBase>    base_weak{ base };
        RCWeak<TestCounterDerived> derived_weak{ derived };

        // assign weak
        {
            RCWeak<TestCounterBase> assign{};

            // assign
            assign = base_weak;
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_EQ(base_weak.ref_count_weak(), 3);
            REQUIRE_EQ(assign.ref_count_weak(), 3);
            REQUIRE_EQ(assign.lock().get(), base_weak.lock().get());

            // assign derived
            assign = derived_weak;
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_EQ(derived_weak.ref_count_weak(), 3);
            REQUIRE_EQ(assign.ref_count_weak(), 3);
            REQUIRE_EQ(assign.lock().get(), derived_weak.lock().get());

            REQUIRE_EQ(base_weak.ref_count_weak(), 2);
        }

        // move assign weak
        {
            RCWeak<TestCounterBase> move_assign{};

            // move assign base
            auto cached_base = base_weak.lock().get();
            move_assign      = std::move(base_weak);
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE(base_weak.is_empty());
            REQUIRE_EQ(move_assign.ref_count_weak(), 2);
            REQUIRE_EQ(move_assign.lock().get(), cached_base);

            // move assign derived
            auto cached_derived = derived_weak.lock().get();
            move_assign         = std::move(derived_weak);
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE(derived_weak.is_empty());
            REQUIRE_EQ(move_assign.ref_count_weak(), 2);
            REQUIRE_EQ(move_assign.lock().get(), cached_derived);

            REQUIRE_EQ(base_weak.ref_count_weak(), 0);
        }

        // assign rc & unique rc
        {
            RCWeak<TestCounterBase> assign{};
            assign = base;
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_EQ(base.ref_count_weak(), 2);
            REQUIRE_EQ(assign.ref_count_weak(), 2);
            REQUIRE_EQ(assign.lock().get(), base.get());

            // assign derived
            assign = derived;
            REQUIRE_EQ(base_count, 2);
            REQUIRE_EQ(derived_count, 1);
            REQUIRE_EQ(derived.ref_count_weak(), 2);
            REQUIRE_EQ(assign.ref_count_weak(), 2);
            REQUIRE_EQ(assign.lock().get(), derived.get());
        }
    }

    SUBCASE("test compare")
    {
        RC<TestCounterBase>    rc_base1{ SkrNew<TestCounterBase>() };
        RC<TestCounterBase>    rc_base2{ SkrNew<TestCounterBase>() };
        RC<TestCounterDerived> rc_derived1{ SkrNew<TestCounterDerived>() };
        RC<TestCounterDerived> rc_derived2{ SkrNew<TestCounterDerived>() };

        RCWeak<TestCounterBase>    base1{ rc_base1 };
        RCWeak<TestCounterBase>    base2{ rc_base2 };
        RCWeak<TestCounterDerived> derived1{ rc_derived1 };
        RCWeak<TestCounterDerived> derived2{ rc_derived2 };

        RCWeak<TestCounterBase> copy_base1    = base1;
        RCWeak<TestCounterBase> copy_derived1 = derived1;

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

    // [need't test] unsafe getter
    // [need't test] count getter

    SUBCASE("lock")
    {
        RC<TestCounterBase> rc{ SkrNew<TestCounterBase>() };

        // basic lock
        RCWeak<TestCounterBase> weak{ rc };
        REQUIRE(weak.is_alive());
        REQUIRE_FALSE(weak.is_expired());
        REQUIRE_FALSE(weak.lock().is_empty());
        {
            auto locked = weak.lock();
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_EQ(locked.get(), rc.get());
        }

        // lock a destroyed weak
        rc.reset();
        REQUIRE_FALSE(weak.is_alive());
        REQUIRE(weak.is_expired());
        REQUIRE(weak.lock().is_empty());
        {
            auto locked = weak.lock();
            REQUIRE_EQ(base_count, 0);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_EQ(locked.get(), nullptr);
        }

        // try lock a destroyed weak to rc
        {
            RC<TestCounterBase> locked_rc = weak.lock().rc();
            REQUIRE_EQ(base_count, 0);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_EQ(locked_rc.get(), nullptr);
        }

        // lock and cast to rc
        rc   = SkrNew<TestCounterBase>();
        weak = rc;
        REQUIRE(weak.is_alive());
        REQUIRE_FALSE(weak.is_expired());
        REQUIRE_FALSE(weak.lock().is_empty());
        {
            RC<TestCounterBase> locked_rc = weak.lock();
            REQUIRE_EQ(base_count, 1);
            REQUIRE_EQ(derived_count, 0);
            REQUIRE_EQ(locked_rc.get(), rc.get());
            REQUIRE_EQ(locked_rc.ref_count(), 2);
        }
    }

    // [need't test] is empty
    SUBCASE("test ops")
    {
        RC<TestCounterBase> rc = SkrNew<TestCounterBase>();
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_NE(rc.get(), nullptr);

        RCWeak<TestCounterBase> weak{ rc };
        REQUIRE_EQ(base_count, 1);
        REQUIRE_EQ(derived_count, 0);
        REQUIRE_FALSE(weak.is_empty());
        REQUIRE_EQ(weak.ref_count_weak(), 2);
        REQUIRE_EQ(weak.get_unsafe(), rc.get());

        weak.reset();
        REQUIRE(weak.is_empty());
        REQUIRE_EQ(weak.ref_count_weak(), 0);
        REQUIRE_EQ(weak.get_unsafe(), nullptr);
        REQUIRE_EQ(rc->skr_rc_weak_ref_count(), 1);

        weak.reset(rc);
        REQUIRE_FALSE(weak.is_empty());
        REQUIRE_EQ(weak.ref_count_weak(), 2);
        REQUIRE_EQ(weak.get_unsafe(), rc.get());
        REQUIRE_EQ(rc->skr_rc_weak_ref_count(), 2);

        RCUnique<TestCounterBase> new_rc = SkrNew<TestCounterBase>();
        weak.reset(new_rc);
        REQUIRE_FALSE(weak.is_empty());
        REQUIRE_EQ(weak.ref_count_weak(), 2);
        REQUIRE_EQ(weak.get_unsafe(), new_rc.get());
        REQUIRE_EQ(rc->skr_rc_weak_ref_count(), 1);
        REQUIRE_EQ(new_rc->skr_rc_weak_ref_count(), 2);

        weak.reset(rc.get());
        REQUIRE_FALSE(weak.is_empty());
        REQUIRE_EQ(weak.ref_count_weak(), 2);
        REQUIRE_EQ(weak.get_unsafe(), rc.get());
        REQUIRE_EQ(rc->skr_rc_weak_ref_count(), 2);
        REQUIRE_EQ(new_rc->skr_rc_weak_ref_count(), 1);

        RCWeak<TestCounterBase> new_weak = new_rc;
        new_weak.swap(weak);
        REQUIRE_EQ(new_weak.ref_count_weak(), 2);
        REQUIRE_EQ(weak.ref_count_weak(), 2);
        REQUIRE_EQ(new_rc.ref_count_weak(), 2);
        REQUIRE_EQ(rc->skr_rc_weak_ref_count(), 2);
        REQUIRE_EQ(new_rc.get(), weak.get_unsafe());
        REQUIRE_EQ(rc.get(), new_weak.get_unsafe());
    }

    // [need't test] skr hash
    SUBCASE("test empty")
    {
        RCWeak<TestCounterBase> empty{};
        RCWeak<TestCounterBase> empty_copy{ empty };
        RCWeak<TestCounterBase> empty_move{ std::move(empty) };
        RCWeak<TestCounterBase> empty_assign, empty_move_assign;
        empty_assign      = empty;
        empty_move_assign = std::move(empty);

        empty.ref_count_weak();
        empty.is_empty();
        empty.is_alive();
        empty.is_expired();
        empty.operator bool();
        empty.reset();
        empty.reset(nullptr);
        empty.swap(empty_copy);
    }
}

TEST_CASE("Test Custom Deleter")
{
    using namespace skr;

    RC<TestCustomDeleter> rc{ SkrNew<TestCustomDeleter>() };
    rc.reset();
    REQUIRE_EQ(custom_deleter_count, 1);

    RCUnique<TestCustomDeleter> rc_unique{ SkrNew<TestCustomDeleter>() };
    rc_unique.reset();
    REQUIRE_EQ(custom_deleter_count, 2);

    {
        RC<TestCustomDeleter> rc{ SkrNew<TestCustomDeleter>() };
    }
    REQUIRE_EQ(custom_deleter_count, 3);

    {
        RCUnique<TestCustomDeleter> rc_unique{ SkrNew<TestCustomDeleter>() };
    }
    REQUIRE_EQ(custom_deleter_count, 4);
}