# Test Framework

测试目录用于证明需求已实现，不以“电机能转”作为唯一成功标准。

## 测试层级
1. Unit：换相表、时间换算、PI、滤波、状态机和限幅。
2. Simulation：BEMF、去磁、噪声、开环加速和闭环响应。
3. Hardware：PWM、死区、ADC、CMP、启动、换相和速度闭环。
4. Fault Injection：逐波过流、堵转、欠压、过压、过温、缺相、超速、反转。

状态：NOT RUN / PASS / FAIL / BLOCKED / N/A。

## 安全门槛
在确认母线额定值、限流设置、急停、放电和硬件过流关断前，不执行高能量功率测试。
