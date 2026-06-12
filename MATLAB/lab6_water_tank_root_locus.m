% Lab 6 Part 1: root locus of the second-order coupled water-tank system.
% Source code is ASCII-only for MATLAB encoding compatibility.

clear; clc; close all;

outdir = fullfile(pwd, "results_lab4_5_6");
if ~exist(outdir, "dir")
    mkdir(outdir);
end

s = tf("s");

A1 = 1000;
A2 = 800;
R1 = 0.005;
R2 = 0.005;
Kv = 1250;
KB = 1;
TB = 0.5;

G1 = 1 / (A1*R1*s + 1);
G2 = R2 / (A2*R2*s + 1);
GB = KB / (TB*s + 1);
L0 = minreal(Kv * G1 * G2 * GB);

figure("Name", "Lab 6 Part 1 - Water Tank Root Locus", "Color", "w");
rlocus(L0);
grid on;
title(u([20108 38454 27700 27133 31995 32479 26681 36712 36857]));
xlabel(u([23454 36724]));
ylabel(u([34394 36724]));
saveas(gcf, fullfile(outdir, "lab6_water_tank_root_locus.png"));

kgrid = linspace(0, 100, 20001);
stable = false(size(kgrid));
for i = 1:numel(kgrid)
    stable(i) = isstable(feedback(kgrid(i)*L0, 1));
end

starts = find(diff([false, stable]) == 1);
stops = find(diff([stable, false]) == -1);

if isempty(starts)
    Range = "none";
    Kc_min = NaN;
    Kc_max = NaN;
else
    Range = strings(numel(starts), 1);
    Kc_min = nan(numel(starts), 1);
    Kc_max = nan(numel(starts), 1);
    for r = 1:numel(starts)
        Range(r) = sprintf("stable range %d", r);
        Kc_min(r) = kgrid(starts(r));
        Kc_max(r) = kgrid(stops(r));
    end
end
ranges = table(Range, Kc_min, Kc_max);
writetable(ranges, fullfile(outdir, "lab6_water_tank_stable_range.csv"));

t = 0:0.01:40;
Hopen = G2;
Hclose_K1 = feedback(G2, 1 * Kv * G1 * GB);
Hclose_K05 = feedback(G2, 0.5 * Kv * G1 * GB);

figure("Name", "Lab 6 Part 1 - Water Tank Disturbance Response", "Color", "w");
step(Hopen, "k--", Hclose_K1, "b-", Hclose_K05, "r-", t);
grid on;
title(u([20108 38454 27700 27133 21333 20301 38454 36291 25200 21160 21709 24212]));
xlabel(u([26102 38388 47 115]));
ylabel("h_2");
legend({u([24320 29615]), u([38381 29615 32 75 99 61 49]), ...
    u([38381 29615 32 75 99 61 48 46 53])}, "Location", "best");
saveas(gcf, fullfile(outdir, "lab6_water_tank_disturbance_response.png"));

error_open = dcgain(Hopen);
error_close_K1 = dcgain(Hclose_K1);
error_close_K05 = dcgain(Hclose_K05);
ratio_K1 = error_close_K1 / error_open;
ratio_K05 = error_close_K05 / error_open;

metrics = table(error_open, error_close_K1, error_close_K05, ratio_K1, ratio_K05);
writetable(metrics, fullfile(outdir, "lab6_water_tank_disturbance_metrics.csv"));

fprintf("\nLab 6 water-tank part finished. Results saved in:\n%s\n", outdir);
fprintf("Stable proportional gain range from scanned root-locus data:\n");
disp(ranges);
fprintf("Disturbance steady-state errors: open=%.6g, Kc=1 %.6g, Kc=0.5 %.6g\n", ...
    error_open, error_close_K1, error_close_K05);

function text = u(codes)
    text = char(codes);
end
