# 变频器FOC软件架构思维导图

> GitHub支持Mermaid渲染，直接打开此文件即可查看思维导图。

```mermaid
flowchart TB
    subgraph ROOT["🏠 变频器FOC软件架构"]
        direction TB
    end

    subgraph HMI["🖥️ HMI层（人机交互）<br/><small>10ms · 按键/显示/参数编辑</small>"]
        H1["📋 按键扫描<br/>key_scan.c<br/>消抖/长短按/矩阵键盘"]
        H2["🖼️ LCD显示<br/>lcd_display.c<br/>运行状态页/参数页/故障页"]
        H3["✏️ 参数编辑<br/>param_edit.c<br/>数值增减/写入校验"]
        H4["📡 状态推送<br/>status_push.c<br/>定时刷新显示缓冲区"]
    end

    subgraph APP["⚙️ APP层（应用逻辑）<br/><small>1ms · 工艺业务 · 不直接操作PWM</small>"]
        A1["🚀 加减速控制<br/>app_speed.c<br/>频率斜坡/多源给定优先级<br/>MODBUS>多段速>AI>面板"]
        A2["🎯 多段速逻辑<br/>app_step_speed.c<br/>DI端子组合8段预设速度"]
        A3["🔄 PID工艺调节<br/>app_pid.c<br/>恒压供水/恒张力闭环<br/>输出叠加至频率给定"]
        A4["🔌 DI/DO逻辑<br/>app_io_logic.c<br/>启动/停止/反转/急停<br/>运行接触器/故障继电器/风扇"]
        A5["📡 Modbus指令<br/>app_modbus_cmd.c<br/>0x03读/0x06写/0x10批量写<br/>寄存器映射表"]
        A6["⏰ 定时工艺<br/>app_timer_logic.c<br/>定时运行/睡眠停机/风扇启停"]
    end

    subgraph MC["🔥 MC层（电机控制核心）<br/><small>10kHz电流环 · 1kHz速度环 · 中断驱动</small>"]
        direction LR
        subgraph MC_FOC["FOC矢量闭环控制"]
            M1["📊 ADC采样<br/>mc_adc_sample.c<br/>Ia/Ib/Udc/NTC/AI<br/>均值滤波/物理量换算"]
            M2["🔄 Clarke变换<br/>Iα=Ia<br/>Iβ=(Ia+2Ib)/√3"]
            M3["🔄 Park变换<br/>Id=Iαcosθ+Iβsinθ<br/>Iq=-Iαsinθ+Iβcosθ"]
            M4["🎛️ 电流环PI<br/>Id_PI → Vd<br/>Iq_PI → Vq<br/>离散PI+积分防饱和"]
            M5["🔄 反Park变换<br/>Vα=Vd·cosθ-Vq·sinθ<br/>Vβ=Vd·sinθ+Vq·cosθ"]
            M6["📐 SVPWM调制<br/>mc_svpwm.c<br/>扇区判断→矢量时间计算<br/>七段式对称插入<br/>过调制钳位"]
        end
        subgraph MC_VF["V/F开环控制"]
            M7["📈 V/F曲线<br/>mc_vf.c<br/>压频比曲线+低频转矩补偿"]
            M8["📐 SVPWM<br/>直接Vα=V·cosθ<br/>Vβ=V·sin(θ)"]
        end
        subgraph MC_PROT["保护判断<br/>mc_protect.c"]
            P1["⚡ 瞬时过流<br/>Imax>1.5×I_rated<br/>立即封锁PWM"]
            P2["🔋 过压/欠压<br/>Udc>750V / <350V"]
            P3["🌡️ IGBT过温<br/>NTC_temp>85°C"]
            P4["🔥 反时限过载<br/>I²t曲线 120% 60s"]
            P5["🔒 堵转检测<br/>speed≈0 + Iq>80% 持续2s"]
        end
    end

    subgraph DRV["📦 DRV层（外设驱动抽象）<br/><small>隔离硬件寄存器 · 算法层不直操寄存器</small>"]
        D1["⚡ PWM输出<br/>drv_pwm.c<br/>TIM1三相互补PWM<br/>中心对齐/死区100ns<br/>MOE安全封锁"]
        D2["📊 ADC采样<br/>drv_adc.c<br/>6通道DMA循环采集<br/>12位精度/479周期采样"]
        D3["🔌 GPIO输入输出<br/>drv_gpio.c<br/>DI消抖/ DO继电器驱动<br/>RS-485 DE方向控制"]
        D4["📻 UART通讯<br/>drv_uart.c<br/>环形缓冲区256B<br/>收发中断/RS-485自动方向"]
        D5["📏 编码器计数<br/>drv_encoder.c<br/>TIM4正交编码模式<br/>4倍频/M法测速/滑动均值"]
        D6["💾 EEPROM存储<br/>drv_eeprom.c<br/>Flash/AT24CXX<br/>参数掉电存储"]
        D7["🌡️ 温度换算<br/>drv_temperature.c<br/>NTC Beta方程换算"]
    end

    subgraph BSP["🖥️ BSP层（底层硬件）<br/><small>无业务逻辑 · 仅寄存器初始化</small>"]
        B1["🔧 系统初始化<br/>bsp_init.c<br/>时钟PLL 168MHz<br/>GPIO/ADC/PWM/UART"]
        B2["⚡ 中断向量表<br/>bsp_interrupt.c<br/>TIM1_UP / ADC_DMA<br/>USART1 / EXTI"]
        B3["📍 硬件引脚映射<br/>bsp_hw_config.h<br/>功率板/面板接口区分"]
    end

    subgraph OS["⏱️ OS层（调度内核）<br/><small>裸机分时调度 或 FreeRTOS</small>"]
        subgraph BARE["方案A：裸机调度器"]
            O1["🕐 1ms时基<br/>SysTick/TIM2"]
            O2["📋 任务列表<br/>优先级0~8<br/>周期1ms~100ms"]
            O3["🔄 分时轮询<br/>main()循环<br/> Cooperative"]
        end
        subgraph RTOS["方案B：FreeRTOS"]
            O4["🎯 任务MotorControl<br/>优先级9 · 1ms<br/>FOC电流环"]
            O5["⚙️ 任务AppLogic<br/>优先级8 · 1ms"]
            O6["📡 任务CommModbus<br/>优先级6 · 5ms"]
            O7["🖥️ 任务HMI<br/>优先级4 · 10ms"]
            O8["📊 任务SlowMonitor<br/>优先级2 · 100ms"]
        end
    end

    subgraph GLOBAL["🌐 全局公共层（贯穿所有模块）"]
        G1["📦 GlobalCtx全局上下文<br/>run_state / fault_code / run_mode<br/>电机参数 / 采样数据 / IO状态"]
        G2["💾 参数存储<br/>param_store.c<br/>ParamGroup / 默认值 / 校验CRC"]
        G3["🚨 故障管理<br/>fault_mgr.c<br/>32条环形缓冲区/故障锁定/历史记录"]
        G4["📋 全局定义<br/>global_def.h<br/>枚举/宏/参数索引/硬件引脚"]
    end

    %% 连接关系：单向依赖，自上而下
    HMI --> APP
    APP --> MC
    MC --> DRV
    DRV --> BSP
    GLOBAL --> HMI
    GLOBAL --> APP
    GLOBAL --> MC
    GLOBAL --> DRV
    GLOBAL --> BSP

    %% MC层内部关系
    M1 --> M2
    M2 --> M3
    M3 --> M4
    M4 --> M5
    M5 --> M6
    M1 --> P1
    M1 --> P2
    M1 --> P3
    M1 --> P4
    M1 --> P5
    M6 --> D1

    M7 --> M8
    M8 --> M6

    APP --> A1 & A2 & A3 & A4 & A5 & A6

    %% OS到各层
    O1 --> O2
    O2 --> O3

    %% 图例说明
    subgraph LEGEND["📌 图例"]
        L1["───→ 单向依赖调用"]
        L2["─ ─ ─ → 数据流/触发"]
    end

    style HMI fill:#e1f5fe,stroke:#01579b
    style APP fill:#e8f5e9,stroke:#2e7d32
    style MC fill:#fff3e0,stroke:#e65100
    style DRV fill:#f3e5f5,stroke:#6a1b9a
    style BSP fill:#eceff1,stroke:#546e7a
    style OS fill:#efebe9,stroke:#5d4037
    style MC_FOC fill:#ffccbc,stroke:#bf360c
    style MC_VF fill:#ffe0b2,stroke:#e65100
    style MC_PROT fill:#ffcdd2,stroke:#b71c1c
    style BARE fill:#d7ccc8,stroke:#4e342e
    style RTOS fill:#bcaaa4,stroke:#3e2723
    style GLOBAL fill:#fffde7,stroke:#f57f17
    style ROOT fill:#ffffff,stroke:#ffffff
    style LEGEND fill:#f5f5f5,stroke:#9e9e9e
```

