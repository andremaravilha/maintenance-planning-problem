#include <problem.hpp>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <cassert>


mpp::problem_t::problem_t(const std::string& filename) : data_(json::parse(std::ifstream(filename))) {
    for (auto& [intervention_name, intervention_data] : data_[params::INTERVENTIONS].items()) {

        // Get intervention names
        intervention_names_.push_back(intervention_name);

        // Fix intervention start time limit to a proper numerical type.
        if (!intervention_data[params::INTERVENTION_TMAX].is_number()) {
            intervention_data[params::INTERVENTION_TMAX] = std::stoi(intervention_data[params::INTERVENTION_TMAX].template get<std::string>());
        }

        // Fix delta array values from string to a proper numerical type.
        for (int i = 0; i < intervention_data[params::INTERVENTION_DELTA].size(); ++i) {
            if (!intervention_data[params::INTERVENTION_DELTA][0].is_number()) {
                intervention_data[params::INTERVENTION_DELTA][0] = std::stoi(intervention_data[params::INTERVENTION_DELTA][0].template get<std::string>());
            }
        }
    }

    // Fix seasons data to a proper numerical type.
    for (auto& [season_name, season_data] : data_[params::SEASONS].items()) {
        for (int i = 0; i < season_data.size(); ++i) {
            if (!season_data[i].is_number()) {
                season_data[i] = std::stoi(season_data[i].template get<std::string>());
            }
        }
    }
}


mpp::problem_t::~problem_t() {
    // Does nothing here.
}


