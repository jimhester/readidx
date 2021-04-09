#ifndef READR_DATE_TIME_H_
#define READR_DATE_TIME_H_

#include <cpp11/R.hpp>
#include <clock/clock.h>
#include <stdlib.h>
#include <string>

class DateTime {
  int year_, mon_, day_, hour_, min_, sec_, offset_;
  double psec_;
  std::string tz_;

public:
  DateTime(
      int year,
      int mon,
      int day,
      int hour = 0,
      int min = 0,
      int sec = 0,
      double psec = 0,
      const std::string& tz = "")
      : year_(year),
        mon_(mon),
        day_(day),
        hour_(hour),
        min_(min),
        sec_(sec),
        offset_(0),
        psec_(psec),
        tz_(tz) {}

  // Used to add time zone offsets which can only be easily applied once
  // we've converted into seconds since epoch.
  void setOffset(int offset) { offset_ = offset; }

  // Is this a valid date time?
  bool validDateTime() const { return validDate() && validTime(); }

  bool validDate() const {
    // vroom does not allow negative years, date does
    if (year_ < 0)
      return false;

    return (date::year{year_} / mon_ / day_).ok();
  }

  bool validTime() const {
    if (sec_ < 0 || sec_ > 60)
      return false;
    if (min_ < 0 || min_ > 59)
      return false;
    if (hour_ < 0 || hour_ > 23)
      return false;

    return true;
  }

  double datetime() const { return (tz_ == "UTC") ? utctime() : localtime(); }

  int date() const { return utcdate(); }

  double time() const { return psec_ + sec_ + (min_ * 60) + (hour_ * 3600); }

private:
  // Number of number of seconds since 1970-01-01T00:00:00Z.
  // Compared to usual implementations this returns a double, and supports
  // a wider range of dates. Invalid dates have undefined behaviour.
  double utctime() const { return utcdate() * 86400.0 + time() + offset_; }

  // Find number of days since 1970-01-01.
  // Invalid dates have undefined behaviour.
  int utcdate() const {
    if (!validDate())
      return NA_REAL;

    const date::year_month_day ymd{date::year(year_) / mon_ / day_};
    const date::sys_days st{ymd};
    return st.time_since_epoch().count();
  }

  double localtime() const {
    if (!validDateTime())
      return NA_REAL;

    const date::time_zone* p_time_zone = rclock::zone_name_load(tz_);

    const date::local_seconds lt =
      std::chrono::seconds{sec_} +
      std::chrono::minutes{min_} +
      std::chrono::hours{hour_} +
      date::local_days{date::year{year_} / mon_ / day_};

    const rclock::sys_result result = rclock::local_to_sys(lt, p_time_zone);

    if (result.ok) {
      return result.st.time_since_epoch().count() + psec_ + offset_;
    } else {
      return NA_REAL;
    }
  }
};

#endif
