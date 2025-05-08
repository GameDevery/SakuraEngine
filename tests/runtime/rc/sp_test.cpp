#include "SkrTestFramework/framework.hpp"

#include <SkrCore/sp/sp.hpp>

static uint64_t sp_base_count           = 0;
static uint64_t sp_derived_count        = 0;
static uint64_t sp_custom_deleter_count = 0;

struct TestSPBase {
    TestSPBase()
    {
        ++sp_base_count;
    }
    virtual ~TestSPBase()
    {
        --sp_base_count;
    }
};

struct TestSPDerived : public TestSPBase {
public:
    TestSPDerived()
    {
        ++sp_derived_count;
    }
    ~TestSPDerived()
    {
        --sp_derived_count;
    }
};

struct TestCustomDeleterSP {
    inline void skr_sp_delete()
    {
        ++sp_custom_deleter_count;
        SkrDelete(this);
    }
};

TEST_CASE("Test UPtr")
{
    using namespace skr;
    SUBCASE("ctor & dtor")
    {
        UPtr<TestSPBase> empty_ctor{};
        REQUIRE_EQ(sp_base_count, 0);
        REQUIRE_EQ(sp_derived_count, 0);
        REQUIRE_EQ(empty_ctor.get(), nullptr);
        REQUIRE(empty_ctor.is_empty());

        UPtr<TestSPBase> null_ctor{ nullptr };
        REQUIRE_EQ(sp_base_count, 0);
        REQUIRE_EQ(sp_derived_count, 0);
        REQUIRE_EQ(null_ctor.get(), nullptr);
        REQUIRE(null_ctor.is_empty());

        {
            UPtr<TestSPBase> pointer_ctor{ SkrNew<TestSPBase>() };
            REQUIRE_EQ(sp_base_count, 1);
            REQUIRE_EQ(sp_derived_count, 0);
            REQUIRE_NE(pointer_ctor.get(), nullptr);
            REQUIRE_FALSE(pointer_ctor.is_empty());
        }

        {
            UPtr<TestSPBase> pointer_ctor{ SkrNew<TestSPDerived>() };
            REQUIRE_EQ(sp_base_count, 1);
            REQUIRE_EQ(sp_derived_count, 1);
            REQUIRE_NE(pointer_ctor.get(), nullptr);
            REQUIRE_FALSE(pointer_ctor.is_empty());
        }

        REQUIRE_EQ(sp_base_count, 0);
        REQUIRE_EQ(sp_derived_count, 0);
    }

    SUBCASE("copy & move")
    {
        UPtr<TestSPBase>    base{ SkrNew<TestSPBase>() };
        UPtr<TestSPDerived> derived{ SkrNew<TestSPDerived>() };

        auto             cached_base = base.get();
        UPtr<TestSPBase> move_base   = std::move(base);
        REQUIRE_EQ(sp_base_count, 2);
        REQUIRE_EQ(sp_derived_count, 1);
        REQUIRE_EQ(base.get(), nullptr);
        REQUIRE_EQ(move_base.get(), cached_base);

        auto             cached_derived = derived.get();
        UPtr<TestSPBase> move_derived   = std::move(derived);
        REQUIRE_EQ(sp_base_count, 2);
        REQUIRE_EQ(sp_derived_count, 1);
        REQUIRE_EQ(derived.get(), nullptr);
        REQUIRE_EQ(move_derived.get(), cached_derived);

        // will not pass compile
        // UPtr<TestSPBase> copy_base    = move_base;
        // UPtr<TestSPBase> copy_derived = move_derived;

        move_base    = nullptr;
        move_derived = nullptr;
        REQUIRE_EQ(sp_base_count, 0);
        REQUIRE_EQ(sp_derived_count, 0);
    }

    SUBCASE("assign & move assign")
    {
        UPtr<TestSPBase>    base{ SkrNew<TestSPBase>() };
        UPtr<TestSPDerived> derived{ SkrNew<TestSPDerived>() };

        auto             cached_base = base.get();
        UPtr<TestSPBase> move_assign_base{};
        move_assign_base = std::move(base);
        REQUIRE_EQ(sp_base_count, 2);
        REQUIRE_EQ(sp_derived_count, 1);
        REQUIRE_EQ(base.get(), nullptr);
        REQUIRE_EQ(move_assign_base.get(), cached_base);

        auto             cached_derived = derived.get();
        UPtr<TestSPBase> move_assign_derived{};
        move_assign_derived = std::move(derived);
        REQUIRE_EQ(sp_base_count, 2);
        REQUIRE_EQ(sp_derived_count, 1);
        REQUIRE_EQ(derived.get(), nullptr);
        REQUIRE_EQ(move_assign_derived.get(), cached_derived);

        // will not pass compile
        // UPtr<TestSPBase> assign_base{};
        // assign_base = move_base;
        // UPtr<TestSPBase> assign_derived {}
        // assign_derived = move_derived;

        move_assign_base    = nullptr;
        move_assign_derived = nullptr;
        REQUIRE_EQ(sp_base_count, 0);
        REQUIRE_EQ(sp_derived_count, 0);
    }

    // [need't test] factory
    // [need't test] getter

    SUBCASE("test is empty")
    {
        UPtr<TestSPBase> empty{};
        UPtr<TestSPBase> something{ SkrNew<TestSPBase>() };

        REQUIRE_EQ(empty.is_empty(), true);
        REQUIRE_EQ(something.is_empty(), false);
        REQUIRE_EQ(empty, false);
        REQUIRE_EQ(something, true);
    }

    SUBCASE("test ops")
    {
        UPtr<TestSPBase> rc = SkrNew<TestSPBase>();
        REQUIRE_EQ(sp_base_count, 1);
        REQUIRE_EQ(sp_derived_count, 0);
        REQUIRE_NE(rc.get(), nullptr);

        rc.reset();
        REQUIRE_EQ(sp_base_count, 0);
        REQUIRE_EQ(sp_derived_count, 0);
        REQUIRE_EQ(rc.get(), nullptr);

        rc.reset(SkrNew<TestSPBase>());
        REQUIRE_EQ(sp_base_count, 1);
        REQUIRE_EQ(sp_derived_count, 0);
        REQUIRE_NE(rc.get(), nullptr);

        UPtr<TestSPBase> other;
        rc.swap(other);
        REQUIRE_EQ(sp_base_count, 1);
        REQUIRE_EQ(sp_derived_count, 0);
        REQUIRE_EQ(rc.get(), nullptr);
        REQUIRE_NE(other.get(), nullptr);

        other.reset(SkrNew<TestSPBase>());
        REQUIRE_EQ(sp_base_count, 1);
        REQUIRE_EQ(sp_derived_count, 0);
        REQUIRE_NE(other.get(), nullptr);
    }

    // [need't test] pointer behaviour
    // [need't test] skr hash

    SUBCASE("test empty")
    {
        UPtr<TestSPBase> empty{};
        UPtr<TestSPBase> empty_move{ std::move(empty) };
        UPtr<TestSPBase> empty_move_assign;
        empty_move_assign = std::move(empty);

        empty.get();
        empty.is_empty();
        empty.operator bool();
        empty.reset();
        empty.reset(nullptr);
        empty.release();
        empty.swap(empty_move);
    }
}

TEST_CASE("Test Custom Deleter")
{
    using namespace skr;

    UPtr<TestCustomDeleterSP> uptr{ SkrNew<TestCustomDeleterSP>() };
    uptr.reset();
    REQUIRE_EQ(sp_custom_deleter_count, 1);

    {
        UPtr<TestCustomDeleterSP> uptr{ SkrNew<TestCustomDeleterSP>() };
        REQUIRE_EQ(sp_custom_deleter_count, 1);
    }
    REQUIRE_EQ(sp_custom_deleter_count, 2);
}