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




/*
    GPIO使用详情：
        GPIO 11 12 按键
        GPIO 9 10 NFC
        GPIO 5 6 超声波
        GPIO  9 10 E53-SC2

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "ohos_init.h"

#include <dtls_al.h>
#include <mqtt_al.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>
#include "E53_SC2.h"
#include "wifi_connect.h"

#include "cmsis_os2.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_i2c.h"
#include "nfc.h"
#include "nfctest.c" 


#include "hi_wifi_api.h"
#include "lwip/netifapi.h"
//按钮
#define BUTTON_F1_GPIO 14
//消息队列使用
#define THREAD_STACK_SIZE (1024 * 10)
#define THREAD_PRIO 25
//MQTT参数
#define CONFIG_APP_SERVERIP "117.78.5.125"

#define CONFIG_APP_SERVERPORT "1883"

#define CONFIG_APP_DEVICEID "6433d09840773741f9fd8cce_1231231312" // 替换为注册设备后生成的deviceid

#define CONFIG_APP_DEVICEPWD "12345678" // 替换为注册设备后生成的密钥

#define CONFIG_APP_LIFETIME 60 // < seconds

#define CONFIG_QUEUE_TIMEOUT (5 * 1000)
//下面是线程内的size等参数
#define MSGQUEUE_COUNT 16
#define MSGQUEUE_SIZE 100

#define CLOUD_TASK_STACK_SIZE (1024 * 10)
#define CLOUD_TASK_PRIO 24

#define SENSOR_TASK_STACK_SIZE (1024 * 4)
#define SENSOR_TASK_PRIO 25

#define NFC_TAASK_STACK_SIZE (1024*10)
#define NFC_THREAD_PRIO 23

#define TASK_DELAY 3
#define FLIP_THRESHOLD 100

#define I2C_TASK_STACK_SIZE (1024 * 8)
#define I2C_TASK_PRIO 25


#define TASK_DELAY_1S 1000000


#define recv_buf_len 150
//微信小程序的
#define topic "$oc/devices/6433d09840773741f9fd8cce_test1/user/test"//第二个井盖（虚拟设备）

#define topic_main "$oc/devices/6433d09840773741f9fd8cce_Manhole_Cover/user/test"//井盖第一代的（智慧井盖）
static char recv_buffer[recv_buf_len];   //下发数据接收缓冲区
static int  recv_datalen;                //表示接收数据长度

//这个没用
char* CONFIG_WIFI_SSID= "111"; // 修改为自己的WiFi 热点账号

char* CONFIG_WIFI_PWD ="12345678"; // 修改为自己的WiFi 热点密码


#define PWM_DELAY_10MS 10000

//超声波使用的端口
#define GPIO_8 5//9
#define GPIO_7 6//11
#define GPIO_FUNC 0
////////////////写入flash的结构体///////////////////
//这里的nv写入的是meodem区，与factory工厂区有差别
//存入nv的结构体，用于wifi
typedef struct {
    unsigned char ssid[50];
    unsigned char passwd[50];
} wal_cfg_ssid_my;

//定义nv成为wifi的变量，可用存入flash内
wal_cfg_ssid_my nv_wifi;//用于写入nv的，用一次清零一次
wal_cfg_ssid_my nv_wifi_read;//用于读
//用于定位的，写入nv的
typedef struct{
    unsigned char demision[50];
    unsigned char longitude[50];//懒得改多少个0了
}int_pos_my;
//用于经纬度写入nv
int_pos_my nv_posi;//用于写入nv的，用一次就清零一次
int_pos_my nv_posi_read;//用于读
int_pos_my posi_pre;//前一次的数据，用于判断是不是需要重新向客户端发送
//nv内的wifi所在信息的ID
#define NV_ID_WIFI 0x29
//nv内的位置定位信息所在的ID
#define NV_ID_POSI 0x30
//消息列表的结构体，可用储存数据和index
typedef struct {
    // object data type
    char *buf;
    uint8_t idx;
} MSGQUEUE_OBJ_t;

//订阅topic使用的消息列表的量
MSGQUEUE_OBJ_t rec;

// message queue id，用于x，y,z数据的消息id
osMessageQueueId_t g_msgQueueId;

//接受到topic订阅的消息列表
osMessageQueueId_t recmsg_id;

//超声波距离
osMessageQueueId_t dist;

////////////////////////下面是结构体2类////////////////////////

//上云的链接体
typedef enum {
    en_msg_cmd = 0,
    en_msg_report,
} en_msg_type_t;

typedef struct {
    char *request_id;
    char *payload;
} cmd_t;

//汇总汇报的结构体
typedef struct {
    int temp;
    int acce_x;
    int acce_y;
    int acce_z;
} report_t;

typedef struct {
    en_msg_type_t msg_type;
    union {
        report_t report;
    } msg;
} app_msg_t;

typedef struct {
    osMessageQueueId_t app_msg;
    int connected;
} app_cb_t;
static app_cb_t g_app_cb;




//用于判断井盖的位置
int g_coverStatus = 0;
//按键复位
int flag = 0;
//初始的位置
int X0 = 0, Y0 = 0, Z0 = 0;


//互斥锁1：用于wifi信息储存后再进行连接
osMutexId_t wifi_mutex_id;
//互斥锁2：用于mqtt与wifi的关系
osMutexId_t mqtt_mutex_id;
//////////////////////////下面是主函数/////////////////////

//解析json流,用判断{}在哪的方式进行解析，对应的json处没得空格，用“”：“”这种，就总括起来
void extractData(const char *str, char *ssid_new, char *passwd_new, char *dimension_new, char *longitude_new) {
    strcpy(ssid_new, "");
    strcpy(passwd_new, "");
    strcpy(dimension_new, "");
    strcpy(longitude_new, "");
    const char *start = strchr(str, '{'); // 查找左大括号的位置
    const char *end = strchr(str, '}');   // 查找右大括号的位置
    
    if (start == NULL || end == NULL) {
        printf("======cannot find {}===========\n");
        return;
    }

    // 从左大括号后面的位置开始提取数据
    start++;
    
    // 提取ssid
    const char *ssid_start = strstr(start, "\"ssid\":\""); // 查找ssid的起始位置
    const char *ssid_end = strchr(ssid_start + 8, '\"');   // 查找ssid的结束位置
    strncpy(ssid_new, ssid_start + 8, ssid_end - (ssid_start + 8));
    ssid_new[ssid_end - (ssid_start + 8)] = '\0';
    
    // 提取passwd
    const char *passwd_start = strstr(start, "\"passwd\":\""); // 查找passwd的起始位置
    const char *passwd_end = strchr(passwd_start + 10, '\"');   // 查找passwd的结束位置
    strncpy(passwd_new, passwd_start + 10, passwd_end - (passwd_start + 10));
    passwd_new[passwd_end - (passwd_start + 10)] = '\0';
    
    // 提取dimension
    const char *dimension_start = strstr(start, "\"dimension\":\""); // 查找dimension的起始位置
    const char *dimension_end = strchr(dimension_start + 14, '\"');   // 查找dimension的结束位置
    strncpy(dimension_new, dimension_start + 13, dimension_end - (dimension_start + 13));
    dimension_new[dimension_end - (dimension_start + 13)] = '\0';
    
    // 提取longitude
    const char *longitude_start = strstr(start, "\"longitude\":\""); // 查找longitude的起始位置
    const char *longitude_end = strchr(longitude_start + 14, '\"');   // 查找longitude的结束位置
    strncpy(longitude_new, longitude_start + 13, longitude_end - (longitude_start + 13));
    longitude_new[longitude_end - (longitude_start + 13)] = '\0';
}

//存入flash的函数
char ssid_new[32] = {0};
char passwd_new[32] = {0};
char dimension_new[32] = {0};
char longitude_new[32] = {0};
char s[200]= {0};
uint8_t info_1[16*15];
int wifi_store = 0;
//完成json 的方式,判断是不是相同，不相同就写入nv，利用_read的结构体进行读取并储存
void store_wifi()
{ 
    int ret;
    while(1){
    //读nv内的数据
    NT3H1101_Read_Userpages(15,info_1);//nfc读取函数，网上资料
    snprintf(s,200,"%s",info_1);//格式化一下
    extractData(s, ssid_new, passwd_new, dimension_new, longitude_new);//提取出信息
    if((strcmp(ssid_new,nv_wifi_read.ssid)!=0)||(strcmp(passwd_new,nv_wifi_read.passwd)!=0))//判断
    {
        usleep(1000);
        strcpy(nv_wifi.ssid,ssid_new);//用memcoy_s有问题，改成这个就能正常了
        strcpy(nv_wifi.passwd,passwd_new);
        hi_nv_write(NV_ID_WIFI, &nv_wifi, sizeof(wal_cfg_ssid_my), 0);
        memcpy_s(&nv_wifi_read.ssid[0], sizeof(wal_cfg_ssid_my), "", 1);
        memcpy_s(&nv_wifi_read.passwd[0], sizeof(wal_cfg_ssid_my), "", 1);
    }
    if((strcmp(dimension_new,nv_posi_read.demision)!=0)||(strcmp(longitude_new,nv_posi_read.longitude)!=0))//判断
    {
        usleep(1000);
        strcpy(nv_posi.demision,dimension_new);
        strcpy(nv_posi.longitude,longitude_new);
        ret = hi_nv_write(NV_ID_POSI,&nv_posi,sizeof(int_pos_my),0);
        memcpy_s(&nv_posi_read.demision[0], sizeof(int_pos_my), dimension_new, strlen(dimension_new));
        memcpy_s(&nv_posi_read.longitude[0], sizeof(int_pos_my), longitude_new, strlen(longitude_new));
    }
        hi_nv_read(NV_ID_WIFI, &nv_wifi_read, sizeof(wal_cfg_ssid_my), 0);
        hi_nv_read(NV_ID_POSI, &nv_posi_read, sizeof(int_pos_my), 0);//读
        // printf("==========nv_wifi read : %d,  ssid  :[%s]  psswd [%s]=============\n",ret, nv_wifi_read.ssid, nv_wifi_read.passwd); 
        // printf("==========nv_posi read : %d,  dimen  :[%s]  longi [%s]=============\n",ret, nv_posi_read.demision, nv_posi_read.longitude); 
        sleep(2);
    }
    
   
    
}

////////////////////按键消抖////////////////////////////
//每次按键后直接mask一下，然后延时后进行恢复->PWM
static void F1Pressed(char *arg)
{
    (void)arg;
    IoTGpioSetIsrMask(BUTTON_F1_GPIO,1);
    flag++;
       
}
//按键复位的线程函数，用于定义使用
static void button(void)
{
    
    // init gpio of F2 key and set it as the falling edge to trigger interrupt
    IoTGpioInit(BUTTON_F1_GPIO);
    IoTGpioSetFunc(BUTTON_F1_GPIO, IOT_GPIO_FUNC_GPIO_14_GPIO);
    IoTGpioSetDir(BUTTON_F1_GPIO, IOT_GPIO_DIR_IN);
    IoTGpioSetPull(BUTTON_F1_GPIO, IOT_GPIO_PULL_UP);
    IoTGpioRegisterIsrFunc(BUTTON_F1_GPIO, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, F1Pressed, NULL);
}
//用于按键消抖，延时后恢复电位水平
static void PwmTask(void)
{
    while (1) {
        if (flag==2)
        {
            printf("==================starting to reinit status!!!!=================\n");
            X0 =0;
            Y0=0;
            Z0=0;
            flag =0;
        }
    
        else{
            usleep(PWM_DELAY_10MS);
            usleep(PWM_DELAY_10MS);
            usleep(PWM_DELAY_10MS);
            usleep(PWM_DELAY_10MS);
            usleep(PWM_DELAY_10MS);
        }
IoTGpioSetIsrMask(BUTTON_F1_GPIO,0);
          
    }
}
////////////////////////超声波////////////////////////
//超声波函数及模块
float distance;
float GetDistance  (void) {
    static unsigned long start_time = 0, time = 0;
    float distance = 0.0;
    IotGpioValue value = IOT_GPIO_VALUE0;
    unsigned int flag1 = 0;
    IoTWatchDogDisable();

    hi_io_set_func(GPIO_8, IOT_GPIO_FUNC_GPIO_5_GPIO);
    IoTGpioSetDir(GPIO_8, IOT_GPIO_DIR_IN);
    hi_io_set_func(GPIO_7, IOT_GPIO_FUNC_GPIO_6_GPIO);
    IoTGpioSetDir(GPIO_7, IOT_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(GPIO_7, IOT_GPIO_VALUE1);
    hi_udelay(20);
    IoTGpioSetOutputVal(GPIO_7, IOT_GPIO_VALUE0);

    while (1) {
        IoTGpioGetInputVal(GPIO_8, &value);
        if ( value == IOT_GPIO_VALUE1 && flag1 == 0) {
            start_time = hi_get_us();
            flag1 = 1;
        }
        if (value == IOT_GPIO_VALUE0 && flag1 == 1) {
            time = hi_get_us() - start_time;
            start_time = 0;
            break;
        }
       
    }
    distance = time * 0.034 / 2;
    return distance;
}
//超声波的线程函数
void RobotTask(void* parame) {
    (void)parame;
    printf("start test hcsr04\r\n");
    while(1) {
        distance = GetDistance();
        printf("=================hr-sh04: %f==================\n",distance);
        osDelay(200);
    }
}
/////////////////////wifi&mqtt////////////////////////////
//上报函数
static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t temperature;
    oc_mqtt_profile_kv_t Accel_x;
    oc_mqtt_profile_kv_t Accel_y;
    oc_mqtt_profile_kv_t Accel_z;
    oc_mqtt_profile_kv_t status;
    oc_mqtt_profile_kv_t test;
    oc_mqtt_profile_kv_t nfcinfo;
    oc_mqtt_profile_kv_t Dimension;   
    oc_mqtt_profile_kv_t Longitude;   

    if (g_app_cb.connected != 1) {
        return;
    }

    service.event_time = NULL;
    service.service_id = "Manhole_Cover";
    service.service_property = &temperature;
    service.nxt = NULL;

    temperature.key = "Temperature";
    temperature.value = &report->temp;
    temperature.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    temperature.nxt = &Accel_x;

    Accel_x.key = "Accel_x";
    Accel_x.value = &report->acce_x;
    Accel_x.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Accel_x.nxt = &Accel_y;

    Accel_y.key = "Accel_y";
    Accel_y.value = &report->acce_y;
    Accel_y.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Accel_y.nxt = &Accel_z;

    Accel_z.key = "Accel_z";
    Accel_z.value = &report->acce_z;
    Accel_z.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    Accel_z.nxt = &status;

    status.key = "Cover_Status";
    status.value = g_coverStatus ? "Tilt" : "Level";
    status.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    status.nxt =&test;

    test.key = "Water_level";
    test.value = &distance; 
    test.type = EN_OC_MQTT_PROFILE_VALUE_FLOAT;
    test.nxt =&Dimension;
    
    Dimension.key = "Dimension";
    Dimension.value = &nv_posi_read.demision;
    Dimension.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    Dimension.nxt =&Longitude;

    Longitude.key = "Longitude";
    Longitude.value = &nv_posi_read.longitude;
    Longitude.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    Longitude.nxt =NULL;

    
    oc_mqtt_profile_propertyreport(NULL, &service);
    return;
}

//订阅数据的打印
static int app_msg_deal(oc_mqtt_profile_msgrcv_t *payload)
{
    int ret = -1;
    if(payload ->msg_len < recv_buf_len)
    {
        //放入消息列表中
        rec.buf = NULL;
        rec.buf = payload->msg;
        rec.idx = 0U;
            osMessageQueuePut(recmsg_id,&rec,0U,0U);
        printf("MSG: %s  has been put into queue!!\r\n",payload->msg);
        
    }
    
    return ret;
}


//连接云和订阅函数，用于上报，此topic下发消息的调用函数只能用于down topic，在wifi连接了才开始mqtt
static int CloudMainTaskEntry(void)
{
    app_msg_t *app_msg;
    uint32_t ret;
    osStatus_t status;
    
    WifiConnect(nv_wifi_read.ssid,nv_wifi_read.passwd);
    sleep(2);
    
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();

    g_app_cb.app_msg = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (g_app_cb.app_msg == NULL) {
        printf("Create receive msg queue failed");
    }
    oc_mqtt_profile_connect_t connect_para;
    (void)memset_s(&connect_para, sizeof(connect_para), 0, sizeof(connect_para));

    connect_para.boostrap = 0;
    connect_para.device_id = CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr = CONFIG_APP_SERVERIP;
    connect_para.server_port = CONFIG_APP_SERVERPORT;
    connect_para.life_time = CONFIG_APP_LIFETIME;
    connect_para.rcvfunc = NULL;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);
    if ((ret == (int)en_oc_mqtt_err_ok)) {
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    } else {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }

    while (1) {
        app_msg = NULL;
        (void)osMessageQueueGet(g_app_cb.app_msg, (void **)&app_msg, NULL, 0xFFFFFFFF);
        if (app_msg != NULL) {
            switch (app_msg->msg_type) {
                case en_msg_report:
                    deal_report_msg(&app_msg->msg.report);
                    break;
                default:
                    break;
            }
            free(app_msg);
        }
    }
    return 0;
}

//////////////////////MPU6050///////////////////////////
//六轴陀螺仪数据获取函数
static int SensorTaskEntry(void)
{
    app_msg_t *app_msg;
    uint8_t ret;
    E53SC2Data data;//新获取的数据
    E53SC2Data data_pre;//之前一次的数据
    ret = E53SC2Init();
    if (ret != 0) {
        printf("E53_SC2 Init failed!\r\n");
        return;
    }
    while (1) {
        ret = E53SC2ReadData(&data);
        if (ret != 0) {
            printf("E53_SC2 Read Data!\r\n");
            return;
        }
        osMessageQueueGet(recmsg_id,&rec,0U,0U);
        // printf("\r\n******************************Temperature      is  %d\r\n", (int)data.Temperature);
        // printf("\r\n******************************Accel[ACCEL_X_AXIS] is  %d\r\n", (int)data.Accel[ACCEL_X_AXIS]);
        // printf("\r\n******************************Accel[ACCEL_Y_AXIS] is  %d\r\n", (int)data.Accel[ACCEL_Y_AXIS]);
        // printf("\r\n******************************Accel[ACCEL_Z_AXIS] is  %d\r\n", (int)data.Accel[ACCEL_Z_AXIS]);
        
        if (X0 == 0 && Y0 == 0 && Z0 == 0) {
            printf("====================confirm init status!!====================\n");
            X0 = (int)data.Accel[ACCEL_X_AXIS];
            Y0 = (int)data.Accel[ACCEL_Y_AXIS];
            Z0 = (int)data.Accel[ACCEL_Z_AXIS];
        } else {
            if (X0 + FLIP_THRESHOLD < data.Accel[ACCEL_X_AXIS] || X0 - FLIP_THRESHOLD > data.Accel[ACCEL_X_AXIS] ||
                Y0 + FLIP_THRESHOLD < data.Accel[ACCEL_Y_AXIS] || Y0 - FLIP_THRESHOLD > data.Accel[ACCEL_Y_AXIS] ||
                Z0+ FLIP_THRESHOLD < data.Accel[ACCEL_Z_AXIS] || Z0 - FLIP_THRESHOLD > data.Accel[ACCEL_Z_AXIS]) {
                LedD1StatusSet(OFF);
                LedD2StatusSet(ON);
                g_coverStatus = 1;
                printf("error situation!!!\n");
            } else {
                LedD1StatusSet(ON);
                LedD2StatusSet(OFF);
                g_coverStatus = 0;
            }
        }
        app_msg = malloc(sizeof(app_msg_t));
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;
            app_msg->msg.report.temp = (int)data.Temperature;
            app_msg->msg.report.acce_x = (int)data.Accel[ACCEL_X_AXIS];
            app_msg->msg.report.acce_y = (int)data.Accel[ACCEL_Y_AXIS];
            app_msg->msg.report.acce_z = (int)data.Accel[ACCEL_Z_AXIS];

            if (osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT) != 0) {
                free(app_msg);
            }
        }
        //此处为发送给微信小程序
        char meg[200]="";
        // if(posi_pre.demision!= nv_posi.demision && posi_pre.longitude!=nv_posi.longitude){
         if (posi_pre.demision!= nv_posi.demision || posi_pre.longitude!=nv_posi.longitude||data_pre.Accel[ACCEL_X_AXIS]+ FLIP_THRESHOLD < data.Accel[ACCEL_X_AXIS] || data_pre.Accel[ACCEL_X_AXIS] - FLIP_THRESHOLD > data.Accel[ACCEL_X_AXIS] ||
                data_pre.Accel[ACCEL_Y_AXIS] + FLIP_THRESHOLD < data.Accel[ACCEL_Y_AXIS] || data_pre.Accel[ACCEL_Y_AXIS] - FLIP_THRESHOLD > data.Accel[ACCEL_Y_AXIS] ||
                data_pre.Accel[ACCEL_Z_AXIS]+ FLIP_THRESHOLD < data.Accel[ACCEL_Z_AXIS] || data_pre.Accel[ACCEL_Z_AXIS] - FLIP_THRESHOLD > data.Accel[ACCEL_Z_AXIS]) {
        snprintf(meg,200,"temperature: %d\naccel_x:%d\naccel_y:%d\naccel_z:%d\nCover_Status:%s\nWater_Level:%f\ndeviceid:%d\ndimension: %s\nlongitude:%s\n",(int)data.Temperature,(int)data.Accel[ACCEL_X_AXIS],(int)data.Accel[ACCEL_Y_AXIS],(int)data.Accel[ACCEL_Z_AXIS],g_coverStatus ? "Tilt" : "Level",distance,4,nv_posi_read.demision,nv_posi_read.longitude);//设备号记得修改
        printf("%s\n",meg);
        posi_pre=nv_posi;
        data_pre = data;
        if(!oc_mqtt_publish(topic,&meg,strlen(meg),2))
        {
            printf("============success publish!!====================\n");
            ;
        }
        }
        // }
      sleep(TASK_DELAY);
        
    }    
    return 0;


}
//总线程函数，互斥锁定义了但是没用
static void IotMainTaskEntry(void)
{
     g_msgQueueId = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (g_msgQueueId == NULL) {
        printf("Failed to create Message Queue!\n");
    }
    recmsg_id = osMessageQueueNew(1, MSGQUEUE_SIZE, NULL);
    if(recmsg_id==NULL)
    {
        printf("Fail to create topic message queue!!\r\n");
    }
    osThreadAttr_t attr;
    // attr.name = "CloudMainTaskEntry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CLOUD_TASK_STACK_SIZE;
    attr.priority = CLOUD_TASK_PRIO;
    osThreadId_t store_id = osThreadNew((osThreadFunc_t)store_wifi, NULL, &attr);
    osThreadId_t cloud_id = osThreadNew((osThreadFunc_t)CloudMainTaskEntry, NULL, &attr);
    osThreadId_t nfc_id = osThreadNew((osThreadFunc_t)I2cTask, NULL, &attr);
    osThreadId_t sensor_id = osThreadNew((osThreadFunc_t)SensorTaskEntry, NULL, &attr);
    osThreadId_t button_id =   osThreadNew((osThreadFunc_t)button, NULL, &attr);
    osThreadId_t pwm_id = osThreadNew((osThreadFunc_t)PwmTask, NULL, &attr);
    osThreadId_t distance_id = osThreadNew((osThreadFunc_t)RobotTask, NULL, &attr);
    if (cloud_id == NULL) {
        printf("Failed to create CloudMainTaskEntry!\n");
    }  

    attr.name = "SensorTaskEntry";
    attr.stack_size = SENSOR_TASK_STACK_SIZE;
    attr.priority = SENSOR_TASK_PRIO;
    if (sensor_id == NULL) {
        printf("Failed to create SensorTaskEntry!\n");
    }

    attr.name = "I2cTask";
    attr.stack_size = NFC_TAASK_STACK_SIZE;
    attr.priority = NFC_THREAD_PRIO;    
    if (nfc_id == NULL) {
    printf("Falied to create I2cTask!\n");
    }

    attr.name = "button";
    attr.stack_size = NFC_TAASK_STACK_SIZE;
    attr.priority = NFC_THREAD_PRIO; 
    
    if (button_id == NULL) {
    printf("Falied to create buttonTask!\n");
    }
    
    attr.name = "store_wifi";
    attr.stack_size= NFC_TAASK_STACK_SIZE;
    attr.priority = NFC_THREAD_PRIO;
    if(store_id==NULL)
    {
        printf("Falied to create farthTask!\n");
    }
    
    if (distance_id == NULL) {
        printf("Failed to create measuredistanceTaskEntry!\n");
    }  

     if(pwm_id==NULL)
    {
        printf("Falied to create pwmTask!\n");
    }
   
}

APP_FEATURE_INIT(IotMainTaskEntry);