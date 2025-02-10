#ifndef TIME_FUNCTIONS_H
#define TIME_FUNCTIONS_H

#include <time.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <iomanip>

using CLOCK_T = std::chrono::high_resolution_clock;

const long int NANOSEC_IN_SEC = 1000000000;
const long int MILLISEC_IN_SEC = 1000;
const long int NANOSEC_IN_MILLISEC = 1000000;


inline timespec operator+(const timespec &lhs, const timespec &rhs);
inline timespec operator-(const timespec &lhs, const timespec &rhs);
inline bool operator>(const timespec &lhs, const timespec &rhs);
inline bool operator<(const timespec &lhs, const timespec &rhs);
inline bool operator>=(const timespec &lhs, const timespec &rhs);
inline bool operator<=(const timespec &lhs, const timespec &rhs);
inline std::ostream &operator<<(std::ostream &stream, const timespec &ts);

// check if the clock is monolithic (for std::chrono)
template <typename Clock>
bool is_monotonic() {
    auto t1 = Clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto t2 = Clock::now();
    return t2 >= t1;
}


template <typename T>
struct TimeTraits;

template <>
struct TimeTraits<timespec> {
    using duration = timespec;
    using timepoint = timespec;

    static timepoint create_timepoint(int sec, int nsec) {
        return {sec, nsec};
    }

    static duration create_duration(int sec, int nsec) {
        return {sec, nsec};
    }

    static timepoint get_default_timepoint() {
        return {0, 0};
    }

    static duration get_default_duration() {
        return {0, 0};
    }

    static void get_time(timepoint *tp) {
        if (!tp)
        {
            throw std::invalid_argument("Null pointer passed to get_time");
        }

        if (clock_gettime(CLOCK_MONOTONIC, tp) != 0) {
            throw std::runtime_error("Failed to get time using clock_gettime");
        }
    }

    static duration diff(const timepoint &tp1, const timepoint &tp2) {
        timepoint result;
        if (tp1 > tp2) {
            result = tp1 - tp2;
        } else {
            result = tp2 - tp1;
        }
        return result;
    }

    static void sleep_until(const timepoint &tp) {
        timespec curr_time;
        get_time(&curr_time);

        if (curr_time > tp) return;
        
        timespec diff = tp - curr_time;

        if (nanosleep(&diff, nullptr) != 0) {
            throw std::runtime_error("nanosleep failed while sleeping until target time");
        }
        // while (nanosleep(&diff, &diff) != 0) {
        //     if ((diff.tv_sec == 0) && (diff.tv_nsec <= 0)) {
        //         break;
        //     }
        // }

    }

    static void sleep_for(const duration &dur) {
        if (nanosleep(&dur, nullptr) != 0) {
            throw std::runtime_error("nanosleep failed while sleeping for duration");
        }
    }

    static void busy_work(const duration &length) {
        timespec start_time;
        get_time(&start_time);

        timespec target_time = start_time + length;
        timespec curr_time;

        do {
            get_time(&curr_time);
        } while (curr_time < target_time);
    }

};

template <typename Clock, typename Duration>
struct TimeTraits<std::chrono::time_point<Clock, Duration>> {
    using duration = Duration;
    using timepoint = std::chrono::time_point<Clock, Duration>;

    static timepoint create_timepoint(int sec, int nsec) {
        return timepoint(Duration(std::chrono::seconds(sec) + std::chrono::nanoseconds(nsec)));
    }

    static duration create_duration(int sec, int nsec) {
        return Duration(std::chrono::seconds(sec) + std::chrono::nanoseconds(nsec));
    }

    static timepoint get_default_timepoint() {
        return timepoint(Duration(0));
    }

    static duration get_default_duration() {
        return Duration(0);
    }

    static void get_time(timepoint *tp) {
        if (!tp) {
            throw std::invalid_argument("Null pointer passed to get_time");
        }
        *tp = Clock::now();
    }

