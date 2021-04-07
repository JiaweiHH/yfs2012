// the lock server implementation

#include "lock_server.h"
#include <arpa/inet.h>
#include <sstream>
#include <stdio.h>
#include <unistd.h>

lock_server::lock_server() : nacquire(0) {}

lock_protocol::status lock_server::stat(int clt, lock_protocol::lockid_t lid,
                                        int &r) {
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status lock_server::acquire(int clt, lock_protocol::lockid_t lid,
                                           int &r) {
  lock_protocol::status ret = lock_protocol::OK;
  std::unique_lock<std::mutex> lk(mutex); // 先获取锁
  std::map<lock_protocol::lockid_t, lock *>::iterator iter = lock_map.find(lid);
  lock *m_lock = nullptr;
  if (iter == lock_map.end()) { // lock 不存在
    m_lock = new lock;
    lock_map[lid] = m_lock;
  } else { // lock 已经存在
    std::pair<lock_protocol::lockid_t, lock *> p = *iter;
    m_lock = p.second;
    while (m_lock->status_ == lock::LOCKED) {
      // 条件等待，阻塞的时候会释放锁，以便其他进程能够执行
      m_lock->cv.wait(lk, [&] { return m_lock->status_ == lock::FREE; });
    }
  }
  m_lock->lid_ = lid;
  m_lock->clt_ = clt;
  m_lock->status_ = lock::LOCKED;
  return ret;
}

lock_protocol::status lock_server::release(int clt, lock_protocol::lockid_t lid,
                                           int &r) {
  lock_protocol::status ret = lock_protocol::OK;
  std::unique_lock<std::mutex> lk(mutex); // 获取锁
  auto m_lock = lock_map[lid];
  m_lock->status_ = lock::FREE;
  m_lock->clt_ = -1;
  m_lock->cv.notify_one();
  return ret;
}