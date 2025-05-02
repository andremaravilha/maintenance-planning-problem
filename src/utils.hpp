#ifndef INCLUDE_MPP_UTILS_HPP_
#define INCLUDE_MPP_UTILS_HPP_

#include <cmath>
#include <algorithm>


namespace mpp {
    namespace utils {

        inline int bounded_round(double value, int lb, int ub) {
            return std::max(lb, std::min(ub, static_cast<int>(value + 0.5)));
        }

        inline int compare(double a, double b, double eps = 1E-6) {
            if (std::fabs(a - b) < eps) return 0;
            return (a < b) ? -1 : 1;
        }

    } // namespace utils
} // namespace mpp


#endif //INCLUDE_MPP_UTILS_HPP_
