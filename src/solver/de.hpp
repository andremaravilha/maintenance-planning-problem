#ifndef INCLUDE_MPP_SOLVER_DE_HPP_
#define INCLUDE_MPP_SOLVER_DE_HPP_

#include <tuple>
#include <algorithm>
#include <problem.hpp>


namespace mpp {
    namespace solver{

        std::tuple<solution_t, objective_t, constraints_t>
        de(const problem_t& problem);

        inline int bounded_round_(double value, int lb, int ub);

    }
}


int mpp::solver::bounded_round_(double value, int lb, int ub) {
    return std::max(lb, std::min(ub, static_cast<int>(value + 0.5)));
}


#endif // INCLUDE_MPP_SOLVER_DE_HPP_
