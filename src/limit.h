#pragma once
#include <sys/time.h>

namespace ZeroLogic {

    enum LimitType{
        DEPTH = 0, MOVETIME = 1, GAMETIME = 2, NODES = 3,
        PERFT = 4, SINGLE = 5,
        INVALID_LIMIT = 10
    };

    struct Time {
        timeval t;

        static Time now(){
            timeval n{};
            gettimeofday(&n, nullptr);
            return Time{n};
        }
    };
    inline u64 operator-(Time t1, Time t2) {
        u64 sec = t1.t.tv_sec - t2.t.tv_sec;
        u64 msec = t1.t.tv_usec - t2.t.tv_usec;
        return sec * 1000000 + msec;
    }

    class Limit{
    public:
        LimitType type;

        Time start_time;
        u64 allowed_time;
        u64 allowed_nodes;
        u8 allowed_depth;

        u64 otime;
        u64 etime;
        u64 oinc;
        u64 einc;
        u64 togo;

        u64 nodecount;

        void set_time(bool us, u64 t)
        {
            type = GAMETIME;
            if (us) otime = t;
            else    etime = t;
        }
        void set_inc(bool us, u64 i)
        {
            type = GAMETIME;
            if (us) oinc = i;
            else    einc = i;
        }

        inline void adjust()
        {
            allowed_time = otime / togo + oinc;
            type = MOVETIME;
        }

        inline bool stop() const
        {
            if (type == MOVETIME)   return (Time::now() - start_time) / 1000 >= allowed_time;
            if (type == NODES)      return nodecount >= allowed_nodes;
            return false;
        }
    };

    Limit limits;

}