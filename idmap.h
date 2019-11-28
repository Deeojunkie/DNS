#include <stdint.h>
#include <netdb.h>
#include "hash.h"
#include <time.h>

#ifndef DNS_IDMAP_H
#define DNS_IDMAP_H

typedef HashTable IdMap;
#define AGE 30

//IdMap中哈希节点的value的数据结构
typedef struct {
	struct sockaddr_in clientAddr;    //客户端地址
	uint16_t id;      //客户端使用的dns报文中的旧id
	time_t requireTime;     //时间戳
}IpId;

//初始化
IdMap IdMap_init();

IpId *IdMap_search(IdMap idMap, uint16_t id);

/*
 * @note 插入新的id映射
 *
 * @param uint16_t id     新id
 *        struct sockaddr_in clientAddr      客户端地址
 *        uint16_t idOld      旧id
 *
 */
void IdMap_insert(IdMap idMap, uint16_t id, struct sockaddr_in clientAddr,uint16_t idOld);

void IdMap_update(IdMap idMap);

IpId * IdMap_remove(IdMap idMap, uint16_t id);

void IdMap_destory(IdMap idMap);

#endif //DNS_IDMAP_H
