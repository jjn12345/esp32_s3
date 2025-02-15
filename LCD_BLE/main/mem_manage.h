#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* 平台适配层（根据具体RTOS修改） */
#define DISABLE_INTERRUPTS()  // 关闭中断代码
#define ENABLE_INTERRUPTS()   // 开启中断代码
typedef uint32_t mutex_t;
#define MUTEX_LOCK(m)         // 互斥锁加锁
#define MUTEX_UNLOCK(m)       // 互斥锁解锁

/* 内存块头结构（8字节）*/
typedef struct block_header{
    uint32_t magic;         // 魔数校验 0x55AA55AA
    uint16_t size;          // 数据区大小（字节数）
    bool used;              // 使用标志
    struct block_header* prev;
    struct block_header* next;
} block_header_t;

/* 内存池控制结构 */
typedef struct {
    uint8_t* pool_start;     // 内存池起始地址
    uint32_t pool_size;      // 内存池总大小
    block_header_t* free_list;// 空闲链表头
    mutex_t mutex;           // 线程安全互斥锁
#ifdef DEBUG
    uint32_t alloc_count;    // 分配计数
    uint32_t free_count;     // 释放计数
    uint32_t frag_score;     // 碎片评分
#endif
} mem_pool_t;











/* 内存初始化（必须4字节对齐） */
bool my_mem_init(mem_pool_t* pool, void* buffer, uint32_t size);
/* 内存分配（带4字节对齐） */
void* mem_alloc(mem_pool_t* pool, uint32_t size);
/* 内存释放 */
bool my_mem_free(mem_pool_t* pool, void* ptr);
/* 内存状态查询 */
typedef struct {
    uint32_t total;     
    uint32_t used;
    uint32_t free;
    uint32_t largest_free;
    uint32_t frag_factor; // 0-100%
} mem_status_t;
void mem_get_status(mem_pool_t* pool, mem_status_t* status);
