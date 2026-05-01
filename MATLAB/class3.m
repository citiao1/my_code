clc; clear;


syms y(x) 

ode = x * diff(y, x) + y - exp(x) == 0;
ySol(x) = dsolve(ode);


disp('微分方程的通解为:');
disp(ySol)