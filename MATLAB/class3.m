
clear; clc; close all;


num_G1 = 20;
den_G1 = [1, 5, 4]; 
G1 = tf(num_G1, den_G1);


H1 = tf(0.449, 1); 

sys_inner = feedback(G1, H1);


num_G2 = 1;
den_G2 = [1, 0];
G2 = tf(num_G2, den_G2);


sys_open = series(sys_inner, G2);


sys_closed = feedback(sys_open, 1);

figure(1);
step(sys_closed);
title('Exam4\_7: Step Response'); 
grid on;



zeta = [0.3, 0.5, 0.7, 0.8]; 
wn = [1, 2, 3, 6];           


figure(2);
hold on; 

for i = 1:4
    z = zeta(i);
    w = wn(i);

    num = w^2;
    den = [1, 2*z*w, w^2];
    sys_2nd = tf(num, den);
    
    step(sys_2nd);
end

hold off;
grid on;
title('Step Response of Second-Order Systems');
legend('\zeta=0.3, \omega_n=1', '\zeta=0.5, \omega_n=2', ...
       '\zeta=0.7, \omega_n=3', '\zeta=0.8, \omega_n=6');