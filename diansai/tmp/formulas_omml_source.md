$$e_y=\operatorname{sat}_{[-1,1]}\left\{\frac{\sum (w_i b_i)}{3.5\sum b_i}\right\}$$

$$\omega_d(k)=\operatorname{sat}\left\{K_{p,e}e_y(k)+K_{d,e}\frac{e_y(k)-e_y(k-1)}{T_s}\right\}$$

$$\Delta v=K_{p,\omega}(\omega_d-\omega_z)+K_{i,\omega}\int(\omega_d-\omega_z)\,\mathrm{d}t$$

$$v_L^*=v_0+0.4\Delta v,\qquad v_R^*=v_0-0.4\Delta v$$

$$\Delta u(k)=K_p[e(k)-e(k-1)]+K_i e(k)T_s+K_d\frac{e(k)-2e(k-1)+e(k-2)}{T_s}$$

$$h(\varphi)\approx r[\sin(\varphi+\varphi_0)-\sin\varphi_0],\qquad \theta(\varphi)=\arctan\left[\frac{h(\varphi)}{L}\right]\approx\frac{h(\varphi)}{L}$$

$$e(k)=x_r-x(k)$$

$$\dot e_f(k)=\operatorname{LPF}\left\{\frac{e(k)-e(k-1)}{T_k}\right\}$$

$$\theta_d(k)=\operatorname{sat}_{[-\theta_{\max},\theta_{\max}]}\left\{s[K_p e(k)+K_d\dot e_f(k)]\right\}$$
