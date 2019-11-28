#include <stdio.h>
#include <string.h>
#include <sys/malloc.h>
#include "ipcache.h"
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>

/* 字符串哈希函数实质上是把每个不同的字符串映射成不同的整数
   哈希函数需要保证唯一性，避免冲突 */
unsigned int SDBMHash(const void *key)
{
	register unsigned int hash = 0;
	const char *str = key;
	unsigned int ch;
	while (ch = (size_t)*str++)
	{
		hash = 65599 * hash + ch;
		//hash = (size_t)ch + (hash << 6) + (hash << 16) - hash;
	}
	return hash;
}

unsigned int ipHash(const void *key)    //IP地址的哈希函数
{
	return (unsigned int) *(uint32_t *)key;
}


int cmpHost(const void *x, const void *y){    //x=y返回0，否则返回非0
	return strcmp(x,y);
}

int cmpIp(const void *x, const void *y){
	return (*(uint32_t *)x)-(*(uint32_t *)y);
}

static void freeKey(const void *key){
	free((void *)key);
}

IpCache IpCache_init()
{
	IpCache ipCache;
	ipCache = HashTable_create(100,cmpHost,SDBMHash,freeKey);
	return ipCache;
}


//从文件中读取ip和域名的映射
void IpCache_read(IpCache ipCache, char filename[])
{
	FILE * readfile = fopen(filename, "r");
	char hostbuff[256], *host;
	char ipbuff[256];
	uint32_t *ip = NULL;
	if (readfile == NULL)
	{
		fprintf(stderr, "Can't open file %s\n", filename);
		exit(-1);
	}

	fscanf(readfile, " %s", ipbuff);
	fscanf(readfile, " %s", hostbuff);
	while (!feof(readfile))
	{
		host = strdup(hostbuff);   //字符串拷贝
		ip = (uint32_t *)malloc(sizeof(uint32_t));
		if (ip == NULL)
		{
			fprintf(stderr,"malloc error %s",strerror(errno));
			exit(-1);
		}
		if(inet_pton(AF_INET,ipbuff,ip) < 0)   //将ipbuff中的点分十进制串转换成网络字节序二进制值ip
		{
			fprintf(stderr,"convert: %s",strerror(errno));
			exit(-1);
		}

		HashSet tmpSet = HashTable_get(ipCache,host);
		if (tmpSet == NULL)      //该域名还没有对应的ip地址，创建一个新的hashset
		{
			tmpSet = HashSet_create(100,cmpIp,ipHash);
			HashSet_insert(tmpSet,ip);
			HashTable_insert(ipCache,host,tmpSet);   //表，键，值
		}
		else   //该域名已经有对应的hashset
		{
			free(host);
			HashSet_insert(tmpSet,ip);   //将ip插入到host对应的hashset中
		}
		fscanf(readfile, " %s", ipbuff);
		fscanf(readfile, " %s", hostbuff);
	}

}

uint32_t **IpCache_search(IpCache ipCache, char *host)
{
	uint32_t **ipArray;
	HashSet tmpSet = HashTable_get(ipCache,host);   //得到该host对应的hashset
	if(tmpSet == NULL)
	{
		return NULL;
	}
	else
	{
		ipArray = (uint32_t **) HashSet_toArray(tmpSet, NULL);
		return ipArray;
	}
}

void IpCache_insert(IpCache ipCache, char *host, uint32_t ip)
{
	HashSet tmpSet = HashTable_get(ipCache,host);
	if (tmpSet == NULL)
	{
		tmpSet = HashSet_create(5,cmpIp,ipHash);
		HashTable_insert(ipCache,host,tmpSet);
	}
	else
	{
		free(host);
		uint32_t *ip_ptr = (uint32_t*)malloc(sizeof(uint32_t));
		*ip_ptr = ip;
		HashSet_insert(tmpSet,ip_ptr);
	}
}



void freeIp(const void *key, void **value, void *c1)
{
	free(*value);
}

void freeSet(const void *key, void **value, void *c1)
{
	HashSet tmpSet = *value;
	HashSet_map(tmpSet,freeIp,c1);
	HashSet_destory(&tmpSet);
}

void IpCache_destory(IpCache ipCache)
{
	HashTable_map(ipCache,freeSet,NULL);
	HashTable_destory(&ipCache);
}
