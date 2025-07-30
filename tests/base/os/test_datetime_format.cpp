#include "SkrTestFramework/framework.hpp"
#include "SkrBase/containers/string/format.hpp"
#include "SkrOS/datetime.hpp"

using namespace skr;

TEST_CASE("DateTime and TimeSpan Formatting")
{
    SUBCASE("DateTime Default Formatting")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45, 123);
        
        // Default format
        auto result = skr::format(u8"{}", dt);
        REQUIRE_EQ(result, u8"2024-03-15 14:30:45");
        
        // Explicit default format
        auto result_d = skr::format(u8"{:d}", dt);
        REQUIRE_EQ(result_d, u8"2024-03-15 14:30:45");
    }
    
    SUBCASE("DateTime ISO 8601 Formatting")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45, 123);
        
        auto result = skr::format(u8"{:s}", dt);
        REQUIRE(result.starts_with(u8"2024-03-15T14:30:45."));
        REQUIRE(result.ends_with(u8"Z"));
    }
    
    SUBCASE("DateTime Time Only Formatting")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45);
        
        auto result = skr::format(u8"{:t}", dt);
        REQUIRE_EQ(result, u8"14:30:45");
    }
    
    SUBCASE("DateTime Date Only Formatting")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45);
        
        auto result = skr::format(u8"{:D}", dt);
        REQUIRE_EQ(result, u8"2024-03-15");
    }
    
    SUBCASE("DateTime Full Formatting with Milliseconds")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45, 123);
        
        auto result = skr::format(u8"{:f}", dt);
        REQUIRE_EQ(result, u8"2024-03-15 14:30:45.123");
    }
    
    SUBCASE("TimeSpan Default Formatting")
    {
        // 2 hours, 30 minutes, 45 seconds
        TimeSpan ts = TimeSpan::from_hours(2) + TimeSpan::from_minutes(30) + TimeSpan::from_seconds(45);
        
        auto result = skr::format(u8"{}", ts);
        REQUIRE_EQ(result, u8"02:30:45");
        
        // With days
        TimeSpan ts_days = TimeSpan::from_days(3) + ts;
        auto result_days = skr::format(u8"{}", ts_days);
        REQUIRE_EQ(result_days, u8"3.02:30:45");
    }
    
    SUBCASE("TimeSpan General Short Formatting")
    {
        TimeSpan ts = TimeSpan::from_hours(2) + TimeSpan::from_minutes(30) + TimeSpan::from_seconds(45);
        
        auto result = skr::format(u8"{:g}", ts);
        REQUIRE_EQ(result, u8"2:30:45");
        
        // With days
        TimeSpan ts_days = TimeSpan::from_days(3) + ts;
        auto result_days = skr::format(u8"{:g}", ts_days);
        REQUIRE_EQ(result_days, u8"3:2:30:45");
    }
    
    SUBCASE("TimeSpan General Long Formatting")
    {
        TimeSpan ts = TimeSpan::from_days(1) + TimeSpan::from_hours(2) + TimeSpan::from_minutes(30) + TimeSpan::from_seconds(45);
        
        auto result = skr::format(u8"{:G}", ts);
        REQUIRE(result.starts_with(u8"1:02:30:45."));
    }
    
    SUBCASE("Negative TimeSpan Formatting")
    {
        TimeSpan ts = -TimeSpan::from_hours(2);
        
        auto result = skr::format(u8"{}", ts);
        REQUIRE_EQ(result, u8"-02:00:00");
        
        auto result_g = skr::format(u8"{:g}", ts);
        REQUIRE_EQ(result_g, u8"-2:00:00");
    }
    
    SUBCASE("Mixed DateTime and TimeSpan Formatting")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45);
        TimeSpan ts = TimeSpan::from_hours(8);
        
        auto result = skr::format(u8"Current time: {}, Time zone offset: {}", dt, ts);
        REQUIRE(result.contains(u8"Current time: 2024-03-15 14:30:45"));
        REQUIRE(result.contains(u8"Time zone offset: 08:00:00"));
    }
}

