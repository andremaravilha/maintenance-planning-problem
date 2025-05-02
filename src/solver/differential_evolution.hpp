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
         * @param best_ratio Proportion of using DE/best/1 mutation strategy. The remaining proportion is DE/rand/1.
         * @param scaling_factor Scaling factor for mutation.
         * @param crossover_rho Rho parameter for crossover recombination.constraints_t
         * @param enable_hot_start Enable hot start from relaxed MIP solution.
         * @param enable_parallel Enable parallel processing.
         * @param seed Random seed for generating a random solution.
         * @param max_iterations Maximum number of iterations for the DE algorithm.
         * @param max_time Maximum time for the DE algorithm in seconds.
         * @param verbose Enable verbose output.
         */
        struct differential_evolution_settings_t {
            int pool_size = 30;
            double best_ratio = 0.5;
            double scaling_factor = 0.5;
            double crossover_rho = 0.3;
            bool enable_hot_start = true;
            bool enable_parallel = true;
            unsigned int seed = 0;
            long long int max_iterations = 1000;
            double max_time = 60.0;
            bool verbose = false;
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
