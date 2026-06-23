function draw_homework_5_8_5_15()
% Generate clean reference plots and numeric results for control homework.

outDir = fileparts(mfilename('fullpath'));

%% 5-8
w = logspace(-2, 2, 1200);
G58 = 10 ./ ((0.2j*w + 1) .* (1j*w + 2) .* (1j*w + 0.5));
mag58 = 20 * log10(abs(G58));
phase58 = unwrap(angle(G58)) * 180 / pi;

corner58 = [0.5 1 5];
asym58 = 20 * log10(10 / (2 * 0.5)) * ones(size(w));
for c = corner58
    idx = w > c;
    asym58(idx) = asym58(idx) - 20 * log10(w(idx) / c);
end

wc58 = find_crossing(w, abs(G58), 1);
phiwc58 = interp1(w, phase58, wc58);
gamma58 = 180 + phiwc58;

phasefun58 = @(x) -(atan(0.2*x) + atan(0.5*x) + atan(2*x)) * 180 / pi;
wpc58 = fzero(@(x) phasefun58(x) + 180, [2 8]);
magwpc58 = abs(10 ./ ((0.2j*wpc58 + 1) .* (1j*wpc58 + 2) .* (1j*wpc58 + 0.5)));
Kg58 = 1 / magwpc58;
Kgdb58 = 20 * log10(Kg58);

figure('Color','w','Position',[100 100 980 680]);
tiledlayout(2,1,'TileSpacing','compact','Padding','compact');
nexttile;
semilogx(w, mag58, 'b', 'LineWidth', 1.6); hold on;
semilogx(w, asym58, 'r--', 'LineWidth', 1.8);
yline(0, 'k-', 'LineWidth', 1);
for c = corner58
    xline(c, ':', sprintf('\\omega=%.1g', c), 'LabelVerticalAlignment','bottom');
end
xline(wc58, 'm-.', sprintf('\\omega_c=%.2f', wc58), 'LineWidth', 1.2);
xline(wpc58, 'g-.', sprintf('\\omega_g=%.2f', wpc58), 'LineWidth', 1.2);
grid on; xlim([0.03 60]); ylim([-85 25]);
ylabel('20lg|G(j\omega)| / dB');
title('5-8 对数幅频曲线：实线为精确曲线，虚线为渐近线');
legend('精确幅频','渐近线','0 dB','Location','southwest');

nexttile;
semilogx(w, phase58, 'b', 'LineWidth', 1.6); hold on;
yline(-180, 'k-', 'LineWidth', 1);
xline(wc58, 'm-.', sprintf('\\gamma=%.1f^\\circ', gamma58), 'LineWidth', 1.2);
xline(wpc58, 'g-.', sprintf('K_g=%.3g', Kg58), 'LineWidth', 1.2);
for c = corner58
    xline(c, ':');
end
grid on; xlim([0.03 60]); ylim([-280 10]);
xlabel('\omega / (rad/s)'); ylabel('\angle G(j\omega) / deg');
title('5-8 相频曲线');
exportgraphics(gcf, fullfile(outDir, 'fig_5_8_bode.png'), 'Resolution', 180);
close(gcf);

%% 5-9
w = logspace(-2, 3, 1600);
K = 1;
G59 = K * (0.2j*w + 1) ./ ((1j*w).^2 .* (0.02j*w + 1));
mag59 = 20 * log10(abs(G59));
phase59 = unwrap(angle(G59)) * 180 / pi;
wc59 = find_crossing(w, abs(G59), 1);
phiwc59 = interp1(w, phase59, wc59);
gamma59 = 180 + phiwc59;

% PM = 45 deg: atan(0.2w)-atan(0.02w)=45 deg.
rootLow = fzero(@(x) atan(0.2*x) - atan(0.02*x) - pi/4, [1 10]);
Klow = rootLow^2 * sqrt(1 + (0.02*rootLow)^2) / sqrt(1 + (0.2*rootLow)^2);
rootHigh = fzero(@(x) atan(0.2*x) - atan(0.02*x) - pi/4, [20 80]);
Khigh = rootHigh^2 * sqrt(1 + (0.02*rootHigh)^2) / sqrt(1 + (0.2*rootHigh)^2);

