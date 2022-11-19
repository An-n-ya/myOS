#include "string.h"
#include "global.h"
#include "debug.h"

/**
 * @brief 将dst_起始的size个字节置为value
 *
 * @param dst_
 * @param value
 * @param size
 */
void memset(void *dst_, uint8_t value, uint32_t size)
{
    ASSERT(dst_ != NULL);
    uint8_t *dst = (uint8_t *)dst_; // 将地址转为8位
    while (size-- > 0)
    {
        *dst++ = value; // 挨个赋值
    }
}

/**
 * @brief 将src_起始的size个字节复制到dst_开始的地址处
 *
 * @param dst_
 * @param src_
 * @param size
 */
void memcpy(void *dst_, const void *src_, uint32_t size)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    uint8_t *dst = (uint8_t *)dst_;
    const uint8_t *src = (uint8_t *)src_;
    while (size-- > 0)
    {
        *dst++ = *src; // 挨个赋值
    }
}

/**
 * @brief 比较以a_地址和b_地址开头的size个字节的大小，如果a_大于b_，返回1，如果a_小于b_返回-1，如果相等返回0
 *
 * @param a_
 * @param b_
 * @param size
 * @return int 三种值：1 -1 0
 */
int memcmp(const void *a_, const void *b_, uint32_t size)
{
    const char *a = a_;
    const char *b = b_;
    ASSERT(a != NULL && b != NULL);
    while (size-- > 0)
    {
        if (*a != *b)
        {
            return *a > *b ? 1 : -1; // 如果a和b不同，返回比较后的结
        }
        a++;
        b++;
    }
    // 运行到这里说明每个字节都相等
    return 0;
}

/**
 * @brief 将字符串由src_复制到dst_
 *
 * @param dst_
 * @param src_
 * @return char*
 */
char *strcpy(char *dst_, const char *src_)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    char *r = dst_; // 记录目的字符串的起始地址
    while ((*dst_++ = *src_++))
        ;
    return r;
}

/**
 * @brief 返回字符串长度
 *
 * @param str
 * @return uint32_t
 */
uint32_t strlen(const char *str)
{
    ASSERT(str != NULL);
    const char *p = str;
    while (*p++)
        ;
    return (p - str - 1);
}

/**
 * @brief 比较两个字符串
 *
 * @param a
 * @param b
 * @return int8_t
 */
int8_t strcmp(const char *a, const char *b)
{
    ASSERT(a != NULL && b != NULL);
    while (*a != 0 && *a == *b)
    {
        a++;
        b++;
    }
    if (*a == *b)
    {
        // 这时候a和b都来到了字符串末尾, 两个字符串相等
        return 0;
    }
    return *a > *b ? -1 : 1;
}

/**
 * @brief 从左到右查找字符串str中首次出现ch字符的地址
 *
 * @param str 字符串 char*
 * @param ch 目标字符 uint8_t
 * @return char* 如果找到了返回地址，没找到返回NULL
 */
char *strchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    while (*str != 0)
    {
        if (*str == ch)
        {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}

char *strrchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    const char *last_char = NULL;
    while (*str != 0)
    {
        if (*str == ch)
        {
            last_char = str;
        }
        str++;
    }
    return (char *)last_char;
}

/**
 * @brief 将字符串src_拼接到dst_之后
 *
 * @param dst_
 * @param src_
 * @return char*
 */
char *strcat(char *dst_, const char *src_)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    char *str = dst_; // 内部使用的字符串地址
    while (*str++)
        ;
    str--;                     // 让str指向dst字符串的末尾
    while ((*str++ = *src_++)) // 从dst字符串末尾逐个赋值src
        ;

    return dst_; // 返回dst_头
}

/**
 * @brief 在字符串str中查找出现的次数
 *
 * @param str
 * @param ch
 * @return uint32_t
 */
uint32_t strchrs(const char *str, uint8_t ch)
{
    ASSERT(str != NULL);
    uint32_t ch_cnt = 0; // 计数值，初始为0
    const char *p = str;
    while (*p != 0)
    {
        if (*p == ch)
        {
            ch_cnt++;
        }
        p++;
    }
    return ch_cnt;
}