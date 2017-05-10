# 工程简介
电子科技大学2017年综合课程设计题目。
## 题目要求：
> 实现一简单的Web服务器，并实现相关的请求处理机制

> (1)学习理解HTTP协议原理及协议格式；

> (2)设计Web服务器：（a）开发多线程服务器通信程序，可接收远程数据，每个线程对应一个TCP链接；（b)定义HTTP请求/响应数据传输格式；（c）开发数据解析模块；（d）开发与HTTP的GET和POST等方法对应的服务器处理程序；（e）根据请求报文处理完成后，发送响应报文到客户端，若服务器出错也发送相应错误响应。

>(3)利用Web浏览器测试服务器，并可在线演示。

## 实现方法

Epoll + 线程池
## 其他需求

需要安装boost库，由于使用Epoll，必须在Linux系统下运行。

以Ubuntu为例，可使用如下命令安装boost库。

```
sudo apt-get install libboost-dev
```
## 用法说明
在httpServer.cpp文件中设置服务器文件路径：绝对路径，如下所示
```C++
const char HttpServe::rootDir_[] = "/home/tinoryj/Documents/Code/Server/webSrc";
```
同时，设置主页文件名，如下所示：

```C++
const char HttpServe::homePage_[] = "index.html";
```
在httpServer.cpp中设置文件路径后，在webEpollThreadPool.cpp中设置线程池线程数。（可无视）

在进入工程文件夹后，可直接使用*make*指令完成编译链接，生成可执行文件webEpollThreadPool。
启动时需要添加端口号，如下所示：

```
sudo ./webEpollThreadPool 80
```
或
```
./webEpollThreadPool 8080
```
其中端口号请自行指定，使用时若要开启0-1023范围内的端口，请使用*sudo*。

