#include <solver/de.hpp>
#include <solver/relaxed_mip.hpp>
#include <tuple>
#include <vector>
#include <limits>
#include <cmath>
#include <random>
#include <iostream>


std::tuple<mpp::solution_t, mpp::objective_t, mpp::constraints_t>
mpp::solver::de(const mpp::problem_t& problem) {

    // Problem data
    const auto& data = problem.get_data();
    const auto& interventions = problem.get_intervention_names();
    const size_t n_var = interventions.size();

    std::vector<int> lb(interventions.size(), 1);
    std::vector<int> ub(interventions.size(), 1);
    for (int i = 0; i < interventions.size(); ++i) {
        const auto& intervention_data = data[mpp::params::INTERVENTIONS][interventions[i]];
        ub[i] = intervention_data[mpp::params::INTERVENTION_TMAX].template get<int>();
    }

    // Some constants and types
    using solution_t = std::vector<int>;
    using fitness_t = std::tuple<mpp::constraints_t, mpp::objective_t>;
    constexpr double INF = std::numeric_limits<double>::max();

    // DE parameters
    const int pool_size = 30; // Number of solutions in the pool
    const double scaling_factor = 0.5; // Scaling factor for mutation
    const double crossover_rho = 0.3; // Crossover parameter

    std::vector<double> crossover_weights(n_var, 0.0);
    for (int i = 1; i <= n_var; ++i) {
        crossover_weights[i-1] = std::pow(crossover_rho, i - 1) - std::pow(crossover_rho, i);
    }
    std::discrete_distribution<int> crossorver_dist(crossover_weights.begin(), crossover_weights.end());

    // Stopping criteria
    const long long int max_iterations = 1000; // Maximum number of iterations
    const double max_time = 60.0; // Maximum time in seconds

    // Random number generator (Mersenne Twister 19937 generator)
    const unsigned int seed = 42;
    std::mt19937 rng(seed);

    // Initialize the pool of solutions
    std::vector< solution_t > pool_solutions;
    std::vector< fitness_t > pool_fitness;

    auto [start_solution, start_objective, start_constraints] = mpp::solver::relaxed_mip(problem);
    pool_fitness.emplace_back(start_constraints, start_objective);
    for (int j = 0; j < n_var; ++j) {
        pool_solutions.emplace_back(n_var);
        pool_solutions[0][j] = start_solution[interventions[j]];
    }

    // Best solution found
    int idx_best = 0;
    int idx_worst = 0;

    for (int i = 1; i < pool_size; ++i) {
        pool_solutions.emplace_back(n_var);
        for (int j = 0; j < n_var; ++j) {
            pool_solutions[i][j] = rng() % (ub[j] - lb[j] + 1) + lb[j];
        }

        auto [objective, constraints] = problem.evaluate(pool_solutions[i], interventions);
        pool_fitness.emplace_back(constraints, objective);

        if (pool_fitness[i] < pool_fitness[idx_best]) {
            idx_best = i;
        }
        
        if (pool_fitness[i] > pool_fitness[idx_worst]) {
            idx_worst = i;
        }
    }

    // Main loop
    long long int current_iteration = 0;
    while (current_iteration < max_iterations) {

        for (int i = 0; i < pool_size; ++i) {

            // Select three random solutions
            int idx1, idx2, idx3;
            idx1 = rng() % pool_size;
            do { idx2 = rng() % pool_size; } while (idx2 == idx1);
            do { idx3 = rng() % pool_size; } while (idx3 == idx1 || idx3 == idx2);

            // Set idx1 to the best solution
            idx1 = idx_best;

            // Create a mutant vector
            solution_t mutant(n_var);
            for (int j = 0; j < n_var; ++j) {
                double value = pool_solutions[idx1][j] + scaling_factor * (pool_solutions[idx2][j] - pool_solutions[idx3][j]);
                mutant[j] = bounded_round_(value, lb[j], ub[j]);
            }

            // Create a trial vector using exponential crossover
            int d = crossorver_dist(rng) + 1;
            int k1 = rng() % n_var;
            int k2 = k1 + d;

            solution_t trial(n_var);
            for (int j = 0; j < n_var; ++j) {
                if (k2 < n_var && j >= k1 && j <= k2) {
                    trial[j] = mutant[j];
                } else if (k2 >= n_var && (j >= k1 || j <= (k2 % n_var))) {
                    trial[j] = mutant[j];
                } else {
                    trial[j] = pool_solutions[i][j];
                }
            }

            // Evaluate the trial vector
            auto [objective, constraints] = problem.evaluate(trial, interventions);
            fitness_t trial_fitness(constraints, objective);

            if (trial_fitness < pool_fitness[i]) {
                pool_solutions[i] = trial;
                pool_fitness[i] = trial_fitness;
            }

            // Update the best solution
            if (trial_fitness < pool_fitness[idx_best]) {
                idx_best = i;
            }

            // Update the worst solution
            if (trial_fitness > pool_fitness[idx_worst]) {
                idx_worst = i;
            }

            //std::cout << "Iteration " << current_iteration << ": " << "Worst is " << idx_worst << " | Best is " << idx_best << std::endl;

        }

        // Print best solution (objective and constraints)
        //if (current_iteration % 100 == 0) {
            std::cout << "Iteration: " << current_iteration << ", Best Objective: " << std::get<1>(pool_fitness[idx_best]) << ", Constraints: [ " << std::get<0>(std::get<0>(pool_fitness[idx_best])) << ", " << std::get<1>(std::get<0>(pool_fitness[idx_best])) << ", " << std::get<2>(std::get<0>(pool_fitness[idx_best]))  << " ]" << std::endl;
        //}

        // Increment the iteration counter
        ++current_iteration;

    }

    // Decode the best solution
    auto [best_constraints, best_objective] = pool_fitness[idx_best];
    mpp::solution_t best_solution;
    for (int j = 0; j < n_var; ++j) {
        best_solution[interventions[j]] = pool_solutions[idx_best][j];
    }

    // Return the best solution and its objective value
    return {best_solution, best_objective, best_constraints};
}