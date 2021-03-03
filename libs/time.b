/*
This modules provides Bird's implementation of date and time
manipulation methods.

Time is stored internally as the number of seconds 
with fraction since the Epoch, January 1, 1970 00:00 UTC.

@copyright 2020, Ore Richard Muyiwa
*/

/*
Jade's implementation of date

A date here refers to a calendar day consisting of
year, month and day
*/
class Date {

  static var MIN_YEAR = 1
  static var MAX_YEAR = 9999
  static var MIN_DAY = 1
  static var MAX_DAY = 31
  static var MIN_MONTH = 1
  static var MAX_MONTH = 12
  static var MAX_HOUR = 23
  static var MAX_MINUTE = 59
  static var MAX_SECOND = 59

  _Check_Int_Field(field, name) {
    if !is_int(field)
      die Exception('${name} must be an integer')
  }

  _Check_Date_Fields(year, month, day, hour, minute, second) {
    self._Check_Int_Field(year, 'year')
    self._Check_Int_Field(month, 'month')
    self._Check_Int_Field(day, 'day')

    if Date.MIN_YEAR > year or year > Date.MAX_YEAR
      die Exception('year must be in ${Date.MIN_YEAR}..${Date.MAX_YEAR}')   
    if 1 > month or month > 12
      die Exception('month must be in 1..12') 
    var dim = self.days_in_month(year, month)   
    if 1 > month or month > dim
      die Exception('month must be in 1..${dim}')   

    self._Check_Int_Field(hour, 'hour')
    self._Check_Int_Field(minute, 'minute')
    self._Check_Int_Field(second, 'second')

    if 0 > hour or hour > 23
      die Exception('hour must be in 0..23')
    if 0 > minute or minute > 59
      die Exception('minute must be in 0..59')
    if 0 > second or second > 59
      die Exception('second must be in 0..59')
  }

  # the number of days in each month of the year
  # the -1 is a placeholder for proper indexing
  static var _Days_In_Month = [-1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 31, 31]

  # the name of months in a year
  # the nil is meant for proper indexing
  var _Months_In_Year = [
      nil,
      'January', 'February', 'March', 'April', 'May', 'June', 
      'July', 'August', 'September', 'October', 'November', 'December'
  ]

  # the short name of months in a year
  # the nil is meant for proper indexing
  var _Months_In_Year_Short = [
    nil,
    'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
    'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'
  ]

  # internal declaration to get the
  # days before month
  static var _Days_before_month = |m, g| {
    var days_before_month = []
    var days = 0
    for f in Date._Days_In_Month[m,-1] {
      days_before_month.append(days)
      days += f 
    }
    return days_before_month[g]
  }

  static var _Days_before_year = |year| {
    var y = year - 1
    return y*365 + y//4 - y//100 + y//400
  }

  static var _Is_leap = |year| {
    return year % 4 == 0 and 
      (year % 100 != 0 or year % 400 == 0)
  }

  static var _Ymd_to_ordinal = |year, month, day| {
    assert month >= 1 and month <= 12, 'month must be in 1..12'

    var dim
    if month == 2 and Date._Is_leap(year)
      dim = 29
    else dim = Date._Days_In_Month[month]
    
    assert day >= 1 and day <= dim, 'day must be in 1..%d'
    return (Date._Days_before_year(year) +
            Date._Days_before_month(0, month) +
            day)
  }

  /*
  Constructor
  -----------
  required argument is year
  */
  Date(year, month, day, hour, minute, second) {

    if year {
      self.year = year

      if month self.month = month
      else self.month = Date.MIN_MONTH

      if day self.day = day
      else self.day = Date.MIN_DAY

      if hour self.hour = hour
      else self.hour = 0

      if minute self.minute = minute
      else self.minute = 0

      if second self.second = second
      else self.second = 0
    
      self._Check_Date_Fields(self.year, self.month, self.day, 
          self.hour, self.minute, self.second)

      /* self.weekday = (Date._Ymd_to_ordinal(self.year, self.month, self.day) + 6) % 7
      self.day_of_year = Date._Days_before_month(1, self.month) + self.day */

    } else {
      var date = Date.localtime()

      self.year = date.year
      self.month = date.month
      self.day = date.day
      self.hour = date.hour
      self.minute = date.minute
      self.second = date.second
    }
  }

