# 设备端开发代码

`man.c`函数为主函数所在C文件。

## 本代码包含多个模块：

- 信息NV操作存储至FLASH

  - ```cpp
    void store_wifi();//nv储存
    void extractData(const char *str, char *ssid_new, char *passwd_new, char *dimension_new, char *longitude_new);//获取NFC中数据并提取
    
    ```

  

- 连接华为云

  - ```cpp
    static int CloudMainTaskEntry(void)//连接wifi并连接华为云
    ```

    

- 传感器数据接收并上传

  - ```cpp
    static int SensorTaskEntry(void)//MPU6050以及上报微信小程序
    float GetDistance  (void)//超声波模块
    static void deal_report_msg(report_t *report)//上报华为云函数
    ```

    

- NFC工作

  - ```cpp
    I2cTask(void)//NFC工作线程
    ```

    

- 按键消抖以及按键工作

  - ```cpp
    static void button(void)//按键初始化
    static void F1Pressed(char *arg)//按键调用函数
    static void PwmTask(void)//按键消抖
    ```

    

​	
