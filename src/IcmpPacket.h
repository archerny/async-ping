#ifndef _ICMP_PACKET_H_
#define _ICMP_PACKET_H_

#include <string>
#include <cstring>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>

using std::string;

// ICMP header for both IPv4 and IPv6.
//
// The wire format of an ICMP header is:
//
// 0               8               16                             31
// +---------------+---------------+------------------------------+      ---
// |               |               |                              |       ^
// |     type      |     code      |          checksum            |       |
// |               |               |                              |       |
// +---------------+---------------+------------------------------+    8 bytes
// |                               |                              |       |
// |          identifier           |       sequence number        |       |
// |                               |                              |       v
// +-------------------------------+------------------------------+      ---

// private protocal for xping
// 2 bytes thread number; 8 bytes timestamp; 2 bytes index
//
// 0               8               16                             31
// +---------------+---------------+------------------------------+      ---
// |                               |                              |       ^
// |        thread number          |         timestamp            |       |
// |                               |                              |       |
// +---------------+---------------+------------------------------+    8 bytes
// |                                                              |       |
// |                            timestamp                         |       |
// |                                                              |       v
// +-------------------------------+------------------------------+      ---
// |                               |                              |       ^
// |         timestamp             |          index               |    4 bytes
// |                               |                                      v
// +-------------------------------+------------------------------+      ---

#define ICMP_HEADER_SIZE 8

enum
{ 
  echo_reply = 0,
  destination_unreachable = 3,
  source_quench = 4,
  redirect = 5,
  echo_request = 8,
  time_exceeded = 11,
  parameter_problem = 12, 
  timestamp_request = 13,
  timestamp_reply = 14,
  info_request = 15, 
  info_reply = 16,
  address_request = 17,
  address_reply = 18
};

class IcmpPacket
{
public:
  IcmpPacket(unsigned char type, unsigned char code, unsigned short identifier, unsigned short seq, int size, const char *buf);
  IcmpPacket(unsigned char type, unsigned char code, unsigned short identifier, unsigned short seq, const string& pktContent);
  IcmpPacket(const unsigned char *srcBuf, size_t size);
  ~IcmpPacket();

  inline unsigned char *getBuf() { return buf; }
  inline size_t getPktSize() { return packetSize; }

private:
  unsigned short calcCheckSum();

private:
  size_t packetSize;
  unsigned char *buf;
};

IcmpPacket::IcmpPacket(unsigned char type, unsigned char code, unsigned short identifier, unsigned short seq, const string& pktContent)
{
  packetSize = ICMP_HEADER_SIZE + pktContent.size() + 1;
  buf = new unsigned char[packetSize];
  memcpy(buf + ICMP_HEADER_SIZE, pktContent.c_str(), pktContent.size());
  buf[packetSize - 1] = 0;

  struct icmp *icmpTmp = (struct icmp *) buf;
  icmpTmp->icmp_type = ICMP_ECHO;
  icmpTmp->icmp_code = code;
  icmpTmp->icmp_id = 0xffff & identifier;
  icmpTmp->icmp_seq = seq;
  icmpTmp->icmp_cksum = htons(calcCheckSum());
}

IcmpPacket::IcmpPacket(unsigned char type, unsigned char code, unsigned short identifier, unsigned short seq, int size, const char *srcBuf)
{
  packetSize = ICMP_HEADER_SIZE + size + 1;
  buf = new unsigned char[packetSize];
  memcpy(buf + ICMP_HEADER_SIZE, srcBuf, size);
  buf[packetSize - 1] = 0;
  
  struct icmp *icmpTmp = (struct icmp *) buf;
  icmpTmp->icmp_type = ICMP_ECHO;
  icmpTmp->icmp_code = code;
  icmpTmp->icmp_id = 0xffff & identifier;
  icmpTmp->icmp_seq = seq;
  icmpTmp->icmp_cksum = htons(calcCheckSum());
}

IcmpPacket::IcmpPacket(const unsigned char *srcBuf, size_t size)
{
  packetSize = size;
  buf = new unsigned char[size];
  memcpy(buf, srcBuf, size);
}

IcmpPacket::~IcmpPacket()
{
  delete [] buf;
}

//treat the checked part as 16bif number， sum its complete
unsigned short IcmpPacket::calcCheckSum()
{
  unsigned int sum = (buf[0] << 8) + buf[1] + (buf[4] << 8) + buf[5] + (buf[6] << 8) + buf[7];
  unsigned char *dataBegin = buf + ICMP_HEADER_SIZE;
  unsigned char *iter = dataBegin;
  unsigned char *dataEnd = buf + packetSize - 1;
  while (iter != dataEnd)
  {
    sum += (static_cast<unsigned char>(*iter++) << 8);
    if (iter != dataEnd)
    {
      sum += static_cast<unsigned char>(*iter++);
    }
  }

  sum = (sum >> 16) + (sum & 0xffff);         /* add high 16 to low 16 */
  sum += (sum >> 16);                         /* add carry */
  return static_cast<unsigned short>(~sum);   /* ones-complement, truncate */
}

#endif