TEST_CASE("DateTime Complete Format Spec Tests")
{
    SUBCASE("All DateTime Format Specifiers")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45, 123);
        
        // Default format 'd' - "yyyy-MM-dd HH:mm:ss"
        auto result_default = skr::format(u8"{}", dt);
        REQUIRE_EQ(result_default, u8"2024-03-15 14:30:45");
        
        auto result_d = skr::format(u8"{:d}", dt);
        REQUIRE_EQ(result_d, u8"2024-03-15 14:30:45");
        
        // ISO 8601 format 's' - "yyyy-MM-ddTHH:mm:ss.fffffffZ"
        auto result_s = skr::format(u8"{:s}", dt);
        REQUIRE(result_s.starts_with(u8"2024-03-15T14:30:45."));
        REQUIRE(result_s.ends_with(u8"Z"));
        REQUIRE(result_s.size() == 28); // Fixed length with 7 decimal places
        
        // Time only format 't' - "HH:mm:ss"
        auto result_t = skr::format(u8"{:t}", dt);
        REQUIRE_EQ(result_t, u8"14:30:45");
        
        // Date only format 'D' - "yyyy-MM-dd"
        auto result_D = skr::format(u8"{:D}", dt);
        REQUIRE_EQ(result_D, u8"2024-03-15");
        
        // Full with milliseconds format 'f' - "yyyy-MM-dd HH:mm:ss.fff"
        auto result_f = skr::format(u8"{:f}", dt);
        REQUIRE_EQ(result_f, u8"2024-03-15 14:30:45.123");
    }
    
    SUBCASE("DateTime Format with Various Values")
    {
        // Single digit values
        DateTime dt1(2024, 1, 5, 9, 7, 3, 5);
        REQUIRE_EQ(skr::format(u8"{:d}", dt1), u8"2024-01-05 09:07:03");
        REQUIRE_EQ(skr::format(u8"{:t}", dt1), u8"09:07:03");
        REQUIRE_EQ(skr::format(u8"{:D}", dt1), u8"2024-01-05");
        REQUIRE_EQ(skr::format(u8"{:f}", dt1), u8"2024-01-05 09:07:03.005");
        
        // No milliseconds
        DateTime dt2(2024, 12, 31, 23, 59, 59, 0);
        REQUIRE_EQ(skr::format(u8"{:f}", dt2), u8"2024-12-31 23:59:59.000");
        
        // Maximum milliseconds
        DateTime dt3(2024, 6, 15, 12, 30, 45, 999);
        REQUIRE_EQ(skr::format(u8"{:f}", dt3), u8"2024-06-15 12:30:45.999");
    }
    
    SUBCASE("DateTime ISO8601 Format Details")
    {
        // Test microsecond precision in ISO format
        DateTime dt1(2024, 3, 15, 14, 30, 45, 123);
        auto iso_result = skr::format(u8"{:s}", dt1);
        
        // Parse the fractional seconds part
        auto dot_pos = iso_result.find(u8'.');
        REQUIRE(dot_pos);
        auto z_pos = iso_result.find(u8'Z');
        REQUIRE(z_pos);
        auto fraction_part = iso_result.substr_copy(dot_pos.index() + 1, z_pos.index() - dot_pos.index() - 1);
        REQUIRE_EQ(fraction_part.size(), 7); // Should have 7 digits
        
        // Edge case: midnight
        DateTime midnight(2024, 1, 1, 0, 0, 0, 0);
        auto midnight_iso = skr::format(u8"{:s}", midnight);
        REQUIRE(midnight_iso.starts_with(u8"2024-01-01T00:00:00."));
    }
}

