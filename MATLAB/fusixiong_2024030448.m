A = [7.5  3.5  0    0; 
     8    33   4.1  0; 
     0    9    103 -1.5; 
     0    0    3.7  19.3];

B = [5  7  6  5; 
     7  10 8  7; 
     6  8  10 9; 
     5  7  9  10];
C = -2;
save('datafile.mat');
sqrt_C = sqrt(C);
disp('C的平方根为:');
disp(sqrt_C);