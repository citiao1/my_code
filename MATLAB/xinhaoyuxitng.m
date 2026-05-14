% 题目 3-28: 抽样信号幅度频谱绘制
clear; clc; close all;

% 1. 定义基本参数
Wm = 3000*pi;        % 原信号最大角频率
Ws = 6000*pi;        % 抽样角频率 (临界抽样)
T_max = 1 / 3000;    % 最大抽样间隔
scale = 1 / T_max;   % 抽样导致的幅度缩放因子 1/T

% 2. 构造频率轴 (绘制 -2.5Ws 到 2.5Ws 的范围)
w = linspace(-2.5*Ws, 2.5*Ws, 10000);
Fs_mag = zeros(size(w)); % 初始化幅度谱

% 3. 构造等腰梯形并进行周期延拓
% 原频谱最大高度理论值
H_base = 0.5e-3; 
H_scaled = H_base * scale; % 抽样后的最大高度 (1.5)

% 叠加几个周期的梯形 (k表示平移的周期数)
for k = -3:3
    wk = w - k*Ws; % 频移
    
    % 生成单个梯形脉冲
    trapz = zeros(size(wk));
    
    % 平顶部分: [-1000pi, 1000pi]
    idx_top = abs(wk) <= 1000*pi;
    trapz(idx_top) = H_scaled;
    
    % 斜坡部分: [1000pi, 3000pi] 和 [-3000pi, -1000pi]
    idx_slope = (abs(wk) > 1000*pi) & (abs(wk) <= 3000*pi);
    trapz(idx_slope) = H_scaled * (3000*pi - abs(wk(idx_slope))) / (2000*pi);
    
    % 累加到总频谱中
    Fs_mag = Fs_mag + trapz;
end

% 4. 绘图配置
figure('Name', '抽样信号的幅度频谱', 'Position', [100, 100, 900, 400]);
plot(w / pi, Fs_mag, 'b', 'LineWidth', 2); % X轴除以pi方便显示
title('抽样间隔 T = T_{max} 时, f_s(t) 的幅度频谱 |F_s(\Omega)|');
xlabel('\Omega / \pi (rad/s)');
ylabel('幅度 |F_s(\Omega)|');

% 设置X轴刻度
xticks([-18000, -12000, -6000, -3000, -1000, 0, 1000, 3000, 6000, 12000, 18000]);
xticklabels({'-18000', '-12000', '-6000', '-3000', '-1000', '0', '1000', '3000', '6000', '12000', '18000'});

% 设置Y轴和网格
ylim([0 1.8]);
yticks([0 1.5]);
grid on;

% 添加注释标明关键点
hold on;
plot([-1000, 1000], [1.5, 1.5], 'r--', 'LineWidth', 1);
text(0, 1.55, '最大高度 = 1.5', 'HorizontalAlignment', 'center', 'Color', 'r');