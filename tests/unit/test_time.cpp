#include <NeForce/core/time/datetime.hpp>
#include <gtest/gtest.h>
using namespace neforce;

TEST(DateTest, DefaultConstructor) {
    date d;
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
    EXPECT_TRUE(d.is_valid());
}

TEST(DateTest, ValidConstructor) {
    date d(2024, 6, 15);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 15);
    EXPECT_TRUE(d.is_valid());
}

TEST(DateTest, InvalidConstructorYearLow) {
    date d(1800, 1, 1);
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, InvalidConstructorYearHigh) {
    date d(10000, 1, 1);
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, InvalidConstructorMonthLow) {
    date d(2024, 0, 1);
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, InvalidConstructorMonthHigh) {
    date d(2024, 13, 1);
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, InvalidConstructorDayLow) {
    date d(2024, 6, 0);
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, InvalidConstructorDayHigh) {
    date d(2024, 6, 31);
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, CopyConstructor) {
    date d1(2024, 6, 15);
    date d2(d1);
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 6);
    EXPECT_EQ(d2.day(), 15);
}

TEST(DateTest, CopyAssignment) {
    date d1(2024, 6, 15);
    date d2;
    d2 = d1;
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 6);
    EXPECT_EQ(d2.day(), 15);
}

TEST(DateTest, MoveConstructor) {
    date d1(2024, 6, 15);
    date d2(move(d1));
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 6);
    EXPECT_EQ(d2.day(), 15);
}

TEST(DateTest, MoveAssignment) {
    date d1(2024, 6, 15);
    date d2;
    d2 = move(d1);
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 6);
    EXPECT_EQ(d2.day(), 15);
}

TEST(DateTest, IsValidStatic) {
    EXPECT_TRUE(date::is_valid(2024, 6, 15));
    EXPECT_FALSE(date::is_valid(1800, 1, 1));
    EXPECT_FALSE(date::is_valid(10000, 1, 1));
    EXPECT_FALSE(date::is_valid(2024, 0, 1));
    EXPECT_FALSE(date::is_valid(2024, 13, 1));
    EXPECT_FALSE(date::is_valid(2024, 6, 0));
    EXPECT_FALSE(date::is_valid(2024, 6, 31));
    EXPECT_FALSE(date::is_valid(2023, 2, 29));
}

TEST(DateTest, IsValidMember) {
    date d(2024, 6, 15);
    EXPECT_TRUE(d.is_valid());
    date d2(1800, 1, 1);
    EXPECT_TRUE(d2.is_valid()); // auto set default
}

TEST(DateTest, Epoch) {
    date d = date::epoch();
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, IsLeapYear) {
    EXPECT_TRUE(date::is_leap_year(2024));
    EXPECT_TRUE(date::is_leap_year(2000));
    EXPECT_FALSE(date::is_leap_year(1900));
    EXPECT_FALSE(date::is_leap_year(2023));
    EXPECT_TRUE(date::is_leap_year(2020));
    EXPECT_FALSE(date::is_leap_year(2100));
}

TEST(DateTest, DaysOfWeek) {
    date d1(2024, 1, 1);
    EXPECT_EQ(d1.days_of_week(), 1);

    date d2(2024, 1, 7);
    EXPECT_EQ(d2.days_of_week(), 0);

    date d3(2024, 6, 15);
    EXPECT_EQ(d3.days_of_week(), 6);

    date d4(2023, 2, 28);
    EXPECT_EQ(d4.days_of_week(), 2);

    date d5(2023, 3, 1);
    EXPECT_EQ(d5.days_of_week(), 3);

    date d6(1970, 1, 1);
    EXPECT_EQ(d6.days_of_week(), 4);
}

TEST(DateTest, DaysOfMonth) {
    EXPECT_EQ(date::days_of_month(2024, 1), 31);
    EXPECT_EQ(date::days_of_month(2024, 2), 29);
    EXPECT_EQ(date::days_of_month(2023, 2), 28);
    EXPECT_EQ(date::days_of_month(2024, 4), 30);
    EXPECT_EQ(date::days_of_month(2024, 6), 30);
    EXPECT_EQ(date::days_of_month(2024, 12), 31);
    EXPECT_EQ(date::days_of_month(2000, 2), 29);
    EXPECT_EQ(date::days_of_month(1900, 2), 28);
}

TEST(DateTest, DaysOfYear) {
    date d1(2024, 1, 1);
    EXPECT_EQ(d1.days_of_year(), 1);

    date d2(2024, 12, 31);
    EXPECT_EQ(d2.days_of_year(), 366);

    date d3(2023, 12, 31);
    EXPECT_EQ(d3.days_of_year(), 365);

    date d4(2024, 2, 29);
    EXPECT_EQ(d4.days_of_year(), 60);

    date d5(2023, 2, 28);
    EXPECT_EQ(d5.days_of_year(), 59);
}

TEST(DateTest, JulianDayConversion) {
    date d1(1970, 1, 1);
    int64_t jd1 = d1.to_julian_day();
    date d1_back = date::from_julian_day(jd1);
    EXPECT_EQ(d1_back.year(), 1970);
    EXPECT_EQ(d1_back.month(), 1);
    EXPECT_EQ(d1_back.day(), 1);

    date d2(2024, 6, 15);
    int64_t jd2 = d2.to_julian_day();
    date d2_back = date::from_julian_day(jd2);
    EXPECT_EQ(d2_back.year(), 2024);
    EXPECT_EQ(d2_back.month(), 6);
    EXPECT_EQ(d2_back.day(), 15);

    date d3(1900, 1, 1);
    int64_t jd3 = d3.to_julian_day();
    date d3_back = date::from_julian_day(jd3);
    EXPECT_EQ(d3_back.year(), 1900);
    EXPECT_EQ(d3_back.month(), 1);
    EXPECT_EQ(d3_back.day(), 1);

    date d4(9999, 12, 31);
    int64_t jd4 = d4.to_julian_day();
    date d4_back = date::from_julian_day(jd4);
    EXPECT_EQ(d4_back.year(), 9999);
    EXPECT_EQ(d4_back.month(), 12);
    EXPECT_EQ(d4_back.day(), 31);
}

