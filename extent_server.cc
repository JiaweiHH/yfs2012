// the extent server implementation

#include "extent_server.h"
#include <algorithm>
#include <fcntl.h>
#include <random>
#include <sstream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extent_server::extent_server() {
  std::string content = "";
  const extent_protocol::extentid_t root_id = 0x000000001;
  put_content(root_id, content);
}

std::map<extent_protocol::extentid_t, std::string>
extent_server::parse_dir(const std::string &str) const {
  std::istringstream istr(str);
  std::map<extent_protocol::extentid_t, std::string> ret_map;
  extent_protocol::extentid_t inum;
  while (istr >> inum) {
    istr >> ret_map[inum];
  }
  return ret_map;
}

void extent_server::put_content(const extent_protocol::extentid_t &id,
                                const std::string &buf) {
  extent_protocol::attr attr;
  attr.mtime = attr.ctime = time(nullptr);
  file_map[id].attr_.size = buf.size();
  file_map[id].data_ = std::move(buf);
  file_map[id].attr_ = std::move(attr);
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &) {
  // You fill this in for Lab 2.
  std::unique_lock<std::mutex> lk(mutex);
  put_content(id, buf);
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf) {
  // You fill this in for Lab 2.
  std::unique_lock<std::mutex> lk(mutex);
  auto iter = file_map.find(id);
  if (iter != file_map.end()) {
    buf = iter->second.data_;
    iter->second.attr_.atime = time(nullptr);
    return extent_protocol::OK;
  }

  return extent_protocol::IOERR;
}

int extent_server::getattr(extent_protocol::extentid_t id,
                           extent_protocol::attr &a) {
  // You fill this in for Lab 2.
  // You replace this with a real implementation. We send a phony response
  // for now because it's difficult to get FUSE to do anything (including
  // unmount) if getattr fails.

  std::unique_lock<std::mutex> lk(mutex);
  auto iter = file_map.find(id);
  if (iter != file_map.end()) {
    a = iter->second.attr_;
    return extent_protocol::OK;
  }

  return extent_protocol::IOERR;
}

int extent_server::remove(extent_protocol::extentid_t id, int &) {
  // You fill this in for Lab 2.
  std::unique_lock<std::mutex> lk(mutex);
  auto iter = file_map.find(id);
  if (iter != file_map.end()) {
    file_map.erase(iter);
    return extent_protocol::OK;
  }
  return extent_protocol::IOERR;
}

int extent_server::lookup(extent_protocol::extentid_t parent_id,
                          std::string name,
                          extent_protocol::extentid_t &child_id) {
  std::unique_lock<std::mutex> lk(mutex);
  // std::string name(c_name);
  std::cout << "lookup begin. name " << name << " parent id " << parent_id
            << "\n";
  auto p_iter = file_map.find(parent_id);
  if (p_iter != file_map.end()) {
    // 获取目录项分割列表
    std::map<extent_protocol::extentid_t, std::string> dirents =
        parse_dir(p_iter->second.data_);
    // 查找 name 这个目录项
    auto content_iter = std::find_if(
        dirents.begin(), dirents.end(),
        [&](const std::map<extent_protocol::extentid_t, std::string>::value_type
                &value) { return value.second == name; });
    // 找到了
    if (content_iter != dirents.end()) {
      child_id = content_iter->first;
      return extent_protocol::OK;
    }
  }
  return extent_protocol::IOERR;
}

int extent_server::readdir(
    extent_protocol::extentid_t id,
    std::map<extent_protocol::extentid_t, std::string> &map) {
  std::unique_lock<std::mutex> lk(mutex);
  auto iter = file_map.find(id);
  if (iter != file_map.end()) {
    // 获取目录分割列表
    map = parse_dir(iter->second.data_);
    return extent_protocol::OK;
  }
  return extent_protocol::IOERR;
}

int extent_server::create(extent_protocol::extentid_t pid, std::string name,
                          extent_protocol::extentid_t &cid) {
  std::unique_lock<std::mutex> lk(mutex);
  // printf("create begin. parent inum %016llx\n", pid);
  std::cout << "create begin. parent id " << pid << " name " << name << "\n";
  auto iter = file_map.find(pid);
  if (iter != file_map.end()) {
    // 检查父目录中是否已经存在该文件
    auto contents = parse_dir(iter->second.data_);
    for (auto it = contents.begin(); it != contents.end(); ++it)
      if (it->second == name)
        return extent_protocol::EXIST;

    // 添加文件
    cid = random_inum();
    std::string content{};
    put_content(cid, content);
    // 添加目录项
    std::string &parent_content = iter->second.data_;
    parent_content += std::to_string(cid) + " ";
    parent_content += name + " ";
    return extent_protocol::OK;
  }

  return extent_protocol::IOERR;
}
