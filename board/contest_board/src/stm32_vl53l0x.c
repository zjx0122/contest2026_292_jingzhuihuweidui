/****************************************************************************
 * boards/arm/stm32/stm32f407_health/src/stm32_vl53l0x.c
 *
 * VL53L0X 激光测距传感器驱动
 * I2C1: PB6=SCL, PB9=SDA, 地址 0x29
 * XSHUT: PC5, GPIO1: PC4
 *
 * 基于 packages/demos/sensor_transfer/tof.c 移植
 ****************************************************************************/

#include <nuttx/config.h>
#include <errno.h>
#include <debug.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#include <nuttx/i2c/i2c_master.h>
#include "stm32_i2c.h"
#include "stm32_gpio.h"
#include "stm32.h"
#include <arch/board/board.h>

#ifdef CONFIG_SENSORS_VL53L0X

#define VL53L0X_I2C_ADDR        0x29
#define VL53L0X_I2C_FREQ        400000

#define REG_IDENTIFICATION_MODEL_ID          0xc0
#define REG_IDENTIFICATION_REVISION_ID       0xc2
#define REG_SYSRANGE_START                   0x00
#define RESULT_RANGE_STATUS                  0x14
#define REG_MSRC_CONFIG_CONTROL              0x60
#define FINAL_RANGE_CONFIG_VCSEL_PERIOD      0x70
#define FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x71
#define MSRC_CONFIG_TIMEOUT_MACROP           0x46
#define FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define SYSTEM_SEQUENCE_CONFIG               0x01
#define SYSTEM_INTERRUPT_CONFIG_GPIO         0x0A
#define SYSTEM_INTERRUPT_CLEAR               0x0B
#define RESULT_INTERRUPT_STATUS              0x13
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_0     0xB0
#define GPIO_HV_MUX_ACTIVE_HIGH              0x84
#define PRE_RANGE_CONFIG_VCSEL_PERIOD        0x50
#define PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI   0x51

#define SEQUENCE_ENABLE_PRE_RANGE    0x40
#define calcMacroPeriod(p) ((((uint32_t)2304*(p)*1655)+500)/1000)
#define encodeVcselPeriod(p) (((p)>>1)-1)

typedef enum { VcselPeriodPreRange, VcselPeriodFinalRange } vcselPeriodType;

typedef struct
{
  uint16_t pre_range_vcsel_period_pclks;
  uint16_t final_range_vcsel_period_pclks;
  uint16_t msrc_dss_tcc_mclks;
  uint16_t pre_range_mclks;
  uint16_t final_range_mclks;
  uint32_t msrc_dss_tcc_us;
  uint32_t pre_range_us;
  uint32_t final_range_us;
} sequence_step_timeouts_t;

static struct i2c_master_s *g_vl53l0x_i2c = NULL;
static struct i2c_config_s g_vl53l0x_config =
{
  .frequency = VL53L0X_I2C_FREQ,
  .address   = VL53L0X_I2C_ADDR,
  .addrlen   = 7
};

static uint8_t g_stop_variable;
static uint32_t g_measurement_timing_budget_us;
static sem_t g_vl53l0x_sem;

static int vl53l0x_gpio1_isr(int irq, void *context, void *arg)
{
  sem_post(&g_vl53l0x_sem);
  return OK;
}

/* I2C 寄存器操作 */

static uint8_t vl53l0x_read_reg(uint8_t reg)
{
  uint8_t value;
  i2c_write(g_vl53l0x_i2c, &g_vl53l0x_config, &reg, 1);
  i2c_read(g_vl53l0x_i2c, &g_vl53l0x_config, &value, 1);
  return value;
}

static uint16_t vl53l0x_read_reg16(uint8_t reg)
{
  uint8_t buf[2];
  i2c_write(g_vl53l0x_i2c, &g_vl53l0x_config, &reg, 1);
  i2c_read(g_vl53l0x_i2c, &g_vl53l0x_config, buf, 2);
  return (uint16_t)buf[0] << 8 | buf[1];
}

static void vl53l0x_write_reg(uint8_t reg, uint8_t value)
{
  uint8_t buf[2] = {reg, value};
  i2c_write(g_vl53l0x_i2c, &g_vl53l0x_config, buf, 2);
}

