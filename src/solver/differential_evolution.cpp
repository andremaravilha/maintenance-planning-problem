#include <solver/differential_evolution.hpp>
#include <solver/relaxed_mip.hpp>
#include <utils.hpp>
#include <tuple>
#include <vector>
#include <limits>
#include <cmath>
#include <random>
#include <utility>
#include <iostream>
#include <algorithm>
#include <execution>
#include <mutex>
#include <cxxtimer.hpp>


std::tuple<mpp::solution_t, mpp::objective_t, mpp::risk_metric_t, mpp::constraints_t>
mpp::solver::differential_evolution(const mpp::problem_t& problem, const differential_evolution_settings_t& settings) {

    // Get settings from the input parameters
    const size_t pool_size = settings.pool_size;                // Number of solutions in the pool
    const double best1_ratio = settings.best1_ratio;            // Proportion of using DE/best/1 mutation strategy
    const double scaling_factor = settings.scaling_factor;      // Scaling factor for mutation
    const double crossover_rho = settings.crossover_rho;        // Rho parameter for crossover recombination
    const long long int timelimit = settings.timelimit;         // Limits the runtime in seconds
    const long long int mip_timelimit = settings.mip_timelimit; // Limits the runtime of the MIP solver in seconds
    const int threads = settings.threads;                       // Number of threads for parallel processing
    const unsigned int seed = settings.seed;                    // Random seed for generating a random solution
    const bool verbose = settings.verbose;                      // Enable verbose output

    // Start the timer
    cxxtimer::Timer timer(true);

    // Initialize the Random number generator (Mersenne Twister 19937 generator)
    std::mt19937 rng(seed);

    // Problem data
    const auto& data = problem.get_data();
    const auto& interventions = problem.get_intervention_names();
    const size_t n_var = interventions.size();
    std::vector<int> lb(interventions.size(), 1);
    std::vector<int> ub(interventions.size(), 1);

    for (size_t i = 0; i < interventions.size(); ++i) {
        const auto& intervention_data = data[mpp::params::INTERVENTIONS][interventions[i]];
        ub[i] = intervention_data[mpp::params::INTERVENTION_TMAX].template get<int>();
    }

    // Define some types for better readability 
    using solution_t = std::vector<int>;
    using solution_evaluation_t = std::tuple<mpp::objective_t, mpp::risk_metric_t, mpp::constraints_t>;
    constexpr double INF = std::numeric_limits<double>::max();

    // Compute the fitness from solution evaluation
    // The fitness is a tuple of (exclusions + resource_count, resource_sum, objective)
    auto make_fitness = [](const solution_evaluation_t& eval) {
        const auto& [objective, risk_metric, constraints] = eval;
        const auto& [exclusions, resource_count, resource_sum] = constraints;
        return std::make_tuple(exclusions + resource_count, resource_sum, objective);
    };

    // Define the fitness type for better readability
    using fitness_t = std::invoke_result_t<decltype(make_fitness), solution_evaluation_t>;
    
    // Create the probability distribution for exponential crossover operator
    std::vector<double> crossover_weights(n_var);       
    for (size_t i = 1; i <= n_var; ++i) { 
        crossover_weights[i-1] = std::pow(crossover_rho, i-1) - std::pow(crossover_rho, i);
    }
    std::discrete_distribution<int> crossorver_dist(crossover_weights.begin(), crossover_weights.end());

    // Auxiliary vector of indices for parallel processing
    std::vector<int> indices(pool_size);
    std::iota(indices.begin(), indices.end(), 0); // Fill with 0, 1, ..., pool_size-1

    // Pool of solutions
    std::vector<solution_t> pool_solutions; // Pool of solutions
    std::vector<fitness_t> pool_fitness;    // Fitness values of solutions
    size_t idx_best = 0;                    // Index of the best solution
    size_t idx_worst = 0;                   // Index of the worst solution

    // Reserve space in memory for better performance
    pool_solutions.reserve(pool_size);
    pool_fitness.reserve(pool_size);

    // Generate random solutions and evaluate them
    for (size_t i = 0; i < pool_size; ++i) {

        pool_solutions.emplace_back(n_var);
        for (size_t j = 0; j < n_var; ++j) {
            pool_solutions[i][j] = rng() % (ub[j] - lb[j] + 1) + lb[j];
        }

        pool_fitness.emplace_back(make_fitness(problem.evaluate(pool_solutions[i], interventions)));

        // Track the best and worst solutions
        if (pool_fitness[i] < pool_fitness[idx_best]) idx_best = i;
        if (pool_fitness[i] > pool_fitness[idx_worst]) idx_worst = i;
    }

    // Replace the worst solution with a solution from the Relaxed MIP
    if (verbose) std::cout << "Solving the Relaxed MIP..." << std::endl;
    try {

        // Solve the MIP model
        auto [hot_solution, hot_objective, hot_risk, hot_constraints] = mpp::solver::relaxed_mip(problem, mip_timelimit, threads, verbose);
        pool_fitness[idx_worst] = make_fitness(std::make_tuple(hot_objective, hot_risk, hot_constraints));
        for (size_t j = 0; j < n_var; ++j) {
            pool_solutions[idx_worst][j] = hot_solution[interventions[j]];
        }

        // Update the best solution if necessary
        if (pool_fitness[idx_worst] < pool_fitness[idx_best]) {
            idx_best = idx_worst;
        }

        if (verbose) std::cout << "Done!"<< std::endl;
    } catch (...) {
        if (verbose) {
            std::cout << "Failed to find a solution using the Relaxed MIP." << std::endl;
            std::cout << "Continuing with the current pool of solutions." << std::endl;
        }
    }

    // Pool of offspring solutions generated from the main pool of solutions
    std::vector<solution_t> offspring_solutions(pool_solutions);
    std::vector<fitness_t> offspring_fitness(pool_fitness);

    // Main loop
    long long int current_iteration = 0;
    while (timer.count<cxxtimer::s>() < timelimit) {

        // Track the best solution in the offspring pool
        size_t idx_best_offspring = 0;  // Index of the best offspring solution
        std::mutex mtx_best_offspring;  // Mutex for thread-safe access when updating the best offspring solution

        // Lambda function to generate offspring solutions
        auto generate_offspring_solution = [&](const int i) {

            // Mutation parameters
            size_t idx1, idx2, idx3;
            idx1 = (rng() / static_cast<double>(rng.max()) < best1_ratio) ? idx_best : rng() % pool_size;
            do { idx2 = rng() % pool_size; } while (idx2 == i || idx2 == idx1);
            do { idx3 = rng() % pool_size; } while (idx3 == i || idx3 == idx1 || idx3 == idx2);

            // Crossover parameters
            size_t k1 = rng() % n_var;
            size_t k2 = k1 + crossorver_dist(rng) + 1;

            // Create a trial vector using mutation and crossover
            for (size_t j = 0; j < n_var; ++j) {
                if ((k2 < n_var && j >= k1 && j <= k2) || (k2 >= n_var && (j >= k1 || j <= (k2 % n_var)))) {
                    offspring_solutions[i][j] = mpp::utils::bounded_round(pool_solutions[idx1][j] + scaling_factor * (pool_solutions[idx2][j] - pool_solutions[idx3][j]), lb[j], ub[j]);
                } else {
                    offspring_solutions[i][j] = pool_solutions[i][j];
                }
            }

            // Evaluate the trial vector and update the offspring pool
            fitness_t trial_fitness(make_fitness(problem.evaluate(offspring_solutions[i], interventions)));

            if (trial_fitness < pool_fitness[i]) {
                offspring_fitness[i] = trial_fitness;
            } else {
                offspring_fitness[i] = pool_fitness[i];
                offspring_solutions[i] = pool_solutions[i];
            }

            // Critical section: Update the best solution in the offspring pool
            std::lock_guard<std::mutex> lock(mtx_best_offspring);
            if (offspring_fitness[i] < offspring_fitness[idx_best_offspring]) {
                idx_best_offspring = i;
            }

            // Unlock the mutex (optional, as it will be unlocked when going out of scope)
            //lock.unlock(); 

        };

        // Create offspring solutions (in parallel, if enabled)
        if (threads > 1) {
            std::for_each(std::execution::par, indices.begin(), indices.end(), generate_offspring_solution);
        } else {
            std::for_each(std::execution::seq, indices.begin(), indices.end(), generate_offspring_solution);
        }

        // Update the main pool of solutions and fitness values
        // Swap the offspring pool with the main pool for better performance
        // This avoids copying the entire pool of solutions and fitness values
        std::swap(pool_solutions, offspring_solutions);
        std::swap(pool_fitness, offspring_fitness);
        idx_best = idx_best_offspring;

        // Increment the iteration counter
        ++current_iteration;

        // Logging, if enabled
        if (verbose) {
            const auto& [violated_constraints, exceeded_resources, objective] = pool_fitness[idx_best];
            std::cout << std::fixed << std::setprecision(7) << current_iteration << " | "
                      << std::fixed << std::setprecision(5) << timer.count<cxxtimer::s>() << " | "
                      << violated_constraints << " | "
                      << exceeded_resources << " | "
                      << objective << std::endl;
        }
    }

    // Decode the best solution found
    mpp::solution_t best_solution;
    for (size_t j = 0; j < n_var; ++j) {
        best_solution[interventions[j]] = pool_solutions[idx_best][j];
    }

    // Evaluate the best solution and return it
    auto [best_objective, best_risk_metric, best_constraints] = problem.evaluate(best_solution);
    return { best_solution, best_objective, best_risk_metric, best_constraints };
}