TEST(DateTest, Clear) {
    date d(2024, 6, 15);
    d.clear();
    EXPECT_EQ(d.year(), 1970);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, EqualTo) {
    date d1(2024, 6, 15);
    date d2(2024, 6, 15);
    date d3(2024, 6, 16);
    EXPECT_TRUE(d1.equal_to(d2));
    EXPECT_FALSE(d1.equal_to(d3));
    EXPECT_TRUE(d1 == d2);
    EXPECT_FALSE(d1 == d3);
    EXPECT_TRUE(d1 != d3);
    EXPECT_FALSE(d1 != d2);
}

TEST(DateTest, LessThan) {
    date d1(2024, 6, 15);
    date d2(2024, 6, 16);
    date d3(2024, 7, 1);
    date d4(2025, 1, 1);
    EXPECT_TRUE(d1.less_than(d2));
    EXPECT_TRUE(d1.less_than(d3));
    EXPECT_TRUE(d1.less_than(d4));
    EXPECT_FALSE(d2.less_than(d1));
    EXPECT_TRUE(d1 < d2);
    EXPECT_TRUE(d1 <= d1);
    EXPECT_TRUE(d1 >= d1);
    EXPECT_TRUE(d2 > d1);
}

TEST(DateTest, AddDaysSimple) {
    date d(2024, 6, 15);
    d += 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 16);

    d += 14;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 30);

    d += 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 7);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, AddDaysCrossMonth) {
    date d(2024, 1, 31);
    d += 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 2);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, AddDaysCrossYear) {
    date d(2024, 12, 31);
    d += 1;
    EXPECT_EQ(d.year(), 2025);
    EXPECT_EQ(d.month(), 1);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, AddDaysLeapYear) {
    date d(2024, 2, 28);
    d += 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 2);
    EXPECT_EQ(d.day(), 29);
    d += 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 3);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, AddDaysNonLeapYear) {
    date d(2023, 2, 28);
    d += 1;
    EXPECT_EQ(d.year(), 2023);
    EXPECT_EQ(d.month(), 3);
    EXPECT_EQ(d.day(), 1);
}

TEST(DateTest, AddDaysLarge) {
    date d(1970, 1, 1);
    d += 20000;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 10);
    EXPECT_EQ(d.day(), 4);
}

TEST(DateTest, AddDaysZero) {
    date d(2024, 6, 15);
    d += 0;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 15);
}

TEST(DateTest, AddDaysNegative) {
    date d(2024, 6, 15);
    d += -1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 14);
}

TEST(DateTest, SubtractDaysSimple) {
    date d(2024, 6, 15);
    d -= 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 14);

    d -= 13;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 1);

    d -= 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 5);
    EXPECT_EQ(d.day(), 31);
}

TEST(DateTest, SubtractDaysCrossMonth) {
    date d(2024, 3, 1);
    d -= 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 2);
    EXPECT_EQ(d.day(), 29);
}

TEST(DateTest, SubtractDaysCrossYear) {
    date d(2025, 1, 1);
    d -= 1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 12);
    EXPECT_EQ(d.day(), 31);
}

TEST(DateTest, SubtractDaysNegative) {
    date d(2024, 6, 15);
    d -= -1;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 16);
}

TEST(DateTest, OperatorPlusDays) {
    date d(2024, 6, 15);
    date d2 = d + 1;
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 6);
    EXPECT_EQ(d2.day(), 16);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 15);
}

TEST(DateTest, OperatorMinusDays) {
    date d(2024, 6, 15);
    date d2 = d - 1;
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 6);
    EXPECT_EQ(d2.day(), 14);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 15);
}

TEST(DateTest, PreIncrement) {
    date d(2024, 6, 15);
    ++d;
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 16);
}

TEST(DateTest, PostIncrement) {
    date d(2024, 6, 15);
    date d2 = d++;
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 6);
    EXPECT_EQ(d2.day(), 15);
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 16);
}

TEST(DateTest, DateDifference) {
    date d1(2024, 6, 15);
    date d2(2024, 6, 14);
    EXPECT_EQ(d1 - d2, 1);
    EXPECT_EQ(d2 - d1, -1);

    date d3(2025, 1, 1);
    date d4(2024, 1, 1);
    EXPECT_EQ(d3 - d4, 366);

    date d5(2024, 1, 1);
    date d6(2023, 1, 1);
    EXPECT_EQ(d5 - d6, 365);
}

TEST(DateTest, Hash) {
    date d1(2024, 6, 15);
    date d2(2024, 6, 15);
    date d3(2024, 6, 16);
    EXPECT_EQ(d1.to_hash(), d2.to_hash());
    EXPECT_NE(d1.to_hash(), d3.to_hash());
}

TEST(DateTest, ToString) {
    date d(2024, 6, 15);
    EXPECT_EQ(d.to_string(), "2024-06-15");

    date d2(1970, 1, 1);
    EXPECT_EQ(d2.to_string(), "1970-01-01");

    date d3(9999, 12, 31);
    EXPECT_EQ(d3.to_string(), "9999-12-31");
}

TEST(DateTest, ParseValid) {
    date d = date::parse("2024-06-15");
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 15);
}

TEST(DateTest, ParseInvalidFormat) {
    EXPECT_THROW(ignore = date::parse("2024/06/15"), value_exception);
    EXPECT_THROW(ignore = date::parse("2024-06"), value_exception);
    EXPECT_THROW(ignore = date::parse("2024-06-150"), value_exception);
}

TEST(DateTest, Swap) {
    date d1(2024, 6, 15);
    date d2(2023, 1, 1);
    d1.swap(d2);
    EXPECT_EQ(d1.year(), 2023);
    EXPECT_EQ(d1.month(), 1);
    EXPECT_EQ(d1.day(), 1);
    EXPECT_EQ(d2.year(), 2024);
    EXPECT_EQ(d2.month(), 6);
    EXPECT_EQ(d2.day(), 15);
}

TEST(TimeTest, DefaultConstructor) {
    using neforce::time;

    time t;
    EXPECT_EQ(t.hours(), 0);
    EXPECT_EQ(t.minutes(), 0);
    EXPECT_EQ(t.seconds(), 0);
    EXPECT_TRUE(t.is_valid());
}

TEST(TimeTest, ValidConstructor) {
    using neforce::time;

    time t(14, 30, 45);
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 45);
    EXPECT_TRUE(t.is_valid());
}

TEST(TimeTest, InvalidConstructorHour) {
    using neforce::time;

    time t(24, 0, 0);
    EXPECT_EQ(t.hours(), 0);
    EXPECT_EQ(t.minutes(), 0);
    EXPECT_EQ(t.seconds(), 0);
}

