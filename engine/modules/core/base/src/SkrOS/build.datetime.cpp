#include "SkrOS/datetime.hpp"
#include "SkrBase/containers/string/format.hpp"
#include <cstring>

#ifdef _WIN32
#include "./winheaders.h" // IWYU pragma: export
#else
#include <sys/time.h>
#endif

namespace skr
{

// Days in month for non-leap years
static constexpr int s_days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// Cumulative days before each month (non-leap year)
static constexpr int s_days_before_month[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

// Constructor from components
DateTime::DateTime(int year, int month, int day, int hour, int minute, int second, int millisecond)
{
    // Validate input
    if (year < 1 || year > 9999 || month < 1 || month > 12 || day < 1 || day > days_in_month(year, month) ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59 || millisecond < 0 || millisecond > 999)
    {
        ticks = 0;  // Invalid date
        return;
    }
    
    // Calculate total days since year 1
    int total_days = 0;
    
    // Add days for complete years
    for (int y = 1; y < year; y++)
    {
        total_days += is_leap_year(y) ? 366 : 365;
    }
    
    // Add days for complete months in the current year
    for (int m = 1; m < month; m++)
    {
        total_days += days_in_month(year, m);
    }
    
    // Add remaining days
    total_days += day - 1;  // -1 because we count from day 1, not day 0
    
    // Convert to ticks
    ticks = static_cast<uint64_t>(total_days) * TicksPerDay +
            static_cast<uint64_t>(hour) * TicksPerHour +
            static_cast<uint64_t>(minute) * TicksPerMinute +
            static_cast<uint64_t>(second) * TicksPerSecond +
            static_cast<uint64_t>(millisecond) * TicksPerMillisecond;
}

// Get current UTC time
DateTime DateTime::utc_now()
{
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t filetime = (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    return from_filetime(filetime);
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return from_unix_timestamp(tv.tv_sec).add_microseconds(tv.tv_usec);
#endif
}

// Get current local time
DateTime DateTime::now()
{
#ifdef _WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    return DateTime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
    // Get UTC time first
    time_t rawtime;
    time(&rawtime);
    
    // Convert to local time
    struct tm* timeinfo = localtime(&rawtime);
    if (timeinfo == nullptr)
        return utc_now(); // fallback to UTC if conversion fails
    
    return DateTime(timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
                   timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, 0);
#endif
}

// Explicit local time (same as now())
DateTime DateTime::local_now()
{
    return now();
}

// Convert local time to UTC
DateTime DateTime::to_utc(const DateTime& local_time)
{
#ifdef _WIN32
    // Convert DateTime components to SYSTEMTIME
    SYSTEMTIME local_st = {};
    local_st.wYear = local_time.year();
    local_st.wMonth = local_time.month();
    local_st.wDay = local_time.day();
    local_st.wHour = local_time.hour();
    local_st.wMinute = local_time.minute();
    local_st.wSecond = local_time.second();
    local_st.wMilliseconds = local_time.millisecond();
    
    // Convert local time to UTC
    SYSTEMTIME utc_st = {};
    if (!TzSpecificLocalTimeToSystemTime(nullptr, &local_st, &utc_st))
    {
        return local_time; // fallback to original time if conversion fails
    }
    
    return DateTime(utc_st.wYear, utc_st.wMonth, utc_st.wDay, 
                   utc_st.wHour, utc_st.wMinute, utc_st.wSecond, utc_st.wMilliseconds);
#else
    // Create tm structure from DateTime
    struct tm local_tm = {};
    local_tm.tm_year = local_time.year() - 1900;
    local_tm.tm_mon = local_time.month() - 1;
    local_tm.tm_mday = local_time.day();
    local_tm.tm_hour = local_time.hour();
    local_tm.tm_min = local_time.minute();
    local_tm.tm_sec = local_time.second();
    local_tm.tm_isdst = -1; // Let system determine DST
    
    // Convert to time_t (this assumes local_tm is in local time)
    time_t local_time_t = mktime(&local_tm);
    if (local_time_t == -1)
    {
        return local_time; // fallback if conversion fails
    }
    
    // Convert to UTC
    struct tm* utc_tm = gmtime(&local_time_t);
    if (utc_tm == nullptr)
    {
        return local_time; // fallback if conversion fails
    }
    
    return DateTime(utc_tm->tm_year + 1900, utc_tm->tm_mon + 1, utc_tm->tm_mday,
                   utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec, local_time.millisecond());
#endif
}

// Convert UTC to local time
DateTime DateTime::to_local(const DateTime& utc_time)
{
#ifdef _WIN32
    // Convert DateTime components to SYSTEMTIME
    SYSTEMTIME utc_st = {};
    utc_st.wYear = utc_time.year();
    utc_st.wMonth = utc_time.month();
    utc_st.wDay = utc_time.day();
    utc_st.wHour = utc_time.hour();
    utc_st.wMinute = utc_time.minute();
    utc_st.wSecond = utc_time.second();
    utc_st.wMilliseconds = utc_time.millisecond();
    
    // Convert UTC to local time
    SYSTEMTIME local_st = {};
    if (!SystemTimeToTzSpecificLocalTime(nullptr, &utc_st, &local_st))
    {
        return utc_time; // fallback to original time if conversion fails
    }
    
    return DateTime(local_st.wYear, local_st.wMonth, local_st.wDay,
                   local_st.wHour, local_st.wMinute, local_st.wSecond, local_st.wMilliseconds);
#else
    // Create tm structure from DateTime (as UTC)
    struct tm utc_tm = {};
    utc_tm.tm_year = utc_time.year() - 1900;
    utc_tm.tm_mon = utc_time.month() - 1;
    utc_tm.tm_mday = utc_time.day();
    utc_tm.tm_hour = utc_time.hour();
    utc_tm.tm_min = utc_time.minute();
    utc_tm.tm_sec = utc_time.second();
    
    // Convert UTC tm to time_t
    time_t utc_time_t = timegm(&utc_tm);
    if (utc_time_t == -1)
    {
        return utc_time; // fallback if conversion fails
    }
    
    // Convert to local time
    struct tm* local_tm = localtime(&utc_time_t);
    if (local_tm == nullptr)
    {
        return utc_time; // fallback if conversion fails
    }
    
    return DateTime(local_tm->tm_year + 1900, local_tm->tm_mon + 1, local_tm->tm_mday,
                   local_tm->tm_hour, local_tm->tm_min, local_tm->tm_sec, utc_time.millisecond());
#endif
}

// Convert from FileTime
DateTime DateTime::from_filetime(uint64_t filetime)
{
    // FileTime is 100-nanosecond intervals since January 1, 1601
    // DateTime is 100-nanosecond intervals since January 1, 0001
    return DateTime(filetime + FileTimeEpochTicks);
}

// Convert from Unix timestamp
DateTime DateTime::from_unix_timestamp(int64_t unix_seconds)
{
    // Unix timestamp is seconds since January 1, 1970
    return DateTime(UnixEpochTicks + unix_seconds * TicksPerSecond);
}

DateTime DateTime::from_unix_timestamp_ms(int64_t unix_milliseconds)
{
    return DateTime(UnixEpochTicks + unix_milliseconds * TicksPerMillisecond);
}

// Convert to FileTime
uint64_t DateTime::to_filetime() const
{
    if (ticks < FileTimeEpochTicks)
        return 0;  // Cannot represent dates before 1601 in FileTime
    return ticks - FileTimeEpochTicks;
}

// Convert to Unix timestamp
int64_t DateTime::to_unix_timestamp() const
{
    if (ticks < UnixEpochTicks)
        return 0;  // Cannot represent dates before 1970 in Unix timestamp
    return (ticks - UnixEpochTicks) / TicksPerSecond;
}

int64_t DateTime::to_unix_timestamp_ms() const
{
    if (ticks < UnixEpochTicks)
        return 0;
    return (ticks - UnixEpochTicks) / TicksPerMillisecond;
}

// Get year component
int DateTime::year() const
{
    // Calculate year from ticks
    int total_days = static_cast<int>(ticks / TicksPerDay);
    int year = 1;
    
    // Approximate year (400-year cycles)
    int cycles = total_days / 146097;  // Days in 400 years
    year += cycles * 400;
    total_days -= cycles * 146097;
    
    // Fine-tune within the 400-year cycle
    while (total_days >= (is_leap_year(year) ? 366 : 365))
    {
        total_days -= is_leap_year(year) ? 366 : 365;
        year++;
    }
    
    return year;
}

// Get month component
int DateTime::month() const
{
    int y = year();
    int day_of_year = this->day_of_year();
    
    // Find month
    for (int m = 1; m <= 12; m++)
    {
        int days_in_this_month = days_in_month(y, m);
        if (day_of_year <= days_in_this_month)
            return m;
        day_of_year -= days_in_this_month;
    }
    
    return 12;  // Should not reach here
}

// Get day component
int DateTime::day() const
{
    int y = year();
    int day_of_year = this->day_of_year();
    
    // Subtract days for each complete month
    for (int m = 1; m < 12; m++)
    {
        int days_in_this_month = days_in_month(y, m);
        if (day_of_year <= days_in_this_month)
            return day_of_year;
        day_of_year -= days_in_this_month;
    }
    
    return day_of_year;
}

// Get hour component
int DateTime::hour() const
{
    return static_cast<int>((ticks / TicksPerHour) % 24);
}

// Get minute component
int DateTime::minute() const
{
    return static_cast<int>((ticks / TicksPerMinute) % 60);
}

// Get second component
int DateTime::second() const
{
    return static_cast<int>((ticks / TicksPerSecond) % 60);
}

// Get millisecond component
int DateTime::millisecond() const
{
    return static_cast<int>((ticks / TicksPerMillisecond) % 1000);
}

// Get microsecond component
int DateTime::microsecond() const
{
    return static_cast<int>((ticks / TicksPerMicrosecond) % 1000);
}

// Get day of week
DayOfWeek DateTime::day_of_week() const
{
    // January 1, 0001 was a Monday
    int days = static_cast<int>(ticks / TicksPerDay);
    return static_cast<DayOfWeek>((days + 1) % 7);
}

// Get day of year
int DateTime::day_of_year() const
{
    int total_days = static_cast<int>(ticks / TicksPerDay);
    int y = 1;
    
    // Subtract days for complete years
    while (true)
    {
        int days_in_year = is_leap_year(y) ? 366 : 365;
        if (total_days < days_in_year)
            break;
        total_days -= days_in_year;
        y++;
    }
    
    return total_days + 1;  // +1 because day of year starts from 1
}

// Get date portion (time set to midnight)
DateTime DateTime::date() const
{
    uint64_t date_ticks = (ticks / TicksPerDay) * TicksPerDay;
    return DateTime(date_ticks);
}

// Get time of day
TimeSpan DateTime::time_of_day() const
{
    return TimeSpan(static_cast<int64_t>(ticks % TicksPerDay));
}

// Add years
DateTime DateTime::add_years(int years) const
{
    int y = year() + years;
    int m = month();
    int d = day();
    
    // Check for overflow/underflow
    if (y < 1 || y > 9999)
    {
        return DateTime();  // Return invalid DateTime (ticks = 0)
    }
    
    // Handle leap year edge case (Feb 29)
    if (m == 2 && d == 29 && !is_leap_year(y))
        d = 28;
    
    return DateTime(y, m, d, hour(), minute(), second(), millisecond());
}

// Add months
DateTime DateTime::add_months(int months) const
{
    int y = year();
    int m = month() + months;
    int d = day();
    
    // Adjust year and month
    while (m > 12)
    {
        m -= 12;
        y++;
    }
    while (m < 1)
    {
        m += 12;
        y--;
    }
    
    // Check for overflow/underflow after adjustments
    if (y < 1 || y > 9999)
    {
        return DateTime();  // Return invalid DateTime (ticks = 0)
    }
    
    // Adjust day if necessary
    int max_day = days_in_month(y, m);
    if (d > max_day)
        d = max_day;
    
    return DateTime(y, m, d, hour(), minute(), second(), millisecond());
}

// Check if year is leap year
bool DateTime::is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Get days in month
int DateTime::days_in_month(int year, int month)
{
    if (month < 1 || month > 12)
        return 0;
    
    if (month == 2 && is_leap_year(year))
        return 29;
    
    return s_days_in_month[month - 1];
}

// Format to string
skr::String DateTime::to_string() const
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
                  year(), month(), day(), hour(), minute(), second());
    return skr::String::FromRaw(buffer);
}

// Format to ISO 8601 string
skr::String DateTime::to_iso8601_string() const
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%07lldZ",
                  year(), month(), day(), hour(), minute(), second(),
                  static_cast<long long>(ticks % TicksPerSecond));
    return skr::String::FromRaw(buffer);
}

