// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::get, eid, buf);
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::getattr, eid, attr);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  ret = cl->call(extent_protocol::put, eid, buf, r);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  ret = cl->call(extent_protocol::remove, eid, r);
  return ret;
}

extent_protocol::status
extent_client::lookup(extent_protocol::extentid_t eid, const char *c_name, extent_protocol::extentid_t &child_id) {
  extent_protocol::status ret = extent_protocol::OK;
  std::string name(c_name);
  ret = cl->call(extent_protocol::lookup, eid, name, child_id);
  return ret;
}

extent_protocol::status
extent_client::readdir(extent_protocol::extentid_t eid, std::map<extent_protocol::extentid_t, std::string> &map) {
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::readdir, eid, map);
  return ret;
}

extent_protocol::status
extent_client::create(extent_protocol::extentid_t pid, std::string name, extent_protocol::extentid_t &cid) {
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::create, pid, name, cid);
  return ret;
}

extent_protocol::status
extent_client::read(extent_protocol::extentid_t inum, off_t off, size_t size, std::string &buf) {
  extent_protocol::status ret;
  ret = cl->call(extent_protocol::read, inum, (int)off, (int)size, buf);
  return ret;
}

extent_protocol::status
extent_client::write(extent_protocol::extentid_t inum, off_t off, size_t size, std::string buf) {
  extent_protocol::status ret;
  int r;
  ret = cl->call(extent_protocol::write, inum, (int)off, (int)size, buf, r);
  return ret;
}