TEST(TimeTest, InvalidConstructorHourNegative) {
    using neforce::time;

    time t(-1, 0, 0);
    EXPECT_EQ(t.hours(), 0);
    EXPECT_EQ(t.minutes(), 0);
    EXPECT_EQ(t.seconds(), 0);
}

TEST(TimeTest, InvalidConstructorMinute) {
    using neforce::time;

    time t(0, 60, 0);
    EXPECT_EQ(t.hours(), 0);
    EXPECT_EQ(t.minutes(), 0);
    EXPECT_EQ(t.seconds(), 0);
}

TEST(TimeTest, InvalidConstructorSecond) {
    using neforce::time;

    time t(0, 0, 60);
    EXPECT_EQ(t.hours(), 0);
    EXPECT_EQ(t.minutes(), 0);
    EXPECT_EQ(t.seconds(), 0);
}

TEST(TimeTest, CopyConstructor) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2(t1);
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 45);
}

TEST(TimeTest, CopyAssignment) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2;
    t2 = t1;
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 45);
}

TEST(TimeTest, MoveConstructor) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2(move(t1));
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 45);
}

TEST(TimeTest, MoveAssignment) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2;
    t2 = move(t1);
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 45);
}

TEST(TimeTest, IsValidStatic) {
    using neforce::time;

    EXPECT_TRUE(time::is_valid(0, 0, 0));
    EXPECT_TRUE(time::is_valid(23, 59, 59));
    EXPECT_FALSE(time::is_valid(24, 0, 0));
    EXPECT_FALSE(time::is_valid(-1, 0, 0));
    EXPECT_FALSE(time::is_valid(0, 60, 0));
    EXPECT_FALSE(time::is_valid(0, -1, 0));
    EXPECT_FALSE(time::is_valid(0, 0, 60));
    EXPECT_FALSE(time::is_valid(0, 0, -1));
}

TEST(TimeTest, IsValidMember) {
    using neforce::time;

    time t(14, 30, 45);
    EXPECT_TRUE(t.is_valid());
    time t2(24, 0, 0);
    EXPECT_TRUE(t2.is_valid()); // default
}

TEST(TimeTest, Clear) {
    using neforce::time;

    time t(14, 30, 45);
    t.clear();
    EXPECT_EQ(t.hours(), 0);
    EXPECT_EQ(t.minutes(), 0);
    EXPECT_EQ(t.seconds(), 0);
}

TEST(TimeTest, ToSeconds) {
    using neforce::time;

    time t(0, 0, 0);
    EXPECT_EQ(t.to_seconds(), 0);

    time t2(0, 0, 1);
    EXPECT_EQ(t2.to_seconds(), 1);

    time t3(0, 1, 0);
    EXPECT_EQ(t3.to_seconds(), 60);

    time t4(1, 0, 0);
    EXPECT_EQ(t4.to_seconds(), 3600);

    time t5(23, 59, 59);
    EXPECT_EQ(t5.to_seconds(), 86399);

    time t6(14, 30, 45);
    EXPECT_EQ(t6.to_seconds(), 52245);
}

TEST(TimeTest, EqualTo) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2(14, 30, 45);
    time t3(14, 30, 46);
    EXPECT_TRUE(t1.equal_to(t2));
    EXPECT_FALSE(t1.equal_to(t3));
    EXPECT_TRUE(t1 == t2);
    EXPECT_FALSE(t1 == t3);
    EXPECT_TRUE(t1 != t3);
    EXPECT_FALSE(t1 != t2);
}

TEST(TimeTest, LessThan) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2(14, 30, 46);
    time t3(14, 31, 0);
    time t4(15, 0, 0);
    EXPECT_TRUE(t1.less_than(t2));
    EXPECT_TRUE(t1.less_than(t3));
    EXPECT_TRUE(t1.less_than(t4));
    EXPECT_FALSE(t2.less_than(t1));
    EXPECT_TRUE(t1 < t2);
    EXPECT_TRUE(t1 <= t1);
    EXPECT_TRUE(t1 >= t1);
    EXPECT_TRUE(t2 > t1);
}

TEST(TimeTest, AddSeconds) {
    using neforce::time;

    time t(14, 30, 45);
    t += 1;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 46);

    t += 14;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 31);
    EXPECT_EQ(t.seconds(), 0);

    t += 60;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 32);
    EXPECT_EQ(t.seconds(), 0);

    t += 3600;
    EXPECT_EQ(t.hours(), 15);
    EXPECT_EQ(t.minutes(), 32);
    EXPECT_EQ(t.seconds(), 0);
}

TEST(TimeTest, AddSecondsWrapAround) {
    using neforce::time;

    time t(23, 59, 59);
    t += 1;
    EXPECT_EQ(t.hours(), 0);
    EXPECT_EQ(t.minutes(), 0);
    EXPECT_EQ(t.seconds(), 0);

    time t2(23, 59, 59);
    t2 += 2;
    EXPECT_EQ(t2.hours(), 0);
    EXPECT_EQ(t2.minutes(), 0);
    EXPECT_EQ(t2.seconds(), 1);
}

TEST(TimeTest, AddSecondsNegative) {
    using neforce::time;

    time t(14, 30, 45);
    t += -1;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 44);
}

TEST(TimeTest, SubtractSeconds) {
    using neforce::time;

    time t(14, 30, 45);
    t -= 1;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 44);

    t -= 44;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 0);

    t -= 1;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 29);
    EXPECT_EQ(t.seconds(), 59);
}

TEST(TimeTest, SubtractSecondsWrapAround) {
    using neforce::time;

    time t(0, 0, 0);
    t -= 1;
    EXPECT_EQ(t.hours(), 23);
    EXPECT_EQ(t.minutes(), 59);
    EXPECT_EQ(t.seconds(), 59);

    time t2(0, 0, 0);
    t2 -= 2;
    EXPECT_EQ(t2.hours(), 23);
    EXPECT_EQ(t2.minutes(), 59);
    EXPECT_EQ(t2.seconds(), 58);
}

TEST(TimeTest, SubtractSecondsNegative) {
    using neforce::time;

    time t(14, 30, 45);
    t -= -1;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 46);
}

