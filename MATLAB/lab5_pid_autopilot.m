% Lab 5: P, PI, and PID control for the aircraft autopilot model.
% Run this file in MATLAB. It saves plots and quality-index tables.

clear; clc; close all;

outdir = fullfile(pwd, "results_lab4_5_6");
if ~exist(outdir, "dir")
    mkdir(outdir);
end

s = tf("s");

servo = -10 / (s + 10);
aircraft = -(s + 5) / (s^2 + 3.5*s + 6);
plant = minreal(servo * aircraft);

t_step = 0:0.01:30;

fprintf("\nEquivalent controlled object:\n");
plant

% P controller tests.
Kc_values = [0.1, 1.0, 2, 5, 8];
p_cases = strings(numel(Kc_values), 1);
p_systems = cell(numel(Kc_values), 1);
for i = 1:numel(Kc_values)
    Kc = Kc_values(i);
    p_cases(i) = sprintf("P: Kc=%.3g", Kc);
    p_systems{i} = feedback(Kc * plant, 1);
end
plot_step_group(p_systems, p_cases, t_step, ...
    "Lab 5: P controller step responses", ...
    fullfile(outdir, "lab5_p_step.png"));
p_metrics = measure_group(p_cases, p_systems, t_step);
writetable(p_metrics, fullfile(outdir, "lab5_p_metrics.csv"));

% PI controller tests.
Kc = 5;
Ti_values = [1, 2, 4, 6];
pi_cases = strings(numel(Ti_values), 1);
pi_systems = cell(numel(Ti_values), 1);
for i = 1:numel(Ti_values)
    Ti = Ti_values(i);
    C = Kc * (1 + 1/(Ti*s));
    pi_cases(i) = sprintf("PI: Kc=5, Ti=%.3g", Ti);
    pi_systems{i} = feedback(C * plant, 1);
end
plot_step_group(pi_systems, pi_cases, t_step, ...
    "Lab 5: PI controller step responses", ...
    fullfile(outdir, "lab5_pi_step.png"));
pi_metrics = measure_group(pi_cases, pi_systems, t_step);
writetable(pi_metrics, fullfile(outdir, "lab5_pi_metrics.csv"));

% PID controller tests. The lab handout writes PID as:
% C(s)=Kc*(1+1/(Ti*s)+Td*s). MATLAB/Simulink parallel PID uses:
% P=Kc, I=Kc/Ti, D=Kc*Td.
Kc = 5;
Ti = 1;
Td_values = [0.1, 0.5, 2, 4];
pid_cases = strings(numel(Td_values), 1);
pid_systems = cell(numel(Td_values), 1);
for i = 1:numel(Td_values)
    Td = Td_values(i);
    C = Kc * (1 + 1/(Ti*s) + Td*s);
    pid_cases(i) = sprintf("PID: Kc=5, Ti=1, Td=%.3g", Td);
    pid_systems{i} = feedback(C * plant, 1);
end
plot_step_group(pid_systems, pid_cases, t_step, ...
    "Lab 5: PID controller step responses", ...
    fullfile(outdir, "lab5_pid_step.png"));
pid_metrics = measure_group(pid_cases, pid_systems, t_step);
writetable(pid_metrics, fullfile(outdir, "lab5_pid_metrics.csv"));

all_metrics = [p_metrics; pi_metrics; pid_metrics];
writetable(all_metrics, fullfile(outdir, "lab5_all_step_metrics.csv"));

% Optional task 1: automatically search for one controller that meets:
% ess = 0, decay ratio 6-10, settling time < 10, overshoot < 0.3,
% peak time < 5. Here overshoot < 0.3 means peak value is less than 1.3.
% Your Simulink tuning already found a usable controller: P=6, I=11, D=2.
% Keep the broad grid search off by default because it is slow.
run_auto_search = false;
if run_auto_search
    best = tune_for_requirements(plant, s, t_step);
    writetable(best, fullfile(outdir, "lab5_auto_tuning_candidates.csv"));
end

% Optional task 2: ramp response, theta_d(t) = 0.5*t deg.
t_ramp = 0:0.01:20;
ramp = 0.5 * t_ramp;

C_p = 2;
C_pi = 5 * (1 + 1/(2*s));
C_pid = 5 * (1 + 1/(2*s) + 0.5*s);
ramp_systems = {feedback(C_p*plant, 1), feedback(C_pi*plant, 1), ...
    feedback(C_pid*plant, 1)};
ramp_cases = ["P: Kc=2", "PI: Kc=5, Ti=2", "PID: Kc=5, Ti=2, Td=0.5"];

figure("Name", "Lab 5 - Ramp Responses", "Color", "w");
plot(t_ramp, ramp, "k--", "LineWidth", 1.2); hold on;
ramp_error_10s = nan(numel(ramp_systems), 1);
for i = 1:numel(ramp_systems)
    y = lsim(ramp_systems{i}, ramp, t_ramp);
    plot(t_ramp, y, "LineWidth", 1.2);
    [~, idx10] = min(abs(t_ramp - 10));
    ramp_error_10s(i) = ramp(idx10) - y(idx10);
end
grid on;
title("Lab 5: ramp response, theta_d(t)=0.5t deg");
xlabel("Time (s)");
ylabel("Heading angle (deg)");
legend(["input", ramp_cases], "Location", "best");
saveas(gcf, fullfile(outdir, "lab5_ramp_response.png"));

ramp_metrics = table(ramp_cases(:), ramp_error_10s, ...
    'VariableNames', {'Case', 'ErrorAt10s_deg'});
