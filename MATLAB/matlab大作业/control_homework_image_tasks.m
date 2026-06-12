% Homework tasks from the picture.
% Run this file in MATLAB.

clear; clc; close all;

outdir = fullfile(pwd, "results_control_homework");
if ~exist(outdir, "dir")
    mkdir(outdir);
end

%% 1. Solve dy/dx + 2*x*y = x*exp(-x^2)
syms y(x)
ode = diff(y, x) + 2*x*y == x*exp(-x^2);
y_general = dsolve(ode);

disp("1) General solution of dy/dx + 2*x*y = x*exp(-x^2):");
disp(y_general);
disp("Equivalent form: y = (C + x^2/2)*exp(-x^2)");

%% 2. Unity negative feedback system
% Open-loop transfer function:
%       G(s) = 25/(s*(s+5))
% Closed-loop transfer function:
%       Phi(s) = G(s)/(1+G(s)) = 25/(s^2 + 5*s + 25)
s = tf("s");
G = 25/(s*(s + 5));
G_cl = feedback(G, 1);

disp(" ");
disp("2) Open-loop transfer function G(s):");
G
disp("Closed-loop transfer function Phi(s):");
G_cl

%% 3. Unit step response
figure("Name", "Closed-loop Step Response", "Color", "w");
step(G_cl);
grid on;
title(u([38381 29615 31995 32479 21333 20301 38454 36291 21709 24212]));
xlabel(u([26102 38388 47 115]));
ylabel(u([36755 20986 32 99 40 116 41]));
saveas(gcf, fullfile(outdir, "closed_loop_step_response.png"));

%% 4. Peak time
info = stepinfo(G_cl);
fprintf("\n4) Peak time Tp = %.4f s\n", info.PeakTime);
fprintf("   Overshoot = %.4f %%\n", info.Overshoot);
fprintf("   Settling time = %.4f s\n", info.SettlingTime);

step_metrics = table(info.PeakTime, info.Overshoot, info.SettlingTime, ...
    'VariableNames', {'PeakTime_s', 'Overshoot_percent', 'SettlingTime_s'});
writetable(step_metrics, fullfile(outdir, "step_metrics.csv"));

%% 5. Root locus and effect of adding one pole
% Add an extra pole at s = -10.
extra_pole = 10;
G_added_pole = 25/(s*(s + 5)*(s + extra_pole));

figure("Name", "Root Locus Comparison", "Color", "w");
subplot(1, 2, 1);
rlocus(G);
grid on;
title(u([21407 31995 32479 26681 36712 36857]));
xlabel(u([23454 36724]));
ylabel(u([34394 36724]));

subplot(1, 2, 2);
rlocus(G_added_pole);
grid on;
title(u([28155 21152 26497 28857 32 115 61 45 49 48 32 21518 30340 26681 36712 36857]));
xlabel(u([23454 36724]));
ylabel(u([34394 36724]));
saveas(gcf, fullfile(outdir, "root_locus_comparison.png"));

disp(" ");
disp("5) Added one pole at s = -10.");
disp("   Compare root_locus_comparison.png to observe the change.");

%% 6. Bode plot
figure("Name", "Bode Plot", "Color", "w");
bode(G);
grid on;
title(u([24320 29615 31995 32479 32 71 40 115 41 32 30340 20271 24503 22270]));
saveas(gcf, fullfile(outdir, "bode_open_loop.png"));

disp(" ");
disp("6) Bode plot was saved.");
fprintf("\nAll figures were saved in:\n%s\n", outdir);

function text = u(codes)
    text = char(codes);
end
