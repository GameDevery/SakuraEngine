#include "SkrTestFramework/framework.hpp"
#include "SkrOS/datetime.hpp"
#include "SkrOS/filesystem.hpp"
#include <thread>
#include <chrono>

using namespace skr;

TEST_CASE("TimeSpan Basic Operations")
{
    SUBCASE("Default Constructor")
    {
        TimeSpan ts;
        REQUIRE_EQ(ts.ticks, 0);
        REQUIRE_EQ(ts, TimeSpan::Zero);
    }

    SUBCASE("Constructor with Ticks")
    {
        TimeSpan ts(1000000);  // 100 milliseconds
        REQUIRE_EQ(ts.ticks, 1000000);
        REQUIRE_EQ(ts.total_milliseconds(), 100.0);
    }

    SUBCASE("Factory Methods")
    {
        auto ts_days = TimeSpan::from_days(1.5);
        REQUIRE_EQ(ts_days.total_hours(), 36.0);
        
        auto ts_hours = TimeSpan::from_hours(2.5);
        REQUIRE_EQ(ts_hours.total_minutes(), 150.0);
        
        auto ts_minutes = TimeSpan::from_minutes(90);
        REQUIRE_EQ(ts_minutes.total_seconds(), 5400.0);
        
        auto ts_seconds = TimeSpan::from_seconds(45.5);
        REQUIRE_EQ(ts_seconds.total_milliseconds(), 45500.0);
        
        auto ts_ms = TimeSpan::from_milliseconds(1500);
        REQUIRE_EQ(ts_ms.total_seconds(), 1.5);
        
        auto ts_us = TimeSpan::from_microseconds(2500);
        REQUIRE_EQ(ts_us.total_milliseconds(), 2.5);
    }

    SUBCASE("Component Extraction")
    {
        // 2 days, 3 hours, 4 minutes, 5 seconds, 6 milliseconds, 7 microseconds
        auto ts = TimeSpan::from_days(2) + 
                  TimeSpan::from_hours(3) + 
                  TimeSpan::from_minutes(4) + 
                  TimeSpan::from_seconds(5) + 
                  TimeSpan::from_milliseconds(6) +
                  TimeSpan::from_microseconds(7);
        
        REQUIRE_EQ(ts.days(), 2);
        REQUIRE_EQ(ts.hours(), 3);
        REQUIRE_EQ(ts.minutes(), 4);
        REQUIRE_EQ(ts.seconds(), 5);
        REQUIRE_EQ(ts.milliseconds(), 6);
        REQUIRE_EQ(ts.microseconds(), 7);
    }

    SUBCASE("Arithmetic Operations")
    {
        auto ts1 = TimeSpan::from_seconds(30);
        auto ts2 = TimeSpan::from_seconds(15);
        
        auto sum = ts1 + ts2;
        REQUIRE_EQ(sum.total_seconds(), 45.0);
        
        auto diff = ts1 - ts2;
        REQUIRE_EQ(diff.total_seconds(), 15.0);
        
        auto neg = -ts1;
        REQUIRE_EQ(neg.total_seconds(), -30.0);
    }

    SUBCASE("Comparison Operations")
    {
        auto ts1 = TimeSpan::from_seconds(30);
        auto ts2 = TimeSpan::from_seconds(15);
        auto ts3 = TimeSpan::from_seconds(30);
        
        REQUIRE(ts1 > ts2);
        REQUIRE(ts2 < ts1);
        REQUIRE(ts1 >= ts3);
        REQUIRE(ts1 <= ts3);
        REQUIRE(ts1 == ts3);
        REQUIRE(ts1 != ts2);
    }

    SUBCASE("Constants")
    {
        REQUIRE_EQ(TimeSpan::Zero.ticks, 0);
        REQUIRE(TimeSpan::MinValue.ticks < 0);
        REQUIRE(TimeSpan::MaxValue.ticks > 0);
    }
}