writetable(ramp_metrics, fullfile(outdir, "lab5_ramp_errors.csv"));

fprintf("\nLab 5 finished. Results were saved in:\n%s\n", outdir);
fprintf("For your Simulink tuned controller, use parallel PID parameters P=6, I=11, D=2.\n");

function plot_step_group(systems, names, t, plot_title, file_name)
    figure("Name", plot_title, "Color", "w");
    hold on;
    for k = 1:numel(systems)
        step(systems{k}, t);
    end
    grid on;
    title(plot_title);
    legend(cellstr(names(:)), 'Location', 'best');
    saveas(gcf, file_name);
end

function metrics = measure_group(case_names, systems, t)
    n = numel(systems);
    Case = strings(n, 1);
    Stable = false(n, 1);
    Overshoot_percent = nan(n, 1);
    PeakTime_s = nan(n, 1);
    SettlingTime_s = nan(n, 1);
    DecayRatio = nan(n, 1);
    FinalValue = nan(n, 1);
    SteadyStateError = nan(n, 1);
    MaxOvershootValue = nan(n, 1);

    for i = 1:n
        Case(i) = case_names(i);
        sys = systems{i};
        Stable(i) = isstable(sys);
        y = squeeze(step(sys, t));
        final_value = dcgain(sys);
        if ~isfinite(final_value)
            final_value = y(end);
        end

        try
            info = stepinfo(y, t, final_value, "SettlingTimeThreshold", 0.02);
            Overshoot_percent(i) = info.Overshoot;
            PeakTime_s(i) = info.PeakTime;
            SettlingTime_s(i) = info.SettlingTime;
        catch
            % Leave metrics as NaN if MATLAB cannot compute them.
        end

        DecayRatio(i) = estimate_decay_ratio(t, y, final_value);
        FinalValue(i) = final_value;
        SteadyStateError(i) = abs(1 - final_value);
        MaxOvershootValue(i) = max(y) - 1;
    end

    metrics = table(Case, Stable, Overshoot_percent, PeakTime_s, ...
        SettlingTime_s, DecayRatio, FinalValue, SteadyStateError, ...
        MaxOvershootValue);
end

function ratio = estimate_decay_ratio(t, y, final_value)
    t = t(:);
    y = y(:);
    dev = y - final_value;
    ratio = NaN;

    idx = find(dev(2:end-1) > dev(1:end-2) & dev(2:end-1) >= dev(3:end)) + 1;
    idx = idx(t(idx) > 0.05 & dev(idx) > 0.02*max(abs(dev)));
    if numel(idx) >= 2
        ratio = dev(idx(1)) / dev(idx(2));
        return;
    end

    absdev = abs(dev);
    idx = find(absdev(2:end-1) > absdev(1:end-2) & absdev(2:end-1) >= absdev(3:end)) + 1;
    idx = idx(t(idx) > 0.05 & absdev(idx) > 0.02*max(absdev));
    if numel(idx) >= 2
        ratio = absdev(idx(1)) / absdev(idx(2));
    end
end

function candidates = tune_for_requirements(plant, s, t)
    rows = {};

    for Kc = 1:0.25:12
        for Ti = 0.5:0.25:6
            C = Kc * (1 + 1/(Ti*s));
            sys = feedback(C * plant, 1);
            rows = add_candidate(rows, "PI", Kc, Ti, NaN, sys, t);
        end
    end

    for Kc = 1:0.25:12
        for Ti = 0.5:0.25:6
            for Td = 0.05:0.05:2
                C = Kc * (1 + 1/(Ti*s) + Td*s);
                sys = feedback(C * plant, 1);
                rows = add_candidate(rows, "PID", Kc, Ti, Td, sys, t);
            end
        end
    end

    if isempty(rows)
        candidates = table();
        return;
    end

    candidates = cell2table(rows, 'VariableNames', ...
        {'Controller', 'Kc', 'Ti', 'Td', 'OvershootValue', 'PeakTime_s', ...
        'SettlingTime_s', 'DecayRatio', 'SteadyStateError', 'Score', 'MeetsAll'});
    candidates = sortrows(candidates, {'MeetsAll', 'Score'}, {'descend', 'ascend'});
end

function rows = add_candidate(rows, controller, Kc, Ti, Td, sys, t)
    if ~isstable(sys)
        return;
    end

    y = squeeze(step(sys, t));
    final_value = dcgain(sys);
    if ~isfinite(final_value)
        final_value = y(end);
    end

    try
        info = stepinfo(y, t, final_value, "SettlingTimeThreshold", 0.02);
    catch
        return;
    end

    decay = estimate_decay_ratio(t, y, final_value);
    ess = abs(1 - final_value);
    os_value = max(y) - 1;

    if isnan(decay)
        decay_penalty = 10;
    elseif decay < 6
        decay_penalty = 6 - decay;
    elseif decay > 10
        decay_penalty = decay - 10;
    else
        decay_penalty = 0;
    end

    meets_all = ess < 1e-3 && decay_penalty == 0 && ...
        info.SettlingTime < 10 && os_value < 0.3 && info.PeakTime < 5;
    score = 1000*(ess >= 1e-3) + decay_penalty + ...
        max(0, info.SettlingTime - 10) + max(0, os_value - 0.3)*10 + ...
        max(0, info.PeakTime - 5);

    row_index = size(rows, 1) + 1;
    rows(row_index, :) = {controller, Kc, Ti, Td, os_value, info.PeakTime, ...
        info.SettlingTime, decay, ess, score, meets_all};
end