    static duration diff(const timepoint &tp1, const timepoint &tp2) {
        return tp1 > tp2 ? (tp1 - tp2) : (tp2 - tp1);
    }

    static void sleep_until(const timepoint &tp) {
        auto now = Clock::now();
        if (now >= tp) return;

        try {
            std::this_thread::sleep_until(tp);
        } catch (const std::system_error &e) {
            throw std::runtime_error("std::this_thread::sleep_until failed");
        }
    }

    static void sleep_for(const duration &dur) {
        try {
            std::this_thread::sleep_for(dur);
        } catch (const std::system_error &e) {
            throw std::runtime_error("std::this_thread::sleep_for failed");
        }
    }

    static void busy_work(const duration &length) {
        auto start_time = Clock::now();
        auto target_time = start_time + length;

        while (Clock::now() < target_time) {
            // Busy loop
        }
    }
};

// TimeTraits for std::chrono::duration
template <typename Rep, typename Period>
struct TimeTraits<std::chrono::duration<Rep, Period>> {
    using duration = std::chrono::duration<Rep, Period>;

    static duration create_duration(int sec, int nsec) {
        return duration(std::chrono::seconds(sec) + std::chrono::nanoseconds(nsec));
    }

    static void sleep_for(const duration &dur) {
        try {
            std::this_thread::sleep_for(dur);
        } catch (const std::system_error &e) {
            throw std::runtime_error("std::this_thread::sleep_for failed");
        }
    }

    static void busy_work(const duration &length) {
        auto start_time = CLOCK_T::now();
        auto target_time = start_time + length;

        while (CLOCK_T::now() < target_time) {
            // Busy loop
        }
    }
};

// template <>
// struct TimeTraits<std::chrono::nanoseconds> {
//     using duration = std::chrono::nanoseconds;
//     using timepoint = std::chrono::time_point<CLOCK_T, std::chrono::nanoseconds>;

//     static timepoint create_timepoint(int sec, int nsec) {
//         return timepoint(duration(sec * NANOSEC_IN_SEC + nsec));
//     }

//     static duration create_duration(int sec, int nsec) {
//         return duration(sec * NANOSEC_IN_SEC + nsec);
//     }

//     static timepoint get_default_timepoint() {
//         return timepoint(duration(0));
//     }

//     static duration get_default_duration() {
//         return duration(0);
//     }

//     static void get_time(timepoint *tp) {
//         if (!tp) {
//             throw std::invalid_argument("Null pointer passed to get_time");
//         }

//         *tp = CLOCK_T::now();
//     }

//     static duration diff(const timepoint &tp1, const timepoint &tp2) {
//         return tp1 > tp2 ? (tp1 - tp2) : (tp2 - tp1);
//     }

//     static void sleep_until(const timepoint &tp) {
//         auto now = CLOCK_T::now();
//         if (now >= tp) return;

//         try {
//             std::this_thread::sleep_until(tp);
//         } catch (const std::system_error &e) {
//             throw std::runtime_error("std::this_thread::sleep_until failed");
//         }
//     }

//     static void sleep_for(const duration &dur) {
//         try {
//             std::this_thread::sleep_for(dur);
//         } catch (const std::system_error &e) {
//             throw std::runtime_error("std::this_thread::sleep_for failed");
//         }
//     }

//     static void busy_work(const duration &length) {
//         auto start_time = CLOCK_T::now();
//         auto target_time = start_time + length;
        
//         while (CLOCK_T::now() < target_time) {
//             // Busy loop
//         }
//     }
// };

// Operator overloads for timespec
inline timespec operator+(const timespec &lhs, const timespec &rhs) {
    timespec result = {lhs.tv_sec + rhs.tv_sec, lhs.tv_nsec + rhs.tv_nsec};
    if (result.tv_nsec >= NANOSEC_IN_SEC) {
        result.tv_nsec -= NANOSEC_IN_SEC;
        result.tv_sec++;
    }
    return result;
}

