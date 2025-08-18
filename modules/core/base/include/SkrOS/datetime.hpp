#pragma once
#include "SkrBase/config.h"
#include "SkrContainersDef/string.hpp"
#include "SkrBase/containers/string/format.hpp"

namespace skr
{
// Time span representation (100-nanosecond intervals, same as FileTime)
struct TimeSpan
{
    int64_t ticks = 0;  // Can be negative for time differences
    
    // Constants
    static constexpr int64_t TicksPerMicrosecond = 10;
    static constexpr int64_t TicksPerMillisecond = 10000;
    static constexpr int64_t TicksPerSecond = 10000000;
    static constexpr int64_t TicksPerMinute = 600000000;
    static constexpr int64_t TicksPerHour = 36000000000;
    static constexpr int64_t TicksPerDay = 864000000000;
    
    // Constructors
    constexpr TimeSpan() = default;
    constexpr explicit TimeSpan(int64_t ticks) : ticks(ticks) {}
    
    // Factory methods
    static constexpr TimeSpan from_days(double days) { return TimeSpan(static_cast<int64_t>(days * TicksPerDay)); }
    static constexpr TimeSpan from_hours(double hours) { return TimeSpan(static_cast<int64_t>(hours * TicksPerHour)); }
    static constexpr TimeSpan from_minutes(double minutes) { return TimeSpan(static_cast<int64_t>(minutes * TicksPerMinute)); }
    static constexpr TimeSpan from_seconds(double seconds) { return TimeSpan(static_cast<int64_t>(seconds * TicksPerSecond)); }
    static constexpr TimeSpan from_milliseconds(double ms) { return TimeSpan(static_cast<int64_t>(ms * TicksPerMillisecond)); }
    static constexpr TimeSpan from_microseconds(double us) { return TimeSpan(static_cast<int64_t>(us * TicksPerMicrosecond)); }
    
    // Getters
    constexpr double total_days() const { return static_cast<double>(ticks) / TicksPerDay; }
    constexpr double total_hours() const { return static_cast<double>(ticks) / TicksPerHour; }
    constexpr double total_minutes() const { return static_cast<double>(ticks) / TicksPerMinute; }
    constexpr double total_seconds() const { return static_cast<double>(ticks) / TicksPerSecond; }
    constexpr double total_milliseconds() const { return static_cast<double>(ticks) / TicksPerMillisecond; }
    constexpr double total_microseconds() const { return static_cast<double>(ticks) / TicksPerMicrosecond; }
    
    // Components
    constexpr int32_t days() const { return static_cast<int32_t>(ticks / TicksPerDay); }
    constexpr int32_t hours() const { return static_cast<int32_t>((ticks / TicksPerHour) % 24); }
    constexpr int32_t minutes() const { return static_cast<int32_t>((ticks / TicksPerMinute) % 60); }
    constexpr int32_t seconds() const { return static_cast<int32_t>((ticks / TicksPerSecond) % 60); }
    constexpr int32_t milliseconds() const { return static_cast<int32_t>((ticks / TicksPerMillisecond) % 1000); }
    constexpr int32_t microseconds() const { return static_cast<int32_t>((ticks / TicksPerMicrosecond) % 1000); }
    
    // Operators
    constexpr TimeSpan operator+(const TimeSpan& other) const { return TimeSpan(ticks + other.ticks); }
    constexpr TimeSpan operator-(const TimeSpan& other) const { return TimeSpan(ticks - other.ticks); }
    constexpr TimeSpan operator-() const { return TimeSpan(-ticks); }
    
    constexpr bool operator==(const TimeSpan& other) const { return ticks == other.ticks; }
    constexpr bool operator!=(const TimeSpan& other) const { return ticks != other.ticks; }
    constexpr bool operator<(const TimeSpan& other) const { return ticks < other.ticks; }
    constexpr bool operator<=(const TimeSpan& other) const { return ticks <= other.ticks; }
    constexpr bool operator>(const TimeSpan& other) const { return ticks > other.ticks; }
    constexpr bool operator>=(const TimeSpan& other) const { return ticks >= other.ticks; }
    
    // Constants
    static const TimeSpan Zero;
    static const TimeSpan MinValue;
    static const TimeSpan MaxValue;
};

inline constexpr TimeSpan TimeSpan::Zero{0};
inline constexpr TimeSpan TimeSpan::MinValue{INT64_MIN};
inline constexpr TimeSpan TimeSpan::MaxValue{INT64_MAX};

// Day of week enumeration
enum class DayOfWeek : uint8_t
{
    Sunday = 0,
    Monday = 1,
    Tuesday = 2,
    Wednesday = 3,
    Thursday = 4,
    Friday = 5,
    Saturday = 6
};

// DateTime representation
struct DateTime
{
private:
    uint64_t ticks = 0;  // 100-nanosecond intervals since January 1, 0001 00:00:00.000
    
public:
    // Constants
    static constexpr int64_t TicksPerMicrosecond = 10;
    static constexpr int64_t TicksPerMillisecond = 10000;
    static constexpr int64_t TicksPerSecond = 10000000;
    static constexpr int64_t TicksPerMinute = 600000000;
    static constexpr int64_t TicksPerHour = 36000000000;
    static constexpr int64_t TicksPerDay = 864000000000;
    
    // FileTime epoch: January 1, 1601 00:00:00 UTC
    // DateTime epoch: January 1, 0001 00:00:00
    // Difference: 584388 days
    static constexpr int64_t FileTimeEpochTicks = 504911232000000000LL;
    
    // Unix epoch: January 1, 1970 00:00:00 UTC
    // DateTime epoch: January 1, 0001 00:00:00
    // Difference: 719162 days
    static constexpr int64_t UnixEpochTicks = 621355968000000000LL;
    
