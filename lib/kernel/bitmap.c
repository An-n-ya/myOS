#include "bitmap.h"
#include "string.h"
#include "print.h"
#include "interrupt.h"
#include "debug.h"

/**
 * @brief bitmap初始化
 *
 * @param btmp
 */
void bitmap_init(struct bitmap *btmp)
{
    // 将bits开始btmp_bytes_len长度的字节置0
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

/**
 * @brief 查看位图btmp的bit_idx位是否是1
 *
 * @param btmp
 * @param bit_idx
 * @return true
 * @return false
 */
bool bitmap_scan_test(struct bitmap *btmp, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8;
    uint32_t byte_res = bit_idx % 8;
    return (btmp->bits[byte_idx] & (BITMAP_MASK << byte_res));
}

/**
 * @brief 在位图中申请连续cnt个位，申请成功返回起始位索引，否则返回-1
 *
 * @param btmp
 * @param cnt
 * @return int
 */
int bitmap_scan(struct bitmap *btmp, uint32_t cnt)
{
    int bit_idx_start = -1;
    uint32_t bit_left = btmp->btmp_bytes_len * 8;
    uint32_t count = 0;   // 用于记录当前连续空闲的位的个数
    uint32_t cur_bit = 0; // 当前位置
    while (bit_left-- > 0)
    {
        if (!(bitmap_scan_test(btmp, cur_bit)))
        {
            // 当前位空闲，count加1
            count++;
        }
        else
        {
            count = 0;
        }

        if (count == cnt)
        {
            // 如果找到连续个cnt空闲位了
            // 直接返回开头地址
            return cur_bit - cnt + 1;
        }
        cur_bit++;
    }
    return bit_idx_start;
}

/**
 * @brief 将btmp的bit_idx为置为value
 *
 * @param btmp
 * @param bit_idx
 * @param value
 */
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value)
{
    // value应该是0或1
    ASSERT((value == 1) || (value == 0));
    uint32_t byte_idx = bit_idx / 8;
    uint32_t byte_res = bit_idx % 8;

    if (value)
    { // 如果value为1
        btmp->bits[byte_idx] |= (BITMAP_MASK << byte_res);
    }
    else
    { // 如果value为0
        btmp->bits[byte_idx] &= ~(BITMAP_MASK << byte_res);
    }
}