TEST(TimeTest, SubtractSecondsLargeNegative) {
    using neforce::time;

    time t(0, 0, 0);
    t -= -86400;
    EXPECT_EQ(t.hours(), 0);
    EXPECT_EQ(t.minutes(), 0);
    EXPECT_EQ(t.seconds(), 0);
}

TEST(TimeTest, OperatorPlusSeconds) {
    using neforce::time;

    time t(14, 30, 45);
    time t2 = t + 1;
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 46);
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 45);
}

TEST(TimeTest, OperatorMinusSeconds) {
    using neforce::time;

    time t(14, 30, 45);
    time t2 = t - 1;
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 44);
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 45);
}

TEST(TimeTest, PreIncrement) {
    using neforce::time;

    time t(14, 30, 45);
    ++t;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 46);
}

TEST(TimeTest, PostIncrement) {
    using neforce::time;

    time t(14, 30, 45);
    time t2 = t++;
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 45);
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 46);
}

TEST(TimeTest, PreDecrement) {
    using neforce::time;

    time t(14, 30, 45);
    --t;
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 44);
}

TEST(TimeTest, PostDecrement) {
    using neforce::time;

    time t(14, 30, 45);
    time t2 = t--;
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 45);
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 44);
}

TEST(TimeTest, TimeDifference) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2(14, 30, 44);
    EXPECT_EQ(t1 - t2, 1);
    EXPECT_EQ(t2 - t1, -1);

    time t3(15, 0, 0);
    time t4(14, 0, 0);
    EXPECT_EQ(t3 - t4, 3600);

    time t5(0, 0, 0);
    time t6(23, 59, 59);
    EXPECT_EQ(t5 - t6, -86399);
}

TEST(TimeTest, Hash) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2(14, 30, 45);
    time t3(14, 30, 46);
    EXPECT_EQ(t1.to_hash(), t2.to_hash());
    EXPECT_NE(t1.to_hash(), t3.to_hash());
}

TEST(TimeTest, ToString) {
    using neforce::time;

    time t(14, 30, 45);
    EXPECT_EQ(t.to_string(), "14:30:45");

    time t2(0, 0, 0);
    EXPECT_EQ(t2.to_string(), "00:00:00");

    time t3(23, 59, 59);
    EXPECT_EQ(t3.to_string(), "23:59:59");
}

TEST(TimeTest, ParseValid) {
    using neforce::time;

    time t = time::parse("14:30:45");
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 45);
}

TEST(TimeTest, ParseInvalidFormat) {
    using neforce::time;

    EXPECT_THROW(ignore = time::parse("14/30/45"), value_exception);
    EXPECT_THROW(ignore = time::parse("14:30"), value_exception);
    EXPECT_THROW(ignore = time::parse("14:30:450"), value_exception);
}

TEST(TimeTest, Swap) {
    using neforce::time;

    time t1(14, 30, 45);
    time t2(10, 20, 30);
    t1.swap(t2);
    EXPECT_EQ(t1.hours(), 10);
    EXPECT_EQ(t1.minutes(), 20);
    EXPECT_EQ(t1.seconds(), 30);
    EXPECT_EQ(t2.hours(), 14);
    EXPECT_EQ(t2.minutes(), 30);
    EXPECT_EQ(t2.seconds(), 45);
}

TEST(DatetimeTest, DefaultConstructor) {
    datetime dt;
    EXPECT_EQ(dt.year(), 1970);
    EXPECT_EQ(dt.month(), 1);
    EXPECT_EQ(dt.day(), 1);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
    EXPECT_FALSE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 0);
}

TEST(DatetimeTest, FullConstructor) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_FALSE(dt.has_timezone());
}

TEST(DatetimeTest, FullConstructorWithOffset) {
    datetime dt(2024, 6, 15, 14, 30, 45, 28800);
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 28800);
}

TEST(DatetimeTest, DateAndTimeConstructor) {
    using neforce::time;

    date d(2024, 6, 15);
    time t(14, 30, 45);
    datetime dt(d, t);
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_FALSE(dt.has_timezone());
}

TEST(DatetimeTest, DateAndTimeConstructorWithOffset) {
    using neforce::time;

    date d(2024, 6, 15);
    time t(14, 30, 45);
    datetime dt(d, t, 28800);
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 28800);
}

TEST(DatetimeTest, DateOnlyConstructor) {
    date d(2024, 6, 15);
    datetime dt(d);
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, TimeOnlyConstructor) {
    using neforce::time;

    time t(14, 30, 45);
    datetime dt(t);
    EXPECT_EQ(dt.year(), 1970);
    EXPECT_EQ(dt.month(), 1);
    EXPECT_EQ(dt.day(), 1);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, MoveDateOnlyConstructor) {
    date d(2024, 6, 15);
    datetime dt(move(d));
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, MoveTimeOnlyConstructor) {
    using neforce::time;

    time t(14, 30, 45);
    datetime dt(move(t));
    EXPECT_EQ(dt.year(), 1970);
    EXPECT_EQ(dt.month(), 1);
    EXPECT_EQ(dt.day(), 1);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, MoveDateAndTimeConstructor) {
    using neforce::time;

    date d(2024, 6, 15);
    time t(14, 30, 45);
    datetime dt(move(d), move(t));
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, MoveDateAndTimeConstructorWithOffset) {
    using neforce::time;

    date d(2024, 6, 15);
    time t(14, 30, 45);
    datetime dt(move(d), move(t), 28800);
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 28800);
}

TEST(DatetimeTest, CopyConstructor) {
    datetime dt1(2024, 6, 15, 14, 30, 45);
    datetime dt2(dt1);
    EXPECT_EQ(dt2.year(), 2024);
    EXPECT_EQ(dt2.month(), 6);
    EXPECT_EQ(dt2.day(), 15);
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 45);
}

TEST(DatetimeTest, CopyAssignment) {
    datetime dt1(2024, 6, 15, 14, 30, 45);
    datetime dt2;
    dt2 = dt1;
    EXPECT_EQ(dt2.year(), 2024);
    EXPECT_EQ(dt2.month(), 6);
    EXPECT_EQ(dt2.day(), 15);
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 45);
}

TEST(DatetimeTest, MoveConstructor) {
    datetime dt1(2024, 6, 15, 14, 30, 45);
    datetime dt2(move(dt1));
    EXPECT_EQ(dt2.year(), 2024);
    EXPECT_EQ(dt2.month(), 6);
    EXPECT_EQ(dt2.day(), 15);
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 45);
}

