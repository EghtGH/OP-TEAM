
var app = getApp();
var self;
var mqtt = require('../../js/mqtt.js');
var CryptoJS = require('../../js/hmac-sha256.js');

Page({

  /**
   * 页面的初始数据
   */
  data: {
      host: '6a72f7eb4c.st1.iotda-device.cn-north-4.myhuaweicloud.com',
      deviceId:'6433d09840773741f9fd8cce_test1',
      password:'d4b4c2e9203b038091e6139f2de54c31',
      log:'',
      showModal: false
  },

  getHost(e) {
    this.setData({
      host: e.detail.value
    })
  },

  getDeviceId(e) {
    this.setData({
      deviceId: e.detail.value
    })
  },

  getPassword(e) {
    this.setData({
      password: e.detail.value
    })
  },

//事件处理函数
connect: function (e) {
  self = this;
  var serverUrl = 'wxs://'+ this.data.host + '/mqtt'; //IoT平台mqtt对接地址
  var deviceId = this.data.deviceId; //请填写在平台注册的设备ID
  var secret = this.data.password; //请填写在平台注册的设备密钥
  
  //退避重连参数
  var minBackoff = 1000;
  var maxBackoff = 30 * 1000;
  var defaultBackoff = 1000;
  var retryTimes = 0;
  
  //连接参数设置
  var timestamp = dateFormat('YYYYmmddHH', new Date())
  const options = {
    username: deviceId,
    password: CryptoJS.HmacSHA256(secret, timestamp).toString(),
    clientId: getClientId(deviceId),
    keepalive: 120, //120s
    clean: true, //cleanSession不保持持久会话
    reconnectPeriod: 0, 
    connectTimeout: 1000,
    protocolVersion: 4, //MQTT v3.1.1
    
  }

  startConnect();
  
  function startConnect() {
    var client = mqtt.connect(serverUrl, options);
    app.globalData.client = client;
    app.globalData.deviceId = deviceId;
    client.on('connect', () => {
        log('mqttOverWebsocket onConnect');
      });

    client.on('error', (e) => {
        log('mqtt error: ' + e);
        reconnect();
      });

    client.on('close', () => {
        log('mqtt server is disconnected');
        reconnect();
      });

    client.on('offline', () => {
        log('mqtt client is offline');
        reconnect();
      });
  }

  function reconnect() {
      log("reconnect is starting");
      //退避重连
      let lowBound = Number(defaultBackoff)*Number(0.8);
      let highBound = Number(defaultBackoff)*Number(1.2);
      let randomBackOff = parseInt(Math.random()*(highBound-lowBound+1),10);
      let backOffWithJitter = (Math.pow(2.0, retryTimes)) * (randomBackOff + lowBound);
      let waitTImeUtilNextRetry = (minBackoff + backOffWithJitter) > maxBackoff ? maxBackoff : (minBackoff + backOffWithJitter);
      log("next retry time: " + waitTImeUtilNextRetry);
      setTimeout(()=> { 
        startConnect();
      }, waitTImeUtilNextRetry);
      retryTimes++;
  }

  function getClientId(deviceId) {
      return deviceId + '_0_0_' + timestamp;
  }

  function dateFormat(fmt, date) {
      let ret;
      const opt = {
          'Y+': date.getFullYear().toString(), // 年
          'm+': (date.getMonth() + 1).toString(), // 月
          'd+': date.getDate().toString(), // 日
          'H+': date.getHours().toString(), // 时
          'M+': date.getMinutes().toString(), // 分
          'S+': date.getSeconds().toString(), // 秒
          // 有其他格式化字符需求可以继续添加，必须转化成字符串
      };
      for (let k in opt) {
          ret = new RegExp('(' + k + ')').exec(fmt);
          if (ret) {
              fmt = fmt.replace(
                  ret[1],
                  ret[1].length === 1
                      ? opt[k]
                      : opt[k].padStart(ret[1].length, '0')
              );
          }
      }
      return fmt;
  }

  function log(msg) {
      console.log(new Date() + ' - ' + msg);
      //this.data.password
      self.data.log = self.data.log + new Date() + ' - ' + msg + '\r\n';
      self.setData({
        log: self.data.log
      })
  }
  this.setData({
    showModal: true
  });

  // 3秒后自动关闭弹窗
  setTimeout(() => {
    this.setData({
      showModal: false
    });
  }, 3000);
},
closePopup() {
  this.setData({
    showModal: false
  });
}


})