    // Constructors
    constexpr DateTime() = default;
    constexpr explicit DateTime(uint64_t ticks) : ticks(ticks) {}
    DateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int millisecond = 0);
    
    // Factory methods
    static DateTime now();          // Returns local time
    static DateTime utc_now();      // Returns UTC time
    static DateTime local_now();    // Explicit local time (same as now())
    static DateTime from_filetime(uint64_t filetime);                    // FileTime is always UTC
    static DateTime from_unix_timestamp(int64_t unix_seconds);          // Unix timestamp is always UTC
    static DateTime from_unix_timestamp_ms(int64_t unix_milliseconds);  // Unix timestamp is always UTC
    
    // Conversion methods
    uint64_t to_filetime() const;
    int64_t to_unix_timestamp() const;
    int64_t to_unix_timestamp_ms() const;
    
    // Timezone conversion methods
    static DateTime to_utc(const DateTime& local_time);     // Convert local time to UTC
    static DateTime to_local(const DateTime& utc_time);     // Convert UTC to local time
    
    // Component getters
    int year() const;
    int month() const;        // 1-12
    int day() const;           // 1-31
    int hour() const;          // 0-23
    int minute() const;        // 0-59
    int second() const;        // 0-59
    int millisecond() const;   // 0-999
    int microsecond() const;   // 0-999
    DayOfWeek day_of_week() const;
    int day_of_year() const;   // 1-366
    
    // Properties
    DateTime date() const;     // Time portion set to 00:00:00
    TimeSpan time_of_day() const;  // Time elapsed since midnight
    uint64_t get_ticks() const { return ticks; }
    
    // Arithmetic operations
    DateTime add(const TimeSpan& span) const { return DateTime(ticks + span.ticks); }
    DateTime subtract(const TimeSpan& span) const { return DateTime(ticks - span.ticks); }
    TimeSpan subtract(const DateTime& other) const { return TimeSpan(static_cast<int64_t>(ticks - other.ticks)); }
    
    DateTime add_years(int years) const;
    DateTime add_months(int months) const;
    DateTime add_days(double days) const { return add(TimeSpan::from_days(days)); }
    DateTime add_hours(double hours) const { return add(TimeSpan::from_hours(hours)); }
    DateTime add_minutes(double minutes) const { return add(TimeSpan::from_minutes(minutes)); }
    DateTime add_seconds(double seconds) const { return add(TimeSpan::from_seconds(seconds)); }
    DateTime add_milliseconds(double ms) const { return add(TimeSpan::from_milliseconds(ms)); }
    DateTime add_microseconds(double us) const { return add(TimeSpan::from_microseconds(us)); }
    
    // Operators
    DateTime operator+(const TimeSpan& span) const { return add(span); }
    DateTime operator-(const TimeSpan& span) const { return subtract(span); }
    TimeSpan operator-(const DateTime& other) const { return subtract(other); }
    
    bool operator==(const DateTime& other) const { return ticks == other.ticks; }
    bool operator!=(const DateTime& other) const { return ticks != other.ticks; }
    bool operator<(const DateTime& other) const { return ticks < other.ticks; }
    bool operator<=(const DateTime& other) const { return ticks <= other.ticks; }
    bool operator>(const DateTime& other) const { return ticks > other.ticks; }
    bool operator>=(const DateTime& other) const { return ticks >= other.ticks; }
    
    // Formatting
    skr::String to_string() const;                    // Default format: "yyyy-MM-dd HH:mm:ss"
    skr::String to_string(const char* format) const;  // Custom format
    skr::String to_iso8601_string() const;            // ISO 8601: "yyyy-MM-ddTHH:mm:ss.fffffffZ"
    
    // Parsing
    static bool try_parse(const skr::StringView& str, DateTime& out_datetime);
    static bool try_parse_exact(const skr::StringView& str, const char* format, DateTime& out_datetime);
    
    // Validation
    static bool is_leap_year(int year);
    static int days_in_month(int year, int month);
    
    // Constants
    static const DateTime MinValue;
    static const DateTime MaxValue;
    static const DateTime UnixEpoch;
    static const DateTime FileTimeEpoch;
};

inline constexpr DateTime DateTime::MinValue{0};
inline constexpr DateTime DateTime::MaxValue{UINT64_MAX};
inline constexpr DateTime DateTime::UnixEpoch{DateTime::UnixEpochTicks};
inline constexpr DateTime DateTime::FileTimeEpoch{DateTime::FileTimeEpochTicks};

// Helper functions for FileTime conversion (compatible with filesystem.hpp)
inline DateTime filetime_to_datetime(uint64_t filetime)
{
    return DateTime::from_filetime(filetime);
}

inline uint64_t datetime_to_filetime(const DateTime& datetime)
{
    return datetime.to_filetime();
}

} // namespace skr

namespace skr::container
{

// 类型擦除的格式化函数
void format_datetime_erased(const StringFunctionTable& table, const skr::DateTime& value, const skr_char8* spec_data, size_t spec_size);
void format_timespan_erased(const StringFunctionTable& table, const skr::TimeSpan& value, const skr_char8* spec_data, size_t spec_size);

template <>
struct Formatter<skr::DateTime> {
    template <typename TString>
    static void format(TString& out, const skr::DateTime& value, typename TString::ViewType spec)
    {
        auto table = __virtualize(out);
        format_datetime_erased(table, value, spec.data(), spec.size());
    }
};

// TimeSpan formatter
template <>
struct Formatter<skr::TimeSpan> {
    template <typename TString>
    static void format(TString& out, const skr::TimeSpan& value, typename TString::ViewType spec)
    {
        auto table = __virtualize(out);
        format_timespan_erased(table, value, spec.data(), spec.size());
    }
};
} // namespace skr::container