TEST(DatetimeTest, MoveAssignment) {
    datetime dt1(2024, 6, 15, 14, 30, 45);
    datetime dt2;
    dt2 = move(dt1);
    EXPECT_EQ(dt2.year(), 2024);
    EXPECT_EQ(dt2.month(), 6);
    EXPECT_EQ(dt2.day(), 15);
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 45);
}

TEST(DatetimeTest, IsValid) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_TRUE(dt.is_valid());

    datetime dt2(1800, 1, 1, 0, 0, 0);
    EXPECT_TRUE(dt2.is_valid()); // default

    datetime dt3(2024, 6, 15, 24, 0, 0);
    EXPECT_TRUE(dt3.is_valid()); // default
}

TEST(DatetimeTest, AssignFromDate) {
    datetime dt;
    date d(2024, 6, 15);
    dt = d;
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, MoveAssignFromDate) {
    datetime dt;
    date d(2024, 6, 15);
    dt = move(d);
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
}

TEST(DatetimeTest, AssignFromTime) {
    using neforce::time;

    datetime dt(2024, 6, 15, 0, 0, 0);
    time t(14, 30, 45);
    dt = t;
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, MoveAssignFromTime) {
    using neforce::time;

    datetime dt(2024, 6, 15, 0, 0, 0);
    time t(14, 30, 45);
    dt = move(t);
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, Accessors) {
    using neforce::time;

    datetime dt(2024, 6, 15, 14, 30, 45, 28800);
    const date& d = dt.date();
    EXPECT_EQ(d.year(), 2024);
    EXPECT_EQ(d.month(), 6);
    EXPECT_EQ(d.day(), 15);

    const time& t = dt.time();
    EXPECT_EQ(t.hours(), 14);
    EXPECT_EQ(t.minutes(), 30);
    EXPECT_EQ(t.seconds(), 45);

    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 28800);
}

TEST(DatetimeTest, Epoch) {
    datetime dt = datetime::epoch();
    EXPECT_EQ(dt.year(), 1970);
    EXPECT_EQ(dt.month(), 1);
    EXPECT_EQ(dt.day(), 1);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, Now) {
    datetime dt = datetime::now();
    EXPECT_GE(dt.year(), 2024);
    EXPECT_TRUE(dt.is_valid());
}

TEST(DatetimeTest, Clear) {
    datetime dt(2024, 6, 15, 14, 30, 45, 28800);
    dt.clear();
    EXPECT_EQ(dt.year(), 1970);
    EXPECT_EQ(dt.month(), 1);
    EXPECT_EQ(dt.day(), 1);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
    EXPECT_FALSE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 0);
}

TEST(DatetimeTest, EqualTo) {
    datetime dt1(2024, 6, 15, 14, 30, 45);
    datetime dt2(2024, 6, 15, 14, 30, 45);
    datetime dt3(2024, 6, 15, 14, 30, 46);
    EXPECT_TRUE(dt1.equal_to(dt2));
    EXPECT_FALSE(dt1.equal_to(dt3));
    EXPECT_TRUE(dt1 == dt2);
    EXPECT_FALSE(dt1 == dt3);
    EXPECT_TRUE(dt1 != dt3);
}

TEST(DatetimeTest, LessThan) {
    datetime dt1(2024, 6, 15, 14, 30, 45);
    datetime dt2(2024, 6, 15, 14, 30, 46);
    datetime dt3(2024, 6, 16, 0, 0, 0);
    datetime dt4(2025, 1, 1, 0, 0, 0);
    EXPECT_TRUE(dt1.less_than(dt2));
    EXPECT_TRUE(dt1.less_than(dt3));
    EXPECT_TRUE(dt1.less_than(dt4));
    EXPECT_FALSE(dt2.less_than(dt1));
    EXPECT_TRUE(dt1 < dt2);
    EXPECT_TRUE(dt1 <= dt1);
    EXPECT_TRUE(dt1 >= dt1);
    EXPECT_TRUE(dt2 > dt1);
}

TEST(DatetimeTest, AddSeconds) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    dt += 1;
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 46);

    dt += 14;
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 31);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, AddSecondsCrossDay) {
    datetime dt(2024, 6, 15, 23, 59, 59);
    dt += 1;
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 16);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, AddSecondsCrossMonth) {
    datetime dt(2024, 1, 31, 23, 59, 59);
    dt += 1;
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 2);
    EXPECT_EQ(dt.day(), 1);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, AddSecondsCrossYear) {
    datetime dt(2024, 12, 31, 23, 59, 59);
    dt += 1;
    EXPECT_EQ(dt.year(), 2025);
    EXPECT_EQ(dt.month(), 1);
    EXPECT_EQ(dt.day(), 1);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, AddSecondsNegative) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    dt += -1;
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 44);
}

TEST(DatetimeTest, SubtractSeconds) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    dt -= 1;
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 44);

    dt -= 44;
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, SubtractSecondsCrossDay) {
    datetime dt(2024, 6, 15, 0, 0, 0);
    dt -= 1;
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 14);
    EXPECT_EQ(dt.hours(), 23);
    EXPECT_EQ(dt.minutes(), 59);
    EXPECT_EQ(dt.seconds(), 59);
}

TEST(DatetimeTest, SubtractSecondsCrossMonth) {
    datetime dt(2024, 3, 1, 0, 0, 0);
    dt -= 1;
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 2);
    EXPECT_EQ(dt.day(), 29);
    EXPECT_EQ(dt.hours(), 23);
    EXPECT_EQ(dt.minutes(), 59);
    EXPECT_EQ(dt.seconds(), 59);
}

TEST(DatetimeTest, SubtractSecondsNegative) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    dt -= -1;
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 46);
}

TEST(DatetimeTest, OperatorPlusSeconds) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    datetime dt2 = dt + 1;
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 46);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, OperatorMinusSeconds) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    datetime dt2 = dt - 1;
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 44);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, PreIncrement) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    ++dt;
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 46);
}

TEST(DatetimeTest, PostIncrement) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    datetime dt2 = dt++;
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 45);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 46);
}

TEST(DatetimeTest, PreDecrement) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    --dt;
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 44);
}

TEST(DatetimeTest, PostDecrement) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    datetime dt2 = dt--;
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 45);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 44);
}

