#include <solver/relaxed_mip.hpp>
#include <problem.hpp>
#include <gurobi_c++.h>
#include <iostream>


std::tuple<mpp::solution_t, mpp::objective_t, mpp::constraints_t>
mpp::solver::relaxed_mip(const ::mpp::problem_t& problem) {

    // Create and configure a Gurobi environment
    GRBEnv env = GRBEnv(true);
    //env.set(GRB_IntParam_OutputFlag, 0);
    env.set(GRB_IntParam_OutputFlag, 1);
    env.start();

    // Create a Gurobi model
    GRBModel model(env);

    // Get the data from the problem
    const auto& data = problem.get_data();
    const auto& intervention_names = problem.get_intervention_names();
    const auto& interventions = data[mpp::params::INTERVENTIONS];
    const auto& resources = data[mpp::params::RESOURCES];
    const auto& exclusions = data[mpp::params::EXCLUSIONS];
    const auto& scenarios_number = data[mpp::params::SCENARIOS_NUMBER];
    const auto& seasons = data[mpp::params::SEASONS];
    int T = data[mpp::params::T].template get<int>();
    double quantile = data[mpp::params::QUANTILE].template get<double>();
    double alpha = data[mpp::params::ALPHA].template get<double>();

    // Create variables for each pair intervention/time
    std::map< std::string, std::map<int, GRBVar> > x;
    for (const auto& [intervention_name, intervention_data] : interventions.items()) {
        int t_max = intervention_data[mpp::params::INTERVENTION_TMAX].template get<int>();
        for (int t = 1; t <= t_max; ++t) {
            x[intervention_name].insert({t, model.addVar(0.0, 1.0, 0.0, GRB_BINARY)});
        }
    }

    // Set the objective function (14)
    GRBLinExpr obj = 0;
    for (const auto& [intervention_name, intervention_data] : interventions.items()) {
        int t_max = intervention_data[mpp::params::INTERVENTION_TMAX].template get<int>();
        const auto& intervention_risk = intervention_data[mpp::params::INTERVENTION_RISK];
        for (int t = 1; t <= T; ++t) {
            const auto& risk_at_period = intervention_risk.find(std::to_string(t));
            if (risk_at_period != intervention_risk.end()) {
                for (int ts = 1; ts <= t_max; ++ts) {
                    const auto& risk = risk_at_period->find(std::to_string(ts));
                    if (risk != risk_at_period->end()) {
                        for (const auto& r : *risk) {
                            obj += (r.template get<double>() / (T * risk->size())) * x[intervention_name][ts];
                        }
                    }
                }
            }
        }
    }

    model.setObjective(obj, GRB_MINIMIZE);

    // Add constraints (2)
    for (const auto& [intervention_name, intervention_data] : interventions.items()) {
        int t_max = intervention_data[mpp::params::INTERVENTION_TMAX].template get<int>();
        GRBLinExpr expr = 0;
        for (int t = 1; t <= t_max; ++t) {
            expr += x[intervention_name][t];
        }
        model.addConstr(expr == 1);
    }

    // Add constraints (3) and (4)
    for (const auto& [resource_name, resource_data] : resources.items()) {
        for (int t = 1; t <= T; ++t) {
            GRBLinExpr expr = 0;
            for (const auto& [intervention_name, intervention_data] : interventions.items()) {
                int t_max = intervention_data[mpp::params::INTERVENTION_TMAX].template get<int>();
                for (int ts = 1; ts <= t_max; ++ts) {
                    const auto& intervention_workload = intervention_data[mpp::params::INTERVENTION_RESOURCE_WORKLOAD].find(resource_name);
                    if (intervention_workload != intervention_data[mpp::params::INTERVENTION_RESOURCE_WORKLOAD].end()) {
                        const auto& workload_at_period = intervention_workload->find(std::to_string(t));
                        if (workload_at_period != intervention_workload->end()) {
                            const auto& workload = workload_at_period->find(std::to_string(ts));
                            if (workload != workload_at_period->end()) {
                                expr += workload->template get<double>() * x[intervention_name][ts];
                            }
                        }
                    }
                }
            }
            model.addConstr(expr <= resource_data[mpp::params::RESOURCE_UPPER_BOUND][t - 1].template get<double>()); // Constraint (3)
            model.addConstr(expr >= resource_data[mpp::params::RESOURCE_LOWER_BOUND][t - 1].template get<double>()); // Constraint (4)
        }
    }

    // Add constraints (5)
    for (const auto& [exclusion_name, exclusion_data] : exclusions.items()) {
        const auto& intervention_1_name = exclusion_data[0].template get<std::string>();
        const auto& intervention_2_name = exclusion_data[1].template get<std::string>();
        const auto& season = seasons[exclusion_data[2].template get<std::string>()];
        const auto& intervention_1 = interventions[intervention_1_name];
        const auto& intervention_2 = interventions[intervention_2_name];
        int t_max_1 = intervention_1[mpp::params::INTERVENTION_TMAX].template get<int>();
        int t_max_2 = intervention_2[mpp::params::INTERVENTION_TMAX].template get<int>();

        for (const auto& t : season) {
            GRBLinExpr expr = 0;

            for (int ts = 1; ts <= t_max_1; ++ts) {
                if (t >= ts && t <= ts + intervention_1[mpp::params::INTERVENTION_DELTA][ts - 1].template get<int>() - 1) {
                    expr += x[intervention_1_name][ts];
                }
            }

            for (int ts = 1; ts <= t_max_2; ++ts) {
                if (t >= ts && t <= ts + intervention_2[mpp::params::INTERVENTION_DELTA][ts - 1].template get<int>() - 1) {
                    expr += x[intervention_2_name][ts];
                }
            }

            model.addConstr(expr <= 1);
        }
    }

    // Optimize the model
    model.set(GRB_DoubleParam_MIPGap, 0.00);
    //model.set(GRB_DoubleParam_TimeLimit, MIP_TIME_LIMIT);
    model.set(GRB_IntParam_MIPFocus,  1);
    model.set(GRB_IntParam_Presolve,  1);  // 1 - conservative, 0 - off, 2 - aggressive
    model.set(GRB_IntParam_PrePasses, 1);  // to not spend too much time in pre-solve
    model.set(GRB_IntParam_Method,  1);
    model.optimize();

    // Extract the solution
    mpp::solution_t solution;
    for (const auto& intervention_name : intervention_names) {
        const auto& intervention_data = interventions[intervention_name];
        int t_max = intervention_data[mpp::params::INTERVENTION_TMAX].template get<int>();
        for (int t = 1; t <= t_max; ++t) {
            if (x[intervention_name][t].get(GRB_DoubleAttr_X) > 0.5) {
                solution[intervention_name] = t;
                break;
            }
        }
    }

    auto [objective_value, constraints_value] = problem.evaluate(solution);
    return {solution, objective_value, constraints_value};
}