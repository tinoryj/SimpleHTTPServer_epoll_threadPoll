#ifndef _HTTPSERVE_HPP_
#define _HTTPSERVE_HPP_

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "csapp.hpp"
#include "cache.hpp"
#include "epoll.hpp"
#include "mutex.hpp"

//server status
#define STATUS_SUCCESS	0
#define STATUS_WRITE	1
#define STATUS_READ		2
#define STATUS_CLOSE	3
#define STATUS_ERROR	-1

class HttpServe{

public:

	void init(int fd);
	void process();
	static void setEpollfd(int epollfd) {

		epollfd_ = epollfd;
	}

private:

	enum States{kRead, kWrite, kOver, kError};
	void setState(States s) { //

		state_ = s;
	}
	States state_;
	//成员函数
	int processRead(); //读操作
	int processWrite(); //写操作
	void reset(); //重置
	bool addResponse(const char* format, ...); 
	bool addResponse(char *const); //
	int getLine(char *buf, int maxsize); //在请求信息缓存中读取一行
	void static getFileType(char *fileName, char *fileType); //获取文件类型信息，用于制作响应头
	void sendErrorMsg(char *cause, char *errnum, char *shortmsg, char *longmsg); //错误信息发送
	void serveStatic(char *filename, size_t fileSize); //静态网页处理
	void serveDynamic(char *text, int len); //动态网页处理
	void readRequestHdrs(); // 读取请求头
	int parseUri(char *uri, char *fileName, char *cgiargs); //处理GET路径
	bool read();
	//常量
	static const int READ_BUFFER_SIZE = 1024; // 读缓冲区的大小
	static const int WRITE_BUFFER_SIZE = 1024; // 写缓冲区的大小
	static const char rootDir_[]; // 网页的根目录
	static const char homePage_[]; // 所指代的网页
	static Cache& cache_; // 全局cache_
	static int epollfd_;
	int sockfd_; //该HTTP连接的socket和对方的socket地址
	boost::shared_ptr<FileInfo> fileInfo_;
	char readBuf_[READ_BUFFER_SIZE]; //读缓冲区
	char postDataBuf_[READ_BUFFER_SIZE]; //请求附加内容
	size_t nRead_; //标志读缓冲区中已经读入的客户数据的最后一个字节的下一个位置
	size_t nChecked_; //当前正在分析的字符在读缓冲区中的位置
	bool keepAlive_; //是否保持连接
	bool sendFile_; //是否发送文件
	char writeBuf_[WRITE_BUFFER_SIZE]; //写缓冲区
	size_t nStored_; //写缓冲区中待发送的字节数
	size_t written_; //已经写了多少字节
};
#endif 
