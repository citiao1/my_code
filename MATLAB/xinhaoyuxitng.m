% =========================================================
% 实验四：传递函数的零极点对系统过渡过程的影响
% =========================================================
clear; clc; close all;

% 基础参数设定
zeta = 0.5;
wn = 1;

% 标准二阶系统传递函数 (T=0, tau=0)
num0 = wn^2;
den0 = [1, 2*zeta*wn, wn^2];
sys0 = tf(num0, den0);

%% ================== 实验内容1：增加闭环极点的影响 ==================
% 选取 T 的值满足实验要求
T1 = 0.2; % 1/T = 5   (> 2.5) 
T2 = 2.0; % 1/T = 0.5 (= 0.5)
T3 = 5.0; % 1/T = 0.2 (< 0.5)

% 构建增加极点后的系统
sys_p1 = tf(1, [T1, 1]) * sys0;
sys_p2 = tf(1, [T2, 1]) * sys0;
sys_p3 = tf(1, [T3, 1]) * sys0;
sys_1st = tf(1, [T3, 1]); % 一阶系统对比

% 绘图：极点影响
figure('Name', '增加闭环极点对系统的影响', 'Position', [100, 100, 1200, 500], 'Color', 'w');

% 1.1 阶跃响应对比
subplot(1, 2, 1);
step(sys0, sys_p1, sys_p2, sys_p3, sys_1st, 20);
title('增加闭环极点 - 单位阶跃响应', 'FontSize', 14);
legend('标准二阶 (T=0)', '远极点 (T=0.2)', '相近极点 (T=2)', '主导极点 (T=5)', '一阶系统 (T=5)', 'Location', 'best');
grid on; set(findobj(gca,'type','line'),'LineWidth',1.5);

% 1.2 零极点分布对比
subplot(1, 2, 2);
pzmap(sys0, sys_p1, sys_p2, sys_p3);
title('增加闭环极点 - 零极点分布图', 'FontSize', 14);
legend('标准二阶', '远极点 (T=0.2)', '相近极点 (T=2)', '主导极点 (T=5)', 'Location', 'best');
grid on; set(findobj(gca,'type','line'),'LineWidth',1.5);


%% ================== 实验内容2：增加闭环零点的影响 ==================
% 选取 tau 的值满足实验要求
tau1 = 0.2; % 1/tau = 5   (> 2.5)
tau2 = 2.0; % 1/tau = 0.5 (= 0.5)
tau3 = 5.0; % 1/tau = 0.2 (< 0.5)

% 构建增加零点后的系统
sys_z1 = tf([tau1, 1], 1) * sys0;
sys_z2 = tf([tau2, 1], 1) * sys0;
sys_z3 = tf([tau3, 1], 1) * sys0;

% 绘图：零点影响
figure('Name', '增加闭环零点对系统的影响', 'Position', [150, 150, 1200, 500], 'Color', 'w');

% 2.1 阶跃响应对比
subplot(1, 2, 1);
step(sys0, sys_z1, sys_z2, sys_z3, 15);
title('增加闭环零点 - 单位阶跃响应', 'FontSize', 14);
legend('标准二阶 (\tau=0)', '远零点 (\tau=0.2)', '相近零点 (\tau=2)', '主导零点 (\tau=5)', 'Location', 'best');
grid on; set(findobj(gca,'type','line'),'LineWidth',1.5);

% 2.2 零极点分布对比
subplot(1, 2, 2);
pzmap(sys0, sys_z1, sys_z2, sys_z3);
title('增加闭环零点 - 零极点分布图', 'FontSize', 14);
legend('标准二阶', '远零点 (\tau=0.2)', '相近零点 (\tau=2)', '主导零点 (\tau=5)', 'Location', 'best');
grid on; set(findobj(gca,'type','line'),'LineWidth',1.5);