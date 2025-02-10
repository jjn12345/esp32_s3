#include "mem_manage.h"


/* 内存初始化（必须4字节对齐） */
bool my_mem_init(mem_pool_t* pool, void* buffer, uint32_t size) {
    if (!pool || !buffer || size < sizeof(block_header_t) + 8) 
        return false;

    pool->pool_start = (uint8_t*)buffer;
    pool->pool_size = size;
    
    // 初始化第一个块头
    block_header_t* first = (block_header_t*)buffer;
    first->magic = 0x55AA55AA;
    first->size = size - sizeof(block_header_t);
    first->used = false;
    first->prev = NULL;
    first->next = NULL;
    
    pool->free_list = first;
    
#ifdef DEBUG
    pool->alloc_count = 0;
    pool->free_count = 0;
    pool->frag_score = 0;
#endif
    return true;
}

/* 块有效性验证 */
static bool block_valid(block_header_t* block) {
    return (block && block->magic == 0x55AA55AA);
}

/* 合并相邻空闲块 */
static void merge_blocks(block_header_t* block) {
    // 向后合并
    block_header_t* next = (block_header_t*)
        ((uint8_t*)block + sizeof(block_header_t) + block->size);
    if (block_valid(next) && !next->used) {
        block->size += sizeof(block_header_t) + next->size;
        block->next = next->next;
        if (block->next)
            block->next->prev = block;
    }

    // 向前合并
    block_header_t* prev = block->prev;
    if (block_valid(prev) && !prev->used) {
        prev->size += sizeof(block_header_t) + block->size;
        prev->next = block->next;
        if (prev->next)
            prev->next->prev = prev;
    }
}

/* 内存分配（带4字节对齐） */
void* mem_alloc(mem_pool_t* pool, uint32_t size) {
    if (!pool || size == 0) return NULL;

    // 对齐处理
    size = (size + 3) & ~0x03;
    
    MUTEX_LOCK(pool->mutex);
    
    block_header_t* best = NULL;
    block_header_t* curr = pool->free_list;
    
    // 最佳适应算法
    while (curr) {
        if (curr->size >= size) {
            if (!best || curr->size < best->size) {
                best = curr;
                if (best->size == size) break;
            }
        }
        curr = curr->next;
    }
    
    if (!best) {
        MUTEX_UNLOCK(pool->mutex);
        return NULL;
    }
    
    // 分割块（剩余空间至少能存放块头）
    if (best->size > size + sizeof(block_header_t) + 4) {
        block_header_t* new_block = (block_header_t*)
            ((uint8_t*)best + sizeof(block_header_t) + size);
        new_block->magic = 0x55AA55AA;
        new_block->size = best->size - size - sizeof(block_header_t);
        new_block->used = false;
        
        // 更新链表
        new_block->prev = best;
        new_block->next = best->next;
        if (best->next)
            best->next->prev = new_block;
        best->next = new_block;
        
        best->size = size;
    }
    
    best->used = true;
    
    // 从空闲链表移除
    if (best->prev) 
        best->prev->next = best->next;
    else 
        pool->free_list = best->next;
    
    if (best->next)
        best->next->prev = best->prev;
    
#ifdef DEBUG
    pool->alloc_count++;
#endif
    
    MUTEX_UNLOCK(pool->mutex);
    return (void*)((uint8_t*)best + sizeof(block_header_t));
}

/* 内存释放 */
bool my_mem_free(mem_pool_t* pool, void* ptr) {
    if (!pool || !ptr) return false;

    block_header_t* block = (block_header_t*)
        ((uint8_t*)ptr - sizeof(block_header_t));
    
    if (!block_valid(block) || !block->used) {
        return false; // 无效释放
    }

    MUTEX_LOCK(pool->mutex);
    
    block->used = false;
    
    // 插入空闲链表头部
    block->prev = NULL;
    block->next = pool->free_list;
    if (pool->free_list)
        pool->free_list->prev = block;
    pool->free_list = block;
    
    // 合并相邻块
    merge_blocks(block);
    
#ifdef DEBUG
    pool->free_count++;
#endif
    
    MUTEX_UNLOCK(pool->mutex);
    return true;
}

/* 内存状态查询 */
void mem_get_status(mem_pool_t* pool, mem_status_t* status) {
    memset(status, 0, sizeof(mem_status_t));
    
    MUTEX_LOCK(pool->mutex);
    
    status->total = pool->pool_size - sizeof(block_header_t);
    
    block_header_t* curr = pool->free_list;
    while (curr) {
        status->free += curr->size;
        if (curr->size > status->largest_free)
            status->largest_free = curr->size;
        curr = curr->next;
    }
    
    status->used = status->total - status->free;
    
    if (status->free > 0) {
        status->frag_factor = 100 - 
            (status->largest_free * 100) / status->free;
    }
    
#ifdef DEBUG
    pool->frag_score = status->frag_factor;
#endif
    
    MUTEX_UNLOCK(pool->mutex);
}