TEST(DatetimeTest, DatetimeDifference) {
    datetime dt1(2024, 6, 15, 14, 30, 45);
    datetime dt2(2024, 6, 15, 14, 30, 44);
    EXPECT_EQ(dt1 - dt2, 1);
    EXPECT_EQ(dt2 - dt1, -1);

    datetime dt3(2024, 6, 16, 0, 0, 0);
    datetime dt4(2024, 6, 15, 0, 0, 0);
    EXPECT_EQ(dt3 - dt4, 86400);
}

TEST(DatetimeTest, DatetimeDifferenceWithTimezone) {
    datetime dt1(2024, 6, 15, 14, 30, 45, 28800);
    datetime dt2(2024, 6, 15, 14, 30, 44, 28800);
    EXPECT_EQ(dt1 - dt2, 1);

    datetime dt3(2024, 6, 15, 14, 30, 45, 28800);
    datetime dt4(2024, 6, 15, 14, 30, 45, 0);
    EXPECT_EQ(dt3 - dt4, -28800);
}

TEST(DatetimeTest, OffsetStringZero) {
    datetime dt1(2024, 6, 15, 14, 30, 45, 0);
    EXPECT_EQ(dt1.to_offset_string(), "Z");
}

TEST(DatetimeTest, OffsetStringPositive) {
    datetime dt1(2024, 6, 15, 14, 30, 45, 28800);
    EXPECT_EQ(dt1.to_offset_string(), "+08:00");

    datetime dt2(2024, 6, 15, 14, 30, 45, 19800);
    EXPECT_EQ(dt2.to_offset_string(), "+05:30");
}

TEST(DatetimeTest, OffsetStringNegative) {
    datetime dt1(2024, 6, 15, 14, 30, 45, -18000);
    EXPECT_EQ(dt1.to_offset_string(), "-05:00");

    datetime dt2(2024, 6, 15, 14, 30, 45, -28800);
    EXPECT_EQ(dt2.to_offset_string(), "-08:00");
}

TEST(DatetimeTest, OffsetStringNoTimezone) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_EQ(dt.to_offset_string(), "");
}

TEST(DatetimeTest, FromUTC) {
    datetime utc(2024, 6, 15, 6, 30, 0, 0);
    datetime local = datetime::from_UTC(utc, 28800);
    EXPECT_EQ(local.year(), 2024);
    EXPECT_EQ(local.month(), 6);
    EXPECT_EQ(local.day(), 15);
    EXPECT_EQ(local.hours(), 14);
    EXPECT_EQ(local.minutes(), 30);
    EXPECT_EQ(local.seconds(), 0);
    EXPECT_TRUE(local.has_timezone());
    EXPECT_EQ(local.offset_seconds(), 28800);
}

TEST(DatetimeTest, FromUTCWithNegativeOffset) {
    datetime utc(2024, 6, 15, 12, 0, 0, 0);
    datetime local = datetime::from_UTC(utc, -18000);
    EXPECT_EQ(local.year(), 2024);
    EXPECT_EQ(local.month(), 6);
    EXPECT_EQ(local.day(), 15);
    EXPECT_EQ(local.hours(), 7);
    EXPECT_EQ(local.minutes(), 0);
    EXPECT_EQ(local.seconds(), 0);
    EXPECT_EQ(local.offset_seconds(), -18000);
}

TEST(DatetimeTest, ToUTC) {
    datetime local(2024, 6, 15, 14, 30, 0, 28800);
    datetime utc = local.to_UTC();
    EXPECT_EQ(utc.year(), 2024);
    EXPECT_EQ(utc.month(), 6);
    EXPECT_EQ(utc.day(), 15);
    EXPECT_EQ(utc.hours(), 6);
    EXPECT_EQ(utc.minutes(), 30);
    EXPECT_EQ(utc.seconds(), 0);
    EXPECT_TRUE(utc.has_timezone());
    EXPECT_EQ(utc.offset_seconds(), 0);
}

TEST(DatetimeTest, ToUTCNoTimezone) {
    datetime dt(2024, 6, 15, 14, 30, 0);
    datetime utc = dt.to_UTC();
    EXPECT_EQ(utc.year(), 2024);
    EXPECT_EQ(utc.month(), 6);
    EXPECT_EQ(utc.day(), 15);
    EXPECT_EQ(utc.hours(), 14);
    EXPECT_EQ(utc.minutes(), 30);
    EXPECT_EQ(utc.seconds(), 0);
    EXPECT_FALSE(utc.has_timezone());
}

TEST(DatetimeTest, ToRFC3339WithUTC) {
    datetime dt(2024, 6, 15, 14, 30, 45, 0);
    EXPECT_EQ(dt.to_RFC3339(), "2024-06-15T14:30:45Z");
}

TEST(DatetimeTest, ToRFC3339WithPositiveOffset) {
    datetime dt(2024, 6, 15, 14, 30, 45, 28800);
    EXPECT_EQ(dt.to_RFC3339(), "2024-06-15T14:30:45+08:00");
}

TEST(DatetimeTest, ToRFC3339WithNegativeOffset) {
    datetime dt(2024, 6, 15, 14, 30, 45, -18000);
    EXPECT_EQ(dt.to_RFC3339(), "2024-06-15T14:30:45-05:00");
}

TEST(DatetimeTest, ParseRFC3339WithoutTimezone) {
    datetime dt = datetime::parse_RFC3339("2024-06-15T14:30:45Z");
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
}

TEST(DatetimeTest, ParseRFC3339WithoutTimezoneShouldThrow) {
    EXPECT_THROW(ignore = datetime::parse_RFC3339("2024-06-15T14:30:45"), value_exception);
}

TEST(DatetimeTest, ParseRFC3339WithUTC) {
    datetime dt = datetime::parse_RFC3339("2024-06-15T14:30:45Z");
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 0);
}

TEST(DatetimeTest, ParseRFC3339WithPositiveOffset) {
    datetime dt = datetime::parse_RFC3339("2024-06-15T14:30:45+08:00");
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 28800);
}

TEST(DatetimeTest, ParseRFC3339WithNegativeOffset) {
    datetime dt = datetime::parse_RFC3339("2024-06-15T14:30:45-05:00");
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), -18000);
}

TEST(DatetimeTest, ParseRFC3339WithPositiveOffsetNoColon) {
    EXPECT_THROW(ignore = datetime::parse_RFC3339("2024-06-15T14:30:45+0800"), value_exception);
}

