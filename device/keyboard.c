#include "keyboard.h"
#include "../lib/kernel/print.h"
#include "../kernel/interrupt.h"
#include "../kernel/io.h"
#include "../kernel/global.h"

#define KBD_BUF_PORT 0x60       // 键盘buffer寄存器端口号

#define esc         '\x1b'
#define backspace   '\b'
#define tab         '\r'
#define enter       '\r'
#define delete      '\x7f'

// 定义控制字符不可见
#define char_invisible  0
#define ctrl_l_char     char_invisible
#define ctrl_r_char     char_invisible
#define shift_l_char    char_invisible
#define shift_r_char    char_invisible
#define alt_l_char      char_invisible
#define alt_r_char      char_invisible
#define caps_lock_char  char_invisible

// 定义控制字符通码和断码
#define shift_l_make    0x2a
#define shift_r_make    0x36
#define alt_l_make      0x38
#define alt_r_make      0xe038
#define alt_r_break     0xe0b8
#define ctrl_l_make     0x1d
#define ctrl_r_make     0xe01d
#define ctrl_r_break    0xe09d
#define caps_lock_make  0x3a

// 以下变量用于记录响应按键是否被按下
// ext_scancode 表示extend码，用来记录makecode是否以0xe0开头
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

// 二维数组： 扫描码   shift+扫描码
static char keymap[][2] = {
        {0,     0}    ,
        {esc,       esc},
        {'1',       '!'},
        {'2',       '@'},
        {'3',       '#'},
        {'4',       '$'},
        {'5',       '%'},
        {'6',       '^'},
        {'7',       '&'},
        {'8',       '*'},
        {'9',       '('},
        {'0',       ')'},
        {'-',       '_'},
        {'=',       '+'},
        {backspace,       backspace},
        {tab,       tab},
        {'q',       'Q'},
        {'w',       'W'},
        {'e',       'E'},
        {'r',       'R'},
        {'t',       'T'},
        {'y',       'Y'},
        {'u',       'U'},
        {'i',       'I'},
        {'o',       'O'},
        {'p',       'P'},
        {'[',       '{'},
        {']',       '}'},
        {enter,       enter},
        {ctrl_l_char,       ctrl_l_char},
        {'a',       'A'},
        {'s',       'S'},
        {'d',       'D'},
        {'f',       'F'},
        {'g',       'G'},
        {'h',       'H'},
        {'j',       'J'},
        {'k',       'K'},
        {'l',       'L'},
        {';',       ':'},
        {'\'',       '"'},
        {'`',       '~'},
        {shift_l_char,       shift_l_char},
        {'\\',       '|'},
        {'z',       'Z'},
        {'x',       'X'},
        {'c',       'C'},
        {'v',       'V'},
        {'b',       'B'},
        {'n',       'N'},
        {'m',       'M'},
        {',',       '<'},
        {'.',       '>'},
        {'/',       '?'},
        {shift_r_char,       shift_r_char},
        {'*',       '*'},
        {alt_l_char,       alt_l_char},
        {' ',       ' '},
        {caps_lock_char,       caps_lock_char},
};

// 键盘中断处理程序
static void intr_keyboard_handler(void) {
    // 保存上一次的控制键状态
    bool ctrl_down_last = ctrl_status;
    bool shift_down_last = shift_status;
    bool caps_lock_last = caps_lock_status;

    bool break_code;

    uint16_t scancode = inb(KBD_BUF_PORT); // 从缓冲区读出数据，否则8042将不再响应键盘中断

    if (scancode == 0xe0) {
        ext_scancode = true;                  // 如果scancode是扩展字符，打开e0标记
        return;
    }

    // 如果是e0开头的, 加上e0扫描码
    if (ext_scancode) {
        scancode = ((0xe000) | scancode);
        ext_scancode = false;                   // 关闭e0标记
    }

    break_code = ((scancode & 0x0080) != 0);    //看是否是断码

    if (break_code) {
        // 如果是断码（即松开按键的时候）
        // 把断码转化成通码
        uint16_t make_code = (scancode &= 0xff7f);

        // 以下任意一个控制键弹起了，设置对应的状态为false
        if (make_code == ctrl_l_make || make_code == ctrl_r_make) {
            ctrl_status = false;
        } else if (make_code == shift_l_make || make_code == shift_r_make) {
            shift_status = false;
        } else if (make_code == alt_l_make || make_code == alt_r_make) {
            alt_status = false;
        }
        // 直接返回
        return;
    } else if ((scancode > 0x00 && scancode < 0x3b) || \
                    (scancode == alt_r_make) || \
                    (scancode == ctrl_r_make) \
    ) {
        // 只处理数组中的元素，一共有0x3b个scancode
        bool shift = false;
        // 处理8位的键
        if ((scancode < 0x0e) || (scancode == 0x29) || \
                (scancode == 0x1a) || (scancode == 0x1b) || \
                (scancode == 0x2b) || (scancode == 0x27) || \
                (scancode == 0x28) || (scancode == 0x33) || \
                (scancode == 0x34) || (scancode == 0x35) \
        ) {
            if (shift_down_last) {
                // 如果同时按下了shift
                shift = true;
            }
        } else {
            // 默认为字母键
            // 处理caps与shift组合的情况
            if (shift_down_last && caps_lock_last) {
                shift = false;
            } else if (shift_down_last || caps_lock_last) {
                shift = true;
            } else {
                shift = false;
            }
        }
        uint8_t index = (scancode &= 0x00ff); // 由scancode获得索引

        char cur_char = keymap[index][shift]; // 在数组中找到对应的字符

        // 如果可见，就打印出来
        if (cur_char) {
            put_char(cur_char);
            return;
        }

        if (scancode == ctrl_l_make || scancode == ctrl_r_make) {
            ctrl_status = true;
        } else if (scancode == shift_l_make || scancode == shift_r_make) {
            shift_status = true;
        } else if (scancode == alt_l_make || scancode == alt_r_make) {
            alt_status = true;
        } else if (scancode == caps_lock_make) {
            // 反转caps状态
            caps_lock_status = !caps_lock_status;
        }
    } else {
        put_str("unknown key\n");
    }
}

void keyboard_init() {
    put_str("keyboard init start\n");
    register_handler(0x21, intr_keyboard_handler); // 注册0x21号中断（键盘中断）
    put_str("keyboard init done\n");
}



