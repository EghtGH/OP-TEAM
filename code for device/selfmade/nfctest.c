/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_i2c.h"
#include "nfc.h"
#include "ohos_init.h"

#define I2C_TASK_STACK_SIZE (1024 * 8)
#define I2C_TASK_PRIO 25


#define WIFI_IOT_IO_FUNC_GPIO_0_I2C1_SDA 1//对应的
#define WIFI_IOT_IO_FUNC_GPIO_1_I2C1_SCL 1//对应的
#define WIFI_IOT_I2C_IDX 1
#define WIFI_IOT_I2C_BAUDRATE 400000
#define TASK_DELAY_1S 1000000

static void I2cTask(void)
{
    // // GPIO_10 multiplexed to I2C1_SDA
    // IoTGpioInit(0);
    // IoTGpioSetFunc(10, WIFI_IOT_IO_FUNC_GPIO_0_I2C1_SDA);

    // // GPIO_9 multiplexed to I2C1_SCL
    // IoTGpioInit(1);
    // IoTGpioSetFunc(9, WIFI_IOT_IO_FUNC_GPIO_1_I2C1_SCL);

    // // baudrate: 400kbps
    // IoTI2cInit(WIFI_IOT_I2C_IDX, WIFI_IOT_I2C_BAUDRATE);

    
}
