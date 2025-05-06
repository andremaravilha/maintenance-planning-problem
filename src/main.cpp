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
        ("instance", "Path to the instance file.", cxxopts::value<std::string>())
        ("output", "Path to the output solution file.", cxxopts::value<std::string>())
        ("pool_size", "Number of solutions in the pool.", cxxopts::value<int>()->default_value("36"))
        ("best1_ratio", "Probability of choosing DE/best/1 mutation strategy instead of DE/rand/1.", cxxopts::value<double>()->default_value("0.37"))
        ("scaling_factor", "Scaling factor for mutation.", cxxopts::value<double>()->default_value("0.16"))
        ("crossover_rho", "Rho parameter for crossover recombination.", cxxopts::value<double>()->default_value("0.30"))
        ("timelimit", "Limits the runtime in seconds. Use -1 for no limit.", cxxopts::value<long long int>()->default_value("900"))
        ("mip_timelimit", "Limits the runtime of the MIP solver in seconds. Use -1 for no limit.", cxxopts::value<long long int>()->default_value("-1"))
        ("threads", "Number of threads for parallel processing.", cxxopts::value<int>()->default_value("2"))
        ("seed", "Random seed for generating a random solution.", cxxopts::value<unsigned int>()->default_value("0"))
        ("v,verbose", "Enable verbose output.", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Show help message.");

        options.parse_positional({"instance", "output"});
        options.positional_help("<INSTANCE> <OUTPUT_FILE>");

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
        settings.best1_ratio = result["best1_ratio"].as<double>();
        settings.scaling_factor = result["scaling_factor"].as<double>();
        settings.crossover_rho = result["crossover_rho"].as<double>();
        settings.timelimit = result["timelimit"].as<long long int>();
        settings.mip_timelimit = result["mip_timelimit"].as<long long int>();
        settings.threads = result["threads"].as<int>();
        settings.seed = result["seed"].as<unsigned int>();
        settings.verbose = result["verbose"].as<bool>();

        // Set timelimit properly
        if (settings.timelimit < 0) settings.timelimit = std::numeric_limits<long long int>::max();

        // Solve the problem
        auto [solution, objective, risk_metrics, constraints] = mpp::solver::differential_evolution(problem, settings);
        auto [risk_mean, risk_excess] = risk_metrics;
        auto [constr_exclusions_count, constr_resource_count, constr_resource_sum] = constraints;

        // Print the results, if verbose is enabled
        if (settings.verbose) {
            std::cout << "Objective Function: " << objective << std::endl;
            std::cout << "Mean Risk: " << risk_mean << std::endl;
            std::cout << "Excess Risk: " << risk_excess << std::endl;
            std::cout << "Num. Exclusion Constraints Violations: " << constr_exclusions_count << std::endl;
            std::cout << "Num. Resource Constraints Violations: " << constr_resource_count << std::endl;
            std::cout << "Sum (excess and defict) Resource Violations: " << constr_resource_sum << std::endl;
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
