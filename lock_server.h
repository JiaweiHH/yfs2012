// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include "lock_client.h"
#include "lock_protocol.h"
#include "rpc.h"

#include <condition_variable>
#include <map>
#include <mutex>
#include <string>

class lock {

public:
  enum lock_status { FREE, LOCKED };
  lock_protocol::lockid_t lid_{0};
  lock_protocol::status status_{FREE};
  int clt_{-1};                 // 锁的持有者
  std::condition_variable cv{}; // 条件等待
  lock() {}
};

class lock_server {

protected:
  std::mutex mutex;
  int nacquire;
  std::map<lock_protocol::lockid_t, lock *> lock_map;

public:
  lock_server();
  ~lock_server(){};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif
