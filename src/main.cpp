#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <cxxopts.hpp>

#include <problem.hpp>
#include <solver/differential_evolution.hpp>


int main(int argc, char** argv) {

    // Parse command line arguments using cxxopts
    cxxopts::Options options("mpp", "Solve the maintenance planning problem.");
    options.add_options()
        ("instance", "Path to the instance file", cxxopts::value<std::string>())
        ("output", "Path to the output solution file", cxxopts::value<std::string>())
        ("pool_size", "Number of solutions in the pool", cxxopts::value<int>()->default_value("30"))
        ("best_ratio", "Proportion of using DE/best/1 mutation strategy", cxxopts::value<double>()->default_value("0.5"))
        ("scaling_factor", "Scaling factor for mutation", cxxopts::value<double>()->default_value("0.50"))
        ("crossover_rho", "Rho parameter for crossover recombination", cxxopts::value<double>()->default_value("0.30"))
        ("hot_start", "Enable hot start from relaxed MIP solution", cxxopts::value<bool>()->default_value("false"))
        ("parallel", "Enable parallel processing", cxxopts::value<bool>()->default_value("false"))
        ("seed", "Random seed for generating a random solution", cxxopts::value<unsigned int>()->default_value("0"))
        ("max_iterations", "Maximum number of iterations", cxxopts::value<long long int>()->default_value("-1"))
        ("max_time", "Maximum time in seconds", cxxopts::value<double>()->default_value("-1"))
        ("v,verbose", "Enable verbose output", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Show help message");

        options.parse_positional({"instance", "output"});
        options.positional_help("INSTANCE_FILE OUTPUT_FILE");

    try {

        // Parse the command line arguments and show help if needed
        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        // Load instance data
        std::string instance_file = result["instance"].as<std::string>();
        mpp::problem_t problem(instance_file);

        // Load DE settings from command line arguments
        mpp::solver::differential_evolution_settings_t settings;
        settings.pool_size = result["pool_size"].as<int>();
        settings.best_ratio = result["best_ratio"].as<double>();
        settings.scaling_factor = result["scaling_factor"].as<double>();
        settings.crossover_rho = result["crossover_rho"].as<double>();
        settings.enable_hot_start = result["hot_start"].as<bool>();
        settings.enable_parallel = result["parallel"].as<bool>();
        settings.seed = result["seed"].as<unsigned int>();
        settings.max_iterations = result["max_iterations"].as<long long int>();
        settings.max_time = result["max_time"].as<double>();
        settings.verbose = result["verbose"].as<bool>();

        // Set max_iterations and max_time properly
        if (settings.max_iterations < 0) settings.max_iterations = std::numeric_limits<long long int>::max();
        if (settings.max_time < 0) settings.max_time = std::numeric_limits<double>::max();

        // Solve the problem using differential evolution
        auto [solution, objective, risk_metrics, constraints] = mpp::solver::differential_evolution(problem, settings);
        auto [constr_exclusions_count, constr_resource_count, constr_resource_sum] = constraints;

        // Print the results, if verbose is enabled
        if (settings.verbose) {
            std::cout << "Objective: " << objective << std::endl;
            std::cout << "Exclusions (violations): " << constr_exclusions_count << std::endl;
            std::cout << "Resource count (violations): " << constr_resource_count << std::endl;
            std::cout << "Resource sum (violations): " << constr_resource_sum << std::endl;
        }

        // Export solution to a file
        std::string solution_file = result["output"].as<std::string>();
        std::ofstream solution_out(solution_file);
        if (!solution_out.is_open()) {
            std::cerr << "Error opening solution file for writing." << std::endl;
            return EXIT_FAILURE;
        }
        for (const auto& [intervention_name, start_time] : solution) {
            solution_out << intervention_name << " " << start_time << std::endl;
        }
        solution_out.close();

    } catch (const cxxopts::exceptions::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}
