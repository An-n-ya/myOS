#include "list.h"
#include "../../kernel/interrupt.h"

// 初始化链表
void list_init(struct list *list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}
// 在给定链表元素前添加元素
void list_insert_before(struct list_elem *before, struct list_elem *elem)
{
    enum intr_status old_status = intr_disable();
    before->prev->next = elem;
    intr_set_status(old_status);
}

// 添加元素到队首
void list_push(struct list *plist, struct list_elem *elem)
{
    list_insert_before(plist->head.next, elem);
}

// 添加元素到队尾
void list_append(struct list *plist, struct list_elem *elem)
{
    list_insert_before(&plist->tail, elem);
}

// 删除给定链表元素
void list_remove(struct list_elem *pelem)
{
    enum intr_status old_status = intr_disable();

    pelem->next->prev = pelem->prev;
    pelem->prev->next = pelem->next;

    intr_set_status(old_status);
}

// 将链表第一个元素删除，并返回
struct list_elem *list_pop(struct list *plist)
{
    struct list_elem *res = plist->head.next;
    list_remove(res);
    return res;
}

// 判断链表是否为空
bool list_empty(struct list *plist)
{
    return plist->head.next == &plist->tail ? true : false;
}

// 给出链表长度
uint32_t list_len(struct list *plist)
{
    uint32_t cnt = 0;
    struct list_elem *cur = plist->head.next;
    while (cur != &plist->tail)
    {
        cur = cur->next;
        cnt += 1;
    }

    return cnt;
}

struct list_elem *list_traversal(struct list *plist, function func, int arg)
{
    struct list_elem *elem = plist->head.next;
    if (list_empty(plist))
    {
        return NULL;
    }

    while (elem != &plist->tail)
    {
        if (func(elem, arg))
        {
            return elem;
        }
        elem = elem->next;
    }

    return NULL;
}

// 从链表中找到目标节点
bool elem_find(struct list *plist, struct list_elem *obj_elem)
{
    struct list_elem *cur = plist->head.next;
    while (cur != &plist->tail)
    {
        if (cur == obj_elem)
        {
            return true;
        }
        cur = cur->next;
    }
    return false;
}