---

## 核心调用链路速览

```
启动流程:
main() 
  └─► BSP_Init()              // 硬件初始化
       └─► GlobalCtx_Init()   // 全局上下文
            └─► Param_Load()  // 参数加载
                 └─► 各层Init()函数
                      └─► Scheduler_Run()  // 永不返回

运行数据流:
  给定源(面板/Modbus/AI/多段速)
       │
       ▼
  APP_Speed_Task() ──► 频率斜坡
       │
       ▼
  MC_VF_Control()  或  MC_FOC_CurrentLoop()
       │
       ├──► SVPWM_Calc() ──► DRV_PWM_SetDuty() ──► TIM1输出
       │
       ▼
  MC_Protect_Check() ──► 故障? ──► DRV_PWM_Disable()
       │
       ▼
  APP_IO_Task() ──► DO输出/故障继电器
       │
       ▼
  HMI_DISP_Update() ──► LCD显示

中断时序:
  TIM1_UP (10kHz) ──► MC_FOC_CurrentLoop() ──► SVPWM
  TIM2 (1ms)      ──► MC_Protect_Check() + APP任务
  SysTick (1ms)   ──► Scheduler_TickISR()
```

---

## 六大核心模块速记

| 层级 | 模块 | 核心文件 | 调用频率 | 职责 |
|------|------|----------|----------|------|
| 🖥️ HMI | 按键显示 | hmi_key.c / hmi_display.c | 10ms | 人机交互入口 |
| ⚙️ APP | 速度/PID/IO | app_speed.c / app_pid.c / app_io_logic.c | 1ms | 工艺逻辑 |
| 🔥 MC | FOC/SVPWM | mc_foc.c / mc_svpwm.c / mc_vf.c | 10kHz | 电机算法核心 |
| 📦 DRV | PWM/ADC/UART | drv_pwm.c / drv_adc.c / drv_uart.c | 中断级 | 硬件抽象 |
| 🖥️ BSP | 初始化 | bsp_init.c / bsp_interrupt.c | 启动1次 | 寄存器配置 |
| ⏱️ OS | 调度器 | scheduler_bare.c | 1ms tick | 任务分时 |