figure('Color','w','Position',[100 100 980 680]);
tiledlayout(2,1,'TileSpacing','compact','Padding','compact');
nexttile;
semilogx(w, mag59, 'b', 'LineWidth', 1.6); hold on;
yline(0, 'k-'); xline(wc59, 'm-.', sprintf('\\omega_c=%.2f', wc59));
xline(rootLow, 'r--', sprintf('\\omega=%.2f, K\\approx%.1f', rootLow, Klow));
grid on; xlim([0.03 200]); ylim([-90 90]);
ylabel('20lg|G(j\omega)| / dB');
title('5-9 K=1 时 Bode 图，并标出相角裕度');
nexttile;
semilogx(w, phase59, 'b', 'LineWidth', 1.6); hold on;
yline(-180, 'k-'); yline(-135, 'r--', 'PM=45^\circ 对应相位 -135^\circ');
xline(wc59, 'm-.', sprintf('\\gamma=%.1f^\\circ', gamma59));
xline(rootLow, 'r--');
grid on; xlim([0.03 200]); ylim([-190 -80]);
xlabel('\omega / (rad/s)'); ylabel('\angle G(j\omega) / deg');
exportgraphics(gcf, fullfile(outDir, 'fig_5_9_bode.png'), 'Resolution', 180);
close(gcf);

%% 5-13 Nyquist sketches
wpos = logspace(-3, 3, 1600);
G513_1 = (0.1j*wpos + 1) ./ ((1j*wpos) .* (1j*wpos - 1));
G513_3 = 1 ./ ((1j*wpos) .* (0.1j*wpos + 1) .* (0.25j*wpos + 1));

figure('Color','w','Position',[100 100 1080 450]);
tiledlayout(1,2,'TileSpacing','compact','Padding','compact');
nexttile;
plot(real(G513_1), imag(G513_1), 'b', 'LineWidth', 1.5); hold on;
plot(real(G513_1), -imag(G513_1), 'b--', 'LineWidth', 1.0);
plot(-0.1, 0, 'ro', 'MarkerFaceColor','r');
xline(0,'k-'); yline(0,'k-'); grid on; axis equal;
xlim([-1.2 0.4]); ylim([-1.0 1.0]);
title('5-13(1) 归一化 Nyquist 图，临界点为 -1/K=-0.1');
xlabel('Re'); ylabel('Im');
text(-0.1, 0.08, '-0.1');

nexttile;
plot(real(G513_3), imag(G513_3), 'b', 'LineWidth', 1.5); hold on;
plot(real(G513_3), -imag(G513_3), 'b--', 'LineWidth', 1.0);
plot(-1/14, 0, 'ro', 'MarkerFaceColor','r');
xline(0,'k-'); yline(0,'k-'); grid on; axis equal;
xlim([-0.25 0.08]); ylim([-0.25 0.25]);
title('5-13(3) 归一化 Nyquist 图，临界点为 -1/K=-1/14');
xlabel('Re'); ylabel('Im');
text(-1/14, 0.025, '-1/14');
exportgraphics(gcf, fullfile(outDir, 'fig_5_13_nyquist.png'), 'Resolution', 180);
close(gcf);

%% 5-14 schematic from given Nyquist graph
figure('Color','w','Position',[100 100 900 620]);
hold on; grid on; axis equal;
xline(0, 'k-', 'LineWidth', 1.1); yline(0, 'k-', 'LineWidth', 1.1);
plot([-55 -40 -20 -1 -0.05 0], [0 -24 0 -13 0 0], 'b-', 'LineWidth', 2);
plot([-55 -40 -20 -1 -0.05 0], [0 24 0 13 0 0], 'b--', 'LineWidth', 1.4);
plot([-50 -20 -1 -0.05], [0 0 0 0], 'ko', 'MarkerFaceColor','w');
plot(-1, 0, 'ro', 'MarkerFaceColor','r', 'MarkerSize', 8);
text(-50, -4, '-50'); text(-20, 3, '-20'); text(-1, -5, '-1'); text(-0.05, 3, '-0.05');
text(-52, 26, 'K=500 时题图形状'); text(-1.0, 6, '(-1,j0)');
xlim([-60 8]); ylim([-32 32]);
xlabel('Re'); ylabel('Im');
title('5-14 Nyquist 判据示意：改变 K 时曲线按 K/500 等比例缩放');
annotation('textbox',[0.16 0.02 0.72 0.12], 'String', ...
    '临界增益：K=10, 25, 10000；稳定区间：0<K<10 或 25<K<10000；K<10 中题目问“小于何值不稳定”对应边界为 K=10。', ...
    'FitBoxToText','on', 'EdgeColor','none', 'FontSize', 11);
exportgraphics(gcf, fullfile(outDir, 'fig_5_14_nyquist.png'), 'Resolution', 180);
close(gcf);

%% 5-15 closed-loop frequency response
gamma15 = 36;
x = tand(90 - gamma15);
wc15 = 100 / sqrt(1 + x^2);
T15 = x / wc15;
zeta15 = 1 / (20 * sqrt(T15));
Mr15 = 1 / (2 * zeta15 * sqrt(1 - zeta15^2));
wn15 = sqrt(100 / T15);
wr15 = wn15 * sqrt(1 - 2*zeta15^2);

w = logspace(0, 3, 1200);
Phi = 100 ./ (T15*(1j*w).^2 + 1j*w + 100);
figure('Color','w','Position',[100 100 900 560]);
semilogx(w, abs(Phi), 'b', 'LineWidth', 1.8); hold on;
plot(wr15, Mr15, 'ro', 'MarkerFaceColor','r');
xline(wr15, 'r--', sprintf('\\omega_r=%.1f', wr15));
yline(Mr15, 'r--', sprintf('M_r=%.2f', Mr15));
grid on; xlim([1 500]); ylim([0 1.9]);
xlabel('\omega / (rad/s)'); ylabel('|\Phi(j\omega)|');
title(sprintf('5-15 闭环幅频特性：T=%.4f s, M_r=%.2f', T15, Mr15));
exportgraphics(gcf, fullfile(outDir, 'fig_5_15_closed_loop.png'), 'Resolution', 180);
close(gcf);

%% Save plain-text results
fid = fopen(fullfile(outDir, 'homework_5_8_5_15_results.txt'), 'w');
fprintf(fid, '5-8: wc=%.4f rad/s, phase(wc)=%.4f deg, gamma=%.4f deg, w_phase=%.4f rad/s, Kg=%.4f, Kg_dB=%.4f dB\n', wc58, phiwc58, gamma58, wpc58, Kg58, Kgdb58);
fprintf(fid, '5-9: K=1, wc=%.4f rad/s, gamma=%.4f deg. PM=45 deg: low-frequency solution wc=%.4f, K=%.4f; high-frequency solution wc=%.4f, K=%.4f.\n', wc59, gamma59, rootLow, Klow, rootHigh, Khigh);
fprintf(fid, '5-13(1): stable K>10; unstable K<10; K=10 critical.\n');
fprintf(fid, '5-13(3): stable 0<K<14; unstable K>14 or K<=0; K=14 critical.\n');
fprintf(fid, '5-14: stable 0<K<10 or 25<K<10000; K=10,25,10000 critical. Therefore K<10 boundary is 10.\n');
fprintf(fid, '5-15: T=%.6f s, zeta=%.6f, Mr=%.6f, wr=%.6f rad/s.\n', T15, zeta15, Mr15, wr15);
fclose(fid);
end

function wc = find_crossing(w, y, level)
idx = find((y(1:end-1)-level) .* (y(2:end)-level) <= 0, 1, 'first');
if isempty(idx)
    error('No crossing found.');
end
wc = interp1(log(y(idx:idx+1)), log(w(idx:idx+1)), log(level), 'linear');
wc = exp(wc);
end
