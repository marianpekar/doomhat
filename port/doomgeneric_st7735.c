/*
 * doomgeneric port for Waveshare 1.44" LCD HAT (ST7735S, 128x128)
 * with 5-way joystick and 3 buttons, using the Waveshare driver
 * (DEV_Config / LCD_1in44) built with USE_DEV_LIB (lgpio).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include "doomkeys.h"
#include "doomgeneric.h"

#include "DEV_Config.h"
#include "LCD_1in44.h"

#define BLACK 0x0000

#define LCD_W 128
#define LCD_H 128

#define PIN_UP  6
#define PIN_DOWN 19
#define PIN_LEFT 5
#define PIN_RIGHT 26
#define PIN_PRESS 13
#define PIN_KEY1 21
#define PIN_KEY2 20
#define PIN_KEY3 16

typedef struct {
    int pin;
    unsigned char key;  // primary
    unsigned char key2; // optional second key sent; 0 = none
    int state; // 1 = pressed
} Btn;

static Btn btns[] = {
    { PIN_UP,    KEY_UPARROW,    0,         0 },  // forward
    { PIN_DOWN,  KEY_DOWNARROW,  0,         0 },  // backward
    { PIN_LEFT,  KEY_LEFTARROW,  0,         0 },  // turn left
    { PIN_RIGHT, KEY_RIGHTARROW, 0,         0 },  // turn right
    { PIN_PRESS, KEY_USE,        KEY_ENTER, 0 },  // use/open + confirm
    { PIN_KEY1,  KEY_FIRE,       0,         0 },  // fire
    { PIN_KEY2,  KEY_RALT,       0,         0 },  // strafe modifier
    { PIN_KEY3,  KEY_ESCAPE,     0,         0 },  // menu
};
#define NUM_BTNS ((int)(sizeof(btns) / sizeof(btns[0])))

typedef struct {
    int pressed;
    unsigned char key;
} KeyEvt;

#define QUE_SIZE 16
static KeyEvt que[QUE_SIZE];
static int que_head = 0, que_tail = 0;

static void que_push(int pressed, unsigned char key)
{
    int next = (que_tail + 1) % QUE_SIZE;
    if (next == que_head) return;
    que[que_tail].pressed = pressed;
    que[que_tail].key = key;
    que_tail = next;
}

static void poll_buttons(void)
{
    for (int i = 0; i < NUM_BTNS; i++) {
        int pressed = (DEV_Digital_Read(btns[i].pin) == 0);
        if (pressed != btns[i].state) {
            btns[i].state = pressed;
            que_push(pressed, btns[i].key);
            if (btns[i].key2) que_push(pressed, btns[i].key2);
        }
    }
}

static UWORD lcd_buf[LCD_W * LCD_H];
static int x_map[LCD_W];
static int y_map[LCD_H];
static int dst_h, dst_w;

static void build_maps(void)
{
    dst_h = LCD_H;
    dst_w = (LCD_H - dst_h) / 2;

    for (int x = 0; x < LCD_W; x++) {
       x_map[x] = (x * DOOMGENERIC_RESX + DOOMGENERIC_RESX / 2) / LCD_W;
    }
    for (int y = 0; y < dst_h; y++) {
       y_map[y] = (y * DOOMGENERIC_RESY + DOOMGENERIC_RESY / 2) / dst_h;
    }
}

static void cleanup(void)
{
    LCD_1in44_Clear(BLACK);
    DEV_ModuleExit();
}

static void on_signal(int sig)
{
    (void)sig;
    cleanup();
    _exit(0);
}

void DG_Init(void)
{
    if (DEV_ModuleInit() != 0) {
        fprintf(stderr, "DEV_ModuleInit failed\n");
        _exit(1);
    }

    LCD_1in44_Init(SCAN_DIR_DFT);
    LCD_1in44_Clear(BLACK);

    for (int i = 0; i < NUM_BTNS; i++)
        DEV_GPIO_Mode(btns[i].pin, 0);

    memset(lcd_buf, 0, sizeof(lcd_buf));
    build_maps();

    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);
}

void DG_DrawFrame(void)
{
    for (int y = 0; y < dst_h; y++) {
        const uint32_t *src = DG_ScreenBuffer + y_map[y] * DOOMGENERIC_RESX;
        UWORD *dst = lcd_buf + (y + dst_w) * LCD_W;

        for (int x = 0; x < LCD_W; x++) {
            uint32_t p = src[x_map[x]];
            UWORD c = (UWORD)(((p >> 8) & 0xF800) |
                              ((p >> 5) & 0x07E0) |
                              ((p >> 3) & 0x001F));
            c = (UWORD)((c >> 8) | (c << 8));
            dst[x] = c;
        }
    }

    LCD_1in44_Display(lcd_buf);
}

void DG_SleepMs(uint32_t ms)
{
    usleep(ms * 1000);
}

uint32_t DG_GetTicksMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

int DG_GetKey(int *pressed, unsigned char *key)
{
    if (que_head == que_tail) {
        poll_buttons();
    }

    if (que_head == que_tail) 
        return 0;

    *pressed = que[que_head].pressed;
    *key = que[que_head].key;
    que_head = (que_head + 1) % QUE_SIZE;
    return 1;
}

void DG_SetWindowTitle(const char *title)
{
    (void)title;
}

int main(int argc, char **argv)
{
    doomgeneric_Create(argc, argv);

    while(1) {
        doomgeneric_Tick();
    }

    return 0;
}