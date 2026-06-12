% Lab 6: root-locus experiments.
% Source code is ASCII-only for MATLAB encoding compatibility.

clear; clc; close all;

outdir = fullfile(pwd, "results_lab4_5_6");
if ~exist(outdir, "dir")
    mkdir(outdir);
end

s = tf("s");

%% Part 1 placeholder. Use lab6_water_tank_root_locus.m for the textbook
% water-tank system from problem 3-16.

%% Part 2: G(s) = K/(s^2*(s+a)), then add a negative open-loop zero z=-b.
a = 2;
b_values = [4, 1];  % b>a and b<a
kgrid = linspace(0, 100, 10001);

G0 = 1 / (s^2 * (s + a));
figure("Name", "Lab 6 - Root Locus Without Added Zero", "Color", "w");
rlocus(G0);
grid on;
xlabel(u([23454 36724]));
ylabel(u([34394 36724]));
title(u([21407 22987 31995 32479 26681 36712 36857]));
saveas(gcf, fullfile(outdir, "lab6_no_zero_rlocus.png"));

no_zero_ranges = stable_gain_ranges(G0, kgrid);
writetable(no_zero_ranges, fullfile(outdir, "lab6_no_zero_stable_k_range.csv"));

figure("Name", "Lab 6 - Root Locus With Added Zeros", "Color", "w");
for i = 1:numel(b_values)
    b = b_values(i);
    G = (s + b) / (s^2 * (s + a));
    subplot(1, numel(b_values), i);
    rlocus(G);
    grid on;
    xlabel(u([23454 36724]));
    ylabel(u([34394 36724]));
    if b > a
        ttl = sprintf("%s z=-%.2g, b>a", u([38468 21152 38646 28857]), b);
    else
        ttl = sprintf("%s z=-%.2g, b<a", u([38468 21152 38646 28857]), b);
    end
    title(ttl);
end
saveas(gcf, fullfile(outdir, "lab6_added_zero_rlocus.png"));

Case = strings(numel(b_values) + 1, 1);
CharacteristicEquation = strings(numel(b_values) + 1, 1);
RouthConclusion = strings(numel(b_values) + 1, 1);

Case(1) = "no added zero";
CharacteristicEquation(1) = "s^3 + a*s^2 + K = 0";
RouthConclusion(1) = "unstable for K>0";

for i = 1:numel(b_values)
    b = b_values(i);
    Case(i+1) = sprintf("added zero z=-%.3g", b);
    CharacteristicEquation(i+1) = "s^3 + a*s^2 + K*s + K*b = 0";
    if b < a
        RouthConclusion(i+1) = "can be stable for K>0";
    else
        RouthConclusion(i+1) = "unstable for K>0";
    end
end

routh_table = table(Case, CharacteristicEquation, RouthConclusion);
writetable(routh_table, fullfile(outdir, "lab6_content2_routh_conclusions.csv"));

fprintf("\nLab 6 finished. Results were saved in:\n%s\n", outdir);
fprintf("For G(s)=K/(s^2(s+a)), K>0 gives no stable closed-loop range.\n");
fprintf("After adding z=-b, the characteristic equation is s^3+a*s^2+K*s+K*b=0.\n");
fprintf("Routh gives stability for K>0 only when b<a.\n");

function ranges = stable_gain_ranges(G, kgrid)
    stable = false(size(kgrid));
    for i = 1:numel(kgrid)
        poles = pole(feedback(kgrid(i) * G, 1));
        stable(i) = all(real(poles) < -1e-8);
    end

    starts = find(diff([false, stable]) == 1);
    stops = find(diff([stable, false]) == -1);

    if isempty(starts)
        ranges = table("none", NaN, NaN, 'VariableNames', ...
            {'Range', 'Kmin', 'Kmax'});
        return;
    end

    Range = strings(numel(starts), 1);
    Kmin = nan(numel(starts), 1);
    Kmax = nan(numel(starts), 1);
    for r = 1:numel(starts)
        Range(r) = sprintf("stable range %d", r);
        Kmin(r) = kgrid(starts(r));
        Kmax(r) = kgrid(stops(r));
    end
    ranges = table(Range, Kmin, Kmax);
end

function text = u(codes)
    text = char(codes);
end
