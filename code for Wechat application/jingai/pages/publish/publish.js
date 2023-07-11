// miniprogram/pages/userAuth.js
var app = getApp();
var self;
var mqtt = require('../../js/mqtt.js');

Page({

  /**
   * 页面的初始数据
   */
  data: {
      topic: '',
      payload:'',
      log:''
  },

  onLoad: function (options) {
    var self = this;
    self.setData({
      topic: '$oc/devices/' + app.globalData.deviceId + '/sys/properties/report',
      payload:'{"services":[{"properties":{"luminance":20},"service_id":"BasicData"}]}'
      });
  },

  getTopic(e) {
    this.setData({
      topic:  e.detail.value
    })
  },

  getPayload(e) { e.detail.value
    this.setData({
      payload: e.detail.value
    })
  },

  //事件处理函数
  reportData: function (e)  {
    self = this;
    const client = app.globalData.client;
    if (client == null || client.connected == false) {
      log('device is not connect to platform');
      return;
    }
    log('topic:' + self.data.topic);
    client.publish(self.data.topic, self.data.payload);
    log('publish message successful');
    function log(msg) {
        console.log(new Date() + ' - ' + msg
        );
        self.data.log = self.data.log + new Date() + ' - ' + msg + '\r\n';
        self.setData({
          log: self.data.log
        })
    }
}

})