TEST(DatetimeTest, ParseRFC3339WithUTCAndTrailingCharacters) {
    EXPECT_THROW(ignore = datetime::parse_RFC3339("2024-06-15T14:30:45Zextra"), value_exception);
}

TEST(DatetimeTest, ParseRFC3339WithInvalidOffsetValues) {
    EXPECT_THROW(ignore = datetime::parse_RFC3339("2024-06-15T14:30:45+24:00"), value_exception);
    EXPECT_THROW(ignore = datetime::parse_RFC3339("2024-06-15T14:30:45+00:60"), value_exception);
}

TEST(DatetimeTest, ParseRFC3339WithMissingOffsetColon) {
    EXPECT_THROW(ignore = datetime::parse_RFC3339("2024-06-15T14:30:45+08-00"), value_exception);
}

TEST(DatetimeTest, ParseRFC3339Invalid) {
    EXPECT_THROW(ignore = datetime::parse_RFC3339("2024-06-15 14:30:45"), value_exception);
    EXPECT_THROW(ignore = datetime::parse_RFC3339("invalid"), value_exception);
}

TEST(DatetimeTest, TryParseRFC3339WithoutTimezoneShouldFail) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_FALSE(dt.try_parse_RFC3339("2024-06-15T14:30:45"));
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
}

TEST(DatetimeTest, TryParseRFC3339Valid) {
    datetime dt;
    EXPECT_TRUE(dt.try_parse_RFC3339("2024-06-15T14:30:45Z"));
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
    EXPECT_TRUE(dt.has_timezone());
    EXPECT_EQ(dt.offset_seconds(), 0);
}

TEST(DatetimeTest, TryParseRFC3339Invalid) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_FALSE(dt.try_parse_RFC3339("invalid"));
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
}

TEST(DatetimeTest, ToRFC3339WithoutTimezoneDefaultsToUTC) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_EQ(dt.to_RFC3339(), "2024-06-15T14:30:45Z");
}

TEST(DatetimeTest, ToRFC1123) {
    datetime dt(2022, 12, 21, 10, 0, 0);
    string result = dt.to_RFC1123();
    EXPECT_NE(result.find("21 Dec 2022"), string::npos);
    EXPECT_NE(result.find("10:00:00 GMT"), string::npos);
}

TEST(DatetimeTest, ParseRFC1123) {
    datetime dt = datetime::parse_RFC1123("Wed, 21 Dec 2022 10:00:00 GMT");
    EXPECT_EQ(dt.year(), 2022);
    EXPECT_EQ(dt.month(), 12);
    EXPECT_EQ(dt.day(), 21);
    EXPECT_EQ(dt.hours(), 10);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, ParseRFC1123AllMonths) {
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < 12; ++i) {
        string str = string("Mon, 15 ") + months[i] + " 2024 12:00:00 GMT";
        datetime dt = datetime::parse_RFC1123(str.view());
        EXPECT_EQ(dt.month(), i + 1);
        EXPECT_EQ(dt.day(), 15);
        EXPECT_EQ(dt.year(), 2024);
    }
}

TEST(DatetimeTest, ParseRFC1123Invalid) {
    EXPECT_THROW(ignore = datetime::parse_RFC1123("invalid date string"), value_exception);
    EXPECT_THROW(ignore = datetime::parse_RFC1123("Wed, 21 Dec 2022 10:00:00 PST"), value_exception);
}

TEST(DatetimeTest, TryParseRFC1123Valid) {
    datetime dt;
    EXPECT_TRUE(dt.try_parse_RFC1123("Wed, 21 Dec 2022 10:00:00 GMT"));
    EXPECT_EQ(dt.year(), 2022);
    EXPECT_EQ(dt.month(), 12);
    EXPECT_EQ(dt.day(), 21);
    EXPECT_EQ(dt.hours(), 10);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(DatetimeTest, TryParseRFC1123Invalid) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_FALSE(dt.try_parse_RFC1123("invalid"));
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
}

TEST(DatetimeTest, ToISO8601) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_EQ(dt.to_ISO8601(), "2024-06-15T14:30:45");
}

TEST(DatetimeTest, ParseISO8601) {
    datetime dt = datetime::parse_ISO8601("2024-06-15T14:30:45");
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, ParseISO8601Invalid) {
    EXPECT_THROW(ignore = datetime::parse_ISO8601("2024-06-15 14:30:45"), value_exception);
    EXPECT_THROW(ignore = datetime::parse_ISO8601("invalid"), value_exception);
}

TEST(DatetimeTest, TryParseISO8601Valid) {
    datetime dt;
    EXPECT_TRUE(dt.try_parse_ISO8601("2024-06-15T14:30:45"));
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, TryParseISO8601Invalid) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_FALSE(dt.try_parse_ISO8601("invalid"));
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
}

TEST(DatetimeTest, ToString) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    EXPECT_EQ(dt.to_string(), "2024-06-15 14:30:45");
}

TEST(DatetimeTest, Parse) {
    datetime dt = datetime::parse("2024-06-15 14:30:45");
    EXPECT_EQ(dt.year(), 2024);
    EXPECT_EQ(dt.month(), 6);
    EXPECT_EQ(dt.day(), 15);
    EXPECT_EQ(dt.hours(), 14);
    EXPECT_EQ(dt.minutes(), 30);
    EXPECT_EQ(dt.seconds(), 45);
}

TEST(DatetimeTest, ParseInvalid) {
    EXPECT_THROW(ignore = datetime::parse("2024-06-15T14:30:45"), value_exception);
    EXPECT_THROW(ignore = datetime::parse("invalid"), value_exception);
    EXPECT_THROW(ignore = datetime::parse("2024-06-15 14:30:450"), value_exception);
}

TEST(DatetimeTest, Hash) {
    datetime dt1(2024, 6, 15, 14, 30, 45);
    datetime dt2(2024, 6, 15, 14, 30, 45);
    datetime dt3(2024, 6, 15, 14, 30, 46);
    EXPECT_EQ(dt1.to_hash(), dt2.to_hash());
    EXPECT_NE(dt1.to_hash(), dt3.to_hash());
}

