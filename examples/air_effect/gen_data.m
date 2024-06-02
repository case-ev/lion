%% Set simulation parameters
% Parameters
air_params = dictionary("cp", 100, ...
    "cair", 1000, ...
    "rin", 3, ...
    "rout", 9, ...
    "rair", 2, ...
    "in_temp", 298, ...
    "air_temp", 298);
noair_params = dictionary("cp", 100, ...
    "cair", 1e-6, ...
    "rin", 3, ...
    "rout", 9, ...
    "rair", 1e-6, ...
    "in_temp", 298, ...
    "air_temp", 298);

% Write parameters to file
encoded = jsonencode(struct("cp", air_params("cp"), ...
    "cair", air_params("cair"), ...
    "rin", air_params("rin"), ...
    "rout", air_params("rout"), ...
    "rair", air_params("rair"), ...
    "in_temp", air_params("in_temp"), ...
    "air_temp", air_params("air_temp")));
fid = fopen("examples/air_effect/air_params.json", "w");
fprintf(fid, "%s", encoded);
fclose(fid);
encoded = jsonencode(struct("cp", noair_params("cp"), ...
    "cair", noair_params("cair"), ...
    "rin", noair_params("rin"), ...
    "rout", noair_params("rout"), ...
    "rair", noair_params("rair"), ...
    "in_temp", noair_params("in_temp"), ...
    "air_temp", noair_params("air_temp")));
fid = fopen("examples/air_effect/noair_params.json", "w");
fprintf(fid, "%s", encoded);
fclose(fid);

%% Data loading
% Experiment for Rout estimation
exp1_time = linspace(0, 20000, 100000)';
exp1_power = 0.05 * ones(size(exp1_time));
exp1_amb_temp = 273 * ones(size(exp1_time));

% Experiment for Cp and Rin
exp2_time = linspace(0, 100000, 100000)';
exp2_power = 3 * (square(2 * pi * (5 / (exp2_time(end) - exp2_time(1))) * exp2_time));
exp2_amb_temp = 273 * ones(size(exp2_time));

%% Generate timeseries
exp1_power_profile = timeseries(exp1_power, exp1_time);
exp1_amb_profile = timeseries(exp1_amb_temp, exp1_time);
exp1_time_delta = exp1_time(2) - exp1_time(1);
exp1_end_time = exp1_time(end);

exp2_power_profile = timeseries(exp2_power, exp2_time);
exp2_amb_profile = timeseries(exp2_amb_temp, exp2_time);
exp2_time_delta = exp2_time(2) - exp2_time(1);
exp2_end_time = exp2_time(end);

%% Set simulation parameters and run simulation
mdl = "sanity_check_sim";

% Model with air considerations
disp("Evaluating model with air");
disp("Running experiment 1");
power_profile = exp1_power_profile;
amb_profile = exp1_amb_profile;
out1_air = evaluate_model(exp1_time_delta, exp1_end_time, mdl, air_params, 0.1);
out_table1_air = table(out1_air.tout, out1_air.simout.sf_temp.Data, ...
    out1_air.simout.air_temp.Data, ...
    out1_air.simout.q_gen.Data, out1_air.ambient.Data, ...
    VariableNames=["time", "sf_temp", "air_temp", "q_gen", "amb_temp"]);
writetable(out_table1_air, "examples/air_effect/exp1_air_sim.csv");

disp("Running experiment 2");
power_profile = exp2_power_profile;
amb_profile = exp2_amb_profile;
out2_air = evaluate_model(exp2_time_delta, exp2_end_time, mdl, air_params, 0.8);
out_table2_air = table(out2_air.tout, out2_air.simout.sf_temp.Data, out2_air.simout.air_temp.Data, ...
    out2_air.simout.q_gen.Data, out2_air.ambient.Data, ...
    VariableNames=["time", "sf_temp", "air_temp", "q_gen", "amb_temp"]);
writetable(out_table2_air, "examples/air_effect/exp2_air_sim.csv");


% Model without air considerations
disp("Evaluating model without air");
disp("Running experiment 1");
power_profile = exp1_power_profile;
amb_profile = exp1_amb_profile;
out1_noair = evaluate_model(exp1_time_delta, exp1_end_time, mdl, noair_params, 0.1);
out_table1_noair = table(out1_noair.tout, out1_noair.simout.sf_temp.Data, ...
    out1_noair.simout.air_temp.Data, ...
    out1_noair.simout.q_gen.Data, out1_noair.ambient.Data, ...
    VariableNames=["time", "sf_temp", "air_temp", "q_gen", "amb_temp"]);
writetable(out_table1_noair, "examples/air_effect/exp1_noair_sim.csv");

disp("Running experiment 2");
power_profile = exp2_power_profile;
amb_profile = exp2_amb_profile;
out2_noair = evaluate_model(exp2_time_delta, exp2_end_time, mdl, noair_params, 0.8);
out_table2_noair = table(out2_noair.tout, out2_noair.simout.sf_temp.Data, ...
    out2_noair.simout.air_temp.Data, ...
    out2_noair.simout.q_gen.Data, out2_noair.ambient.Data, ...
    VariableNames=["time", "sf_temp", "air_temp", "q_gen", "amb_temp"]);
writetable(out_table2_noair, "examples/air_effect/exp2_noair_sim.csv");

%% Save the workspace
save("examples/air_effect/ws.mat");

%% Helper functions
function out = evaluate_model(time_delta, end_time, mdl, params, initial_soc)
cp = params("cp");
cair = params("cair");
rin = params("rin");
rout = params("rout");
rair = params("rair");
in_temp = params("in_temp");
air_temp = params("air_temp");
load_system(mdl);
simin = Simulink.SimulationInput(mdl);
set_param(mdl, "SimulationCommand", "update");
simin = setModelParameter(simin, ...
    Solver="ode14x", ...
    StopTime=string(end_time), ...
    FixedStep=string(time_delta));
simin = setBlockParameter(simin, ...
    strcat(mdl, "/Battery"), "th_cp", string(cp), ...
    strcat(mdl, "/Battery"), "th_cair", string(cair), ...
    strcat(mdl, "/Battery"), "th_rin", string(rin), ...
    strcat(mdl, "/Battery"), "th_rout", string(rout), ...
    strcat(mdl, "/Battery"), "th_rair", string(rair), ...
    strcat(mdl, "/Battery"), "initial_in_temp", string(in_temp), ...
    strcat(mdl, "/Battery"), "initial_air_temp", string(air_temp), ...
    strcat(mdl, "/Battery"), "initial_soc", string(initial_soc));

% Run simulation
out = sim(simin);
end