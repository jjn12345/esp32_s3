#include "link_list.h"


/* 链表初始化（绑定内存池） */
bool dlist_init(dlist_t* list, mem_pool_t* pool) {
    if (!list || !pool) return false;
    
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->mempool = pool;
    return true;
}

/* 安全创建节点（带内存分配检查） */
static dlist_node_t* create_node(dlist_t* list) {
    dlist_node_t* node = mem_alloc(list->mempool, sizeof(dlist_node_t));
    if (node) {
        memset(node, 0, sizeof(dlist_node_t));
        return node;
    }
    return NULL;
}

/* 插入头节点（线程安全） */
bool dlist_insert_head(dlist_t* list, void* data) {
    if (!list) return false;

    dlist_node_t* new_node = create_node(list);
    if (!new_node) return false;

    MUTEX_LOCK(list->mempool->mutex);
    
    new_node->data = data;
    new_node->next = list->head;
    
    if (list->head) {
        list->head->prev = new_node;
    } else {
        list->tail = new_node;  // 空链表时更新尾指针
    }
    
    list->head = new_node;
    list->count++;
    
    MUTEX_UNLOCK(list->mempool->mutex);
    return true;
}

/* 插入尾节点（带内存分配回滚保护） */
bool dlist_insert_tail(dlist_t* list, void* data) {
    if (!list) return false;

    dlist_node_t* new_node = create_node(list);
    if (!new_node) return false;

    MUTEX_LOCK(list->mempool->mutex);
    
    new_node->data = data;
    new_node->prev = list->tail;
    
    if (list->tail) {
        list->tail->next = new_node;
    } else {
        list->head = new_node;  // 空链表时更新头指针
    }
    
    list->tail = new_node;
    list->count++;
    
    MUTEX_UNLOCK(list->mempool->mutex);
    return true;
}

/* 安全移除节点（自动内存回收） */
bool dlist_remove(dlist_t* list, dlist_node_t* node) {
    if (!list || !node) return false;

    MUTEX_LOCK(list->mempool->mutex);
    
    // 从链表中解除链接
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }
    
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }
    
    list->count--;
    
    // 内存回收
    my_mem_free(list->mempool, node);
    
    MUTEX_UNLOCK(list->mempool->mutex);
    return true;
}

// /* 示例使用 */
// void example_usage(mem_pool_t* pool) {
//     dlist_t my_list;
//     dlist_init(&my_list, pool);
    
//     // 插入数据
//     int data1 = 42;
//     dlist_insert_head(&my_list, &data1);
    
//     // 安全遍历
//     dlist_node_t *current, *tmp;
//     DLIST_FOREACH_SAFE(&my_list, current, tmp) {
//         printf("Data: %d\n", *(int*)current->data);
//     }
    
//     // 删除所有节点
//     while (my_list.head) {
//         dlist_remove(&my_list, my_list.head);
//     }
// }