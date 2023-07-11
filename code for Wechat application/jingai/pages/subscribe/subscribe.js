// miniprogram/pages/userAuth.js
var app = getApp();
var mqtt = require('../../js/mqtt.js');

Page({

  /**
   * 页面的初始数据
   */
  data: {
      topic: '',
      log:'',
      temp:'',
      Cover_Status:'',
      Water_level:'',
      deviceid:'',
      dimension:'',
      longtitude:''
  },

  onLoad: function (options) {
    var self = this;
    self.setData({
      topic: '$oc/devices/' + app.globalData.deviceId + '/user/test'
      });
  },

  getTopic(e) {
    this.setData({
      topic:  e.detail.value
    })
  },

  //事件处理函数
  subscribe: function (e){
    self = this;
    const client = app.globalData.client;
    console.log(app.globalData.deviceId);
    if (client == null || client.connected == false) {
      log('device is not connect to platform');
      return;
    }
    log('subscribe topic:' + self.data.topic);
    client.subscribe(self.data.topic, {qos: 1},  function(err, granted){
      if (!err) {
        log('subscribe message successful');
      } 
    });

    client.on('message', (topic, message, packet) => {
      log('receive message topic:' + topic);
      log('receive message content:' + message.toString());
      //收到命令下发请求时，返回命令的响应
      sendResponse(topic.split("=")[1]);
    });
    
    function sendResponse(requestId) {
      let responseReqJson = {
          result_code: 0,
          response_name: 'COMMAND_RESPONSE',
          paras: {result: 'success'}
      };
      let responseReq = JSON.stringify(responseReqJson);
      log('response topic is ' + getResponseTopic(requestId));
      log('response message is ' + responseReq);
      client.publish(getResponseTopic(requestId), responseReq);
  }
  function getResponseTopic(requestId) {
      return (
          '$oc/devices/' +
          app.globalData.deviceId +
          '/sys/commands/response/request_id=' +
          requestId
      );
  }

  function log(msg) {
      console.log(new Date() + ' - ' + msg
      );
      
      self.data.log = self.data.log + new Date() + ' - ' + msg + '\r\n';
      self.setData({
        log: self.data.log
      })
      const temperatureRegex = /temperature:\s*(\d+)/;
      const match = msg.match(temperatureRegex);

      if (match) {
        const temperatureValue = match[1];
        console.log("Temperature:", temperatureValue);
        self.setData({
          temp: temperatureValue+'°C'
        })
      } else {
        console.log("Temperature not found");
      }
      const deviceidRegex = /deviceid:\s*(\d+)/;
      const matchi = msg.match(deviceidRegex);
      if (matchi) {
        const deviceidValue = matchi[1];
        console.log("deviceid:", deviceidValue);
        self.setData({
          deviceid: deviceidValue
        })
      } else {
        console.log("deviceid not found");
      }
      const Cover_StatusRegex = /Cover_Status:\s*(\w+)/;
      const match1 = msg.match(Cover_StatusRegex);

      if (match1) {
        const Cover_StatusValue = match1[1];
        console.log("Cover_Status:", Cover_StatusValue);
        self.setData({
          Cover_Status: Cover_StatusValue
        })
        if (Cover_StatusValue === "Tilt") {
          wx.showToast({
            title: '状态异常',
            icon: 'warn',
            image: '../../images/1111.png',
            duration: 3000
          })
        }
      } else {
        console.log("Cover_Status not found");
      }
      const Water_levelRegex = /Water_Level:\s*([\d.]+)/;
      const match2 = msg.match(Water_levelRegex);

      if (match2) {
        const Water_levelValue = match2[1];
        console.log("Water_level:", Water_levelValue);
        self.setData({
          Water_level: Water_levelValue
        })
      } else {
        console.log("Water_level not found");
      }


      const dimensionRegex = /dimension:\s*([\d.]+)/;
      const matchd = msg.match(dimensionRegex);

      if (matchd) {
        const dValue = matchd[1];
        console.log("dimension:", dValue);
        self.setData({
          dimension: dValue+'N'
        })
      } else {
        console.log("dimension not found");
      }
      const longitudeRegex = /longitude:\s*([\d.]+)/;
      const matchl = msg.match(longitudeRegex);

      if (matchl) {
        const lValue = matchl[1];
        console.log("longitude:", lValue);
        self.setData({
          longitude: lValue+'E'
        })
      } else {
        console.log("longitude not found");
      }
  };
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