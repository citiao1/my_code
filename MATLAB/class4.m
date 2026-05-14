% --- 第二题 ---
clear; clc;

% 定义题目给定的四组参数
zeta = [0.3, 0.5, 0.7, 0.8]; % 阻尼比
wn = [1, 2, 3, 6];           % 自然频率

figure(2);
hold on; % 保持图像，用于绘制多条曲线

% 循环遍历四种情况
for i = 1:length(zeta)
    z = zeta(i);
    w = wn(i);
    
    % 定义当前参数下的传递函数
    % 分子: wn^2, 分母: s^2 + 2*zeta*wn*s + wn^2
    num = w^2;
    den = [1, 2*z*w, w^2];
    sys = tf(num, den);
    
    % 绘制阶跃响应
    step(sys);
end

hold off;
grid on;

% 添加图例和标题
legend('\zeta=0.3, \omega_n=1', '\zeta=0.5, \omega_n=2', ...
       '\zeta=0.7, \omega_n=3', '\zeta=0.8, \omega_n=6', ...
       'Location', 'best');
title('四种不同参数下二阶系统的单位阶跃响应');