  # cask method for native gmtime
  static gmtime() {}

  # cask method for native localtime
  static localtime() {}

  /*
  is_leap() returns true if the year is a leap year or false otherwise
  */
  is_leap() {
    return Date._Is_leap(self.year)
  }

  /*
  days_before_year() returns the number of days before January 1st of year
  */
  days_before_year(year) {
    assert year >= Date.MIN_YEAR and year <= Date.MAX_YEAR,
      'year must be in 1..' + Date.MAX_YEAR
    assert year > self.year, 'year must be greater than current year'

    if self.month < Date.MAX_MONTH {
      var days_left_in_year = self.days_before_month(Date.MAX_MONTH) + 
          Date._Days_In_Month[Date.MAX_MONTH]
    } else {
      var days_left_in_year = Date._Days_In_Month[Date.MAX_MONTH] - self.day
    }

    if self.is_leap() {
      var days_in_year = 366 - days_left_in_year
    } else {
      var days_in_year = 365 - days_left_in_year
    }
      
    var days_till_today = Date._Days_before_year(self.year) + days_in_year
    var days_before_year = Date._Days_before_year(year)

    return days_before_year - days_till_today
  }

  /*
  days_in_month() returns the number of days in month for the specified year
  */
  days_in_month(year, month) {
    if month == 2 and Date._Is_leap(year)
      return 29
    return Date._Days_In_Month[month]
  }

  /*
  days_before_month()

  returns the number of days in the year preceeding the first 
  day of the month
  */
  days_before_month(month) {
    assert month >= 1 and month <= 12, 'month must be in 1..12'

    var start = self.month, end = month, day = self.day, comparator = end < 2
    if self.month > month {
      start = month
      end = self.month 
      day = -(self.day)
      comparator = start <= 2
    }

    var days_before = Date._Days_before_month(start, end - start)
    if comparator and self.is_leap() {
      return days_before + 1 - day
    }

    # when going back in time, we must remember not to count today
    if self.month > month
      return -(days_before - day - 1)

    return days_before - day
  }

  /*
  from_time(time: seconds)
  returns a date object from a unix timestamp
  */
  static from_time(time) {

    var _DI400Y = Date._Days_before_year(401) # number of days in 400 years
    var _DI100Y = Date._Days_before_year(101) # number of days in 100 years
    var _DI4Y = Date._Days_before_year(5)     # number of days in 4 years

    assert _DI4Y == 4 * 365 + 1
    assert _DI400Y == 4 * _DI100Y + 1
    assert _DI100Y == 25 * _DI4Y - 1

    var _SI1D = 86400 # seconds in a day
    var _DTUNIX = 719162 # days from UTC year 1 to unix year 1970

    var n = to_int(time / _SI1D + _DTUNIX)

    # n is a 1-based index, starting at 1-Jan-1.  The pattern of leap years
    # repeats exactly every 400 years.  The basic strategy is to find the
    # closest 400-year boundary at or before n, then work with the offset
    # from that boundary to n.  Life is much clearer if we subtract 1 from
    # n first -- then the values of n at 400-year boundaries are exactly
    # those divisible by _DI400Y:
    #
    #     D  M   Y            n              n-1
    #     -- --- ----        ----------     ----------------
    #     31 Dec -400        -_DI400Y       -_DI400Y -1
    #      1 Jan -399         -_DI400Y +1   -_DI400Y      400-year boundary
    #     ...
    #     30 Dec  000        -1             -2
    #     31 Dec  000         0             -1
    #      1 Jan  001         1              0            400-year boundary
    #      2 Jan  001         2              1
    #      3 Jan  001         3              2
    #     ...
    #     31 Dec  400         _DI400Y        _DI400Y -1
    #      1 Jan  401         _DI400Y +1     _DI400Y      400-year boundary
    n--
    var n400 = to_int(n / _DI400Y)
    n %= _DI400Y
    var year = n400 * 400 + 1   # ..., -399, 1, 401, ...

    # Now n is the (non-negative) offset, in days, from January 1 of year, to
    # the desired date.  Now compute how many 100-year cycles precede n.
    # Note that it's possible for n100 to equal 4!  In that case 4 full
    # 100-year cycles precede the desired day, which implies the desired
    # day is December 31 at the end of a 400-year cycle.
    var n100 = to_int(n / _DI100Y)
    n %= _DI100Y

    # Now compute how many 4-year cycles precede it.
    var n4 = to_int(n / _DI4Y)
    n %= _DI4Y

    # And now how many single years.  Again n1 can be 4, and again meaning
    # that the desired day is December 31 at the end of the 4-year cycle.
    var n1 = to_int(n / 365)
    n %= 365

    year += n100 * 100 + n4 * 4 + n1
    var month, day

    if n1 == 4 or n100 == 4 {
      assert n == 0
      year -= 1
      month = 12
      day = 31
      # return Date(year - 1, 12, 31)
    } else {

      # Now the year is correct, and n is the offset from January 1.  We find
      # the month via an estimate that's either exact or one too large.
      var leapyear = n1 == 3 and (n4 != 24 or n100 == 3)
      assert leapyear == Date._Is_leap(year)

      month = (n + 50) >> 5
      var preceding = Date._Days_before_month(0, month) + to_number(month > 2 and leapyear)
      
      if preceding > n {  # estimate is too large
        month -= 1
        preceding -= Date._Days_In_Month[month] + to_number(month == 2 and leapyear)
      }
      day = n - preceding + 1
    }

    var today = time - ((Date._Ymd_to_ordinal(year, month, day) - _DTUNIX) * _SI1D)
    var hour = to_int(today / 3600) - Date.MAX_HOUR # where 3600 is number of seconds in hour
    
    if hour == Date.MAX_HOUR + 1 hour = 0
    else if hour < 1 hour += Date.MAX_HOUR + 1

    var now = today % 3600

    var minute = to_int(now / 60) # 60 seconds in a minute
    now %= 60

    # Now the year and month are correct, and n is the offset from the
    # start of that month:  we're done!
    return Date(year, month, day, hour, minute, int(now))
  }

