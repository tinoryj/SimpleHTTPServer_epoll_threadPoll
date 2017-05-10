# 用法说明
在httpServer.cpp文件中设置服务器文件路径：绝对路径，如下所示
```C++
const char HttpServe::rootDir_[] = "/home/tinoryj/Documents/Code/Server/webSrc";
```
同时，设置主页文件名，如下所示：

```C++
const char HttpServe::homePage_[] = "index.html";
```
在httpServer.cpp中设置文件路径后，在webEpollThreadPool.cpp中设置
