#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "idmap.h"

int cmpId(const void *x, const void *y){
	return (*(uint16_t *)x)-(*(uint16_t *)y);
}

static void freeKey(const void *key){

}

unsigned int idHash(const void *key)   //id的哈希函数
{
	return (unsigned int) *(uint16_t *)key;
}

IdMap IdMap_init()
{
	IdMap idMap;
	idMap = HashTable_create(300,cmpId,idHash,freeKey);
	return idMap;
}

IpId *IdMap_search(IdMap idMap, uint16_t id)
{
	IpId *result;
	result = HashTable_get(idMap,&id);
	return result;
}

void IdMap_insert(IdMap idMap, uint16_t id, struct sockaddr_in clientAddr, uint16_t idOld)
{
	uint16_t *id_ptr;
	IpId * ipId_ptr;

	id_ptr = malloc(sizeof(uint16_t));
	if (id_ptr == NULL)
	{
		fprintf(stderr,"malloc error %s",strerror(errno));
		exit(-1);
	}
	*id_ptr = id;

	ipId_ptr = malloc(sizeof(IpId));
	if (ipId_ptr == NULL)
	{
		fprintf(stderr,"malloc error %s",strerror(errno));
		exit(-1);
	}
	ipId_ptr->id = idOld;   //客户端发过来的旧id
	ipId_ptr->clientAddr = clientAddr;   //客户端的ip地址和端口
	ipId_ptr->requireTime = time(NULL);    //id映射的时间

	HashTable_insert(idMap, id_ptr, ipId_ptr);   // key：新id    value：Ipid结构体『旧id、客户端地址、时间戳』
}

typedef struct{
	uint16_t **idArray;
	int length;
}IdArray;

void apply(const void *key, void **value, void *c1)
{
	time_t timeNow = time(NULL);
	uint16_t **idArray = ((IdArray *)c1)->idArray;
	IpId *ipId = *value;
	assert(ipId);
	if (timeNow - ipId->requireTime > AGE)   //删除超过30秒
	{
	    idArray[((IdArray *)c1)->length++] = key;
	}
}

void IdMap_update(IdMap idMap)    //刷新IdMap，删除超时映射
{
	IdArray * idArray = malloc(sizeof(IdArray));
	idArray->length = 0;
	idArray->idArray = malloc(sizeof(uint16_t *)*HashTable_length(idMap));

	HashTable_map(idMap,apply,idArray);   //idArray->idArray： 超时的id的指针数组

	for (int i = 0; i < idArray->length; ++i)
	{
		HashTable_remove(idMap,idArray->idArray[i]);
	}

	free(idArray->idArray);
	free(idArray);
}

IpId * IdMap_remove(IdMap idMap, uint16_t id)
{
	IpId *ipId;
	ipId = HashTable_remove(idMap, &id);
	return ipId;
}


void freeIdIp(const void *key, void **value, void *c1)
{
	free(*value);
}

void IdMap_destory(IdMap idMap)
{
	HashTable_map(idMap,freeIdIp,NULL);
	HashTable_destory(&idMap);
}

