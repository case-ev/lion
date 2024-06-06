%% Set simulation parameters
% Real parameters
fid = fopen("examples/air_effect/params_expected_noair.json", "r");
raw = fread(fid, inf);
str = char(raw');
fclose(fid);
real_params_ = jsondecode(str);
real_params = dictionary("cp", real_params_.cp, ...
    "cair", real_params_.cair, ...
    "rair", real_params_.rair, ...
    "rin", real_params_.rin, ...
    "rout", real_params_.rout, ...
    "in_temp", real_params_.in_temp, ...
    "air_temp", real_params_.air_temp);

% Estimated parameters
fid = fopen("examples/air_effect/params_obtained_noair.json", "r");
raw = fread(fid, inf);
str = char(raw');
fclose(fid);
est_params_ = jsondecode(str);
est_params = dictionary("cp", est_params_.cp, ...
    "cair", est_params_.cair, ...
    "rair", est_params_.rair, ...
    "rin", est_params_.rin, ...
    "rout", est_params_.rout, ...
    "in_temp", real_params_.in_temp, ...
    "air_temp", real_params_.air_temp);


%% Data loading
% Load csv file
csv_dir = "data/240209_temptest_C6B2/TestData.csv";
opts = detectImportOptions(csv_dir);
opts = setvaropts(opts, "Seconds", "InputFormat", "MM/dd/uuuu HH:mm:ss.SSS");
csv_table = readtable(csv_dir, opts);

% Determine relevant parameters
SEGMENT = 4;
TIME_LIMIT = 50;

% Extract data
time = csv_table(:, "Seconds") - csv_table(1, "Seconds");
time = seconds(time.(1));
current = csv_table(:, "Current").(1);
voltage = csv_table(:, "Voltage").(1);
[time, power] = get_segment(time, current, voltage, SEGMENT, TIME_LIMIT);
amb_temp = 298;

% time = linspace(0, 100000, 100000)';
% power = 3 * (square(2 * pi * (5 / (time(end) - time(1))) * time));
% amb_temp = 273 * ones(size(time));

%% Generate timeseries
power_profile = timeseries(power, time);
amb_profile = timeseries(amb_temp, time);
time_delta = time(2) - time(1);
end_time = time(end);

%% Set simulation parameters and run simulation
mdl = "sim_noair";

disp("Running experiment for real parameters");
out1 = evaluate_model(time_delta, end_time, mdl, real_params, 0.8);
out1_table = table(out1.tout, out1.simout.sf_temp.Data, out1.simout.air_temp.Data, ...
    out1.simout.q_gen.Data, out1.ambient.Data, ...
    VariableNames=["time", "sf_temp", "air_temp", "q_gen", "amb_temp"]);
writetable(out1_table, "examples/air_effect/validation_noair_expected.csv");

disp("Running experiment for estimated parameters");
out2 = evaluate_model(time_delta, end_time, mdl, est_params, 0.8);
out2_table = table(out2.tout, out2.simout.sf_temp.Data, out2.simout.air_temp.Data, ...
    out2.simout.q_gen.Data, out2.ambient.Data, ...
    VariableNames=["time", "sf_temp", "air_temp", "q_gen", "amb_temp"]);
writetable(out2_table, "examples/air_effect/validation_noair_obtained.csv");

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


function [time, power] = get_segment(table_time, table_current, table_voltage, segment, time_limit)
% Initialize helper variables
curr_seg = 1;
prev_time = 0;
segment_id = zeros(length(table_time), 1);

% Identify segments
for i = 1:length(table_time)
    t = table_time(i);
    if abs(t - prev_time) >= time_limit
        curr_seg = curr_seg + 1;
    end
    segment_id(i) = curr_seg;
    prev_time = t;
end

time = table_time(segment_id == segment);
time = time - time(1);
current = table_current(segment_id == segment);
voltage = table_voltage(segment_id == segment);
power = current .* voltage;
end
