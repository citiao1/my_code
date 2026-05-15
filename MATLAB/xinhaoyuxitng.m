% 6. 绘制图形
figure('Color', 'white');

% 绘制抽样后的幅度谱
plot(w, abs(Fs), 'b-', 'LineWidth', 1.5); 
hold on;

% 绘制外侧虚线包络
plot(w, Envelope, 'r--', 'LineWidth', 1.5); 

% ================== 核心修改：细分横坐标 ==================

% 1. 限制横坐标的显示范围（可选：比如只看 -3ws 到 3ws，让中心细节更大）
% xlim([-3*ws, 3*ws]); 

% 2. 细分刻度：每隔 0.5*ws 画一个刻度
tick_values = -5*ws : 0.5*ws : 5*ws; 
xticks(tick_values);

% 3. 优化标签：将纯数字坐标转换为 "倍数 + ws" 的文字格式
tick_labels = strings(1, length(tick_values));
for i = 1:length(tick_values)
    multiple = tick_values(i) / ws;
    if multiple == 0
        tick_labels(i) = "0";
    elseif multiple == 1
        tick_labels(i) = "\omega_s";
    elseif multiple == -1
        tick_labels(i) = "-\omega_s";
    else
        tick_labels(i) = num2str(multiple) + "\omega_s";
    end
end
xticklabels(tick_labels);

% =======================================================

% 画垂直辅助线：在整数倍 ws 处画黑色的点状线，方便对齐波峰
for k = -5:5
    xline(k*ws, 'k:', 'HandleVisibility', 'off'); 
end

% 图形美化
title('抽样信号幅频特性 |F_s(\omega)|', 'FontSize', 14, 'FontWeight', 'bold');
xlabel('角频率 \omega', 'FontSize', 12);
ylabel('幅度 |F_s(\omega)|', 'FontSize', 12);
legend('抽样频谱 |F_s(\omega)|', '包络线 E(\omega)', 'FontSize', 11);
grid on;
hold off;