TEST_CASE("DateTime Basic Operations")
{
    SUBCASE("Default Constructor")
    {
        DateTime dt;
        REQUIRE_EQ(dt.get_ticks(), 0);
        REQUIRE_EQ(dt, DateTime::MinValue);
    }

    SUBCASE("Component Constructor")
    {
        DateTime dt1(2024, 3, 15, 14, 30, 45, 123);
        REQUIRE_EQ(dt1.year(), 2024);
        REQUIRE_EQ(dt1.month(), 3);
        REQUIRE_EQ(dt1.day(), 15);
        REQUIRE_EQ(dt1.hour(), 14);
        REQUIRE_EQ(dt1.minute(), 30);
        REQUIRE_EQ(dt1.second(), 45);
        REQUIRE_EQ(dt1.millisecond(), 123);
        
        // Test invalid dates
        DateTime invalid1(2024, 13, 1);  // Invalid month
        REQUIRE_EQ(invalid1.get_ticks(), 0);
        
        DateTime invalid2(2024, 2, 30);  // Invalid day for February
        REQUIRE_EQ(invalid2.get_ticks(), 0);
    }

    SUBCASE("Leap Year Handling")
    {
        REQUIRE(DateTime::is_leap_year(2024));
        REQUIRE_FALSE(DateTime::is_leap_year(2023));
        REQUIRE(DateTime::is_leap_year(2000));
        REQUIRE_FALSE(DateTime::is_leap_year(1900));
        
        REQUIRE_EQ(DateTime::days_in_month(2024, 2), 29);
        REQUIRE_EQ(DateTime::days_in_month(2023, 2), 28);
        
        // February 29 in leap year
        DateTime leap_day(2024, 2, 29);
        REQUIRE_NE(leap_day.get_ticks(), 0);
        REQUIRE_EQ(leap_day.day(), 29);
    }

    SUBCASE("Day of Week")
    {
        // January 1, 2024 is Monday
        DateTime dt1(2024, 1, 1);
        REQUIRE_EQ(dt1.day_of_week(), DayOfWeek::Monday);
        
        // Test other days
        DateTime dt2(2024, 1, 7);  // Sunday
        REQUIRE_EQ(dt2.day_of_week(), DayOfWeek::Sunday);
        
        DateTime dt3(2024, 1, 5);  // Friday
        REQUIRE_EQ(dt3.day_of_week(), DayOfWeek::Friday);
    }

    SUBCASE("Day of Year")
    {
        DateTime dt1(2024, 1, 1);
        REQUIRE_EQ(dt1.day_of_year(), 1);
        
        DateTime dt2(2024, 12, 31);
        REQUIRE_EQ(dt2.day_of_year(), 366);  // Leap year
        
        DateTime dt3(2023, 12, 31);
        REQUIRE_EQ(dt3.day_of_year(), 365);  // Non-leap year
        
        DateTime dt4(2024, 3, 1);
        REQUIRE_EQ(dt4.day_of_year(), 61);  // 31 (Jan) + 29 (Feb) + 1
    }

    SUBCASE("Date and Time Properties")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45, 123);
        
        auto date_only = dt.date();
        REQUIRE_EQ(date_only.year(), 2024);
        REQUIRE_EQ(date_only.month(), 3);
        REQUIRE_EQ(date_only.day(), 15);
        REQUIRE_EQ(date_only.hour(), 0);
        REQUIRE_EQ(date_only.minute(), 0);
        REQUIRE_EQ(date_only.second(), 0);
        REQUIRE_EQ(date_only.millisecond(), 0);
        
        auto time_of_day = dt.time_of_day();
        REQUIRE_EQ(time_of_day.hours(), 14);
        REQUIRE_EQ(time_of_day.minutes(), 30);
        REQUIRE_EQ(time_of_day.seconds(), 45);
        REQUIRE_EQ(time_of_day.milliseconds(), 123);
    }
}

