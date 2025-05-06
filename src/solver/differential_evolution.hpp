#ifndef INCLUDE_MPP_SOLVER_DIFFERENTIAL_EVOLUTION_HPP_
#define INCLUDE_MPP_SOLVER_DIFFERENTIAL_EVOLUTION_HPP_

#include <tuple>
#include <algorithm>
#include <problem.hpp>


namespace mpp {
    namespace solver{

        /**
         * @brief Settings for the Differential Evolution (DE) solver.
         * @details This struct contains the parameters for the DE algorithm.
         * @param pool_size Number of solutions in the pool.
         * @param best1_ratio Probability of choosing DE/best/1 mutation strategy instead of DE/rand/1.
         * @param scaling_factor Scaling factor for mutation.
         * @param crossover_rho Rho parameter for crossover recombination.
         * @param timelimit Limits the runtime in seconds (default is 900 seconds, use -1 for no limit).
         * @param mip_timelimit Limits the runtime of the MIP solver in seconds (-1 for no limit).
         * @param threads Number of threads for parallel processing.
         * @param seed Random seed for generating a random solution.
         * @param verbose Enable verbose output.
         */
        struct differential_evolution_settings_t {
            size_t pool_size = 36;
            double best1_ratio = 0.37;
            double scaling_factor = 0.16;
            double crossover_rho = 0.3;
            long long int timelimit = 900;
            long long int mip_timelimit = -1;
            int threads = 2;
            unsigned int seed = 0;
            bool verbose = true;
        };


        /**
         * @brief DE solver for the maintenance planning problem.
         * @details This function implements the Differential Evolution algorithm to solve the maintenance planning problem.
         * @param problem The maintenance planning problem instance.
         * @param settings The DE settings (optional).
         * @return A tuple containing the best solution, objective value, risk metric, and constraints.
         */
        std::tuple<solution_t, objective_t, risk_metric_t, constraints_t>
        differential_evolution(const problem_t& problem, 
            const differential_evolution_settings_t& settings = differential_evolution_settings_t());


    } // namespace solver
} // namespace mpp


#endif // INCLUDE_MPP_SOLVER_DIFFERENTIAL_EVOLUTION_HPP_
