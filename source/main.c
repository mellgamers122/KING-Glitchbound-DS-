#include <nds.h>
#include <stdio.h>

#define SCREEN_W 256
#define SCREEN_H 192
#define PLAYER_W 14
#define PLAYER_H 20
#define GRAVITY 1
#define JUMP_VELOCITY -10
#define MAX_FALL_SPEED 7

#define COLOR(r, g, b) (RGB15((r), (g), (b)) | BIT(15))

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Rect;

typedef struct {
    int x;
    int y;
    int collected;
} Fragment;

typedef struct {
    int x;
    int y;
    int vx;
    int vy;
    int onGround;
    int facing;
    int fragments;
    int lives;
    int dashCooldown;
} Player;

static Player king = { 24, 132, 0, 0, 0, 1, 0, 3, 0 };

static const Rect platforms[] = {
    { 0, 176, 256, 16 },
    { 28, 142, 54, 8 },
    { 108, 120, 55, 8 },
    { 184, 94, 45, 8 },
    { 34, 72, 54, 8 }
};

static Fragment fragments[] = {
    { 52, 130, 0 },
    { 132, 108, 0 },
    { 204, 82, 0 }
};

static int frameCount = 0;

static int overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void putPixel(u16 *fb, int x, int y, u16 color)
{
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        fb[y * SCREEN_W + x] = color;
    }
}

static void fillRect(u16 *fb, int x, int y, int w, int h, u16 color)
{
    int px;
    int py;

    for (py = y; py < y + h; py++) {
        for (px = x; px < x + w; px++) {
            putPixel(fb, px, py, color);
        }
    }
}

static void drawRect(u16 *fb, Rect rect, u16 color)
{
    fillRect(fb, rect.x, rect.y, rect.w, rect.h, color);
}

static void drawDiamond(u16 *fb, int cx, int cy, int radius, u16 color, u16 shine)
{
    int y;

    for (y = -radius; y <= radius; y++) {
        int span = radius - ((y < 0) ? -y : y);
        fillRect(fb, cx - span, cy + y, span * 2 + 1, 1, color);
    }

    putPixel(fb, cx - 1, cy - radius + 1, shine);
    putPixel(fb, cx, cy - radius + 2, shine);
}

static void drawBackground(u16 *fb)
{
    int x;
    int y;
    u16 scanline;

    for (y = 0; y < SCREEN_H; y++) {
        int blue = 5 + (y / 10);
        int red = 2 + (y / 28);
        scanline = COLOR(red, 3, blue);
        for (x = 0; x < SCREEN_W; x++) {
            fb[y * SCREEN_W + x] = scanline;
        }
    }

    for (x = 0; x < SCREEN_W; x += 16) {
        for (y = 0; y < SCREEN_H; y += 2) {
            putPixel(fb, x, y, COLOR(4, 6, 12));
        }
    }

    for (y = 16; y < SCREEN_H; y += 24) {
        for (x = 0; x < SCREEN_W; x += 2) {
            putPixel(fb, x, y, COLOR(5, 7, 14));
        }
    }

    fillRect(fb, 190, 18, 38, 24, COLOR(10, 4, 18));
    fillRect(fb, 196, 23, 26, 14, COLOR(17, 6, 25));
    fillRect(fb, 202, 27, 14, 6, COLOR(25, 9, 31));
}

static void drawPlatforms(u16 *fb)
{
    unsigned int i;

    for (i = 0; i < sizeof(platforms) / sizeof(platforms[0]); i++) {
        Rect top = platforms[i];
        drawRect(fb, platforms[i], COLOR(5, 18, 12));
        top.h = 2;
        drawRect(fb, top, COLOR(11, 29, 20));
    }
}

static void drawFragments(u16 *fb)
{
    unsigned int i;
    int pulse = (frameCount / 8) & 1;

    for (i = 0; i < sizeof(fragments) / sizeof(fragments[0]); i++) {
        if (!fragments[i].collected) {
            drawDiamond(fb, fragments[i].x, fragments[i].y + pulse, 6, COLOR(30, 25, 6), COLOR(31, 31, 22));
        }
    }
}

static void drawPortal(u16 *fb)
{
    u16 glow = (king.fragments >= 3) ? COLOR(14, 28, 31) : COLOR(10, 9, 18);

    fillRect(fb, 220, 54, 24, 42, COLOR(2, 2, 5));
    fillRect(fb, 224, 58, 16, 34, glow);
    fillRect(fb, 229, 63, 6, 24, COLOR(30, 9, 31));
    fillRect(fb, 218, 94, 28, 4, COLOR(5, 18, 12));
}