inline timespec operator-(const timespec &lhs, const timespec &rhs) {
    timespec result = {lhs.tv_sec - rhs.tv_sec, lhs.tv_nsec - rhs.tv_nsec};
    if (result.tv_nsec < 0) {
        result.tv_nsec += NANOSEC_IN_SEC;
        result.tv_sec--;
    }
    return result;
}

inline bool operator>(const timespec &lhs, const timespec &rhs) {
    return (lhs.tv_sec > rhs.tv_sec) || (lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec > rhs.tv_nsec);
}

inline bool operator<(const timespec &lhs, const timespec &rhs) {
    return (lhs.tv_sec < rhs.tv_sec) || (lhs.tv_sec == rhs.tv_sec && lhs.tv_nsec < rhs.tv_nsec);
}

inline bool operator>=(const timespec &lhs, const timespec &rhs) {
    return !(lhs < rhs);
}

inline bool operator<=(const timespec &lhs, const timespec &rhs) {
    return !(lhs > rhs);
}

inline std::ostream &operator<<(std::ostream &stream, const timespec &ts) {
    stream << ts.tv_sec << ".";
    long nsec = ts.tv_nsec;
    for (unsigned i = 0; i < 9; ++i, nsec /= 10) {
        if (nsec == 0) {
            stream << "0";
        }
    }
    stream << ts.tv_nsec;
    return stream;
}

template <typename Clock, typename Duration>
std::ostream &operator<<(std::ostream &stream, const std::chrono::time_point<Clock, Duration> &tp) {
    auto time_since_epoch = tp.time_since_epoch();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time_since_epoch).count();
    auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(time_since_epoch).count() % NANOSEC_IN_SEC;

    stream << seconds << "." << std::setfill('0') << std::setw(9) << nanoseconds;
    return stream;
}

template <typename Rep, typename Period>
std::ostream &operator<<(std::ostream &stream, const std::chrono::duration<Rep, Period> &d) {
    auto total_nano = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    long seconds = total_nano / 1'000'000'000;
    long nanoseconds = total_nano % 1'000'000'000;

    stream << seconds << ".";
    long nsec = nanoseconds;
    for (unsigned i = 0; i < 9; ++i, nsec /= 10) {
        if (nsec == 0) {
            stream << "0";
        }
    }

    stream << nanoseconds;
    return stream;
}

// Cross-type get_time with pointer
template <typename T>
void get_time(T *tp) {
    TimeTraits<T>::get_time(tp);
}

// Cross-type sleep_until
template <typename T>
void sleep_until_ts(const T &tp) {
    TimeTraits<T>::sleep_until(tp);
}

// Cross-type sleep_for
template <typename T>
void sleep_for_ts(const typename TimeTraits<T>::duration &dur) {
    TimeTraits<T>::sleep_for(dur);
}

// Cross-type busy_work
template <typename T>
void busy_work(const typename TimeTraits<T>::duration &length) {
    TimeTraits<T>::busy_work(length);
}

template <typename T, typename R>
typename TimeTraits<T>::duration ts_diff(const T &tp1, const R &tp2) {
    if constexpr (std::is_same_v<T, R>) {
        return TimeTraits<T>::diff(tp1, tp2);
    } else if constexpr (std::is_same_v<T, std::chrono::nanoseconds> && std::is_same_v<R, timespec>) {
        auto tp1_ns = tp1.time_since_epoch().count();
        auto tp2_ns = R::tv_sec * NANOSEC_IN_SEC + R::tv_nsec;
        return std::chrono::nanoseconds(tp1_ns - tp2_ns);
    } else {
        throw std::invalid_argument("Unsupported types for ts_diff");
    }
}

template <typename Clock, typename Duration>
std::chrono::nanoseconds ts_diff(
    const std::chrono::time_point<Clock, Duration> &tp1,
    const std::chrono::time_point<Clock, Duration> &tp2) {
    return tp1 > tp2 ? (tp1 - tp2) : (tp2 - tp1);
}

#endif // TIME_FUNCTIONS_H