TEST_CASE("DateTime Arithmetic Operations")
{
    SUBCASE("Add/Subtract TimeSpan")
    {
        DateTime dt(2024, 3, 15, 12, 0, 0);
        
        auto dt2 = dt.add(TimeSpan::from_days(1));
        REQUIRE_EQ(dt2.day(), 16);
        
        auto dt3 = dt.add(TimeSpan::from_hours(13));
        REQUIRE_EQ(dt3.day(), 16);
        REQUIRE_EQ(dt3.hour(), 1);
        
        auto dt4 = dt.subtract(TimeSpan::from_hours(13));
        REQUIRE_EQ(dt4.day(), 14);
        REQUIRE_EQ(dt4.hour(), 23);
        
        // Using operators
        auto dt5 = dt + TimeSpan::from_minutes(90);
        REQUIRE_EQ(dt5.hour(), 13);
        REQUIRE_EQ(dt5.minute(), 30);
        
        auto dt6 = dt - TimeSpan::from_minutes(90);
        REQUIRE_EQ(dt6.hour(), 10);
        REQUIRE_EQ(dt6.minute(), 30);
    }

    SUBCASE("Subtract DateTime")
    {
        DateTime dt1(2024, 3, 15, 14, 30, 0);
        DateTime dt2(2024, 3, 15, 12, 0, 0);
        
        auto diff = dt1.subtract(dt2);
        REQUIRE_EQ(diff.total_hours(), 2.5);
        
        // Using operator
        auto diff2 = dt1 - dt2;
        REQUIRE_EQ(diff2.total_minutes(), 150.0);
    }

    SUBCASE("Add Years")
    {
        DateTime dt1(2024, 3, 15);
        auto dt2 = dt1.add_years(1);
        REQUIRE_EQ(dt2.year(), 2025);
        REQUIRE_EQ(dt2.month(), 3);
        REQUIRE_EQ(dt2.day(), 15);
        
        // Leap year edge case
        DateTime leap(2024, 2, 29);
        auto non_leap = leap.add_years(1);
        REQUIRE_EQ(non_leap.year(), 2025);
        REQUIRE_EQ(non_leap.month(), 2);
        REQUIRE_EQ(non_leap.day(), 28);  // Adjusted to valid date
    }

    SUBCASE("Add Months")
    {
        DateTime dt1(2024, 1, 31);
        
        auto dt2 = dt1.add_months(1);
        REQUIRE_EQ(dt2.year(), 2024);
        REQUIRE_EQ(dt2.month(), 2);
        REQUIRE_EQ(dt2.day(), 29);  // Adjusted to last day of February (leap year)
        
        auto dt3 = dt1.add_months(3);
        REQUIRE_EQ(dt3.month(), 4);
        REQUIRE_EQ(dt3.day(), 30);  // Adjusted to last day of April
        
        auto dt4 = dt1.add_months(13);
        REQUIRE_EQ(dt4.year(), 2025);
        REQUIRE_EQ(dt4.month(), 2);
        REQUIRE_EQ(dt4.day(), 28);  // Adjusted to last day of February (non-leap year)
        
        // Negative months
        DateTime dt5(2024, 3, 15);
        auto dt6 = dt5.add_months(-2);
        REQUIRE_EQ(dt6.year(), 2024);
        REQUIRE_EQ(dt6.month(), 1);
        REQUIRE_EQ(dt6.day(), 15);
    }

    SUBCASE("Add Time Units")
    {
        DateTime dt(2024, 3, 15, 12, 0, 0);
        
        auto dt2 = dt.add_days(1.5);
        REQUIRE_EQ(dt2.day(), 17);
        REQUIRE_EQ(dt2.hour(), 0);
        
        auto dt3 = dt.add_hours(25.5);
        REQUIRE_EQ(dt3.day(), 16);
        REQUIRE_EQ(dt3.hour(), 13);
        REQUIRE_EQ(dt3.minute(), 30);
        
        auto dt4 = dt.add_minutes(90.5);
        REQUIRE_EQ(dt4.hour(), 13);
        REQUIRE_EQ(dt4.minute(), 30);
        REQUIRE_EQ(dt4.second(), 30);
        
        auto dt5 = dt.add_seconds(3661);
        REQUIRE_EQ(dt5.hour(), 13);
        REQUIRE_EQ(dt5.minute(), 1);
        REQUIRE_EQ(dt5.second(), 1);
        
        auto dt6 = dt.add_milliseconds(1500);
        REQUIRE_EQ(dt6.second(), 1);
        REQUIRE_EQ(dt6.millisecond(), 500);
        
        auto dt7 = dt.add_microseconds(2500);
        REQUIRE_EQ(dt7.millisecond(), 2);
        REQUIRE_EQ(dt7.microsecond(), 500);
    }
}

TEST_CASE("DateTime Comparison Operations")
{
    DateTime dt1(2024, 3, 15, 14, 30, 0);
    DateTime dt2(2024, 3, 15, 12, 0, 0);
    DateTime dt3(2024, 3, 15, 14, 30, 0);
    DateTime dt4(2024, 3, 16, 14, 30, 0);
    
    REQUIRE(dt1 > dt2);
    REQUIRE(dt2 < dt1);
    REQUIRE(dt1 >= dt3);
    REQUIRE(dt1 <= dt3);
    REQUIRE(dt1 == dt3);
    REQUIRE(dt1 != dt2);
    REQUIRE(dt4 > dt1);
}