static void vl53l0x_write_reg16(uint8_t reg, uint16_t value)
{
  uint8_t buf[3] = {reg, (value >> 8) & 0xff, value & 0xff};
  i2c_write(g_vl53l0x_i2c, &g_vl53l0x_config, buf, 3);
}

static void vl53l0x_read_multi(uint8_t reg, uint8_t *buf, int count)
{
  i2c_write(g_vl53l0x_i2c, &g_vl53l0x_config, &reg, 1);
  i2c_read(g_vl53l0x_i2c, &g_vl53l0x_config, buf, count);
}

static void vl53l0x_write_multi(uint8_t reg, uint8_t *buf, int count)
{
  uint8_t tmp[32];
  if (count + 1 > (int)sizeof(tmp)) return;
  tmp[0] = reg;
  memcpy(&tmp[1], buf, count);
  i2c_write(g_vl53l0x_i2c, &g_vl53l0x_config, tmp, count + 1);
}

static void vl53l0x_write_reg_list(uint8_t *list)
{
  int i = 0;
  while (1)
    {
      uint8_t reg = list[i];
      uint8_t count = list[i + 1];
      uint8_t value = list[i + 2];
      if (count == 0xff) break;
      if (count == 0x00)
        {
          vl53l0x_write_reg(reg, value);
          i += 3;
        }
      else
        {
          i += 3;
          vl53l0x_write_multi(reg, &list[i], count);
          i += count;
        }
    }
}

/* 时序计算 */

static uint32_t vl53l0x_decode_timeout(uint16_t reg_val)
{
  return ((uint32_t)(reg_val & 0xff) << ((reg_val & 0xff00) >> 8)) + 1;
}

static uint16_t vl53l0x_encode_timeout(uint32_t mclks)
{
  uint32_t ls = 0;
  uint16_t ms = 0;
  if (mclks > 0)
    {
      ls = mclks - 1;
      while ((ls & 0xffffff00) > 0) { ls >>= 1; ms++; }
      return (ms << 8) | (ls & 0xff);
    }
  return 0;
}

static uint32_t vl53l0x_mclks_to_us(uint32_t mclks, uint32_t pclks)
{
  uint32_t ns = calcMacroPeriod(pclks);
  return ((mclks * ns) + (ns / 2)) / 1000;
}

static uint32_t vl53l0x_us_to_mclks(uint32_t us, uint32_t pclks)
{
  uint32_t ns = calcMacroPeriod(pclks);
  return (((us * 1000) + (ns / 2)) / ns);
}

static uint8_t vl53l0x_decode_vcsel_period(uint8_t reg_val)
{
  return (reg_val + 1) << 1;
}

static uint8_t vl53l0x_get_vcsel_period(vcselPeriodType type)
{
  if (type == VcselPeriodPreRange)
    return vl53l0x_decode_vcsel_period(
      vl53l0x_read_reg(PRE_RANGE_CONFIG_VCSEL_PERIOD));
  return vl53l0x_decode_vcsel_period(
    vl53l0x_read_reg(FINAL_RANGE_CONFIG_VCSEL_PERIOD));
}

