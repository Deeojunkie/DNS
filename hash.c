//#include <values.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
//#include <malloc.h>
#include <sys/malloc.h>
#include "hash.h"

static int cmpAtom(const void *x, const void *y) {
    return x != y;
}

static unsigned long hashAtom(const void *key) {
    return (unsigned long)key>>2;
}

static void freeKeyAtom(const void *key) {
    
}

HashTable HashTable_create(int hint, int (*cmp)(const void *, const void *), unsigned int (*hash)(const void *),void (*freeKey)(const void *key))
{
    /*
     params:
     hint:哈希表中bucket的数目
     int (*cmp)(const void *x, const void * y): 函数指针，在调用HashTable_create的文件中写cmp的定义
     */
    HashTable hashTable;
    int i;
    static unsigned int primes[] = {509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX};
    
    //一进入函数先进行参数的检查
    assert(hint >= 0);
    
    //确定bucket的数目，分配内存，bucket内存紧跟着hashTable
    for (i = 1; primes[i] < hint; ++i); //给哈希表分配空间, 找到prime数组中第一个大于hint的数
    hashTable = malloc(sizeof(*hashTable)+primes[i-1]*sizeof(HashNode));
    
    //初始化
    hashTable->size = primes[i-1];
    //以下3个参数都是函数的地址，表示使用哪个函数
    hashTable->cmp = cmp?cmp:cmpAtom;
    hashTable->hash = hash?hash:hashAtom;
    hashTable->freeKey = freeKey?freeKey:freeKeyAtom;
    //意思应该是紧跟着hashTable之后的内存分给bucket,hashTable+1就是从hashTable的起始地址开始，又往后移动了一个hashTable大小的位置。
    hashTable->bucket = (HashNode **)(hashTable+1); //bucket存的是地址
    for (i = 0; i < hashTable->size; ++i)
        hashTable->bucket[i] = NULL;
    hashTable->length = 0;
    hashTable->timestamp = 0;
    
    return hashTable;
}

void HashTable_destory(HashTable *hashTable)
{
    assert(hashTable && *hashTable);
    
    if ((*hashTable)->length > 0)
    {
        HashNode *p, *q;
        for (int i = 0; i < (*hashTable)->length; ++i)
        {
            for (p = (*hashTable)->bucket[i]; p; p = q)
            {
                q = p->next;
                ((*hashTable)->freeKey)(p->key);
                free(p);
            }
        }
    }
    free(hashTable);
}

int HashTable_length(HashTable hashTable)
{
    return hashTable->length;
}

void *HashTable_insert(HashTable hashTable, const void *key, void *value)
{
    void *prev = NULL;//之前的值
    HashNode *p;
    unsigned int index;
    
    assert(hashTable);
    assert(key);
    
    //在hashTable里面寻找key对应的index
    index = hashTable->hash(key) % hashTable->size;
    //printf("insert index:%d\n", index);
    //若key在这一行不存在，则新建一个节点，插到这一行对应链表的头节点;
    //若存在, 则替换。
    for(p = hashTable->bucket[index]; p; p = p->next)
    {
        if (hashTable->cmp(key, p->key) == 0)
        {
            break;
        }
    }
    
    if (p == NULL)
    {
        p = malloc(sizeof(*p));
        p->key = key;
        //注意，我用的是头插法
        p->next = hashTable->bucket[index];
        hashTable->bucket[index] = p;
        hashTable->length++;
    }else
    {
        prev = p->value;
    }
    
    p->value = value;
    hashTable->timestamp++;
    
    return prev;
}

void *HashTable_get(HashTable hashTable, const void *key)
{
    //key存的是域名
    unsigned int index;
    HashNode *p;
    
    assert(hashTable);
    assert(key);
    //printf("get key:%d\n", (unsigned int) *(uint32_t *)key);
    
    //使用存在hashTable中的hash函数(默认是SDBMHash)
    index = hashTable->hash(key) % hashTable->size;
    //printf("get index:%d\n", index);
    for(p = hashTable->bucket[index]; p; p = p->next)
    {
        if (hashTable->cmp(key, p->key) == 0)
        {
            break;
        }
    }
    
    return p?p->value:NULL;
}

void *HashTable_remove(HashTable hashTable, const void *key)
{
    HashNode **pp;
    unsigned int index;
    
    assert(hashTable);
    assert(key);
    
    //printf("key:%d\n", *(int *) key);
    
    hashTable->timestamp++;
    index = (hashTable->hash)(key) % hashTable->size;
    //printf("index:%d\n", index);
    for (pp = &hashTable->bucket[index]; *pp; pp = &((*pp)->next))
    {
        if ((hashTable->cmp)(key, (*pp)->key) == 0)
        {
            HashNode *p = *pp;
            void *value = p->value;
            *pp = p->next;
            (hashTable->freeKey)(p->key);
            free(p);
            hashTable->length--;
            return value;
        }
    }
    
    return NULL;
}

void HashTable_map(HashTable hashTable, void (*apply)(const void *key, void **value, void *c1), void *c1)
{
    HashNode *p;
    unsigned int stamp;
    
    assert(hashTable);
    assert(apply);
    
    stamp = hashTable->timestamp;
    for (int i = 0; i < hashTable->size; ++i)
    {
        for (p = hashTable->bucket[i]; p; p = p->next)
        {
            apply(p->key,&(p->value),c1);
            assert(stamp == hashTable->timestamp);
        }
    }
    
}

void **HashTable_toArray(HashTable hashTable, void *end) {
    int i, j = 0;
    void **array;
    HashNode *p;
    assert(hashTable);
    array = malloc((2*hashTable->length + 1)*sizeof (*array));
    for (i = 0; i < hashTable->size; i++)
        for (p = hashTable->bucket[i]; p; p = p->next) {
            array[j++] = (void *)p->key;
            array[j++] = p->value;
        }
    array[j] = end;
    return array;
}