TEST(DatetimeTest, HashWithTimezone) {
    datetime dt1(2024, 6, 15, 14, 30, 45, 28800);
    datetime dt2(2024, 6, 15, 14, 30, 45, 28800);
    datetime dt3(2024, 6, 15, 14, 30, 45, 0);
    EXPECT_EQ(dt1.to_hash(), dt2.to_hash());
    EXPECT_NE(dt1.to_hash(), dt3.to_hash());
}

TEST(DatetimeTest, Swap) {
    datetime dt1(2024, 6, 15, 14, 30, 45, 28800);
    datetime dt2(2023, 1, 1, 0, 0, 0);
    dt1.swap(dt2);
    EXPECT_EQ(dt1.year(), 2023);
    EXPECT_EQ(dt1.month(), 1);
    EXPECT_EQ(dt1.day(), 1);
    EXPECT_EQ(dt1.hours(), 0);
    EXPECT_EQ(dt1.minutes(), 0);
    EXPECT_EQ(dt1.seconds(), 0);
    EXPECT_FALSE(dt1.has_timezone());
    EXPECT_EQ(dt2.year(), 2024);
    EXPECT_EQ(dt2.month(), 6);
    EXPECT_EQ(dt2.day(), 15);
    EXPECT_EQ(dt2.hours(), 14);
    EXPECT_EQ(dt2.minutes(), 30);
    EXPECT_EQ(dt2.seconds(), 45);
    EXPECT_TRUE(dt2.has_timezone());
    EXPECT_EQ(dt2.offset_seconds(), 28800);
}

TEST(TimestampTest, DefaultConstructor) {
    timestamp ts;
    EXPECT_EQ(ts.value(), 0);
}

TEST(TimestampTest, ValueConstructor) {
    timestamp ts(1234567890);
    EXPECT_EQ(ts.value(), 1234567890);
}

TEST(TimestampTest, DatetimeConstructor) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    timestamp ts(dt);
    datetime dt_back = ts.to_datetime();
    EXPECT_EQ(dt_back.year(), 2024);
    EXPECT_EQ(dt_back.month(), 6);
    EXPECT_EQ(dt_back.day(), 15);
    EXPECT_EQ(dt_back.hours(), 14);
    EXPECT_EQ(dt_back.minutes(), 30);
    EXPECT_EQ(dt_back.seconds(), 45);
}

TEST(TimestampTest, DatetimeConstructorWithTimezone) {
    datetime dt(2024, 6, 15, 14, 30, 45, 28800);
    timestamp ts(dt);
    datetime dt_back = ts.to_datetime();
    EXPECT_EQ(dt_back.year(), 2024);
    EXPECT_EQ(dt_back.month(), 6);
    EXPECT_EQ(dt_back.day(), 15);
    EXPECT_EQ(dt_back.hours(), 6);
    EXPECT_EQ(dt_back.minutes(), 30);
    EXPECT_EQ(dt_back.seconds(), 45);
    EXPECT_FALSE(dt_back.has_timezone());
}

TEST(TimestampTest, CopyConstructor) {
    timestamp ts1(1234567890);
    timestamp ts2(ts1);
    EXPECT_EQ(ts2.value(), 1234567890);
}

TEST(TimestampTest, CopyAssignment) {
    timestamp ts1(1234567890);
    timestamp ts2;
    ts2 = ts1;
    EXPECT_EQ(ts2.value(), 1234567890);
}

TEST(TimestampTest, MoveConstructor) {
    timestamp ts1(1234567890);
    timestamp ts2(move(ts1));
    EXPECT_EQ(ts2.value(), 1234567890);
}

TEST(TimestampTest, MoveAssignment) {
    timestamp ts1(1234567890);
    timestamp ts2;
    ts2 = move(ts1);
    EXPECT_EQ(ts2.value(), 1234567890);
}

TEST(TimestampTest, Now) {
    timestamp ts = timestamp::now();
    EXPECT_GT(ts.value(), 0);
}

TEST(TimestampTest, ToDatetimeEpoch) {
    timestamp ts(0);
    datetime dt = ts.to_datetime();
    EXPECT_EQ(dt.year(), 1970);
    EXPECT_EQ(dt.month(), 1);
    EXPECT_EQ(dt.day(), 1);
    EXPECT_EQ(dt.hours(), 0);
    EXPECT_EQ(dt.minutes(), 0);
    EXPECT_EQ(dt.seconds(), 0);
}

TEST(TimestampTest, ToDatetimeRoundTrip) {
    datetime dt(2024, 6, 15, 14, 30, 45);
    timestamp ts(dt);
    datetime dt_back = ts.to_datetime();
    EXPECT_EQ(dt_back.year(), dt.year());
    EXPECT_EQ(dt_back.month(), dt.month());
    EXPECT_EQ(dt_back.day(), dt.day());
    EXPECT_EQ(dt_back.hours(), dt.hours());
    EXPECT_EQ(dt_back.minutes(), dt.minutes());
    EXPECT_EQ(dt_back.seconds(), dt.seconds());
}

TEST(TimestampTest, ToString) {
    timestamp ts(1234567890);
    EXPECT_EQ(ts.to_string(), "1234567890");

    timestamp ts2(-1234567890);
    EXPECT_EQ(ts2.to_string(), "-1234567890");

    timestamp ts3(0);
    EXPECT_EQ(ts3.to_string(), "0");
}

TEST(TimestampTest, Parse) {
    timestamp ts = timestamp::parse("1234567890");
    EXPECT_EQ(ts.value(), 1234567890);
}

TEST(TimestampTest, ParseNegative) {
    timestamp ts = timestamp::parse("-1234567890");
    EXPECT_EQ(ts.value(), -1234567890);
}

TEST(TimestampTest, Clear) {
    timestamp ts(1234567890);
    ts.clear();
    EXPECT_EQ(ts.value(), 0);
}

TEST(TimestampTest, OperatorEqual) {
    timestamp ts1(1234567890);
    timestamp ts2(1234567890);
    timestamp ts3(987654321);
    EXPECT_TRUE(ts1 == ts2);
    EXPECT_FALSE(ts1 == ts3);
    EXPECT_TRUE(ts1 != ts3);
}

TEST(TimestampTest, OperatorLessThan) {
    timestamp ts1(100);
    timestamp ts2(200);
    EXPECT_TRUE(ts1 < ts2);
    EXPECT_TRUE(ts1 <= ts1);
    EXPECT_TRUE(ts1 >= ts1);
    EXPECT_TRUE(ts2 > ts1);
}
