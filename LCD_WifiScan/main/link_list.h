#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "mem_manage.h" 


/* 双向链表节点结构（保持4字节对齐） */
typedef struct dlist_node {
    struct dlist_node *prev;
    struct dlist_node *next;
    void* data;
}dlist_node_t;

/* 链表控制结构（含内存池引用） */
typedef struct {
    dlist_node_t *head;
    dlist_node_t *tail;
    uint16_t count;
    mem_pool_t *mempool;  // 关联内存池
} dlist_t;


/* 前向遍历宏（节省函数调用开销） */
#define DLIST_FOREACH(list, node) \
    for (dlist_node_t *(node) = (list)->head; (node) != NULL; (node) = (node)->next)

/* 反向遍历宏 */
#define DLIST_FOREACH_REVERSE(list, node) \
    for (dlist_node_t *(node) = (list)->tail; (node) != NULL; (node) = (node)->prev)


/* 安全遍历宏（防止遍历过程中节点被释放） */
#define DLIST_FOREACH_SAFE(list, node, tmp) \
    for (dlist_node_t*(node) = (list)->head, *(tmp) = (node)?(node)->next:NULL; \
         (node) != NULL; \
         (node) = (tmp), (tmp) = (node)?(node)->next:NULL)


/* 链表初始化（绑定内存池） */
bool dlist_init(dlist_t* list, mem_pool_t* pool);
/* 插入头节点（线程安全） */
bool dlist_insert_head(dlist_t* list, void* data);
/* 插入尾节点（带内存分配回滚保护） */
bool dlist_insert_tail(dlist_t* list, void* data);
/* 安全移除节点（自动内存回收） */
bool dlist_remove(dlist_t* list, dlist_node_t* node);
/* 示例使用 */
void example_usage(mem_pool_t* pool);