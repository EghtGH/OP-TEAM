const app = getApp()
var QQMapWX = require('../../utils/qqmap-wx-jssdk.js')
var qqmapsdk;
// 实例化API核心类
var qqmapsdk = new QQMapWX({
    key: 'NUHBZ-74O3Z-J4ZXB-Z3ICA-5RDQE-L2B2I' // 必填
});
Page({
  data: {
    markers: [],
  },
onLoad: function () {
  qqmapsdk = new QQMapWX({
    key: 'NUHBZ-74O3Z-J4ZXB-Z3ICA-5RDQE-L2B2I' //这里自己的key秘钥进行填充
  });
  var that = this
  wx.showLoading({
    title: "定位中",
    mask: true
  })

  wx.getSystemInfo({
    success: (res) => {
      this.setData({
         controls: [
          {
            id: 1,
            iconPath: '/images/mapmk.png',   //  大头针图片
            position: {
              left: res.windowWidth / 2 - 11,
              top: res.windowHeight / 2 - 70,
              width: 26,
              height: 45
            },

            clickable: true
          },
          {
            id: 2,
            iconPath: '/images/location.png', // 左下角定位图片
            position: {
              left: 20,
              top: res.windowHeight - 100,
              width: 40,
              height: 40
            },
            clickable: true
          },
        ]
      })
    }
  })
  wx.getLocation({
    type: 'wgs84',
    //定位成功，更新定位结果
    success: function (res) {
      var speed = res.speed
      var accuracy = res.accuracy
      //经纬度转化为地址
      that.getLocal(latitude, longitude)
      that.setData({
        longitude: longitude,
        latitude: latitude,
        speed: speed,
        accuracy: accuracy
      })
      var latitude = res.latitude
      var longitude = res.longitude
      var marker1 = {
        id: 0,
        latitude: 28.233493,
        longitude: 113.010603,
        iconPath: '/images/marker.png',
        width: 40,
        height: 40,
        callout: {
          content: 28.233493 + 'N, ' + 113.010603 + 'E' + ' 设备1',
          color: '#FFFFFF',
          bgColor: '#000000',
          padding: 10,
          borderRadius: 5,
          display: 'BYCLICK'
        }
      };
      
      // 创建第二个标记
      var marker2 = {
        id: 1,
        latitude: 28.237208,  
        longitude: 113.015042,  
        iconPath: '/images/marker.png',
        width: 40,
        height: 40,
        callout: {
          content: 28.237208 + 'N, ' + 113.015042 + 'E' + ' 设备2',
          color: '#FFFFFF',
          bgColor: '#000000',
          padding: 10,
          borderRadius: 5,
          display: 'BYCLICK'
        }
      };
  
      // 将标记对象添加到 markers 数组中
      var markers = [marker1];
      that.setData({
        markers: markers,
        longitude: longitude,
        latitude: latitude
      });

      // 在40秒后显示marker2
      setTimeout(function () {
        markers.push(marker2);
        that.setData({
          markers: markers
        });
      }, 40000);
    },
    //定位失败回调
    fail: function () {
      wx.showToast({
        title: "定位失败",
        icon: "none"
      })
    },

    complete: function () {
      //隐藏定位中信息进度
      wx.hideLoading()
    }

  })
 
},
getLocal: function (latitude, longitude) {
  let vm = this;
  qqmapsdk.reverseGeocoder({
    location: {
      latitude: latitude,
      longitude: longitude
    },
    success: function (res) {
      console.log(JSON.stringify(res));
      let province = res.result.ad_info.province
      let city = res.result.ad_info.city
      let district = res.result.ad_info.district
      vm.setData({
        province: province,//省
        city: city,//市
        district: district,//区
  
      })

    },
    fail: function (res) {
      console.log(res);
    },
    complete: function (res) {
      // console.log(res);
    }
  });
},
bindregionchange: function (e) {
  var that = this
  if (e.type == "begin") {
    console.log("begin");
  } else if (e.type == "end") {
    var mapCtx = wx.createMapContext("ofoMap")
    mapCtx.getCenterLocation({
      success: function (res) {
        var latitude = res.latitude
        var longitude = res.longitude
        that.getLocal(latitude, longitude)
      }
    }) 
  }
},
bindcontroltap: function (e) {
  switch (e.controlId) {
    // 当点击图标location.png的图标，则触发事件movetoPositioon()
    case 2:
        this.movetoPosition();
      break;
   
   }
},
movetoPosition: function () {
  this.mapCtx.moveToLocation();
},
onShow: function () {
  var that=this;
  console.log("onShow");
  that.mapCtx = wx.createMapContext("ofoMap");
  //this.movetoPosition();
  that.mapCtx.getCenterLocation({
    success: function (res) {
      var latitude = res.latitude
      var longitude = res.longitude
      that.getLocal(latitude, longitude)
    }
  }) 
},

})