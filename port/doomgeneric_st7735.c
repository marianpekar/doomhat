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

#define KEY_PREV_WEAPON ','
#define KEY_NEXT_WEAPON '.'
#define KEY_MAP_ZOOMIN '='
#define KEY_MAP_ZOOMOUT '-'

#define MODIFIER_PIN PIN_PRESS

#define BAT_I2C_BUS 1
#define BAT_I2C_ADDR 0x43
#define BAT_REG_BUS_VOLTAGE 0x02
#define BAT_V_EMPTY 3.0f
#define BAT_V_FULL 4.2f
#define BAT_POLL_MS 10000
#define BAT_COLS 10
#define BAT_ROWS 2
#define BAT_GREEN 0xE007
#define BAT_RED 0x00F8

typedef struct {
    int pin;
    unsigned char key; // primary action
    unsigned char key2; // optional second key sent; 0 = none
    unsigned char mod_key; // secondary action while the modifier is held; 0 = none
    int state; // 0 = up, 1 = pressed (primary), 2 = pressed (modifier)
} Btn;

static Btn btns[] = {
    { PIN_UP,    KEY_UPARROW,    0,         KEY_MAP_ZOOMIN,  0 },
    { PIN_DOWN,  KEY_DOWNARROW,  0,         KEY_MAP_ZOOMOUT, 0 },
    { PIN_LEFT,  KEY_LEFTARROW,  0,         0,               0 },
    { PIN_RIGHT, KEY_RIGHTARROW, 0,         0,               0 },
    { PIN_PRESS, KEY_RALT,       0,         0,               0 },
    { PIN_KEY1,  KEY_FIRE,       0,         KEY_PREV_WEAPON, 0 },
    { PIN_KEY2,  KEY_USE,        KEY_ENTER, KEY_TAB,         0 },
    { PIN_KEY3,  KEY_ESCAPE,     0,         KEY_NEXT_WEAPON, 0 },
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
    int mod_held = (DEV_Digital_Read(MODIFIER_PIN) == 0);

    for (int i = 0; i < NUM_BTNS; i++) {
        Btn *b = &btns[i];
        int down = (DEV_Digital_Read(b->pin) == 0);

        if (down && b->state == 0) {
            b->state = (mod_held && b->mod_key) ? 2 : 1;
            if (b->state == 2) {
                que_push(1, b->mod_key);
            } else {
                que_push(1, b->key);
                if (b->key2) que_push(1, b->key2);
            }
        } else if (!down && b->state != 0) {
            if (b->state == 2) {
                que_push(0, b->mod_key);
            } else {
                que_push(0, b->key);
                if (b->key2) que_push(0, b->key2);
            }
            b->state = 0;
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

static int bat_handle = -1;
static int bat_level = -1;
static uint32_t bat_last_poll = 0;

static void bat_init(void)
{
    bat_handle = lgI2cOpen(BAT_I2C_BUS, BAT_I2C_ADDR, 0);
    if (bat_handle < 0) {
        fprintf(stderr, "Battery monitor not available (I2C open failed)\n");
        bat_handle = -1;
    }
}

static void bat_poll(void)
{
    if (bat_handle < 0)
        return;

    uint32_t now = DG_GetTicksMs();
    if (bat_level >= 0 && now - bat_last_poll < BAT_POLL_MS)
        return;
    bat_last_poll = now;

    int raw = lgI2cReadWordData(bat_handle, BAT_REG_BUS_VOLTAGE);
    if (raw < 0) {
        bat_level = -1;
        return;
    }
    
    raw = ((raw & 0xFF) << 8) | ((raw >> 8) & 0xFF); // INA219 is big-endian
    
    float volts = (float)(raw >> 3) * 0.004f;
    float pct = (volts - BAT_V_EMPTY) / (BAT_V_FULL - BAT_V_EMPTY) * 100.0f;

    if (pct < 0.0f) {
        pct = 0.0f;
    }

    if (pct > 100.0f) {
        pct = 100.0f;
    }
    bat_level = (int)(pct / (100.0f / BAT_COLS) + 0.5f);
}

static void bat_draw(void)
{
    if (bat_level < 0)
        return;

    for (int y = 0; y < BAT_ROWS; y++) {
        UWORD *row = lcd_buf + y * LCD_W + (LCD_W - BAT_COLS);
        for (int x = 0; x < BAT_COLS; x++)
            row[x] = (x < bat_level) ? BAT_GREEN : BAT_RED;
    }
}

static void cleanup(void)
{
    if (bat_handle >= 0)
        lgI2cClose(bat_handle);

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
    bat_init();

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

    bat_poll();
    bat_draw();

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