// ---- hand-drawn 16px icons (things the item sheet doesn't have) ----
static void CDisc(int cx, int cy, int r, u8 R, u8 G, u8 B)
{
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx)
            if (dx * dx + dy * dy <= r * r)
            {
                int X = cx + dx, Y = cy + dy;
                if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
                { u8 *p = CPix(X, Y); p[0] = R; p[1] = G; p[2] = B; }
            }
}

static void DayIcon(int x, int y)
{
    CDisc(x + 8, y + 8, 4, 255, 214, 60);
    CFill(x + 8, y + 1, 1, 2, 255, 214, 60);  CFill(x + 8, y + 13, 1, 2, 255, 214, 60);
    CFill(x + 1, y + 8, 2, 1, 255, 214, 60);  CFill(x + 13, y + 8, 2, 1, 255, 214, 60);
    CFill(x + 3, y + 3, 2, 1, 255, 214, 60);  CFill(x + 11, y + 3, 2, 1, 255, 214, 60);
    CFill(x + 3, y + 12, 2, 1, 255, 214, 60); CFill(x + 11, y + 12, 2, 1, 255, 214, 60);
}

static void HalfSunIcon(int x, int y, u8 R, u8 G, u8 B) // sun on the horizon
{
    for (int dy = -4; dy <= 0; ++dy)
        for (int dx = -4; dx <= 4; ++dx)
            if (dx * dx + dy * dy <= 16)
            {
                int X = x + 8 + dx, Y = y + 11 + dy;
                if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
                { u8 *p = CPix(X, Y); p[0] = R; p[1] = G; p[2] = B; }
            }
    CFill(x + 8, y + 3, 1, 2, R, G, B);       // ray up
    CFill(x + 3, y + 5, 2, 1, R, G, B);       // rays diagonal
    CFill(x + 11, y + 5, 2, 1, R, G, B);
    CFill(x + 1, y + 12, 14, 1, (u8)(R * 3 / 4), (u8)(G * 3 / 4), (u8)(B * 3 / 4)); // horizon
}

static void NightIcon(int x, int y)
{
    for (int dy = -5; dy <= 5; ++dy)
        for (int dx = -5; dx <= 5; ++dx)
        {
            if (dx * dx + dy * dy > 25) continue;
            int ox = dx - 3, oy = dy + 2;              // carve an offset disc -> crescent
            if (ox * ox + oy * oy <= 20) continue;
            int X = x + 8 + dx, Y = y + 8 + dy;
            if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
            { u8 *p = CPix(X, Y); p[0] = 240; p[1] = 240; p[2] = 200; }
        }
    CFill(x + 3, y + 3, 1, 1, 255, 244, 180); // stars
    CFill(x + 5, y + 1, 1, 1, 255, 244, 180);
}

static void ClockIcon(int x, int y)
{
    for (int dy = -6; dy <= 6; ++dy)
        for (int dx = -6; dx <= 6; ++dx)
        {
            int rr = dx * dx + dy * dy;
            if (rr > 36 || rr < 25) continue;
            int X = x + 8 + dx, Y = y + 8 + dy;
            if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
            { u8 *p = CPix(X, Y); p[0] = 236; p[1] = 200; p[2] = 120; }
        }
    CFill(x + 8, y + 4, 1, 4, 248, 240, 216); // hands
    CFill(x + 8, y + 8, 3, 1, 248, 240, 216);
}

static void RainIcon(int x, int y)
{
    CDisc(x + 5, y + 6, 3, 208, 212, 222);
    CDisc(x + 9, y + 5, 3, 208, 212, 222);
    CDisc(x + 11, y + 7, 2, 208, 212, 222);
    CFill(x + 3, y + 6, 11, 3, 208, 212, 222);
    CFill(x + 4, y + 11, 1, 3, 110, 170, 240);
    CFill(x + 8, y + 12, 1, 3, 110, 170, 240);
    CFill(x + 12, y + 11, 1, 3, 110, 170, 240);
}