TEST_CASE("DateTime FileTime Conversion")
{
    SUBCASE("FileTime Round-trip")
    {
        // Test with a known date after FileTime epoch
        DateTime dt(2024, 3, 15, 14, 30, 45, 123);
        uint64_t filetime = dt.to_filetime();
        DateTime dt2 = DateTime::from_filetime(filetime);
        
        REQUIRE_EQ(dt, dt2);
        REQUIRE_EQ(dt2.year(), 2024);
        REQUIRE_EQ(dt2.month(), 3);
        REQUIRE_EQ(dt2.day(), 15);
        REQUIRE_EQ(dt2.hour(), 14);
        REQUIRE_EQ(dt2.minute(), 30);
        REQUIRE_EQ(dt2.second(), 45);
        REQUIRE_EQ(dt2.millisecond(), 123);
    }

    SUBCASE("FileTime Epoch")
    {
        // January 1, 1601 should convert to FileTime 0
        DateTime epoch(1601, 1, 1);
        REQUIRE_EQ(epoch.to_filetime(), 0);
        
        // Convert back
        DateTime epoch2 = DateTime::from_filetime(0);
        REQUIRE_EQ(epoch2.year(), 1601);
        REQUIRE_EQ(epoch2.month(), 1);
        REQUIRE_EQ(epoch2.day(), 1);
    }

    SUBCASE("Dates Before FileTime Epoch")
    {
        DateTime old_date(1500, 1, 1);
        REQUIRE_EQ(old_date.to_filetime(), 0);  // Cannot represent
    }

    SUBCASE("Integration with filesystem")
    {
        using namespace skr::fs;
        
        // Test conversion helper functions
        FileTime ft = unix_to_filetime(0);  // Unix epoch
        DateTime dt = filetime_to_datetime(ft);
        REQUIRE_EQ(dt.year(), 1970);
        REQUIRE_EQ(dt.month(), 1);
        REQUIRE_EQ(dt.day(), 1);
        
        // Convert back
        uint64_t ft2 = datetime_to_filetime(dt);
        REQUIRE_EQ(ft, ft2);
    }
}

TEST_CASE("DateTime Unix Timestamp Conversion")
{
    SUBCASE("Unix Timestamp Round-trip")
    {
        // Test with Unix epoch
        DateTime unix_epoch = DateTime::from_unix_timestamp(0);
        REQUIRE_EQ(unix_epoch.year(), 1970);
        REQUIRE_EQ(unix_epoch.month(), 1);
        REQUIRE_EQ(unix_epoch.day(), 1);
        REQUIRE_EQ(unix_epoch.hour(), 0);
        REQUIRE_EQ(unix_epoch.minute(), 0);
        REQUIRE_EQ(unix_epoch.second(), 0);
        
        // Round-trip test
        int64_t timestamp = unix_epoch.to_unix_timestamp();
        REQUIRE_EQ(timestamp, 0);
        
        // Test with a known timestamp
        // March 15, 2024, 14:30:45 UTC
        DateTime dt(2024, 3, 15, 14, 30, 45);
        int64_t ts = dt.to_unix_timestamp();
        DateTime dt2 = DateTime::from_unix_timestamp(ts);
        
        // Note: We lose sub-second precision with unix timestamps
        REQUIRE_EQ(dt2.year(), 2024);
        REQUIRE_EQ(dt2.month(), 3);
        REQUIRE_EQ(dt2.day(), 15);
        REQUIRE_EQ(dt2.hour(), 14);
        REQUIRE_EQ(dt2.minute(), 30);
        REQUIRE_EQ(dt2.second(), 45);
    }

    SUBCASE("Unix Timestamp Milliseconds")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45, 123);
        int64_t ts_ms = dt.to_unix_timestamp_ms();
        DateTime dt2 = DateTime::from_unix_timestamp_ms(ts_ms);
        
        REQUIRE_EQ(dt2.year(), 2024);
        REQUIRE_EQ(dt2.month(), 3);
        REQUIRE_EQ(dt2.day(), 15);
        REQUIRE_EQ(dt2.hour(), 14);
        REQUIRE_EQ(dt2.minute(), 30);
        REQUIRE_EQ(dt2.second(), 45);
        REQUIRE_EQ(dt2.millisecond(), 123);
    }

    SUBCASE("Dates Before Unix Epoch")
    {
        DateTime old_date(1900, 1, 1);
        REQUIRE_EQ(old_date.to_unix_timestamp(), 0);  // Cannot represent
    }
}

