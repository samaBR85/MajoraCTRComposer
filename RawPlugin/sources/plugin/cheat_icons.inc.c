// Which icon illustrates each cheat row (-1 = none, which is fine for most rows).
static int SpriteKeyForCheat(int ch)
{
    switch (ch)
    {
        case CH_EX_DIRECT:  return SPRK_PIN;
        case CH_EX_HOTKEY:  return SPRK_CLOCK;
        case CH_MM_DAY1: case CH_MM_DAY2: case CH_MM_DAY3: return SPRK_CLOCK;
        case CH_MM_TIME_6AM:  return SPRK_SUNRISE;
        case CH_MM_TIME_10AM: return SPRK_DAY;
        case CH_MM_TIME_6PM:  return SPRK_SUNSET;
        case CH_MM_TIME_SCRUB: return SPRK_CLOCK;
        case CH_MM_DAY_SCRUB:  return SPRK_DAY;

        case CH_MM_REFILL_HEARTS: case CH_MM_HEARTS_MAX: return MSPR_HEART;
        case CH_MM_REFILL_MAGIC:  return MSPR_MAGIC_FAIRY;
        case CH_MM_DEFENSE:       return MSPR_DEFENSE;

        case CH_MM_RUPEES_MAX:     return MSPR_RUPEE;
        case CH_MM_GILDED_MIRROR:  return MSPR_SWORD;
        case CH_MM_QUIVER_BOMBBAG: return MSPR_QUIVER;
        case CH_BBUTTON_GILDED:    return MSPR_SWORD;
        case CH_BBUTTON_GFSWORD:   return MSPR_GFSWORD;
        case CH_MM_BANK_FILL:      return MSPR_RUPEE;

        case CH_MM_AMMO_ARROWS: return MSPR_ARROWS;
        case CH_MM_AMMO_BOMBS:  return MSPR_BOMBS;
        case CH_MM_AMMO_CHUS:   return MSPR_BOMBCHUS;
        case CH_MM_AMMO_STICKS: return MSPR_STICKS;
        case CH_MM_AMMO_NUTS:   return MSPR_NUTS;
        case CH_MM_AMMO_BEANS:  return MSPR_BEANS;
        case CH_MM_AMMO_KEG:    return MSPR_KEG;

        case CH_MM_ALL_ITEMS:        return MSPR_ITEMS_DEED;
        case CH_MM_ALL_MASKS:        return MSPR_ALL_MASKS;
        case CH_MM_ALL_BOSSES_SONGS: return MSPR_BOSS_REMAINS;
        case CH_MM_ALL_FAIRIES:      return MSPR_FAIRY;
        case CH_TEST_FISHING:        return MSPR_FISHINGROD;

        case CH_MM_MOONJUMP: return MSPR_MOONJUMP;

        case CH_PLAY_NORMAL:      return SPRK_MASK; // no mask worn - generic hand-drawn mask icon
        case CH_PLAY_ZORA:        return MSPR_ZORA_MASK;
        case CH_PLAY_FIERCEDEITY: return MSPR_FD_MASK;

        // Minigames: an icon that hints at the activity - a bow for the shooting galleries,
        // Zora for the swim race, a jump for boat/jump, a heart for Honey & Darling.
        case CH_MG_TOWN_GALLERY:  case CH_MG_SWAMP_GALLERY: return MSPR_ARROWS;
        case CH_MG_BEAVER:        return MSPR_ZORA_MASK;
        case CH_MG_BOAT_JUMP:     return MSPR_MOONJUMP;
        case CH_MG_HONEY_DARLING: return MSPR_HEART;

        // Settings rows: a mask changes your look, like a theme changes the menu's; a
        // written scroll for picking a language.
        case CH_CFG_THEME: return MSPR_GARO_MASK;
        case CH_CFG_LANG:  return MSPR_SCROLL;
    }
    return -1;
}
