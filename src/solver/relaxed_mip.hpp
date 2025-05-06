#ifndef INCLUDE_MPP_SOLVER_RELAXED_MIP_HPP_
#define INCLUDE_MPP_SOLVER_RELAXED_MIP_HPP_

#include <tuple>
#include <problem.hpp>


namespace mpp {
    namespace solver{

        std::tuple<solution_t, objective_t, risk_metric_t, constraints_t>
        relaxed_mip(const problem_t& problem, long long int timelimit=-1, int threads=1, bool verbose=false);

    }
}


#endif // INCLUDE_MPP_SOLVER_RELAXED_MIP_HPP_
