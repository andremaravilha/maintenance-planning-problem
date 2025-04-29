#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <gurobi_c++.h>
#include <problem.hpp>
#include <solver/relaxed_mip.hpp>
#include <solver/de.hpp>
#include <chrono>
#include <random>


int main(int argc, char** argv) {

    // Load instance data
    std::string instance_file = R"(C:\Users\andre\Workspace\Projects\maintenance-planning-problem\roadef-challenge-2020\instances\A_set\A_04.json)";
    mpp::problem_t problem(instance_file);
    const auto& intervention_names = problem.get_intervention_names();
    const auto& data = problem.get_data();

    // // Generate a random solution
    // unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    // std::mt19937 generator(seed);

    // mpp::solution_t solution;
    // for (const auto& intervention_name : intervention_names) {
    //     const auto& intervention_data = data[mpp::params::INTERVENTIONS][intervention_name];
    //     int t_max = intervention_data[mpp::params::INTERVENTION_TMAX].template get<int>();
    //     int start_time = (generator() % t_max) + 1;
    //     solution[intervention_name] = start_time;
    // }

    // // Evaluate the solution
    // auto [objective, constraints] = problem.evaluate(solution);
    // auto [const_exclusions, constr_resource_count, constr_resource_sum] = constraints;

    //auto [solution, objective, constraints] = mpp::solver::relaxed_mip(problem);
    auto [solution, objective, constraints] = mpp::solver::de(problem);
    auto [const_exclusions, constr_resource_count, constr_resource_sum] = constraints;

    std::cout << "Objective: " << objective << std::endl;
    std::cout << "Exclusions (violations): " << const_exclusions << std::endl;
    std::cout << "Resource count (violations): " << constr_resource_count << std::endl;
    std::cout << "Resource sum (violations): " << constr_resource_sum << std::endl;

    // Export solution to a file
    std::string solution_file = R"(C:\Users\andre\Workspace\Projects\maintenance-planning-problem\roadef-challenge-2020\instances\solution.txt)";
    std::ofstream solution_out(solution_file);
    if (!solution_out.is_open()) {
        std::cerr << "Error opening solution file for writing." << std::endl;
        return EXIT_FAILURE;
    }
    for (const auto& [intervention_name, start_time] : solution) {
        solution_out << intervention_name << " " << start_time << std::endl;
    }
    solution_out.close();


    return EXIT_SUCCESS;
}