TEST_CASE("TimeSpan Complete Format Spec Tests")
{
    SUBCASE("All TimeSpan Format Specifiers")
    {
        // Test case 1: Hours, minutes, seconds
        TimeSpan ts1 = TimeSpan::from_hours(2) + TimeSpan::from_minutes(30) + TimeSpan::from_seconds(45);
        
        // Default format 'c' - constant format
        auto result_default = skr::format(u8"{}", ts1);
        REQUIRE_EQ(result_default, u8"02:30:45");
        
        auto result_c = skr::format(u8"{:c}", ts1);
        REQUIRE_EQ(result_c, u8"02:30:45");
        
        // General short format 'g'
        auto result_g = skr::format(u8"{:g}", ts1);
        REQUIRE_EQ(result_g, u8"2:30:45");
        
        // General long format 'G' with fractional seconds
        auto result_G = skr::format(u8"{:G}", ts1);
        REQUIRE_EQ(result_G, u8"0:02:30:45.0000000");
        
        // Test case 2: With days
        TimeSpan ts2 = TimeSpan::from_days(3) + TimeSpan::from_hours(14) + TimeSpan::from_minutes(30) + TimeSpan::from_seconds(45);
        
        auto result2_c = skr::format(u8"{:c}", ts2);
        REQUIRE_EQ(result2_c, u8"3.14:30:45");
        
        auto result2_g = skr::format(u8"{:g}", ts2);
        REQUIRE_EQ(result2_g, u8"3:14:30:45");
        
        auto result2_G = skr::format(u8"{:G}", ts2);
        REQUIRE_EQ(result2_G, u8"3:14:30:45.0000000");
    }
    
    SUBCASE("TimeSpan Format with Various Values")
    {
        // Single digit values
        TimeSpan ts1 = TimeSpan::from_hours(1) + TimeSpan::from_minutes(5) + TimeSpan::from_seconds(9);
        REQUIRE_EQ(skr::format(u8"{:c}", ts1), u8"01:05:09");
        REQUIRE_EQ(skr::format(u8"{:g}", ts1), u8"1:05:09");
        
        // Zero days with hours > 24
        TimeSpan ts2 = TimeSpan::from_hours(26) + TimeSpan::from_minutes(15);
        REQUIRE_EQ(skr::format(u8"{:c}", ts2), u8"1.02:15:00");
        REQUIRE_EQ(skr::format(u8"{:g}", ts2), u8"1:2:15:00");
        
        // Maximum hours/minutes/seconds without days
        TimeSpan ts3 = TimeSpan::from_hours(23) + TimeSpan::from_minutes(59) + TimeSpan::from_seconds(59);
        REQUIRE_EQ(skr::format(u8"{:c}", ts3), u8"23:59:59");
        REQUIRE_EQ(skr::format(u8"{:g}", ts3), u8"23:59:59");
    }
    
    SUBCASE("TimeSpan with Fractional Seconds")
    {
        // Test milliseconds
        TimeSpan ts1 = TimeSpan::from_seconds(45) + TimeSpan::from_milliseconds(123);
        auto result_G = skr::format(u8"{:G}", ts1);
        REQUIRE(result_G.starts_with(u8"0:00:00:45."));
        
        // Test microseconds
        TimeSpan ts2 = TimeSpan::from_microseconds(123456);
        auto result2_G = skr::format(u8"{:G}", ts2);
        REQUIRE(result2_G.contains(u8"0:00:00:00."));
    }
    
    SUBCASE("Negative TimeSpan Formatting")
    {
        // Negative hours only
        TimeSpan ts1 = -TimeSpan::from_hours(2);
        REQUIRE_EQ(skr::format(u8"{:c}", ts1), u8"-02:00:00");
        REQUIRE_EQ(skr::format(u8"{:g}", ts1), u8"-2:00:00");
        REQUIRE_EQ(skr::format(u8"{:G}", ts1), u8"-0:02:00:00.0000000");
        
        // Negative with days
        TimeSpan ts2 = -(TimeSpan::from_days(1) + TimeSpan::from_hours(12) + TimeSpan::from_minutes(30));
        REQUIRE_EQ(skr::format(u8"{:c}", ts2), u8"-1.12:30:00");
        REQUIRE_EQ(skr::format(u8"{:g}", ts2), u8"-1:12:30:00");
        REQUIRE_EQ(skr::format(u8"{:G}", ts2), u8"-1:12:30:00.0000000");
        
        // Negative with all components
        TimeSpan ts3 = -(TimeSpan::from_days(2) + TimeSpan::from_hours(3) + TimeSpan::from_minutes(15) + TimeSpan::from_seconds(45));
        REQUIRE_EQ(skr::format(u8"{:c}", ts3), u8"-2.03:15:45");
        REQUIRE_EQ(skr::format(u8"{:g}", ts3), u8"-2:3:15:45");
    }
}

