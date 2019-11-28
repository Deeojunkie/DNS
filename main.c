#include <stdio.h>
#include "DNS.h"
#include "hash.h"
#include "ipcache.h"
#include "idmap.h"
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define PORT 53  //DNS服务器的端口

int debug = 0;

int main(int argc, char **argv)
{
    int socketfd;
    char *filename = NULL;
    uint32_t upIp = 0;
    uint16_t id = 1;
    
    /*命令行参数处理模块*/
    /*
     * 参数格式: xxx -d -f c:/xx/ -i 192.168.0.1
     * 报错情况:
     *          (1)-f和-i后,没有内容
     */
    printf("argc:%d\n",argc);
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            printf("argv[%d]:%s\n",i,argv[i]);
            if (argv[i][0] == '-')
            {
                for (int j = 0; argv[i][j] != '\0'; ++j)
                {
                    if (argv[i][j] == 'd')
                    {
                        debug = debug==2?2:debug+1;
                        printf("debug:%d\n",debug);
                    }else if (argv[i][j] == 'f')
                    {
                        i++;
                        filename = argv[i];
                        break;
                    }
                    //处理IP地址参数
                    if (argv[i][j] == 'i')
                    {
                        i++;
                        //inet_pton是IP地址转换函数；AF_INET是IPv4 网络协议的套接字类型
                        if(inet_pton(AF_INET,argv[i],&upIp) < 0)
                        {
                            fprintf(stderr,"convert: %s",strerror(errno));
                            exit(-1);
                        }
                    }
                }
            }
        }
    }
    
    /********************初始化*********************/
    /*读入“IP地址-域名”对照表*/
    IpCache ipCache = IpCache_init();   //创建一个hash表来保存
    if (filename != NULL)
    {
        IpCache_read(ipCache,filename); //文件名不为空
    }
    else{
        //https://blog.csdn.net/yangpan011/article/details/78308860
        //char fn[20]  = "./dnsrelay.txt";
        char fn[200]  = "/Users/mac/Desktop/雷思悦的大四上/计网小学期/project/dns_server_final/dns_server/dnsrelay.txt";
        filename = fn;
        IpCache_read(ipCache,filename); //文件名为空, 默认使用当前路径下的文件
    }
    
    IdMap idMap = IdMap_init();
    
    /*读入外部DNS服务器的地址*/
    if (upIp == 0)
    {
        //没有在命令行参数指定地址，使用默认的8.8.8.8（自行查看自己本机的情况）
        if(inet_pton(AF_INET,"8.8.8.8",&upIp) < 0)
        {
            fprintf(stderr,"convert: %s",strerror(errno));
            exit(-1);
        }
    }
    
    /*初始化DNS报文*/
    DNS dns;
    dns.host = NULL;
    dns.buff = NULL;
    dns.size_n = 0;
    
    /*****************开启UDP服务器***********************/
    socketfd = socket(AF_INET,SOCK_DGRAM,0);//UDP报文流
    if (socketfd < 0)
    {
        fprintf(stderr,"socket: %s\n",strerror(errno));
        exit(1);
    }
    
    /*程序作为服务器，创建一个socket*/
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, len);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);    // 绑定PORT端口
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  //绑定IP地址，
                                        //为什么绑定INADDR_ANY：https://blog.csdn.net/stpeace/article/details/18237845
    
    if (bind(socketfd, (const struct sockaddr *) &addr, len))//绑定
    {
        fprintf(stderr, "bind: %s\n", strerror(errno));
        exit(1);
    }
    
    //
    char buff[65536];
    char addrstr[256];
    struct sockaddr_in client_addr;
    size_t client_len = sizeof(client_addr);
    
    /*和外部DNS服务器绑定*/
    struct sockaddr_in up_addr;
    up_addr.sin_addr.s_addr = upIp;
    up_addr.sin_family = AF_INET;
    up_addr.sin_port = htons(PORT);
    
    /************循环检查是否收到报文*************/
    while (1)
    {
        if (debug == 2)
        {
            printf("waiting!\n");
        }
        IdMap_update(idMap);//删除超时的映射关系
        if (debug == 2)
        {
            printf("deleted idMap! length:%d\n",idMap->length);
        }
        
        memset(buff,0,65536);
        memset(&client_addr,0,client_len);
        dns.buff = buff;
        //从客户端收包
        /*
            int recvfrom(int s, void *buf, int len, unsigned int flags, struct sockaddr *from,int *fromlen);
            函数说明：
            recvfrom()用来接收远程主机经指定的socket 传来的数据, 并把数据存到由参数buf 指向的内存空间, 参数len 为可接收数据的最大长度. 参数flags一般设0, 其他数值定义请参考recv(). 参数from 用来指定欲传送的网络地址, 结构sockaddr 请参考bind(). 参数fromlen 为sockaddr 的结构长度.
            返回值：成功则返回接收到的字符数, 失败则返回-1, 错误原因存于errno 中.
            http://c.biancheng.net/cpp/html/368.html
         */
        ssize_t size_n = recvfrom(socketfd, buff, 65536, 0, (struct sockaddr *) &client_addr, (socklen_t *) &client_len);
        if (size_n < 0)
        {
            fprintf(stderr, "recvfrom: %s\n", strerror(errno));
            exit(1);
        }
        
        printf("收到客户端发的请求包: %s\n",inet_ntop(AF_INET,&client_addr.sin_addr.s_addr,addrstr,256));
        
        dns.size_n = (size_t) size_n;   //收到报文的长度
        if (debug == 2)
        {
            printf("收到客户端发的请求包，包长size_n为:%zi\n",size_n);
        }
        dns= DNS_getHead(dns);
        if (debug == 2)
        {
            printf("id:%d\n",dns.dnsHeader.id);
            printf("QR:%d\n",dns.dnsHeader.QR);
            printf("OPCODE:%d\n",dns.dnsHeader.OPCODE);
            printf("AA:%d\n",dns.dnsHeader.AA);
            printf("TC:%d\n",dns.dnsHeader.TC);
            printf("RD:%d\n",dns.dnsHeader.RD);
            printf("RA:%d\n",dns.dnsHeader.RA);
            printf("RCODE:%d\n",dns.dnsHeader.RCODE);
            printf("QDCOUNT:%d\n",dns.dnsHeader.QDCOUNT);
            printf("ANCOUNT:%d\n",dns.dnsHeader.ANCOUNT);
            printf("NSCOUNT:%d\n",dns.dnsHeader.NSCOUNT);
            printf("ARCOUNT:%d\n",dns.dnsHeader.ARCOUNT);
        }
        if (dns.dnsHeader.QR == 0 && dns.dnsHeader.OPCODE == 0)  //查询报文
        {
            if (debug >= 1)
            {
                //printf("receive a question\n");
                printf("收到客户端发来的查询报文!\n");
            }
            
            dns = DNS_getHost(dns);
            if (debug >= 1)
            {
                //printf("query host: %s\n", dns.host);
                printf("客户端请求的主机是: %s\n", dns.host);
            }
            
            if (debug == 2)
            {
                printf("QTYPE:%d QCLASS:%d\n",dns.QTYPE,dns.QCLASS);
            }
            
            //if (dns.QTYPE == 1 && dns.QCLASS == 1)//其他问题类型不查询
            if(1)   //准备查询本地IPCache
            {
                uint32_t **ipArray = IpCache_search(ipCache,dns.host);
                if (ipArray == NULL || ipArray[0] == NULL)  //本地缓存表中未查询到（为什么要这么写两个？）
                {
                    //没找到，需要发给外部服务器询问，存到idmap中
                    IdMap_insert(idMap,id,client_addr,dns.dnsHeader.id);//添加映射
                    dns = DNS_changeId(dns,id);//更改发送给外部服务器的id
                    if (debug >= 1)
                    {
                        //printf("sendto server id: %d\n",dns.dnsHeader.id);
                        printf("发送给服务器的id: %d\n",dns.dnsHeader.id);
                    }
                    id++; //修改一下自己这的id，便于下次使用
                    //发给外部服务器
                    if(sendto(socketfd, buff, dns.size_n, 0, (const struct sockaddr *) &up_addr, sizeof(up_addr)) < 0)
                    {
                        fprintf(stderr, "sendto: %s\n", strerror(errno));
                        exit(1);
                    }
                    //printf("send to: %s\n",inet_ntop(AF_INET,&up_addr.sin_addr.s_addr,addrstr,256));
                    printf("发送给外部DNS服务器:%s\n",inet_ntop(AF_INET,&up_addr.sin_addr.s_addr,addrstr,256));
                }
                else                                        //本地缓存表中查询到
                {
                    printf("查找到IP为：%d", *(ipArray[0]));
                    if (*(ipArray[0]) == 0)   //非法地址
                    {
                        if (debug == 2)
                        {
                            //printf("blocked site:%s\n", dns.host);
                            printf("被拦截的网站:%s\n", dns.host);
                        }
                        dns = DNS_errorAnswer(dns);//返回找不到域名
                    }else{
                        
                        if (dns.QTYPE == 28 && dns.QCLASS == 1){
                            //查询报文要查的是IPV6地址，且不是非法地址，则转发给外部服务器
                            IdMap_insert(idMap,id,client_addr,dns.dnsHeader.id);//添加映射
                            dns = DNS_changeId(dns,id);//更改发送给外部服务器的id
                            if (debug >= 1)
                            {
                                //printf("sendto server id: %d\n",dns.dnsHeader.id);
                                printf("查询报文为IPV6，转发给外部fDNS服务器！");
                                printf("发送给服务器的id: %d\n",dns.dnsHeader.id);
                            }
                            id++; //修改一下自己这的id，便于下次使用
                            //发给外部服务器
                            if(sendto(socketfd, buff, dns.size_n, 0, (const struct sockaddr *) &up_addr, sizeof(up_addr)) < 0)
                            {
                                fprintf(stderr, "sendto: %s\n", strerror(errno));
                                exit(1);
                            }
                            //printf("send to: %s\n",inet_ntop(AF_INET,&up_addr.sin_addr.s_addr,addrstr,256));
                            printf("发送给外部DNS服务器:%s\n",inet_ntop(AF_INET,&up_addr.sin_addr.s_addr,addrstr,256));
                            
                        }
                        else{
                            if (debug == 2)
                            {
                                //printf("found ip:%u\n", *(ipArray[0]));
                                printf("查询到IP:%u\n", *(ipArray[0]));
                            }
                            //dns = DNS_addAnswer(dns, *(ipArray[0]));//只加一个ip地址进去
                            dns = DNS_addAnswer(dns, htonl(*(ipArray[0])));//只加一个ip地址进去
                            if (debug == 2)
                            {
                                dns= DNS_getHead(dns);
                                printf("id:%d\n",dns.dnsHeader.id);
                                printf("QR:%d\n",dns.dnsHeader.QR);
                                printf("OPCODE:%d\n",dns.dnsHeader.OPCODE);
                                printf("AA:%d\n",dns.dnsHeader.AA);
                                printf("TC:%d\n",dns.dnsHeader.TC);
                                printf("RD:%d\n",dns.dnsHeader.RD);
                                printf("RA:%d\n",dns.dnsHeader.RA);
                                printf("RCODE:%d\n",dns.dnsHeader.RCODE);
                                printf("QDCOUNT:%d\n",dns.dnsHeader.QDCOUNT);
                                printf("ANCOUNT:%d\n",dns.dnsHeader.ANCOUNT);
                                printf("NSCOUNT:%d\n",dns.dnsHeader.NSCOUNT);
                                printf("ARCOUNT:%d\n",dns.dnsHeader.ARCOUNT);
                                
                                printf("size_n:%zi\n",dns.size_n);
                                
                            }
                        }
                        
                    }
                    
                    //发送回包
                    if(sendto(socketfd, buff, dns.size_n, 0, (const struct sockaddr *) &client_addr, (socklen_t) client_len) < 0)
                    {
                        fprintf(stderr, "sendto: %s\n", strerror(errno));
                        exit(1);
                    }
                    free(ipArray);
                }
            }
            
        }else if (dns.dnsHeader.QR == 1 && dns.dnsHeader.RCODE == 0)                 //回答报文
        {
            if (debug >= 1)
            {
                //printf("receive a answer\n");
                printf("收到外部服务器发来的回答报文！\n");
            }
            
            dns = DNS_getHost(dns);
            if (debug >= 1)
            {
                //printf("answer host: %s\n",dns.host);
                printf("响应我的域名: %s\n",dns.host);
            }
            
            IpId *ipId = IdMap_search(idMap,dns.dnsHeader.id);
            if (ipId != NULL)                               //找到映射
            {
                uint16_t deleteId = dns.dnsHeader.id;
                if(debug == 2)
                {
                    printf("found id:%d old id:%d\n",dns.dnsHeader.id,ipId->id);
                }
                dns = DNS_changeId(dns,ipId->id);
                if(sendto(socketfd, buff, dns.size_n, 0, (const struct sockaddr *) &(ipId->clientAddr), sizeof(ipId->clientAddr)) < 0)
                {
                    fprintf(stderr, "sendto: %s\n", strerror(errno));
                    exit(1);
                }
                
                ipId = IdMap_remove(idMap,deleteId);
                if (ipId!=NULL)
                {
                    free(ipId);
                }
            }
            else
            {
                //超时删掉了id映射
                if(debug>=1)
                {
                    //printf("!!!can't find the id!!!\n");
                    printf("无法找到ID, 可能原因为超时删掉了id映射");
                }
            }
        }
        //清除DNS报文n
        DNS_clear(&dns);
    }
    
    close(socketfd);
    
    return 0;
}