static void drawPlayer(u16 *fb)
{
    int x = king.x;
    int y = king.y;
    u16 coat = COLOR(4, 12, 29);
    u16 skin = COLOR(29, 20, 13);
    u16 crown = COLOR(31, 25, 4);
    u16 outline = COLOR(1, 1, 4);

    fillRect(fb, x - 1, y + 3, PLAYER_W + 2, PLAYER_H - 2, outline);
    fillRect(fb, x + 2, y + 7, PLAYER_W - 4, PLAYER_H - 7, coat);
    fillRect(fb, x + 3, y + 3, PLAYER_W - 6, 7, skin);

    fillRect(fb, x + 2, y, 2, 5, crown);
    fillRect(fb, x + 6, y - 2, 2, 7, crown);
    fillRect(fb, x + 10, y, 2, 5, crown);
    fillRect(fb, x + 2, y + 4, 10, 2, crown);

    putPixel(fb, x + ((king.facing > 0) ? 9 : 4), y + 6, COLOR(0, 0, 0));
    fillRect(fb, x + 3, y + PLAYER_H - 2, 3, 4, COLOR(2, 2, 5));
    fillRect(fb, x + 9, y + PLAYER_H - 2, 3, 4, COLOR(2, 2, 5));
}

static void drawGlitchSparks(u16 *fb)
{
    int i;

    for (i = 0; i < 10; i++) {
        int x = (i * 29 + frameCount * 3) & 255;
        int y = 28 + ((i * 17 + frameCount) % 116);
        u16 color = (i & 1) ? COLOR(31, 4, 28) : COLOR(4, 25, 31);

        fillRect(fb, x, y, 8, 2, color);
    }
}

static void collectFragments(void)
{
    unsigned int i;

    for (i = 0; i < sizeof(fragments) / sizeof(fragments[0]); i++) {
        if (!fragments[i].collected &&
            overlap(king.x, king.y, PLAYER_W, PLAYER_H, fragments[i].x - 6, fragments[i].y - 6, 12, 12)) {
            fragments[i].collected = 1;
            king.fragments++;
        }
    }
}

static void movePlayer(void)
{
    unsigned int i;
    int oldY = king.y;
    int held = keysHeld();
    int down = keysDown();

    king.vx = 0;
    if (held & KEY_LEFT) {
        king.vx = -2;
        king.facing = -1;
    }
    if (held & KEY_RIGHT) {
        king.vx = 2;
        king.facing = 1;
    }
    if ((down & KEY_A) && king.onGround) {
        king.vy = JUMP_VELOCITY;
        king.onGround = 0;
    }
    if ((down & KEY_B) && king.dashCooldown == 0) {
        king.vx += king.facing * 6;
        king.dashCooldown = 24;
    }

    king.x += king.vx;
    if (king.x < 0) {
        king.x = 0;
    }
    if (king.x > SCREEN_W - PLAYER_W) {
        king.x = SCREEN_W - PLAYER_W;
    }

    if (king.dashCooldown > 0) {
        king.dashCooldown--;
    }

    king.vy += GRAVITY;
    if (king.vy > MAX_FALL_SPEED) {
        king.vy = MAX_FALL_SPEED;
    }

    king.y += king.vy;
    king.onGround = 0;

    for (i = 0; i < sizeof(platforms) / sizeof(platforms[0]); i++) {
        Rect p = platforms[i];
        if (king.vy >= 0 &&
            oldY + PLAYER_H <= p.y &&
            king.y + PLAYER_H >= p.y &&
            king.x + PLAYER_W > p.x &&
            king.x < p.x + p.w) {
            king.y = p.y - PLAYER_H;
            king.vy = 0;
            king.onGround = 1;
        }
    }

    if (king.y > SCREEN_H) {
        king.x = 24;
        king.y = 132;
        king.vx = 0;
        king.vy = 0;
        king.lives--;
        if (king.lives < 1) {
            king.lives = 3;
            king.fragments = 0;
            for (i = 0; i < sizeof(fragments) / sizeof(fragments[0]); i++) {
                fragments[i].collected = 0;
            }
        }
    }

    collectFragments();
}

static void drawTopScreen(u16 *fb)
{
    drawBackground(fb);
    drawGlitchSparks(fb);
    drawPortal(fb);
    drawPlatforms(fb);
    drawFragments(fb);
    drawPlayer(fb);
}

static void drawHud(PrintConsole *console)
{
    consoleSelect(console);
    consoleClear();
    iprintf("\x1b[1;4HKING: GLITCHBOUND");
    iprintf("\x1b[3;2HNEXUS AWAKENS - prototype 0.2");
    iprintf("\x1b[5;2HFragmentos de Save: %d/3", king.fragments);
    iprintf("\x1b[6;2HVidas: %d", king.lives);
    iprintf("\x1b[8;2HD-Pad move  A pula  B dash");

    if (king.fragments >= 3) {
        iprintf("\x1b[11;2HPortal acordado.");
        iprintf("\x1b[12;2HLeve KING ate a fenda!");
    } else {
        iprintf("\x1b[11;2HObjetivo:");
        iprintf("\x1b[12;2Hrecupere os saves perdidos.");
    }
}

int main(void)
{
    PrintConsole bottom;
    u16 *fb = (u16 *)VRAM_A;

    videoSetMode(MODE_FB0);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_LCD);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleInit(&bottom, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

    while (1) {
        scanKeys();
        movePlayer();
        drawTopScreen(fb);
        drawHud(&bottom);
        frameCount++;
        swiWaitForVBlank();
    }

    return 0;
}