static sequence_step_timeouts_t vl53l0x_get_step_timeouts(uint8_t seq_cfg)
{
  sequence_step_timeouts_t t;
  uint8_t pre_p = vl53l0x_get_vcsel_period(VcselPeriodPreRange);
  uint8_t fin_p = vl53l0x_get_vcsel_period(VcselPeriodFinalRange);
  uint16_t msrc_m = vl53l0x_read_reg(REG_MSRC_CONFIG_CONTROL) + 1;
  t.pre_range_vcsel_period_pclks = pre_p;
  t.final_range_vcsel_period_pclks = fin_p;
  t.msrc_dss_tcc_mclks = msrc_m;
  t.msrc_dss_tcc_us = vl53l0x_mclks_to_us(msrc_m, pre_p);
  t.pre_range_mclks = vl53l0x_decode_timeout(
    vl53l0x_read_reg16(PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
  t.pre_range_us = vl53l0x_mclks_to_us(t.pre_range_mclks, pre_p);
  t.final_range_mclks = vl53l0x_decode_timeout(
    vl53l0x_read_reg16(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));
  if (seq_cfg & SEQUENCE_ENABLE_PRE_RANGE)
    t.final_range_mclks -= t.pre_range_mclks;
  t.final_range_us = vl53l0x_mclks_to_us(t.final_range_mclks, fin_p);
  return t;
}

static uint32_t vl53l0x_get_timing_budget(void)
{
  sequence_step_timeouts_t t;
  uint32_t b = 1910 + 960;
  t = vl53l0x_get_step_timeouts(vl53l0x_read_reg(SYSTEM_SEQUENCE_CONFIG));
  b += t.msrc_dss_tcc_us + t.pre_range_us + t.final_range_us + 550;
  return b;
}

static bool vl53l0x_set_timing_budget(uint32_t budget_us)
{
  sequence_step_timeouts_t t;
  uint32_t used = 1910 + 960;
  uint8_t seq_cfg;
  uint16_t fin_mclks;
  if (budget_us < 20000) return false;
  seq_cfg = vl53l0x_read_reg(SYSTEM_SEQUENCE_CONFIG);
  t = vl53l0x_get_step_timeouts(seq_cfg);
  used += t.msrc_dss_tcc_us + t.pre_range_us + 550;
  if (!(seq_cfg & SEQUENCE_ENABLE_PRE_RANGE)) return false;
  fin_mclks = vl53l0x_us_to_mclks(budget_us - used,
    t.final_range_vcsel_period_pclks);
  if (seq_cfg & SEQUENCE_ENABLE_PRE_RANGE)
    fin_mclks += t.pre_range_mclks;
  vl53l0x_write_reg16(FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
    vl53l0x_encode_timeout(fin_mclks));
  g_measurement_timing_budget_us = vl53l0x_get_timing_budget();
  return true;
}

static bool vl53l0x_get_spad_info(uint8_t *count, bool *is_aperture)
{
  uint8_t tmp;
  uint8_t ref_spad_map[6];
  uint8_t uc[] = {0xff,0x01,0x00,0xff,0x00,0x00,
    0x09,0x00,0x00,0x01,0x00,0x00,0xff,0x00,0x06,0x00,0x02,0x00,
    0x00,0x00,0x00,0x07,0x05};
  int i;

  uc[2] = 0xc0;
  vl53l0x_write_reg_list(uc);
  vl53l0x_write_reg(0x80, 0x01);
  vl53l0x_write_reg(0xff, 0x01);
  vl53l0x_write_reg(0x00, 0x00);
  vl53l0x_write_reg(0xff, 0x06);
  vl53l0x_write_reg(0x83, vl53l0x_read_reg(0x83) | 0x04);
  vl53l0x_write_reg(0xff, 0x07);
  vl53l0x_write_reg(0x81, 0x01);
  vl53l0x_write_reg(0x80, 0x01);
  vl53l0x_write_reg(0x94, 0x6b);
  vl53l0x_write_reg(0x83, 0x00);

  tmp = vl53l0x_read_reg(0x92);
  *count = tmp & 0x7f;
  *is_aperture = (tmp >> 7) & 0x01;

  vl53l0x_write_reg(0x81, 0x00);
  vl53l0x_write_reg(0xff, 0x06);
  vl53l0x_write_reg(0x83, vl53l0x_read_reg(0x83) & ~0x04);
  vl53l0x_write_reg(0xff, 0x01);
  vl53l0x_write_reg(0x00, 0x01);
  vl53l0x_write_reg(0xff, 0x00);
  vl53l0x_write_reg(0x80, 0x00);

  vl53l0x_read_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);
  for (i = 0; i < 48; i++)
    {
      if (i < (int)((*is_aperture ? 12 : 0) + *count))
        ref_spad_map[i / 8] |= (1 << (i % 8));
      else
        ref_spad_map[i / 8] &= ~(1 << (i % 8));
    }
  vl53l0x_write_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);
  return true;
}

static bool vl53l0x_perform_ref_calibration(uint8_t vhv_byte)
{
  vl53l0x_write_reg(REG_SYSRANGE_START, 0x01 | vhv_byte);
  if (vl53l0x_read_reg(RESULT_INTERRUPT_STATUS) != 0x07)
    return false;
  vl53l0x_write_reg(SYSTEM_INTERRUPT_CLEAR, 0x01);
  vl53l0x_write_reg(REG_SYSRANGE_START, 0x00);
  return true;
}

