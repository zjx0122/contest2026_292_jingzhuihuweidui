/****************************************************************************
 * boards/arm/stm32/stm32f407_health/src/stm32_romfs.h
 *
 * SPDX-License-Identifier: BSD-3-Clause
 ****************************************************************************/

#ifndef __BOARDS_ARM_STM32_STM32F407_HEALTH_SRC_STM32_ROMFS_H
#define __BOARDS_ARM_STM32_STM32F407_HEALTH_SRC_STM32_ROMFS_H

#include <nuttx/config.h>

#ifdef CONFIG_STM32_ROMFS

#define ROMFS_SECTOR_SIZE 64

int stm32_romfs_initialize(void);

#endif /* CONFIG_STM32_ROMFS */

#endif /* __BOARDS_ARM_STM32_STM32F407_HEALTH_SRC_STM32_ROMFS_H */