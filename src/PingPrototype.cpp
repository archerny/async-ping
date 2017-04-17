#include "IcmpPacket.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#define IPHDR 20
#define MAX_DATA_SIZE (IP_MAXPACKET - IPHDR - ICMP_MINLEN)
using namespace std;
extern int errno;

int main()
{

    unsigned char type = 8;
    unsigned char code = 0;
    unsigned short id = 6578;
    unsigned short seq = 245;

    const string content = "Hello from xping.";
    const string target = "47.92.91.5";
    // new implementation for icmp
    IcmpPacket packet = IcmpPacket(type, code, id, seq, content);
    size_t size = packet.getPktSize();
    unsigned char *buf = packet.getBuf();
    for (size_t i = 0; i < size; i ++)
    {
        cout << static_cast<int>(buf[i]) << " ";
    }
    cout << endl;

    struct protoent *proto = NULL;
    if (!(proto = getprotobyname("icmp")))
    {
        cout << "no icmp proto" << endl;
        exit(1);
    }
    int rawSocketFd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (rawSocketFd == -1)
    {
        cout << "create raw socket error" << endl;
        exit(1);
    }
    cout << "raw socket fd: " << rawSocketFd << endl;
    // api:
    //ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
    //               const struct sockaddr *dest_addr, socklen_t addrlen);
    // sockaddr_in and sockaddr_in6 are both structures where first member
    // is a sockaddr structure, so a direct cast is acceptable
    struct sockaddr_in sa;
    //sa.sin_family=AF_INET
    inet_pton(AF_INET, target.c_str(), &(sa.sin_addr));
    sa.sin_family=AF_INET;
    cout << "\terrorno: " << errno << endl;
    ssize_t nsent = sendto(rawSocketFd, buf, size, 0, (struct sockaddr *) &sa, sizeof(struct sockaddr_in));
    cout << "nsent: " << nsent  << "\terrorno: " << errno << endl;

    unsigned char recvPacket[MAX_DATA_SIZE];
    int nrecv;
    struct ip *ipPacket = (struct ip *)recvPacket;
    struct icmphdr *icmpHeader;
    int hlen = 0;
    socklen_t slen = sizeof(struct sockaddr);
    struct sockaddr_in remote;
    nrecv = recvfrom(rawSocketFd, recvPacket, sizeof(recvPacket), 0,  (struct sockaddr *)&remote, &slen);
    cout << "nreceive: " << nrecv << "\t" << endl;
    //readfrom()
    if (nrecv < 0)
    {
        cout << "recv error" << endl;
    }

    // one more icmp packet recved
    // IHL: 4 bits Internet Header Length is the length of the internet header in 32 bit words,
    // and thus points to the beginning of the data. Note that the minimum value for a correct
    // header is 5.
    hlen = ipPacket->ip_hl * 4; // in bytes
    if (hlen < 20 || nrecv < hlen + ICMP_MINLEN)
    {
        // too short
        cout << "packet too short, drop it" << endl;
        exit(1);
    }
    for (int i = hlen; i < nrecv; i ++)
    {
        cout << static_cast<int>(recvPacket[i])  << " ";
    }
    cout << endl;


    icmpHeader = (struct icmphdr *)(recvPacket + hlen);
    cout << "icmp->un.echo.id: " << icmpHeader->un.echo.id << endl;
    cout << "icmp->un.echo.seq: " << icmpHeader->un.echo.sequence << endl;
    // check payload
    char charIpBuf[20] = {0};
    if (inet_ntop(AF_INET, &(remote.sin_addr), charIpBuf, 20) == NULL)
    {
        cout << "convert error, error #" << errno << endl;
    }
    printf("target ip is: %s\n", charIpBuf);
    return 0;
}
