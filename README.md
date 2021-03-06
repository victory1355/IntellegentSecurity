## 基于STM32F407+UCOS-III的居家智能安防系统的设计与实现

---------------------------

## `Author:zhanxuegui` :apple:
## `Date:2018.06.06 - 2018.07.06`

-----------------------------
### 注：如果不能正确显示图片，请到show目录下查看演示效果文件

-----------------------------
#### 项目描述
- 功能：
  - 实时监测环境的参数，比如温湿度参数、是否有可燃气体或者火焰的出现，如果出现危险情况 则通过蜂鸣器或者LED等方式发出警告；
  - 用户通过手机可以实时地查询系统的状态或者设置系统的时间和报警值等操作；
  - 可以通过红外发射器或者按键控制系统的硬件，比如LED或者蜂鸣器的开关等；
  - OLED 屏幕显示用户的操作信息。
- 方案：
  - 本系统采用STM32F407作为MCU，搭载ucos-III实时操作系统。
  - 以OLED 屏幕、红外、蓝牙和按键作为输出、输入方式。
  - 系统上电运行后，通过ucos-III创建多个任务，分别负责实时响应用户的查询操作、红外遥控硬件的操作和实时地监测系统周围环境的变化等任务。
- 涉及的技术：ucos-III实时操作系统、STM32F407库函数的开发、超声波、烟雾、火焰、温湿度传感器、RTC实时时钟、RFID射频识别、ADC\DAC、flash、常用的接口技术I2C、UART、SPI。

------------------------------

### 系统运行效果如下
----------------
## demo1
![](https://github.com/victory1355/IntellegentSecurity/blob/master/show/demo1.bmp)
----------------
## demo2
![](https://github.com/victory1355/IntellegentSecurity/blob/master/show/demo2.bmp)
---------------
## demo3
![](https://github.com/victory1355/IntellegentSecurity/blob/master/show/demo3.bmp)
--------------
## demo4
![](https://github.com/victory1355/IntellegentSecurity/blob/master/show/demo4.bmp)




