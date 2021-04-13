// the extent server implementation

#include "extent_server.h"
#include <algorithm>
#include <fcntl.h>
#include <iterator>
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
  attr.size = buf.size();
  file_map[id].data_ = std::move(buf);
  file_map[id].attr_ = std::move(attr);
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &) {
  // You fill this in for Lab 2.
  std::lock_guard<std::mutex> lk(mutex);
  put_content(id, buf);
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf) {
  // You fill this in for Lab 2.
  std::lock_guard<std::mutex> lk(mutex);
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

  std::lock_guard<std::mutex> lk(mutex);
  auto iter = file_map.find(id);
  if (iter != file_map.end()) {
    a = iter->second.attr_;
    return extent_protocol::OK;
  }

  return extent_protocol::IOERR;
}

int extent_server::remove(extent_protocol::extentid_t id) {
  // You fill this in for Lab 2.
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
  std::lock_guard<std::mutex> lk(mutex);
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
  std::lock_guard<std::mutex> lk(mutex);
  auto iter = file_map.find(id);
  if (iter != file_map.end()) {
    // 获取目录分割列表
    map = parse_dir(iter->second.data_);
    return extent_protocol::OK;
  }
  return extent_protocol::IOERR;
}

int extent_server::create(extent_protocol::extentid_t pid, std::string name,
                          bool ordinary_file,
                          extent_protocol::extentid_t &cid) {
  std::lock_guard<std::mutex> lk(mutex);
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
    cid = random_inum(ordinary_file);
    std::string content{};
    put_content(cid, content);
    // 添加目录项
    std::string &parent_content = iter->second.data_;
    parent_content += std::to_string(cid) + " ";
    parent_content += name + " ";
    // 更新父目录的数据大小
    iter->second.attr_.size = parent_content.size();
    iter->second.attr_.ctime = iter->second.attr_.mtime = time(nullptr);
    std::cout << "create inum: " << cid << ". parent content: " << parent_content << "\n";
    return extent_protocol::OK;
  }

  return extent_protocol::IOERR;
}

int extent_server::read(extent_protocol::extentid_t inum, int off, int size,
                        std::string &buf) {
  std::lock_guard<std::mutex> lk(mutex);
  extent_protocol::status ret;
  auto iter = file_map.find(inum);
  if (iter == file_map.end()) {
    return extent_protocol::IOERR;
  }
  const std::string &data_ = iter->second.data_;
  extent_protocol::attr &attr_ = iter->second.attr_;
  if (off > attr_.size) {
    buf = "";
    goto release;
  }
  buf = data_.substr(off, size);
release:
  attr_.atime = time(nullptr);
  return extent_protocol::OK;
}

// off 在 attr.size 内的时候，是覆盖而不是插入
int extent_server::write(extent_protocol::extentid_t inum, int off, int size,
                         std::string buf, int &) {
  std::lock_guard<std::mutex> lk(mutex);
  auto iter = file_map.find(inum);
  if (iter == file_map.end()) {
    return extent_protocol::IOERR;
  }
  std::string &data_ = iter->second.data_;
  extent_protocol::attr &attr_ = iter->second.attr_;
  if (off + size > attr_.size) {
    data_.resize(off + size, '\0');
  }
  // data_.insert(off, buf.substr(0, size));
  for (int i = 0; i < size; ++i) {
    data_[off + i] = buf[i];
  }
  attr_.size = data_.size();
  attr_.mtime = attr_.ctime = time(nullptr);
  return extent_protocol::OK;
}

int extent_server::unlink(extent_protocol::extentid_t parent, std::string d_name, int &) {
  std::lock_guard<std::mutex> lk(mutex);
  auto iter = file_map.find(parent);
  if(iter == file_map.end())
    return extent_protocol::IOERR;
  // 找到目录项
  std::cout << "rm file: " << d_name << "\n";
  std::string &data_ = iter->second.data_;
  std::istringstream istr(data_);

  extent_protocol::extentid_t inum;
  std::string name;
  while(istr >> inum) {
    istr >> name;
    if(name == d_name) {
      std::string s_inum = std::to_string(inum);
      data_.erase(data_.find(s_inum), s_inum.size() + 1);
      data_.erase(data_.find(name), name.length() + 1);
      iter->second.attr_.size -= s_inum.size() + name.size() + 2;
      goto find;
    }
  }
  return extent_protocol::IOERR;

find:
  iter->second.attr_.mtime = iter->second.attr_.ctime = time(nullptr);
  remove(inum);
  std::cout << "parent content: " << data_ << "\n";
  return extent_protocol::OK;
}