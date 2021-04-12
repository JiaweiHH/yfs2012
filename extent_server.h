// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include "extent_protocol.h"
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class extent_server {

public:
  struct extent {
    std::string data_;
    extent_protocol::attr attr_;
  };

  extent_server();

  std::map<extent_protocol::extentid_t, std::string>
  parse_dir(const std::string &) const;
  void put_content(const extent_protocol::extentid_t &, const std::string &);
  extent_protocol::extentid_t random_inum(bool odinary_file = true) const {
    uint32_t inum = rand();
    if (!odinary_file) {
      inum &= 0x7fffffff;
    } else {
      inum |= 0x80000000;
    }
    return inum;
  }

  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
  int lookup(extent_protocol::extentid_t, std::string,
             extent_protocol::extentid_t &);
  int readdir(extent_protocol::extentid_t,
              std::map<extent_protocol::extentid_t, std::string> &);
  int create(extent_protocol::extentid_t, std::string,
             extent_protocol::extentid_t &);
  int read(extent_protocol::extentid_t, int, int, std::string &);
  int write(extent_protocol::extentid_t, int, int, std::string, int &);

private:
  // inum - data
  std::map<extent_protocol::extentid_t, extent> file_map;
  std::mutex mutex{};
};

#endif