TEST_CASE("DateTime String Formatting and Parsing")
{
    SUBCASE("Default String Format")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45);
        skr::String str = dt.to_string();
        REQUIRE_EQ(str, u8"2024-03-15 14:30:45");
    }

    SUBCASE("ISO 8601 Format")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45, 123);
        skr::String iso = dt.to_iso8601_string();
        // Should be something like "2024-03-15T14:30:45.1230000Z"
        REQUIRE(iso.contains(u8"2024-03-15T14:30:45"));
        REQUIRE(iso.ends_with(u8"Z"));
    }

    SUBCASE("Parse String")
    {
        skr::StringView str = u8"2024-03-15 14:30:45";
        DateTime dt;
        bool success = DateTime::try_parse(str, dt);
        
        REQUIRE(success);
        REQUIRE_EQ(dt.year(), 2024);
        REQUIRE_EQ(dt.month(), 3);
        REQUIRE_EQ(dt.day(), 15);
        REQUIRE_EQ(dt.hour(), 14);
        REQUIRE_EQ(dt.minute(), 30);
        REQUIRE_EQ(dt.second(), 45);
        
        // Test invalid string
        skr::StringView invalid = u8"not a date";
        DateTime dt2;
        bool success2 = DateTime::try_parse(invalid, dt2);
        REQUIRE_FALSE(success2);
    }
}

TEST_CASE("DateTime Current Time Functions")
{
    SUBCASE("Now and UtcNow")
    {
        auto start = DateTime::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto end = DateTime::now();
        
        REQUIRE(end > start);
        auto diff = end - start;
        REQUIRE(diff.total_milliseconds() >= 10.0);
        
        // UTC now should also work
        auto utc = DateTime::utc_now();
        REQUIRE(utc.get_ticks() > 0);
    }

    SUBCASE("Today")
    {
        auto now = DateTime::now();
        auto today = DateTime::today();
        
        REQUIRE_EQ(today.year(), now.year());
        REQUIRE_EQ(today.month(), now.month());
        REQUIRE_EQ(today.day(), now.day());
        REQUIRE_EQ(today.hour(), 0);
        REQUIRE_EQ(today.minute(), 0);
        REQUIRE_EQ(today.second(), 0);
        REQUIRE_EQ(today.millisecond(), 0);
    }
}

TEST_CASE("DateTime Special Cases and Edge Cases")
{
    SUBCASE("Year Boundaries")
    {
        // End of year
        DateTime end_year(2023, 12, 31, 23, 59, 59, 999);
        auto next_year = end_year.add_seconds(1);
        REQUIRE_EQ(next_year.year(), 2024);
        REQUIRE_EQ(next_year.month(), 1);
        REQUIRE_EQ(next_year.day(), 1);
        REQUIRE_EQ(next_year.hour(), 0);
        REQUIRE_EQ(next_year.minute(), 0);
        REQUIRE_EQ(next_year.second(), 0);
        
        // Beginning of year
        DateTime begin_year(2024, 1, 1, 0, 0, 0);
        auto prev_year = begin_year.subtract(TimeSpan::from_seconds(1));
        REQUIRE_EQ(prev_year.year(), 2023);
        REQUIRE_EQ(prev_year.month(), 12);
        REQUIRE_EQ(prev_year.day(), 31);
        REQUIRE_EQ(prev_year.hour(), 23);
        REQUIRE_EQ(prev_year.minute(), 59);
        REQUIRE_EQ(prev_year.second(), 59);
    }

    SUBCASE("Month Boundaries with Different Days")
    {
        // 31-day month to 30-day month
        DateTime march31(2024, 3, 31);
        auto april30 = march31.add_months(1);
        REQUIRE_EQ(april30.month(), 4);
        REQUIRE_EQ(april30.day(), 30);  // Clamped to last day
        
        // 31-day month to 28-day month (non-leap)
        DateTime jan31_2023(2023, 1, 31);
        auto feb28_2023 = jan31_2023.add_months(1);
        REQUIRE_EQ(feb28_2023.month(), 2);
        REQUIRE_EQ(feb28_2023.day(), 28);
        
        // 31-day month to 29-day month (leap)
        DateTime jan31_2024(2024, 1, 31);
        auto feb29_2024 = jan31_2024.add_months(1);
        REQUIRE_EQ(feb29_2024.month(), 2);
        REQUIRE_EQ(feb29_2024.day(), 29);
    }

    SUBCASE("Microsecond Precision")
    {
        DateTime dt1(2024, 3, 15, 14, 30, 45, 123);
        auto dt2 = dt1.add_microseconds(456);
        
        REQUIRE_EQ(dt2.millisecond(), 123);
        REQUIRE_EQ(dt2.microsecond(), 456);
        
        auto dt3 = dt1.add_microseconds(1456);
        REQUIRE_EQ(dt3.millisecond(), 124);
        REQUIRE_EQ(dt3.microsecond(), 456);
    }

    SUBCASE("Constants")
    {
        REQUIRE_EQ(DateTime::MinValue.get_ticks(), 0);
        REQUIRE_EQ(DateTime::MaxValue.get_ticks(), UINT64_MAX);
        REQUIRE_EQ(DateTime::UnixEpoch.year(), 1970);
        REQUIRE_EQ(DateTime::UnixEpoch.month(), 1);
        REQUIRE_EQ(DateTime::UnixEpoch.day(), 1);
        REQUIRE_EQ(DateTime::FileTimeEpoch.year(), 1601);
        REQUIRE_EQ(DateTime::FileTimeEpoch.month(), 1);
        REQUIRE_EQ(DateTime::FileTimeEpoch.day(), 1);
    }
}

TEST_CASE("DateTime Performance and Stress Tests")
{
    SUBCASE("Rapid Creation and Comparison")
    {
        const int iterations = 10000;
        auto start = DateTime::now();
        
        for (int i = 0; i < iterations; ++i)
        {
            DateTime dt(2000 + (i % 100), 1 + (i % 12), 1 + (i % 28));
            auto dt2 = dt.add_days(i % 365);
            volatile bool result = dt2 > dt;  // Prevent optimization
            (void)result;
        }
        
        auto end = DateTime::now();
        auto elapsed = end - start;
        
        // Just ensure it completes in reasonable time
        REQUIRE(elapsed.total_seconds() < 1.0);
    }

    SUBCASE("Date Arithmetic Accuracy")
    {
        // Add one day 365 times
        DateTime start(2023, 1, 1);
        DateTime current = start;
        
        for (int i = 0; i < 365; ++i)
        {
            current = current.add_days(1);
        }
        
        REQUIRE_EQ(current.year(), 2024);
        REQUIRE_EQ(current.month(), 1);
        REQUIRE_EQ(current.day(), 1);
        
        // Add one hour 24 times
        DateTime hour_start(2024, 3, 15, 0, 0, 0);
        DateTime hour_current = hour_start;
        
        for (int i = 0; i < 24; ++i)
        {
            hour_current = hour_current.add_hours(1);
        }
        
        REQUIRE_EQ(hour_current.day(), 16);
        REQUIRE_EQ(hour_current.hour(), 0);
    }

    SUBCASE("Extreme Dates")
    {
        // Very early date
        DateTime early(1, 1, 1);
        REQUIRE_EQ(early.year(), 1);
        REQUIRE_EQ(early.get_ticks(), 0);
        
        // Very late date (within valid range)
        DateTime late(9999, 12, 31, 23, 59, 59, 999);
        REQUIRE_EQ(late.year(), 9999);
        REQUIRE_EQ(late.month(), 12);
        REQUIRE_EQ(late.day(), 31);
        
        // Test arithmetic near boundaries
        auto late_plus_one = late.add_days(1);
        REQUIRE(late_plus_one.year() <= 9999);  // Should handle overflow gracefully
    }
}

TEST_CASE("DateTime Thread Safety")
{
    SUBCASE("Concurrent Now() Calls")
    {
        const int thread_count = 4;
        const int iterations_per_thread = 1000;
        std::vector<std::thread> threads;
        std::vector<DateTime> results(thread_count * iterations_per_thread);
        
        auto worker = [&results](int thread_id, int iterations) {
            for (int i = 0; i < iterations; ++i)
            {
                results[thread_id * iterations + i] = DateTime::now();
            }
        };
        
        // Start all threads
        for (int i = 0; i < thread_count; ++i)
        {
            threads.emplace_back(worker, i, iterations_per_thread);
        }
        
        // Wait for completion
        for (auto& t : threads)
        {
            t.join();
        }
        
        // Verify all times are valid and increasing (roughly)
        DateTime prev = results[0];
        for (size_t i = 1; i < results.size(); ++i)
        {
            REQUIRE(results[i].get_ticks() > 0);
            // Times should be close to each other (within a second)
            auto diff = results[i] - prev;
            REQUIRE(std::abs(diff.total_seconds()) < 1.0);
        }
    }
}