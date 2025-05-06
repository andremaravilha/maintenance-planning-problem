#ifndef INCLUDE_MPP_PROBLEM_HPP_
#define INCLUDE_MPP_PROBLEM_HPP_

#include <string>
#include <tuple>
#include <vector>
#include <json.hpp>

namespace mpp {

using json = nlohmann::json;

using solution_t = std::map<std::string, int>;
using constraints_t = std::tuple<double, double, double>;
using risk_metric_t = std::tuple<double, double>;
using objective_t = double;

namespace params {
    const std::string QUANTILE = "Quantile";
    const std::string ALPHA = "Alpha";
    const std::string T = "T";
    const std::string SCENARIOS_NUMBER = "Scenarios_number";
    const std::string SEASONS = "Seasons";
    const std::string EXCLUSIONS = "Exclusions";
    const std::string RESOURCES = "Resources";
    const std::string RESOURCE_LOWER_BOUND = "min";
    const std::string RESOURCE_UPPER_BOUND = "max";
    const std::string INTERVENTIONS = "Interventions";
    const std::string INTERVENTION_TMAX = "tmax";
    const std::string INTERVENTION_DELTA = "Delta";
    const std::string INTERVENTION_RISK = "risk";
    const std::string INTERVENTION_RESOURCE_WORKLOAD = "workload";
}

class problem_t {
    public:
    problem_t(const std::string& filename);
    ~problem_t();

    std::tuple<objective_t, risk_metric_t, constraints_t>
    evaluate(const solution_t& solution) const;

    inline
    std::tuple<objective_t, risk_metric_t, constraints_t>
    evaluate(const std::vector<int>& start_time) const;

    inline
    std::tuple<objective_t, risk_metric_t, constraints_t>
    evaluate(const std::vector<int>& start_time, const std::vector<std::string>& intervention_name) const;

    inline
    const json& get_data() const;

    inline
    const std::vector<std::string>& get_intervention_names() const;

    private:
    json data_;
    std::vector<std::string> intervention_names_;

};

}


std::tuple<mpp::objective_t, mpp::risk_metric_t, mpp::constraints_t>
mpp::problem_t::evaluate(const std::vector<int>& start_time) const {
    evaluate(start_time, intervention_names_);
}


std::tuple<mpp::objective_t, mpp::risk_metric_t, mpp::constraints_t>
mpp::problem_t::evaluate(const std::vector<int>& start_time, const std::vector<std::string>& intervention_name) const {
    solution_t solution;
    for (int i = 0; i < intervention_name.size(); ++i) {
        solution.insert({intervention_name[i], start_time[i]});
    }

    return evaluate(solution);
}

const nlohmann::json&
mpp::problem_t::get_data() const { 
    return data_;
}

const std::vector<std::string>& 
mpp::problem_t::get_intervention_names() const {
    return intervention_names_;
}


#endif // INCLUDE_MPP_PROBLEM_HPP_
