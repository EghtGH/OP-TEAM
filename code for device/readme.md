# 说明

### 在代码中，因有需求，修改了SDK中的一些部分：

- 修改`“\device\hisilicon\hispark_pegasus\sdk_liteos\tools\nvtool\xml_file”`下的`mss_nvi_db.xml`，于：

  ```cpp
  <GROUP NAME="Modem" ID="0x1" FEATURE="1&lt;&lt;0,1&lt;&lt;4,1&lt;&lt;5" USEDMODE="0" PARAM_DEF_FILE="../nv/nv_modem_struct_def.txt">
  ```

  添加：

  ```cpp
   <NV ID="0x29" NAME="INIT_CONFIG_SSID_MY" PARAM_NAME="wal_cfg_ssid_my" PARAM_VALUE="{[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}" CATEGORY="FTM" DEV="CCO-STA-NDM" DESCRIPTION="" />
       <NV ID="0x30" NAME="INIT_CONFIG_POSITION_MY" PARAM_NAME="int_pos_my" PARAM_VALUE="{[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}" CATEGORY="FTM" DEV="CCO-STA-NDM" DESCRIPTION="" />
  ```

  

- 修改`“\device\hisilicon\hispark_pegasus\sdk_liteos\tools\nvtool\h_file\nv”`下`nv_factory_struct_def.txt`，添加：

  ```cpp
  typedef struct {
  
      hi_u8 ssid[50];
  
      hi_u8 passwd[50];
  
  
  
  } wal_cfg_ssid_my;
  
  
  typedef struct{
  
      hi_u8 demision[50];
  
      hi_u8 longitude[50];
      
      
  
  }int_pos_my;
  ```

- 修改`int **WifiConnect**(const char *ssid, const char *psk)`函数

- 添加`void **NT3H1101_Read_Userpages**(uint8_t pagenumber,uint8_t *outbuf)`函数

