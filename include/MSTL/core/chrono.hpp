#ifndef MSTL_CHRONO_HPP__
#define MSTL_CHRONO_HPP__
#include "ratio.hpp"
MSTL_BEGIN_NAMESPACE__

namespace chrono {
    template <typename Rep, typename Period = ratio<1>>
    struct duration;

    template <typename Clock, typename Dur = typename Clock::duration>
    struct time_point;
}



MSTL_END_NAMESPACE__
#endif // MSTL_CHRONO_HPP__
