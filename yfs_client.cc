// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);

}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & 0x80000000)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:

  return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

int yfs_client::lookup(inum p_inum, const char *name, inum &c_inum) {
  int r = OK;
  if(ec->lookup(p_inum, name, c_inum) != extent_protocol::OK) {
    r = IOERR;
  }
  return r;
}

int yfs_client::readdir(inum inum, std::vector<yfs_client::dirent> &v_contents) {
  int r = IOERR;
  printf("readdir %016llx\n", inum);
  std::map<extent_protocol::extentid_t, std::string> contents;
  if(ec->readdir(inum, contents) == extent_protocol::OK) {
    for(auto &pair : contents) {
      dirent d;
      d.inum = pair.first;
      d.name = pair.second;
      v_contents.push_back(d);
    }
    for(auto &pair : contents) {
      std::cout << "pair: (" << pair.first << ", " << pair.second << ")\n";
    }
    r = OK;
  }
  return r;
}

int yfs_client::create(inum p_inum, std::string name, inum &c_inum) {
  extent_protocol::status status = ec->create(p_inum, name, c_inum);
  if(status == extent_protocol::OK) {
    std::cout << name << " create OK\n";
    return OK;
  }else if(status == extent_protocol::EXIST)
    return EXIST;
  return IOERR;
}