// Custom format (simplified implementation)
skr::String DateTime::to_string(const char* format) const
{
    // This is a simplified implementation
    // In a full implementation, you would parse the format string and replace tokens
    return to_string();  // For now, just return default format
}

// Try parse (simplified implementation)
bool DateTime::try_parse(const skr::StringView& str, DateTime& out_datetime)
{
    // This is a simplified implementation that only handles "yyyy-MM-dd HH:mm:ss" format
    // Create a null-terminated buffer for safe parsing
    if (str.size() > 100) // Sanity check
        return false;
        
    char buffer[128];
    std::memcpy(buffer, str.data(), str.size());
    buffer[str.size()] = '\0';
    
    int y, m, d, h, min, s;
    if (std::sscanf(buffer, "%d-%d-%d %d:%d:%d", &y, &m, &d, &h, &min, &s) == 6)
    {
        out_datetime = DateTime(y, m, d, h, min, s);
        return out_datetime.ticks != 0;  // Constructor sets to 0 if invalid
    }
    return false;
}

// Try parse exact (simplified implementation)
bool DateTime::try_parse_exact(const skr::StringView& str, const char* format, DateTime& out_datetime)
{
    // For now, just use try_parse
    return try_parse(str, out_datetime);
}

} // namespace skr


// Formatter implementations
namespace skr::container
{

// DateTime formatter implementation using type erasure
void format_datetime_erased(const StringFunctionTable& table, const skr::DateTime& value, const skr_char8* spec_data, size_t spec_size)
{
    // Parse format specification
    // Supported formats:
    // - empty or "d": default format "yyyy-MM-dd HH:mm:ss"
    // - "s": ISO 8601 format "yyyy-MM-ddTHH:mm:ss.fffffffZ"
    // - "t": time only "HH:mm:ss"
    // - "D": date only "yyyy-MM-dd"
    // - "f": full date/time "yyyy-MM-dd HH:mm:ss.fff"
    
    char format_type = 'd'; // default
    if (spec_size > 0)
    {
        format_type = static_cast<char>(spec_data[0]);
    }
    
    char buffer[64];
    switch (format_type)
    {
        case 's': // ISO 8601
        {
            std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%07lldZ",
                         value.year(), value.month(), value.day(),
                         value.hour(), value.minute(), value.second(),
                         static_cast<long long>(value.get_ticks() % DateTime::TicksPerSecond));
            break;
        }
        case 't': // time only
        {
            std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
                         value.hour(), value.minute(), value.second());
            break;
        }
        case 'D': // date only
        {
            std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
                         value.year(), value.month(), value.day());
            break;
        }
        case 'f': // full with milliseconds
        {
            std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                         value.year(), value.month(), value.day(),
                         value.hour(), value.minute(), value.second(), value.millisecond());
            break;
        }
        case 'd': // default
        default:
        {
            std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
                         value.year(), value.month(), value.day(),
                         value.hour(), value.minute(), value.second());
            break;
        }
    }
    
    table.append_cstr(table.string_ptr, buffer, std::strlen(buffer));
}