TEST_CASE("DateTime and TimeSpan Edge Cases")
{
    SUBCASE("DateTime Boundary Values")
    {
        // Minimum values
        DateTime min_dt(1, 1, 1, 0, 0, 0, 0);
        REQUIRE_EQ(skr::format(u8"{:d}", min_dt), u8"0001-01-01 00:00:00");
        REQUIRE_EQ(skr::format(u8"{:D}", min_dt), u8"0001-01-01");
        REQUIRE_EQ(skr::format(u8"{:t}", min_dt), u8"00:00:00");
        REQUIRE_EQ(skr::format(u8"{:f}", min_dt), u8"0001-01-01 00:00:00.000");
        
        // Maximum reasonable values
        DateTime max_dt(9999, 12, 31, 23, 59, 59, 999);
        REQUIRE_EQ(skr::format(u8"{:d}", max_dt), u8"9999-12-31 23:59:59");
        REQUIRE_EQ(skr::format(u8"{:D}", max_dt), u8"9999-12-31");
        REQUIRE_EQ(skr::format(u8"{:t}", max_dt), u8"23:59:59");
        REQUIRE_EQ(skr::format(u8"{:f}", max_dt), u8"9999-12-31 23:59:59.999");
    }
    
    SUBCASE("TimeSpan Boundary Values")
    {
        // Zero TimeSpan
        TimeSpan zero;
        REQUIRE_EQ(skr::format(u8"{:c}", zero), u8"00:00:00");
        REQUIRE_EQ(skr::format(u8"{:g}", zero), u8"0:00:00");
        REQUIRE_EQ(skr::format(u8"{:G}", zero), u8"0:00:00:00.0000000");
        
        // Very small positive TimeSpan
        TimeSpan tiny = TimeSpan::from_microseconds(1);
        auto tiny_G = skr::format(u8"{:G}", tiny);
        REQUIRE(tiny_G.contains(u8"0:00:00:00."));
        
        // Very large TimeSpan
        TimeSpan large = TimeSpan::from_days(10000) + TimeSpan::from_hours(23) + TimeSpan::from_minutes(59);
        REQUIRE_EQ(skr::format(u8"{:c}", large), u8"10000.23:59:00");
        REQUIRE_EQ(skr::format(u8"{:g}", large), u8"10000:23:59:00");
    }
    
    SUBCASE("Special DateTime Cases")
    {
        // Leap year February 29
        DateTime leap_dt(2024, 2, 29, 12, 0, 0);
        REQUIRE_EQ(skr::format(u8"{:D}", leap_dt), u8"2024-02-29");
        
        // Non-leap year February 28
        DateTime non_leap_dt(2023, 2, 28, 12, 0, 0);
        REQUIRE_EQ(skr::format(u8"{:D}", non_leap_dt), u8"2023-02-28");
        
        // New Year transition
        DateTime nye(2023, 12, 31, 23, 59, 59, 999);
        REQUIRE_EQ(skr::format(u8"{:f}", nye), u8"2023-12-31 23:59:59.999");
        
        DateTime ny(2024, 1, 1, 0, 0, 0, 0);
        REQUIRE_EQ(skr::format(u8"{:f}", ny), u8"2024-01-01 00:00:00.000");
    }
}

TEST_CASE("Format String Corner Cases")
{
    SUBCASE("Empty and Invalid Format Specs")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45);
        TimeSpan ts = TimeSpan::from_hours(2);
        
        // Empty format spec should use default
        REQUIRE_EQ(skr::format(u8"{:}", dt), u8"2024-03-15 14:30:45");
        REQUIRE_EQ(skr::format(u8"{:}", ts), u8"02:00:00");
        
        // Invalid format spec should fallback to default
        REQUIRE_EQ(skr::format(u8"{:x}", dt), u8"2024-03-15 14:30:45");
        REQUIRE_EQ(skr::format(u8"{:x}", ts), u8"02:00:00");
    }
    
    SUBCASE("Multiple Values in Format String")
    {
        DateTime dt(2024, 3, 15, 14, 30, 45);
        TimeSpan ts = TimeSpan::from_hours(8);
        
        auto result = skr::format(u8"Date: {:D}, Time: {:t}, Duration: {:g}", dt, dt, ts);
        REQUIRE_EQ(result, u8"Date: 2024-03-15, Time: 14:30:45, Duration: 8:00:00");
        
        // Mix different format specs
        auto result2 = skr::format(u8"ISO: {:s} | Default: {} | TimeSpan: {:G}", dt, dt, ts);
        REQUIRE(result2.contains(u8"ISO: 2024-03-15T14:30:45."));
        REQUIRE(result2.contains(u8"| Default: 2024-03-15 14:30:45 |"));
        REQUIRE(result2.contains(u8"| TimeSpan: 0:08:00:00.0000000"));
    }
}

