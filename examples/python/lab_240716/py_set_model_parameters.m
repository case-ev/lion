function simin = py_set_model_parameters(mdl, ...
    simin, ...
    params, ...
    initial_conditions, ...
    constant_parameters)
cp = params.cp;
cair = params.cair;
rin = params.rin;
rout = params.rout;
rair = params.rair;
initial_soc = initial_conditions.initial_soc;
in_temp = initial_conditions.initial_in_temp;
internal_resistance = constant_parameters.internal_resistance;
nominal_capacity = constant_parameters.nominal_capacity;
if internal_resistance ~= 0
    simin = setBlockParameter(simin, ...
        strcat(mdl, "/Battery"), ...
        "internal_resistance", ...
        string(internal_resistance));
end
if nominal_capacity ~= 0
    simin = setBlockParameter(simin, ...
        strcat(mdl, "/Battery"), ...
        "nominal_capacity", ...
        string(nominal_capacity));
end
simin = setBlockParameter(simin, ...
    strcat(mdl, "/Battery"), "th_cp", string(cp), ...
    strcat(mdl, "/Battery"), "th_cair", string(cair), ...
    strcat(mdl, "/Battery"), "th_rin", string(rin), ...
    strcat(mdl, "/Battery"), "th_rout", string(rout), ...
    strcat(mdl, "/Battery"), "th_rair", string(rair), ...
    strcat(mdl, "/Battery"), "initial_in_temp", string(in_temp), ...
    strcat(mdl, "/Battery"), "initial_soc", string(initial_soc));
end