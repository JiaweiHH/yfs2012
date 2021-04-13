// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "rpc.h"

class extent_client {
 private:
  rpcc *cl;

 public:
  extent_client(std::string dst);

  extent_protocol::status get(extent_protocol::extentid_t eid, 
			      std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				  extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status lookup(extent_protocol::extentid_t, const char *, extent_protocol::extentid_t &);
  extent_protocol::status readdir(extent_protocol::extentid_t, std::map<extent_protocol::extentid_t, std::string> &);
  extent_protocol::status create(extent_protocol::extentid_t, std::string, bool, extent_protocol::extentid_t &);
  extent_protocol::status read(extent_protocol::extentid_t, off_t, size_t, std::string &);
  extent_protocol::status write(extent_protocol::extentid_t, off_t, size_t, std::string);
  extent_protocol::status unlink(extent_protocol::extentid_t, std::string);
};

#endif 

