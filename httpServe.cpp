#include "httpServe.hpp"

Cache baseCache;
Cache& HttpServe::cache_ = baseCache;
int HttpServe::epollfd_ = -1;
const char HttpServe::rootDir_[] = "/home/tinoryj/Documents/Code/Server/webSrc";
const char HttpServe::homePage_[] = "index.html";

void HttpServe::init(int sockfd){

	sockfd_ = sockfd; // 记录下连接的socket
	int reuse = 1;
	Setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); // 设置端口重用
	reset();
}

void HttpServe::reset(){

	nRead_ = 0;
	nChecked_ = 0;
	nStored_ = 0;
	keepAlive_ = false;
	sendFile_ = false;
	written_ = 0;

	setState(kRead); /* 现在需要读数据 */
	memset(readBuf_, 0, READ_BUFFER_SIZE);
	memset(writeBuf_, 0, WRITE_BUFFER_SIZE);
}

void HttpServe::readRequestHdrs(){

	char buf[MAXLINE];
	getLine(buf, MAXLINE);
	while (strcmp(buf, "\0")) {

		getLine(buf, MAXLINE);
		if (strstr(buf, "keep-alive")) {

			keepAlive_ = true; // 保持连接
		}
	}
	int n = (strstr(readBuf_, "\r\n\r\n") - readBuf_) + 4;
	for(int i = n, j = 0; i < strlen(readBuf_); i++, j++)
		postDataBuf_[j] = readBuf_[i];
	return;
}

int HttpServe::parseUri(char *uri, char *filename, char *cgiargs){

	char *ptr;
	std::cout<<uri<<std::endl;
	if (strstr(uri, "/")) {

		strcpy(cgiargs, "");
		strcpy(filename, rootDir_);
		strcat(filename, uri);
		if (uri[strlen(uri) - 1] == '/')
			strcat(filename, homePage_);
		if (strstr(uri, "/succes.html"))
			strcat(filename, homePage_);
		if (strstr(uri, "/wrong.html"))
			strcat(filename, homePage_);
		//std::cout<<filename<<std::endl;
		return 1;
	}
	else {
		// Dynamic content
		ptr = index(uri, '?');
		if (ptr) {
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		}
		else{

			strcpy(cgiargs, "");
		}
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}
}

int HttpServe::getLine(char *buf, int maxsize) {
	 // 用于读取一行数据
	int n; // 用于记录读取的字节的数目
	for (n = 0; nChecked_ < nRead_;n++) {

		*buf++ = readBuf_[nChecked_];
		if (readBuf_[nChecked_++] == '\n')
			break;
	}
	*buf = 0;
	return n;
}

bool HttpServe::read(){
	 // 由于epoll设置成了是边缘触发,所以要一次性将数据全部读尽
	nRead_ = 0; // 首先要清零
	nChecked_ = 0;
	if (nRead_ >= READ_BUFFER_SIZE) {
		return false;
	}
	int byte_read = 0;
	while (true) {
		byte_read = recv(sockfd_, readBuf_ + nRead_, READ_BUFFER_SIZE - nRead_, 0);
		if (byte_read == -1) {  //出错
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				break; //读取完毕
			}
			return false; // 出错
		}
		else if (byte_read == 0) { // 对方已经关闭了连接
			return false;
		}
		nRead_ += byte_read;
	}
	return true;
}

void HttpServe::process(){

	int res;
	switch (state_){

	case kRead: {
		res = processRead();
		//std::cout<<"processRead res is"<<res<<std::endl;
		if (res == STATUS_WRITE)
			modfd(epollfd_, sockfd_, EPOLLOUT);
		else
			removefd(epollfd_, sockfd_);
		break;
	}
	case kWrite: {
		res = processWrite();
		//std::cout<<"processWrite res is"<<res<<std::endl;
		if (res == STATUS_READ)
			modfd(epollfd_, sockfd_, EPOLLIN);
		else
			removefd(epollfd_, sockfd_);
		break;
	}
	default:
		removefd(epollfd_, sockfd_);
		break;
	}
}

int HttpServe::processRead(){

	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], filename[MAXLINE], cgiargs[MAXLINE], line[MAXLINE];

	if ((false == read()) || (nRead_ == 0)) {
		//对方已经关闭了连接
		return STATUS_CLOSE;
	}
	//解析读入的数据
	getLine(line, MAXLINE); //读取一行数据
	sscanf(line, "%s %s %s", method, uri, version);
	if ( (!strcasecmp(method, "GET")) && (!strcasecmp(method, "POST")) ) {

		sendErrorMsg(method, "501", "Not Implemented","Server does not implement this method");
		goto end;
	}

	readRequestHdrs();  // 处理剩余的请求头部
	if(!strcasecmp(method, "POST")){

		char *flag1 = strstr(postDataBuf_,"username=1234");
		char *flag2 = strstr(postDataBuf_,"password=1234");
		if(flag1 != NULL && flag2 != NULL && strlen(postDataBuf_) == 27){

			memset(filename, 0, MAXLINE);
			strcpy(filename, rootDir_);
			strcat(filename, "/succes.html");
			//std::cout<<filename;
			stat(filename, &sbuf);
			serveStatic(filename, sbuf.st_size);
			goto end;
		}
		else{

			memset(filename, 0, MAXLINE);
			strcpy(filename, rootDir_);
			strcat(filename, "/wrong.html");
			//std::cout<<filename;
			stat(filename, &sbuf);
			serveStatic(filename, sbuf.st_size);
			goto end;
		}
	}
	is_static = parseUri(uri, filename, cgiargs);
	if (stat(filename, &sbuf) < 0) {

		sendErrorMsg(filename, "404", "Not found","Server couldn't find this file"); // 没有找到文件
		goto end;
	}
	if (is_static) {
		//静态页面处理
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {

			sendErrorMsg(filename, "403", "Forbidden","Server couldn't read the file"); // 权限不够
			goto end;
		}
		serveStatic(filename, sbuf.st_size);
	}
	else {
		//动态页面处理
		sendErrorMsg(method, "501", "Not Implemented","Server does not implement this method");
		goto end;
	}
