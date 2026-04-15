% 实验3：美化与创新版 MATLAB 代码 (加入交点自动识别与高亮)
% 已经替换为你的姓名和学号

figure('Color', 'w', 'Position', [100, 100, 800, 800], 'Name', '实验3 - 创新交点版');

% ==========================================
% 第一个子图：两条曲线及交点
% ==========================================
subplot(2, 1, 1);
% 使用更高的采样率让曲线绝对平滑 (采样率越高，交点计算越精确)
t = 0:0.005:10; 
y1 = 2 * exp(-0.5 * t) .* sin(2 * pi * t);
y2 = sin(t);

% 绘制衰减的"包络线"
env = 2 * exp(-0.5 * t);
plot(t, env, 'Color', [0.8 0.8 0.8], 'LineStyle', ':', 'LineWidth', 1.5); hold on;
plot(t, -env, 'Color', [0.8 0.8 0.8], 'LineStyle', ':', 'LineWidth', 1.5);


p1 = plot(t, y1, 'Color', [0 0.4470 0.7410], 'LineWidth', 2); 

p2 = plot(t, y2, 'Color', [0.8500 0.3250 0.0980], 'LineStyle', '--', 'LineWidth', 1.5);

% ----------------------------------------------------
% 【核心创新】：自动计算并标记交点
% ----------------------------------------------------
% 计算两条曲线的差值 y1 - y2
diff_y = y1 - y2;
% 使用 sign() 函数获取差值符号，diff() 函数判断相邻点符号是否变化
% 当 diff 结果不为 0 时，说明穿过了交点
idx = find(diff(sign(diff_y)) ~= 0); 

% 获取交点的坐标 (因为采样率 0.005 已经极高，直接取近似点即可)
t_inter = t(idx);
y_inter = y1(idx);

% 用带有黑色边缘的金黄色圆点高亮标记交点
p3 = plot(t_inter, y_inter, 'ko', 'MarkerFaceColor', '#FFD700', 'MarkerSize', 6);
% ----------------------------------------------------

grid on;
ax = gca;
ax.GridAlpha = 0.3; 
ax.FontSize = 11;
ax.FontName = 'Microsoft YaHei'; 

title('傅思雄 - 2024030448', 'FontSize', 15, 'FontWeight', 'bold');
xlabel('t', 'FontSize', 12);
ylabel('正弦曲线', 'FontSize', 12);


legend([p1, p2, p3], {'y = 2e^{-0.5t}sin(2\pi t)', 'y = sin(t)', '曲线交点'}, ...
    'Location', 'northeast', 'Box', 'off', 'FontSize', 11);
hold off;


subplot(2, 1, 2);
n = 50; 
[x, y, z] = sphere(n);
s = surf(x, y, z);
shading interp;      
colormap('turbo');   

% 添加光照材质效果，使其看起来像一个真实的、有光泽的实体球
lightangle(-45, 30);        
s.FaceLighting = 'gouraud'; 
s.AmbientStrength = 0.3;    
s.DiffuseStrength = 0.8;    
s.SpecularStrength = 0.9;   
s.SpecularExponent = 25;    

% 设置坐标轴比例和视角
axis equal; 
view(3);    
grid on;
ax3d = gca;
ax3d.GridAlpha = 0.3;
title('三维球面 surf(x,y,z)', 'FontSize', 15, 'FontName', 'Microsoft YaHei', 'FontWeight', 'bold');
xlabel('X 轴'); ylabel('Y 轴'); zlabel('Z 轴');