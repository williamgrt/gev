//
// Created by 郭睿韬 on 2020/5/6.
//

#include "ThreadPool.h"
#include <cassert>

using namespace web;
using namespace std;
using namespace std::placeholders;

ThreadPool::ThreadPool(int maxTaskNum, int threadNum)
    : running_(false), maxTaskNum_(maxTaskNum), threadNum_(threadNum), mutex_(), empty_(mutex_), full_(mutex_) {}

ThreadPool::~ThreadPool() {
  if (running_) {
    stop();
  }
}

void ThreadPool::setMaxTaskNum(int maxTaskNum) {
  if (running_) {
    return;
  }
  maxTaskNum_ = maxTaskNum;
}

void ThreadPool::setThreadNum(int threadNum) {
  if (running_) {
    return;
  }
  threadNum_ = threadNum;
}

void ThreadPool::start() {
  assert(!running_);
  running_ = true;

  threads_.reserve(threadNum_);
  for (int i = 0; i < threadNum_; ++i) {
    // 将成员函数的第一个参数绑定在this指针上。
    threads_.emplace_back(new Thread(std::bind(&ThreadPool::runInThread, this)));
    threads_[i]->start();
  }
}

/*
 * 线程池内每个线程运行的函数
 */
void ThreadPool::runInThread() {
  // 保证线程池在运行过程中
  while (running_) {
    LockGuard guard(mutex_);
    //empty_.waitIf(std::bind(&ThreadPool::isEmpty, this));
    while (running_ && isEmpty()) {
      empty_.wait();
    }
    // 有可能主线程已经关闭线程池，清空了阻塞队列
    // 此时线程应该直接退出
    if (running_) {
      auto task = tasks_.front();
      tasks_.pop_front();
      full_.notify();
      // 执行任务
      if (task) {
        task();
      }
    }
  }
}

void ThreadPool::addTask(Task task) {
  if (threads_.empty()) {
    task();
  } else {
    LockGuard guard(mutex_);
    full_.waitIf(std::bind(&ThreadPool::isFull, this));
    tasks_.push_back(task);
    if (running_) {
      empty_.notify();
    }
  }

}

void ThreadPool::stop() {
  if (running_) {
    {
      // 不允许再添加任务到任务队列中
      LockGuard guard(mutex_);
      running_ = false;
      // 清空任务队列
      tasks_.clear();
      // 唤醒所有等待任务的线程
      empty_.notifyAll();
    }
    // 等待所有的线程执行完成
    for (auto &thread: threads_) {
      thread->join(); // 可以join已经终止的线程，主线程立即返回
    }
    // 清空线程池内的线程
    threads_.clear();
  }
}