// TimeSpan formatter implementation using type erasure
void format_timespan_erased(const StringFunctionTable& table, const skr::TimeSpan& value, const skr_char8* spec_data, size_t spec_size)
{
    // Parse format specification
    // Supported formats:
    // - empty or "c": constant format "[-][d.]hh:mm:ss[.fffffff]"
    // - "g": general short format "[-][d:]h:mm:ss[.f[f[f[f[f[f[f]]]]]]"
    // - "G": general long format "[-]d:hh:mm:ss.fffffff"
    
    char format_type = 'c'; // default
    if (spec_size > 0)
    {
        format_type = static_cast<char>(spec_data[0]);
    }
    
    char buffer[64];
    bool is_negative = value.ticks < 0;
    int64_t abs_ticks = is_negative ? -value.ticks : value.ticks;
    
    TimeSpan abs_span(abs_ticks);
    
    switch (format_type)
    {
        case 'g': // general short
        {
            if (abs_span.days() > 0)
            {
                std::snprintf(buffer, sizeof(buffer), "%s%d:%d:%02d:%02d",
                             is_negative ? "-" : "", abs_span.days(), abs_span.hours(),
                             abs_span.minutes(), abs_span.seconds());
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer), "%s%d:%02d:%02d",
                             is_negative ? "-" : "", abs_span.hours(),
                             abs_span.minutes(), abs_span.seconds());
            }
            break;
        }
        case 'G': // general long
        {
            std::snprintf(buffer, sizeof(buffer), "%s%d:%02d:%02d:%02d.%07lld",
                         is_negative ? "-" : "", abs_span.days(), abs_span.hours(),
                         abs_span.minutes(), abs_span.seconds(),
                         static_cast<long long>(abs_ticks % TimeSpan::TicksPerSecond));
            break;
        }
        case 'c': // constant
        default:
        {
            if (abs_span.days() > 0)
            {
                std::snprintf(buffer, sizeof(buffer), "%s%d.%02d:%02d:%02d",
                             is_negative ? "-" : "", abs_span.days(), abs_span.hours(),
                             abs_span.minutes(), abs_span.seconds());
            }
            else
            {
                std::snprintf(buffer, sizeof(buffer), "%s%02d:%02d:%02d",
                             is_negative ? "-" : "", abs_span.hours(),
                             abs_span.minutes(), abs_span.seconds());
            }
            break;
        }
    }
    
    table.append_cstr(table.string_ptr, buffer, std::strlen(buffer));
}

} // namespace skr::container