TEST_CASE("DateTime Extended Timezone Tests")
{
    SUBCASE("Timezone Conversion with DST Consideration")
    {
        // Test around DST transition dates (these are approximate and may vary by region)
        DateTime winter_dt(2024, 1, 15, 12, 0, 0);  // Winter time
        DateTime summer_dt(2024, 7, 15, 12, 0, 0);  // Summer time
        
        // Convert to UTC and back
        DateTime winter_utc = DateTime::to_utc(winter_dt);
        DateTime winter_restored = DateTime::to_local(winter_utc);
        
        DateTime summer_utc = DateTime::to_utc(summer_dt);
        DateTime summer_restored = DateTime::to_local(summer_utc);
        
        // Round-trip should preserve the original times
        REQUIRE_EQ(winter_restored.hour(), winter_dt.hour());
        REQUIRE_EQ(summer_restored.hour(), summer_dt.hour());
        
        // UTC times should be different due to DST (in regions that observe DST)
        // Note: This test might not be meaningful in all timezones
        auto winter_offset = winter_dt - winter_utc;
        auto summer_offset = summer_dt - summer_utc;
        
        // Both offsets should be reasonable
        REQUIRE(std::abs(winter_offset.total_hours()) <= 14.0);
        REQUIRE(std::abs(summer_offset.total_hours()) <= 14.0);
    }
    
    SUBCASE("Edge Cases in Timezone Conversion")
    {
        // Test midnight transitions
        DateTime midnight(2024, 3, 15, 0, 0, 0);
        DateTime near_midnight(2024, 3, 15, 23, 59, 59);
        
        DateTime midnight_utc = DateTime::to_utc(midnight);
        DateTime midnight_restored = DateTime::to_local(midnight_utc);
        
        DateTime near_midnight_utc = DateTime::to_utc(near_midnight);
        DateTime near_midnight_restored = DateTime::to_local(near_midnight_utc);
        
        // Should preserve the original times (allowing for date changes due to timezone)
        REQUIRE_EQ(midnight_restored.hour(), midnight.hour());
        REQUIRE_EQ(midnight_restored.minute(), midnight.minute());
        REQUIRE_EQ(midnight_restored.second(), midnight.second());
        
        REQUIRE_EQ(near_midnight_restored.hour(), near_midnight.hour());
        REQUIRE_EQ(near_midnight_restored.minute(), near_midnight.minute());
        REQUIRE_EQ(near_midnight_restored.second(), near_midnight.second());
    }
    
    SUBCASE("Current Time Consistency Check")
    {
        // Get current times multiple ways
        DateTime local1 = DateTime::now();
        DateTime local2 = DateTime::local_now();
        DateTime utc = DateTime::utc_now();
        
        // Convert UTC to local
        DateTime converted_local = DateTime::to_local(utc);
        
        // All local times should be very close
        auto diff1 = local1 - local2;
        auto diff2 = local1 - converted_local;
        
        REQUIRE(std::abs(diff1.total_seconds()) < 1.0);  // Should be identical
        REQUIRE(std::abs(diff2.total_seconds()) < 2.0);  // Should be very close
        
        // Format current times for visual verification
        auto local_str = skr::format(u8"Local: {:f}", local1);
        auto utc_str = skr::format(u8"UTC: {:f}", utc);
        
        // Both should be valid formatted strings
        REQUIRE(local_str.size() > 10);
        REQUIRE(utc_str.size() > 10);
        REQUIRE(local_str.contains(u8"Local: "));
        REQUIRE(utc_str.contains(u8"UTC: "));
    }
}