  /*
  format(format: string)
  formats the current date based on the specified string

  Birdy Date formatting table
  | Character | Description             | Example   |
  |-----------|-------------------------|-----------|
  | A         | AM or PM                | AM        |
  | d         | short day               | 1         |
  | D         | long day                | 01        |
  | m         | short month             | 9         |
  | M         | long month              | 09        |
  | E         | short month name        | Aug       |
  | F         | long month name         | August    |
  | y         | short year              | 09        |
  | Y         | long year               | 2009      |
  | h         | short hour              | 9         |
  | H         | long hour               | 09        |
  | g         | hour in 12 hrs format   | 9         |
  | i         | short minute            | 9         |
  | I         | long minute             | 09        |
  | s         | short second            | 9         |
  | S         | long second             | 09        |
  */
  format(format) {
    var result = ''
    iter var i = 0; i < format.length(); i++ {
      using format[i] {
        when 'A' {
          if self.hour >= 12 result += 'PM'
          else result += 'AM'
        }
        when 'd' {
          result += self.day
        }
        when 'D' {
          if self.day <= 9 result += ('0' + self.day)
          else result += self.day
        }
        when 'm' {
          result += self.month
        }
        when 'M' {
          if self.month <= 9 result += ('0' + self.month)
          else result += self.month
        }
        when 'h' {
          result += self.hour
        }
        when 'H' {
          if self.hour <= 9 result += ('0' + self.hour)
          else result += self.hour
        }
        when 'g' {
          if self.hour <= 12 result += self.hour
          else result += self.hour - 12
        }
        when 'i' {
          result += self.minute
        }
        when 'I' {
          if self.minute <= 9 result += ('0' + self.minute)
          else result += self.minute
        }
        when 's' {
          result += self.second
        }
        when 'S' {
          if self.second <= 9 result += ('0' + self.second)
          else result += self.second
        }
        when 'y' {
          result += to_string(self.year)[2,-1]
        }
        when 'Y' {
          result += self.year
        }
        when 'E' {
          result += self._Months_In_Year_Short[self.month]
        }
        when 'F' {
          result += self._Months_In_Year[self.month]
        }
        default {
          result += format[i]
        }
      }
    }

    return result
  }
}