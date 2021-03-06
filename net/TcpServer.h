#ifndef WEBSERVER_SRC_NET_TCPSERVER_H
#define WEBSERVER_SRC_NET_TCPSERVER_H

#include <memory>
#include <unordered_map>
#include "Defs.h"
#include "Socket.h"
#include "Acceptor.h"
#include "TcpConnection.h"

namespace gweb {

class EventLoop;
class Acceptor;
class TcpServer;
class Ip4Addr;
class EventLoopThreadPool;

class TcpServer : noncopyable {
 public:
  TcpServer(EventLoop *loop, const std::string &hostname, unsigned int port);
  ~TcpServer();

  // 设置sub-reactor的数量
  void setReactorNum(int num);
  void start();

  // 设置回调函数
  void setConnectionCallback(const ConnectionCallback &cb) {
    connectionCallBack_ = cb;
  }

  void setMessageCallback(const MessageCallback &cb) {
    messageCallBack_ = cb;
  }

  void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
    writeCompleteCallback_ = cb;
  }

  EventLoop *loop() {
    return loop_;
  }

 private:
  using ConnectionMap = std::unordered_map<std::string, TcpConnPtr>;

  // Acceptor创建新连接的回调函数
  void newConnection(Socket &&socket, Ip4Addr peerAddr);
  // 清除TCP连接
  void closeConnection(const TcpConnPtr &conn);
  // 处理sigpipe信号
  void handleSigpipe();
  // 有可能TcpConnection和TcpServer不在同一个EventLoop中，此时需要把关闭连接的操作移到TcpServer的EvetLoop中
  void closeConnectionInLoop(const TcpConnPtr &conn);

  EventLoop *loop_;
  std::unique_ptr<Acceptor> acceptor_;
  MessageCallback messageCallBack_;
  ConnectionCallback connectionCallBack_;
  WriteCompleteCallback writeCompleteCallback_;
  const std::string name_;
  int nextId_;
  bool started_;
  ConnectionMap connectionMap_;
  Ip4Addr localAddr_;
  std::unique_ptr<EventLoopThreadPool> threadPool_;
};
}

#endif //WEBSERVER_SRC_NET_TCPSERVER_H