end:
	setState(kWrite);
	return processWrite();
}

int HttpServe::processWrite(){

	int res;
	//数据要作为两部分发送,第1步,要发送writeBuf_里面的数据.
	size_t nRemain = strlen(writeBuf_) - written_; // writeBuf_中还有多少字节要写
	if (nRemain > 0) {

		while (1) {

			nRemain = strlen(writeBuf_) - written_;
			res = write(sockfd_, writeBuf_ + written_, nRemain);
			if (res < 0) {

				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { // 资源暂时不可用

					setState(kWrite); // 下一步需要写数据
					return STATUS_WRITE;
				}

				setState(kError);
				return STATUS_ERROR;
			}
			written_ += res;
			if (written_ == strlen(writeBuf_)){

				break;
			}
		}
	}
	// 第2步,要发送网页数据.
	if (sendFile_) {

		assert(fileInfo_);
		size_t bytesToSend = fileInfo_->size_ + strlen(writeBuf_); // 总共需要发送的字节数目
		//std::cout<<"bytesToSend! "<<bytesToSend<<std::endl;
		char *fileAddr = (char *)fileInfo_->addr_;
		//std::cout<<"fileAddr "<<fileAddr<<std::endl;
		size_t fileSize = fileInfo_->size_;
		//std::cout<<"fileSize "<<fileSize<<std::endl;
		while (true) {

			size_t offset = written_ - strlen(writeBuf_);
			res = write(sockfd_, fileAddr + offset, fileSize - offset); // 发送
			//std::cout<<"file send once!"<<std::endl;
			if (res < 0) {

				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { // 资源暂时不可用

					setState(kWrite); // 下一步需要写数据
					return STATUS_WRITE;
				}
				setState(kError); // 出现了错误
				return STATUS_ERROR;
			}
			written_ += res;
			if (written_ == bytesToSend)
				break;
		}
	}
	//std::cout<<"send over!"<<std::endl
	if (keepAlive_) {

		reset();
		return STATUS_READ;
	}
	else {

		reset();
		return STATUS_SUCCESS;
	}
}

void HttpServe::getFileType(char *filename, char *filetype){
	 // 获得文件的类型
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".css"))
		strcpy(filetype, "text/css");
	else if (strstr(filename, ".ttf") || strstr(filename, ".otf"))
		strcpy(filetype, "application/octet-stream");
	else
		strcpy(filetype, "text/plain");
}

void HttpServe::serveStatic(char *fileName, size_t fileSize){
	 // 用于处理静态网页
	int srcfd;
	char fileType[MAXLINE], buf[MAXBUF];
	getFileType(fileName, fileType);
	if(keepAlive_){

		sprintf(buf, "HTTP/1.1 200 OK\r\n");
	}
	else{

		sprintf(buf, "HTTP/1.0 200 OK\r\n");
	}
	sprintf(buf, "%sServer: Tinoryj Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, fileSize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, fileType);
	addResponse(buf);
	cache_.getFileAddr(fileName, fileSize, fileInfo_);  // 添加文件
	sendFile_ = true;
}
void HttpServe::serveDynamic(char *text, int len){
	 // 用于处理动态的网页
	int srcfd;
	char buf[MAXBUF];
   	if(keepAlive_){

		sprintf(buf, "HTTP/1.1 200 OK\r\n");
	}
	else{

		sprintf(buf, "HTTP/1.0 200 OK\r\n");
	}
	sprintf(buf, "%sServer: Tinoryj Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf);
	sprintf(buf, "%sContent-type: text/plain\r\n\r\n%s", buf, text);
	addResponse(buf);
}

void HttpServe::sendErrorMsg(char *cause, char *errnum, char *shortmsg, char *longmsg){

	char buf[MAXLINE], body[MAXBUF];
	sprintf(body, "<html><title>Server Error: %s</title>",errnum);
	sprintf(body, "%s<body bgcolor=""6ca6cd"">\r\n", body);
	sprintf(body, "%s<center><h1>%s: %s</h1></center>\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p><center>%s: %s</center>\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em><center>Tinoryj Web Server</center></em>\r\n", body);
	if(keepAlive_){

		addResponse("HTTP/1.1 %s %s\r\n", errnum, shortmsg);
	}
	else{

		addResponse("HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	}
	addResponse("Content-type: text/html\r\n");
	addResponse("Content-length: %d\r\n\r\n", (int)strlen(body));
	addResponse("%s", body);
}

bool HttpServe::addResponse(char* const str){

	int len = strlen(str);
	if ((nStored_ >= WRITE_BUFFER_SIZE) || (nStored_ + len >= WRITE_BUFFER_SIZE)) {

		return false;
	}
	strncpy(writeBuf_ + nStored_, str, len); // 拷贝len个字符
	nStored_ += len; // nStored_是写缓冲区的末尾
	return true;
}

bool HttpServe::addResponse(const char* format, ...){

	if (nStored_ >= WRITE_BUFFER_SIZE) {
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(writeBuf_ + nStored_, WRITE_BUFFER_SIZE - 1 - nStored_, format, arg_list);
	if (len >= (WRITE_BUFFER_SIZE - 1 - nStored_)) {

		return false;
	}
	nStored_ += len;
	va_end(arg_list);
	return true;
}