std::tuple<mpp::objective_t, mpp::constraints_t>
mpp::problem_t::evaluate(const mpp::solution_t& solution) const {

    // Get some data from the problem
    constexpr double tolerance = 1e-5;
    int t_max = data_[params::T].template get<int>();
    double quantil = data_[params::QUANTILE].template get<double>();
    const json& interventions = data_[params::INTERVENTIONS];
    const json& resources = data_[params::RESOURCES];
    const json& scenarios_number = data_[params::SCENARIOS_NUMBER];
    const json& exclusions = data_[params::EXCLUSIONS];
    const json& seasons = data_[params::SEASONS];

    // Asserts that all interventions must have a valid start time
    for (const auto& intervention_name : intervention_names_) {

        // Check if the intervention is in the solution
        assert(solution.find(intervention_name) != solution.end());

        // Check if the start time is valid
        int start_time = solution.at(intervention_name);
        int start_time_max = interventions[intervention_name][params::INTERVENTION_TMAX].template get<int>();
        assert(start_time >= 1 && start_time <= start_time_max);
    }
    
    // Some temporary structures to evaluate the solution
    std::vector<double> mean_risk_by_period(t_max, 0.0);

    std::vector< std::vector<double> > risk;
    risk.reserve(t_max);
    for (int t = 0; t < t_max; ++t) {
        risk.emplace_back(scenarios_number[t].template get<int>(), 0.0);
    }

    std::map< std::string, std::vector<double> > resource_usage;
    for (const auto& [resource_name, resource_data] : resources.items()) {
        resource_usage.emplace(resource_name, std::vector<double>(t_max, 0.0));
    }

    // Iterate over each intervention to calculate the risk and resource usage
    for (const auto& [intervention_name, intervention_data] : interventions.items()) {
        const json& intervention_risk = intervention_data[params::INTERVENTION_RISK];
        int start_time = solution.at(intervention_name);
        int delta = intervention_data[params::INTERVENTION_DELTA][start_time - 1].template get<int>();
        
        // Risk associated with the intervention
        for (int t = start_time - 1; t < start_time + delta - 1; ++t) {
            for (const auto& [i, additional_risk] : intervention_risk[std::to_string(t + 1)][std::to_string(start_time)].items()) {
                risk[t][std::stoi(i)] += additional_risk.template get<double>();
                mean_risk_by_period[t] += additional_risk.template get<double>();
            }
        }

        // Resource usage associated with the intervention
        for (const auto& [resource_name, intervention_workload] : intervention_data[params::INTERVENTION_RESOURCE_WORKLOAD].items()) {
            for (int t = start_time - 1; t < start_time + delta - 1; ++t) {

                // Check if the resource is used by at time period t when intervention starts at start_time
                const auto& workload_at_period = intervention_workload.find(std::to_string(t + 1));
                if (workload_at_period != intervention_workload.end()) {
                    const auto& workload = workload_at_period->find(std::to_string(start_time));
                    if (workload != workload_at_period->end()) {
                        resource_usage[resource_name][t] += workload->template get<double>();
                    }
                }
            }
        }

    }

    // Check resources usage constraints
    double resource_count_violation = 0.0;
    double resource_sum_violation = 0.0;
    for (const auto& [resource_name, resource_data] : resources.items()) {
        const auto& lower_bound = resource_data[params::RESOURCE_LOWER_BOUND];
        const auto& upper_bound = resource_data[params::RESOURCE_UPPER_BOUND];
        for (int t = 0; t < t_max; ++t) {

            // Check upper bound
            if (resource_usage[resource_name][t] > upper_bound[t].template get<double>() + tolerance) {
                resource_sum_violation += resource_usage[resource_name][t] - upper_bound[t].template get<double>();
                resource_count_violation += 1.0;
            }

            // Check lower bound
            if (resource_usage[resource_name][t] < lower_bound[t].template get<double>() - tolerance) {
                resource_sum_violation += lower_bound[t].template get<double>() - resource_usage[resource_name][t];
                resource_count_violation += 1.0;
            }

        }
    }

    // Check exclusions constraints
    double exclusions_violation = 0.0;
    for (const auto& [exclusion_name, exclusion_data] : exclusions.items()) {
        const auto& intervention_1_name = exclusion_data[0].template get<std::string>();
        const auto& intervention_2_name = exclusion_data[1].template get<std::string>();
        const auto& season = seasons[exclusion_data[2].template get<std::string>()];
        const auto& intervention_1 = interventions[intervention_1_name];
        const auto& intervention_2 = interventions[intervention_2_name];

        // Find the intersection of the two interventions
        int start_time_1 = solution.at(intervention_1_name);
        int start_time_2 = solution.at(intervention_2_name);
        int delta_1 = intervention_1[params::INTERVENTION_DELTA][start_time_1 - 1].template get<int>();
        int delta_2 = intervention_2[params::INTERVENTION_DELTA][start_time_2 - 1].template get<int>();
        int end_time_1 = start_time_1 + delta_1 - 1;
        int end_time_2 = start_time_2 + delta_2 - 1;

        int start = std::max(start_time_1, start_time_2);
        int end = std::min(end_time_1, end_time_2);

        for (const auto& t : season) {
            if (t >= start && t <= end) {
                exclusions_violation += 1.0;
            }
        }
    }

    // Compute objective function (mean risk and expected excess)
    double mean_risk = 0.0;
    double expected_excess = 0.0;

    for (int t = 0; t < t_max; ++t) {

        // Sum mean risk over periods
        mean_risk_by_period[t] /= scenarios_number[t].template get<int>();
        mean_risk += mean_risk_by_period[t];

        // Sum expected excess over periods
        int quantil_idx = static_cast<int>(std::ceil(risk[t].size() * quantil) + 0.5) - 1;
        std::nth_element(risk[t].begin(), risk[t].begin() + quantil_idx, risk[t].end());
        expected_excess += std::max(risk[t][quantil_idx] - mean_risk_by_period[t], 0.0);
    }

    mean_risk /= t_max;
    expected_excess /= t_max;

    double alpha = data_[params::ALPHA].template get<double>();
    double objetive = (alpha * mean_risk) + ((1 - alpha) * expected_excess);

    // Return objective and constraints values
    return {objetive, {exclusions_violation, resource_count_violation, resource_sum_violation}};

}