static void PinIcon(int x, int y)
{
    CDisc(x + 8, y + 5, 4, 92, 202, 112);            // round head
    for (int i = 0; i < 7; ++i)                       // taper down to a point
    {
        int w = 7 - i; if (w < 1) w = 1;
        CFill(x + 8 - w / 2, y + 8 + i, w, 1, 92, 202, 112);
    }
    CDisc(x + 8, y + 5, 1, 26, 40, 30);               // hole in the head
}
// Warp: a teleport portal - two concentric rings + a bright core, cyan = "go there".
static void PortalIcon(int x, int y)
{
    for (int dy = -7; dy <= 7; ++dy)
        for (int dx = -7; dx <= 7; ++dx)
        {
            int rr = dx * dx + dy * dy;
            if (!((rr <= 49 && rr >= 32) || (rr <= 16 && rr >= 6))) continue; // outer + inner ring
            int X = x + 8 + dx, Y = y + 8 + dy;
            if ((unsigned)X < TOP_W && (unsigned)Y < TOP_H)
            { u8 *p = CPix(X, Y); p[0] = 96; p[1] = 196; p[2] = 236; }
        }
    CDisc(x + 8, y + 8, 1, 210, 244, 255);            // bright core
}
// Play as...: a stylized mask - round face, two eye holes, a mouth line. Hand-drawn rather than
// pulled from the sprite sheet, since none of the sheet's face cells could be confidently
// identified as Deku/Goron/Zora/Fierce Deity specifically (see gen_sprites_mm3d.py's notes).
static void MaskIcon(int x, int y)
{
    CDisc(x + 8, y + 8, 6, 214, 122, 40);             // mask body (Termina-orange)
    CDisc(x + 5, y + 6, 1, 24, 14, 8);                // left eye hole
    CDisc(x + 11, y + 6, 1, 24, 14, 8);               // right eye hole
    CFill(x + 6, y + 11, 5, 1, 24, 14, 8);             // mouth line
}

// ===================== Item sprites (RGBA4444, from sprites.h) =====================
static const SpriteRef *FindSprite(int key)
{
    if (key < 0) return NULL;
    for (int i = 0; i < NUM_SPRITES; ++i)
        if (sprites[i].key == key) return &sprites[i];
    return NULL;
}

static void DrawSprite(int x, int y, int key, int big)
{
    const SpriteRef *s = FindSprite(key);
    if (!s) return;
    const u16 *px = big ? s->px42 : s->px16;
    int n = big ? SPR42 : SPR16;
    for (int yy = 0; yy < n; ++yy)
        for (int xx = 0; xx < n; ++xx)
        {
            u16 v = px[yy * n + xx];
            u32 a = (u32)(v & 0xF) * 17;
            if (!a) continue;
            int X = x + xx, Y = y + yy;
            if ((unsigned)X >= TOP_W || (unsigned)Y >= TOP_H) continue;
            u8 r = (u8)(((v >> 12) & 0xF) * 17);
            u8 g = (u8)(((v >> 8) & 0xF) * 17);
            u8 b = (u8)(((v >> 4) & 0xF) * 17);
            u8 *p = CPix(X, Y);
            p[0] = (u8)((p[0] * (255 - a) + r * a) / 255);
            p[1] = (u8)((p[1] * (255 - a) + g * a) / 255);
            p[2] = (u8)((p[2] * (255 - a) + b * a) / 255);
        }
}

static void DrawCheatIcon(int x, int y, int ch)
{
    int sk = SpriteKeyForCheat(ch);
    switch (sk)
    {
        case SPRK_SUNRISE: HalfSunIcon(x, y, 255, 220, 90); return;
        case SPRK_DAY:     DayIcon(x, y);    return;
        case SPRK_SUNSET:  HalfSunIcon(x, y, 255, 140, 50); return;
        case SPRK_NIGHT:   NightIcon(x, y);  return;
        case SPRK_CLOCK:   ClockIcon(x, y);  return;
        case SPRK_RAIN:    RainIcon(x, y);   return;
        case SPRK_PIN:     PinIcon(x, y);    return;
        case SPRK_PORTAL:  PortalIcon(x, y); return;
        case SPRK_MASK:    MaskIcon(x, y);   return;
    }
    // >= 0x100: a real RGBA4444 sprite from sprites.h (see MM3D pseudo-ids below).
    if (sk >= 0) DrawSprite(x, y, sk, 0);
}