static bool vl53l0x_init_sensor(bool long_range)
{
  uint8_t spad_count;
  bool spad_is_aperture;
  uint8_t ref_spad_map[6];
  int i;

  uint8_t uc_spad[] = {0xff,0x01,0x00,0xff,0x00,0x00,
    0x09,0x00,0x00,0x01,0x00,0x00,0xff,0x00,0x09,0x00,0x01,0x00,
    0x00,0x00,0x00,0x08,0x05};
  uint8_t uc_tuning[] = {0xff,0x01,0x0f,0x00,0x0e,0x01,
    0x00,0x01,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x24,0xff,0xff,
    0xff,0xff,0x09,0x05,0x01};

  vl53l0x_write_reg(0x88, 0x00);
  vl53l0x_write_reg(0x80, 0x01);
  vl53l0x_write_reg(0xff, 0x01);
  vl53l0x_write_reg(0x00, 0x00);
  g_stop_variable = vl53l0x_read_reg(0x91);
  vl53l0x_write_reg(0x00, 0x01);
  vl53l0x_write_reg(0xff, 0x00);
  vl53l0x_write_reg(0x80, 0x00);

  vl53l0x_write_reg(REG_MSRC_CONFIG_CONTROL,
    vl53l0x_read_reg(REG_MSRC_CONFIG_CONTROL) | 0x12);
  vl53l0x_write_reg16(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, 32);
  vl53l0x_write_reg(SYSTEM_SEQUENCE_CONFIG, 0xFF);

  if (!vl53l0x_get_spad_info(&spad_count, &spad_is_aperture))
    return false;

  vl53l0x_read_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);
  vl53l0x_write_reg_list(uc_spad);

  uint8_t first = spad_is_aperture ? 12 : 0;
  uint8_t enabled = 0;
  for (i = 0; i < 48; i++)
    {
      if (i < first || enabled == spad_count)
        ref_spad_map[i >> 3] &= ~(1 << (i & 7));
      else if (ref_spad_map[i >> 3] & (1 << (i & 7)))
        enabled++;
    }
  vl53l0x_write_multi(GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);
  vl53l0x_write_reg_list(uc_tuning);

  if (long_range)
    {
      vl53l0x_write_reg16(FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, 13);
      vl53l0x_write_reg(PRE_RANGE_CONFIG_VCSEL_PERIOD, encodeVcselPeriod(18));
      vl53l0x_write_reg(FINAL_RANGE_CONFIG_VCSEL_PERIOD, encodeVcselPeriod(14));
    }

  vl53l0x_write_reg(SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
  vl53l0x_write_reg(GPIO_HV_MUX_ACTIVE_HIGH,
    vl53l0x_read_reg(GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
  vl53l0x_write_reg(SYSTEM_INTERRUPT_CLEAR, 0x01);

  g_measurement_timing_budget_us = vl53l0x_get_timing_budget();
  vl53l0x_write_reg(SYSTEM_SEQUENCE_CONFIG, 0xe8);
  vl53l0x_set_timing_budget(g_measurement_timing_budget_us);

  vl53l0x_write_reg(SYSTEM_SEQUENCE_CONFIG, 0x01);
  if (!vl53l0x_perform_ref_calibration(0x40)) return false;
  vl53l0x_write_reg(SYSTEM_SEQUENCE_CONFIG, 0x02);
  if (!vl53l0x_perform_ref_calibration(0x00)) return false;
  vl53l0x_write_reg(SYSTEM_SEQUENCE_CONFIG, 0xe8);
  return true;
}

static uint16_t vl53l0x_read_range_continuous(void)
{
  int timeout = 0;
  uint16_t range;
  while ((vl53l0x_read_reg(RESULT_INTERRUPT_STATUS) & 0x07) == 0)
    {
      if (++timeout > 50) return (uint16_t)-1;
      usleep(5000);
    }
  range = vl53l0x_read_reg16(RESULT_RANGE_STATUS + 10);
  vl53l0x_write_reg(SYSTEM_INTERRUPT_CLEAR, 0x01);
  return range;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32_vl53l0x_setup(void)
{
  uint8_t model;
  int ret;

  g_vl53l0x_i2c = stm32_i2cbus_initialize(1);
  if (g_vl53l0x_i2c == NULL)
    {
      serr("ERROR: 获取 I2C1 失败\n");
      return -ENODEV;
    }

  /* 配置 XSHUT 引脚 */

  stm32_configgpio(GPIO_VL53L0X_XSHUT);

  /* 先拉低 XSHUT 复位传感器 */

  stm32_gpiowrite(GPIO_VL53L0X_XSHUT, false);
  usleep(5000);

  /* 再拉高使传感器工作 */

  stm32_gpiowrite(GPIO_VL53L0X_XSHUT, true);
  usleep(10000);

  /* 初始化信号量 */

  ret = sem_init(&g_vl53l0x_sem, 0, 0);
  if (ret < 0)
    {
      serr("ERROR: 初始化信号量失败: %d\n", ret);
      return -errno;
    }

  /* 初始化传感器（普通精度模式） */

  if (!vl53l0x_init_sensor(false))
    {
      serr("ERROR: VL53L0X 初始化失败\n");
      return -EIO;
    }

  /* 配置 GPIO1 中断（数据就绪通知） */

  stm32_configgpio(GPIO_VL53L0X_GPIO1);
  ret = stm32_gpiosetevent(GPIO_VL53L0X_GPIO1, false, true, false,
                            vl53l0x_gpio1_isr, NULL);
  if (ret < 0)
    {
      serr("ERROR: 配置 VL53L0X GPIO1 中断失败: %d\n", ret);
      return ret;
    }

  /* 验证芯片 ID */

  model = vl53l0x_read_reg(REG_IDENTIFICATION_MODEL_ID);
  if (model != 0xcc)
    {
      serr("ERROR: VL53L0X 芯片 ID 不匹配: 0x%02X (应为 0xCC)\n", model);
      return -ENODEV;
    }

  sinfo("VL53L0X 初始化成功 (ID=0x%02X, XSHUT=PC5, GPIO1=PC4)\n", model);
  return OK;
}

int stm32_vl53l0x_read(uint16_t *distance_mm)
{
  int timeout;
  uint16_t range;

  if (distance_mm == NULL) return -EINVAL;
  if (g_vl53l0x_i2c == NULL) return -ENODEV;

  vl53l0x_write_reg(0x80, 0x01);
  vl53l0x_write_reg(0xff, 0x01);
  vl53l0x_write_reg(0x00, 0x00);
  vl53l0x_write_reg(0x91, g_stop_variable);
  vl53l0x_write_reg(0x00, 0x01);
  vl53l0x_write_reg(0xff, 0x00);
  vl53l0x_write_reg(0x80, 0x00);
  vl53l0x_write_reg(REG_SYSRANGE_START, 0x01);

  timeout = 0;
  while (vl53l0x_read_reg(REG_SYSRANGE_START) & 0x01)
    {
      if (++timeout > 50) return -ETIMEDOUT;
      usleep(5000);
    }

  range = vl53l0x_read_range_continuous();
  if (range == (uint16_t)-1) return -ETIMEDOUT;
  *distance_mm = range;
  return OK;
}

int stm32_vl53l0x_start_continuous(void)
{
  if (g_vl53l0x_i2c == NULL) return -ENODEV;
  vl53l0x_write_reg(0x80, 0x01);
  vl53l0x_write_reg(0xff, 0x01);
  vl53l0x_write_reg(0x00, 0x00);
  vl53l0x_write_reg(0x91, g_stop_variable);
  vl53l0x_write_reg(0x00, 0x01);
  vl53l0x_write_reg(0xff, 0x00);
  vl53l0x_write_reg(0x80, 0x00);
  vl53l0x_write_reg(REG_SYSRANGE_START, 0x02);
  return OK;
}

int stm32_vl53l0x_read_continuous(uint16_t *distance_mm)
{
  if (distance_mm == NULL) return -EINVAL;
  if (g_vl53l0x_i2c == NULL) return -ENODEV;
  *distance_mm = vl53l0x_read_range_continuous();
  return (*distance_mm == (uint16_t)-1) ? -ETIMEDOUT : OK;
}

int stm32_vl53l0x_get_model(int *model, int *revision)
{
  if (g_vl53l0x_i2c == NULL)
    {
      return -ENODEV;
    }

  if (model)
    {
      *model = vl53l0x_read_reg(REG_IDENTIFICATION_MODEL_ID);
    }

  if (revision)
    {
      *revision = vl53l0x_read_reg(REG_IDENTIFICATION_REVISION_ID);
    }

  return OK;
}

#endif /* CONFIG_SENSORS_VL53L0X */
