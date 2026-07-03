# Makefile for Inverter FOC (STM32F4)
# 使用：make DEVICE=STM32F407VG

# ========== 目标芯片 ==========
DEVICE ?= STM32F407VG
CONFIG = Release

# ========== 工具链 ==========
PREFIX = arm-none-eabi-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size
RM      = rm -rf

# ========== 源码目录 ==========
SRC_DIR = src
INC_DIR = inc
LIB_DIR = lib

# ========== 头文件路径 ==========
IPATH  = -I$(INC_DIR)
IPATH += -I$(INC_DIR)/global
IPATH += -I$(INC_DIR)/bsp
IPATH += -I$(INC_DIR)/drv
IPATH += -I$(INC_DIR)/mc
IPATH += -I$(INC_DIR)/app
IPATH += -I$(INC_DIR)/hmi
IPATH += -I$(INC_DIR)/os

# ========== 源文件 ==========
C_SRCS = \
    src/global/global_ctx.c \
    src/global/param_store.c \
    src/global/fault_mgr.c \
    src/bsp/bsp_init.c \
    src/bsp/bsp_interrupt.c \
    src/drv/drv_pwm.c \
    src/drv/drv_adc.c \
    src/drv/drv_gpio.c \
    src/drv/drv_uart.c \
    src/drv/drv_encoder.c \
    src/mc/mc_foc.c \
    src/mc/mc_svpwm.c \
    src/mc/mc_vf.c \
    src/mc/mc_adc_sample.c \
    src/mc/mc_protect.c \
    src/app/app_speed.c \
    src/app/app_pid.c \
    src/app/app_io_logic.c \
    src/app/app_modbus_cmd.c \
    src/hmi/hmi_key.c \
    src/hmi/hmi_display.c \
    src/os/scheduler_bare.c \
    src/main.c

# ========== 汇编源文件 ==========
ASM_SRCS = $(wildcard src/*.S)

# ========== 库 ==========
LIBS = -lm -lc
LIBDIR =

# ========== 编译选项 ==========
DEBUG = 1
OPT = -Og

CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -fno-common -fmessage-length=0
CFLAGS += -fsingle-precision-constant
CFLAGS += $(OPT)
CFLAGS += $(addprefix -D,$(DEFS))
CFLAGS += -Wall -Wextra -Wpedantic
CFLAGS += -fdata-sections -ffunction-sections

ASFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASFLAGS += $(OPT) -x assembler-with-cpp

LDSCRIPT = $(LIB_DIR)/STM32F407VGTx_FLASH.ld

LDFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -specs=nosys.specs -specs=nano.specs
LDFLAGS += $(LIBS)
LDFLAGS += -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--defsym=__FLASH_ORIGIN__=0x08000000
LDFLAGS += -Wl,--defsym=__FLASH_SIZE__=1048576
LDFLAGS += -Wl,--defsym=__RAM_ORIGIN__=0x20000000
LDFLAGS += -Wl,--defsym=__RAM_SIZE__=192k

# ========== 输出目录 ==========
BUILD_DIR = build/$(CONFIG)

# ========== 对象文件 ==========
OBJS = $(patsubst %,$(BUILD_DIR)/%,$(notdir $(C_SRCS:.c=.o)))
OBJS += $(patsubst %,$(BUILD_DIR)/%,$(notdir $(ASM_SRCS:.S=.o)))

# ========== 默认目标 ==========
TARGET = Inverter_FOC

.PHONY: all clean flash debug openocd

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@echo "CC  $<"
	@$(CC) $(CFLAGS) $(IPATH) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	@echo "AS  $<"
	@$(AS) $(ASFLAGS) $(IPATH) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	@echo "LD  $@"
	@$(CC) $(OBJS) $(LDFLAGS) -o $@
	@$(SIZE) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo "HEX $@"
	@$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo "BIN $@"
	@$(OBJCOPY) -O binary -R .eeprom $< $@

$(BUILD_DIR):
	@mkdir -p $@

clean:
	$(RM) $(BUILD_DIR)

# ========== 烧录（ST-Link） ==========
flash: $(BUILD_DIR)/$(TARGET).bin
	@echo "Flashing..."
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x08000000

# ========== 调试（OpenOCD + GDB） ==========
debug: $(BUILD_DIR)/$(TARGET).elf
	@echo "Starting OpenOCD + GDB..."
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	        -c "program $(BUILD_DIR)/$(TARGET).elf reset exit" &
	sleep 2
	$(PREFIX)gdb -ex "target remote localhost:3333" \
	             -ex "load" \
	             -ex "monitor reset halt" \
	             $(BUILD_DIR)/$(TARGET).elf
