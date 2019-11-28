#include "main.h"

#include "DNS.h"
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/malloc.h>
#include <stdlib.h>

static void updateHead(DNSHeader dnsHeader, char *buff)
{
    //更新buff里面的头部信息
    uint16_t temp;
    
    temp = htons(dnsHeader.id);		//htons()将一个16位数从主机字节顺序转换成网络字节顺序
    memcpy(buff,&temp,sizeof(temp));
    
    memcpy(&temp,(void *)&dnsHeader + 2,sizeof(temp));
    temp = htons(temp);
    memcpy(buff+2,&temp,sizeof(temp));
    
    temp = htons(dnsHeader.QDCOUNT);
    memcpy(buff+4,&temp,sizeof(temp));
    temp = htons(dnsHeader.ANCOUNT);
    memcpy(buff+6,&temp,sizeof(temp));
    temp = htons(dnsHeader.NSCOUNT);
    memcpy(buff+8,&temp,sizeof(temp));
    temp = htons(dnsHeader.ARCOUNT);
    memcpy(buff+10,&temp,sizeof(temp));
}

DNS DNS_getHead(DNS dns)
{
    uint16_t temp;
    //ntohs()函数将一个16位数由网络字节顺序转换为主机字节顺序
    dns.dnsHeader.id = ntohs(*(uint16_t *)dns.buff);    //从buff的起始地址取16个bit
    memcpy(&temp,dns.buff+2,sizeof(temp));  //从buff之后的2个字节开始，读16bit，存到temp里面
    temp = ntohs(temp);
    memcpy(((char *)&(dns.dnsHeader))+2,&temp,sizeof(temp));
    dns.dnsHeader.QDCOUNT = ntohs(*(uint16_t *)(dns.buff + 4));
    dns.dnsHeader.ANCOUNT = ntohs(*(uint16_t *)(dns.buff + 6));
    dns.dnsHeader.NSCOUNT = ntohs(*(uint16_t *)(dns.buff + 8));
    dns.dnsHeader.ARCOUNT = ntohs(*(uint16_t *)(dns.buff + 10));
    return dns;
}

DNS DNS_getHost(DNS dns)
{
    assert(dns.dnsHeader.QDCOUNT>0);	//如果它的条件返回错误，则终止程序执行
    int i;
    char *host = dns.buff + 12;

    //i指向的是问题域的最后一个字符0
    //host是一个或多个标识符的序列,每个标识符以首字节的计数值来说明随后标识符的字节长度
	//每个名字以最后字节为0结束，长度为0的标识符是根标识符

    for (i = 0; host[i] != 0;)
    {
        i = i + host[i] + 1;
    }
    
    if (dns.host != NULL)
    {
        free(dns.host);
        dns.host = NULL;
    }
    
    dns.host = malloc((size_t) i);//给'/0'留个位置
    
    for (int j = 0; j < i; )
    {
        for (int k = 0; k < host[j]; ++k)
        {
            dns.host[j+k] = host[j+k+1];
        }
        j=j+host[j];
        dns.host[j] = '.';
        j++;
    }
    dns.host[i-1] = '\0';
    
    //12+i指向0，下一位是QTYPE的开始
    dns.QTYPE = ntohs(*(uint16_t *)(dns.buff + 12 + i + 1));
    dns.QCLASS = ntohs(*(uint16_t *)(dns.buff + 12 + i + 1 + 2));
    
    dns.questionLength = i + 1 + 4;
    
    dns.answerOffset = 12 + i + 1 + 4;//回答域的开始，在回答包中有用
    
    return dns;
}

DNS DNS_addAnswer(DNS dns, uint32_t ip)
{
    uint16_t uint16;
    uint32_t uint32;
    dns.dnsHeader.ANCOUNT = 1;
    dns.dnsHeader.QR = 1;
    dns.dnsHeader.RCODE = 0;
    updateHead(dns.dnsHeader,dns.buff);
    dns.ip = ip;
    memcpy(dns.buff+dns.answerOffset,dns.buff+12, (size_t) dns.questionLength);//资源中的域名和类型和类
    uint32 = htonl(172800);	//TTL 两天的生存期
    memcpy(dns.buff+dns.answerOffset+dns.questionLength,&uint32,sizeof(uint32));
    uint16 = htons(4);	//RDLENGTH：资源数据长度
    memcpy(dns.buff+dns.answerOffset+dns.questionLength+4,&uint16,sizeof(uint16));
    uint32 = htonl(ip);	//RDATA：资源数据
    memcpy(dns.buff+dns.answerOffset+dns.questionLength+6,&uint32,sizeof(uint32));
    
    dns.size_n = (size_t) (12 + dns.questionLength + dns.questionLength + 6 + 4);
    
    return dns;
}

DNS DNS_changeId(DNS dns, uint16_t id)
{
    dns.dnsHeader.id = id;
    updateHead(dns.dnsHeader,dns.buff);
    return dns;
}

DNS DNS_errorAnswer(DNS dns)
{
    dns.dnsHeader.RCODE = 3;
    dns.dnsHeader.AA = 1;
    updateHead(dns.dnsHeader,dns.buff);
    return dns;
}

void DNS_clear(DNS *dns)
{
    if (dns->host!=NULL)
    {
        free(dns->host);
    }
    memset(dns,0,sizeof(DNS));
}


