% 单独绘制 K* = 4 时，以 a 为参变量的根轨迹 (带箭头与渐近线)
clear; clc; close all;

figure('Position', [200, 150, 700, 500], 'Name', 'K*=4 详细参数根轨迹');
hold on; grid on;
axis equal; % 保持等比例，确保圆是圆

% 1. 构造等效传递函数 Ga(s) = s / (s^2 + 4)
sys_K4 = tf([1, 0], [1, 0, 4]); 

% 获取底层数据以绘制箭头
[r, k] = rlocus(sys_K4);

% 2. 绘制渐近线 (180度，沿负实轴)
% 我们用显眼的黑色点划线画出这条渐近线
plot([-10, 0], [0, 0], 'k-.', 'LineWidth', 1.5, 'DisplayName', '渐近线 (180^\circ)');

% 3. 绘制实际的根轨迹 (蓝色实线)
plot(real(r(1,:)), imag(r(1,:)), 'b-', 'LineWidth', 2, 'DisplayName', '参数根轨迹');
plot(real(r(2,:)), imag(r(2,:)), 'b-', 'LineWidth', 2, 'HandleVisibility', 'off');

% 4. 沿着轨迹添加方向箭头
% 我们从轨迹数据中选取特定的索引点来画箭头，展示极点的流动方向
% 选取圆弧上的点和实轴上的点
arrow_indices = [15, 30, 42, 60, 80]; 
arrow_L = 0.35; % 箭头长度

for i = 1:size(r, 1) % 遍历两条分支
    for idx = arrow_indices
        if idx < length(k) - 2
            p_curr = r(i, idx);
            p_next = r(i, idx+2); 
            
            dx = real(p_next) - real(p_curr);
            dy = imag(p_next) - imag(p_curr);
            
            len = sqrt(dx^2 + dy^2);
            if len > 1e-3
                ux = (dx / len) * arrow_L;
                uy = (dy / len) * arrow_L;
                
                % 画箭头 (不参与图例显示)
                quiver(real(p_curr), imag(p_curr), ux, uy, ...
                    0, 'MaxHeadSize', 0.8, 'Color', 'b', 'LineWidth', 1.5, 'HandleVisibility', 'off');
            end
        end
    end
end

% 5. 绘制几何基准圆 x^2 + y^2 = 4 作为对比
theta = linspace(0, 2*pi, 100);
plot(2*cos(theta), 2*sin(theta), 'k--', 'LineWidth', 1, 'DisplayName', '基准圆 x^2+y^2=4');

% 6. 标记核心特征点
plot(0, 2, 'rx', 'MarkerSize', 10, 'LineWidth', 2, 'DisplayName', '起点 \pm j2 (a=0)');
plot(0, -2, 'rx', 'MarkerSize', 10, 'LineWidth', 2, 'HandleVisibility','off');
plot(0, 0, 'ro', 'MarkerSize', 10, 'LineWidth', 2, 'DisplayName', '终点 0 (a\rightarrow\infty)');
plot(-2, 0, 'k*', 'MarkerSize', 10, 'LineWidth', 1.5, 'DisplayName', '会合点 (-2, 0)');

% 7. 图表美化与视角锁定
title('当 K^* = 4 时，参数 a 从 0\rightarrow\infty 的根轨迹 (含箭头与渐近线)');
xlabel('实轴 (Real Axis)');
ylabel('虚轴 (Imaginary Axis)');

% 将 X 轴向左多留一点空间，以便看清楚渐近线和趋于无穷的箭头
axis([-6, 3, -3, 3]); 
xline(0, 'k-', 'HandleVisibility', 'off'); 
yline(0, 'k-', 'HandleVisibility', 'off');
legend('Location', 'northeastoutside'); % 将图例放外面防止遮挡图形
hold off;