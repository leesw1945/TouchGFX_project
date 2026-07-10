#ifndef MENUITEMS_HPP
#define MENUITEMS_HPP

#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

struct MenuItem
{
    uint16_t iconId;
    uint16_t gradientId;
    uint16_t titleId;
};

static const MenuItem menuItems[] =
{
    {
        BITMAP_MENU_ICON_HEAT_ID,
        BITMAP_MENU_GRADIENT_HEAT_ID,
        T_MENU_TEMPERATURE
    },
    {
        BITMAP_MENU_ICON_BATTERY_ID,
        BITMAP_MENU_GRADIENT_BATTERY_ID,
        T_MENU_BATTERY
    },
    {
        BITMAP_MENU_ICON_SOUND_ID,
        BITMAP_MENU_GRADIENT_SOUND_ID,
        T_MENU_VOLUME
    },
    {
        BITMAP_MENU_ICON_GAUGE_ID,
        BITMAP_MENU_GRADIENT_GAUGE_ID,
        T_MENU_GUAGE
    },
    {
        BITMAP_MENU_ICON_LIGHT_ID,
        BITMAP_MENU_GRADIENT_LIGHT_ID,
        T_MENU_LIGHT
    },
    {
        BITMAP_MENU_ICON_INFO_ID,
        BITMAP_MENU_GRADIENT_INFO_ID,
        T_MENU_INFO
    }
};

#endif
