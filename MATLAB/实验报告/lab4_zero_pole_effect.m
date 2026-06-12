% Lab 4: effect of closed-loop zeros and poles on transient response.
% Source code is ASCII-only for MATLAB encoding compatibility.

clear; clc; close all;

outdir = fullfile(pwd, "results_lab4_5_6");
if ~exist(outdir, "dir")
    mkdir(outdir);
end

s = tf("s");
zeta = 0.5;
wn = 1;
sigma = zeta * wn;
t = 0:0.01:30;

base = wn^2 / (s^2 + 2*zeta*wn*s + wn^2);

T_values = [0.2, 2, 5];
pole_systems = {base, base/(T_values(1)*s + 1), ...
    base/(T_values(2)*s + 1), base/(T_values(3)*s + 1), ...
    1/(T_values(3)*s + 1)};
pole_names = {u([26631 20934 20108 38454 31995 32479]), ...
    u([38468 21152 26497 28857 36828 31163 34394 36724]), ...
    u([38468 21152 26497 28857 25509 36817 21407 26497 28857]), ...
    u([38468 21152 26497 28857 25104 20026 20027 23548 26497 28857]), ...
    u([19968 38454 31995 32479 23545 29031])};

figure("Name", "Lab 4 - Added Pole Step Responses", "Color", "w");
hold on;
for k = 1:numel(pole_systems)
    step(pole_systems{k}, t);
end
grid on;
xlabel(u([26102 38388 47 115]));
ylabel(u([36755 20986 21709 24212 32 121 40 116 41]));
title(u([22686 21152 38381 29615 26497 28857 26102 30340 21333 20301 38454 36291 21709 24212]));
legend(pole_names, "Location", "best");
saveas(gcf, fullfile(outdir, "lab4_added_poles_step.png"));

figure("Name", "Lab 4 - Added Pole Pole-Zero Map", "Color", "w");
hold on;
for k = 1:4
    pzmap(pole_systems{k});
end
grid on;
xlabel(u([23454 36724]));
ylabel(u([34394 36724]));
title(u([22686 21152 38381 29615 26497 28857 26102 30340 38646 26497 28857 20998 24067]));
legend(pole_names(1:4), "Location", "best");
saveas(gcf, fullfile(outdir, "lab4_added_poles_pzmap.png"));

tau_values = [0.2, 2, 5];
zero_systems = {base, (tau_values(1)*s + 1)*base, ...
    (tau_values(2)*s + 1)*base, (tau_values(3)*s + 1)*base};
zero_names = {u([26631 20934 20108 38454 31995 32479]), ...
    u([38468 21152 38646 28857 36828 31163 34394 36724]), ...
    u([38468 21152 38646 28857 25509 36817 21407 26497 28857]), ...
    u([38468 21152 38646 28857 38752 36817 34394 36724])};

figure("Name", "Lab 4 - Added Zero Step Responses", "Color", "w");
hold on;
for k = 1:numel(zero_systems)
    step(zero_systems{k}, t);
end
grid on;
xlabel(u([26102 38388 47 115]));
ylabel(u([36755 20986 21709 24212 32 121 40 116 41]));
title(u([22686 21152 38381 29615 38646 28857 26102 30340 21333 20301 38454 36291 21709 24212]));
legend(zero_names, "Location", "best");
saveas(gcf, fullfile(outdir, "lab4_added_zeros_step.png"));

figure("Name", "Lab 4 - Added Zero Pole-Zero Map", "Color", "w");
hold on;
for k = 1:numel(zero_systems)
    pzmap(zero_systems{k});
end
grid on;
xlabel(u([23454 36724]));
ylabel(u([34394 36724]));
title(u([22686 21152 38381 29615 38646 28857 26102 30340 38646 26497 28857 20998 24067]));
legend(zero_names, "Location", "best");
saveas(gcf, fullfile(outdir, "lab4_added_zeros_pzmap.png"));

pole_metrics = make_metrics_table(pole_names, pole_systems, t);
zero_metrics = make_metrics_table(zero_names, zero_systems, t);

writetable(pole_metrics, fullfile(outdir, "lab4_added_poles_metrics.csv"));
writetable(zero_metrics, fullfile(outdir, "lab4_added_zeros_metrics.csv"));

fprintf("\nLab 4 finished. Results were saved in:\n%s\n", outdir);
fprintf("zeta*wn = %.3f. For T = %.1f, the added pole is %.3f, so it is dominant.\n", ...
    sigma, T_values(3), -1/T_values(3));
fprintf("The second-order poles are:\n");
disp(pole(base));

function metrics = make_metrics_table(case_names, systems, t)
    t = t(:);
    n = numel(systems);
    Case = strings(n, 1);
    Overshoot_percent = nan(n, 1);
    PeakTime_s = nan(n, 1);
    SettlingTime_s = nan(n, 1);
    FinalValue = nan(n, 1);
    SteadyStateError = nan(n, 1);

    for i = 1:n
        Case(i) = string(case_names{i});
        sys = systems{i};
        y = squeeze(step(sys, t));
        y = y(:);
        final_value = dcgain(sys);
        if ~isfinite(final_value)
            final_value = y(end);
        end

        info = stepinfo(y, t, final_value, "SettlingTimeThreshold", 0.02);
        Overshoot_percent(i) = info.Overshoot;
        PeakTime_s(i) = info.PeakTime;
        SettlingTime_s(i) = info.SettlingTime;
        FinalValue(i) = final_value;
        SteadyStateError(i) = abs(1 - final_value);
    end

    metrics = table(Case, Overshoot_percent, PeakTime_s, ...
        SettlingTime_s, FinalValue, SteadyStateError);
end

function text = u(codes)
    text = char(codes);
end
