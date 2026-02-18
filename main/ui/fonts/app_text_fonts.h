#pragma once

#include "lvgl.h"

#ifndef APP_HAVE_POPPINS_REGULAR_12
#define APP_HAVE_POPPINS_REGULAR_12 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_14
#define APP_HAVE_POPPINS_REGULAR_14 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_16
#define APP_HAVE_POPPINS_REGULAR_16 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_18
#define APP_HAVE_POPPINS_REGULAR_18 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_20
#define APP_HAVE_POPPINS_REGULAR_20 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_22
#define APP_HAVE_POPPINS_REGULAR_22 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_24
#define APP_HAVE_POPPINS_REGULAR_24 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_28
#define APP_HAVE_POPPINS_REGULAR_28 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_34
#define APP_HAVE_POPPINS_REGULAR_34 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_36
#define APP_HAVE_POPPINS_REGULAR_36 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_38
#define APP_HAVE_POPPINS_REGULAR_38 0
#endif
#ifndef APP_HAVE_POPPINS_REGULAR_40
#define APP_HAVE_POPPINS_REGULAR_40 0
#endif

#if APP_HAVE_POPPINS_REGULAR_12
LV_FONT_DECLARE(poppins_regular_12);
#define APP_FONT_TEXT_12 (&poppins_regular_12)
#elif LV_FONT_MONTSERRAT_14
#define APP_FONT_TEXT_12 (&lv_font_montserrat_14)
#else
#define APP_FONT_TEXT_12 LV_FONT_DEFAULT
#endif

#if APP_HAVE_POPPINS_REGULAR_14
LV_FONT_DECLARE(poppins_regular_14);
#if APP_HAVE_POPPINS_REGULAR_12
#define APP_FONT_TEXT_14 (&poppins_regular_12)
#else
#define APP_FONT_TEXT_14 (&poppins_regular_14)
#endif
#elif LV_FONT_MONTSERRAT_14
#define APP_FONT_TEXT_14 (&lv_font_montserrat_14)
#else
#define APP_FONT_TEXT_14 APP_FONT_TEXT_12
#endif

#if APP_HAVE_POPPINS_REGULAR_16
LV_FONT_DECLARE(poppins_regular_16);
#define APP_FONT_TEXT_16 (&poppins_regular_16)
#elif LV_FONT_MONTSERRAT_16
#define APP_FONT_TEXT_16 (&lv_font_montserrat_16)
#else
#define APP_FONT_TEXT_16 APP_FONT_TEXT_14
#endif

#if APP_HAVE_POPPINS_REGULAR_18
LV_FONT_DECLARE(poppins_regular_18);
#define APP_FONT_TEXT_18 (&poppins_regular_18)
#elif LV_FONT_MONTSERRAT_18
#define APP_FONT_TEXT_18 (&lv_font_montserrat_18)
#else
#define APP_FONT_TEXT_18 APP_FONT_TEXT_16
#endif

#if APP_HAVE_POPPINS_REGULAR_20
LV_FONT_DECLARE(poppins_regular_20);
#if APP_HAVE_POPPINS_REGULAR_14
#define APP_FONT_TEXT_20 (&poppins_regular_14)
#else
#define APP_FONT_TEXT_20 (&poppins_regular_20)
#endif
#elif LV_FONT_MONTSERRAT_20
#define APP_FONT_TEXT_20 (&lv_font_montserrat_20)
#else
#define APP_FONT_TEXT_20 LV_FONT_DEFAULT
#endif

#if APP_HAVE_POPPINS_REGULAR_22
LV_FONT_DECLARE(poppins_regular_22);
#if APP_HAVE_POPPINS_REGULAR_20
#define APP_FONT_TEXT_22 (&poppins_regular_20)
#else
#define APP_FONT_TEXT_22 (&poppins_regular_22)
#endif
#elif LV_FONT_MONTSERRAT_22
#define APP_FONT_TEXT_22 (&lv_font_montserrat_22)
#else
#define APP_FONT_TEXT_22 APP_FONT_TEXT_20
#endif

#if APP_HAVE_POPPINS_REGULAR_24
LV_FONT_DECLARE(poppins_regular_24);
#if APP_HAVE_POPPINS_REGULAR_22
#define APP_FONT_TEXT_24 (&poppins_regular_22)
#else
#define APP_FONT_TEXT_24 (&poppins_regular_24)
#endif
#elif LV_FONT_MONTSERRAT_24
#define APP_FONT_TEXT_24 (&lv_font_montserrat_24)
#else
#define APP_FONT_TEXT_24 APP_FONT_TEXT_22
#endif

#if APP_HAVE_POPPINS_REGULAR_28
LV_FONT_DECLARE(poppins_regular_28);
#if APP_HAVE_POPPINS_REGULAR_24
#define APP_FONT_TEXT_28 (&poppins_regular_24)
#else
#define APP_FONT_TEXT_28 (&poppins_regular_28)
#endif
#elif LV_FONT_MONTSERRAT_28
#define APP_FONT_TEXT_28 (&lv_font_montserrat_28)
#else
#define APP_FONT_TEXT_28 APP_FONT_TEXT_24
#endif

#if APP_HAVE_POPPINS_REGULAR_34
LV_FONT_DECLARE(poppins_regular_34);
#if APP_HAVE_POPPINS_REGULAR_28
#define APP_FONT_TEXT_34 (&poppins_regular_28)
#else
#define APP_FONT_TEXT_34 (&poppins_regular_34)
#endif
#elif LV_FONT_MONTSERRAT_34
#define APP_FONT_TEXT_34 (&lv_font_montserrat_34)
#else
#define APP_FONT_TEXT_34 APP_FONT_TEXT_28
#endif

/* Display fonts keep the original size for large headline values (e.g. temperatures). */
#if APP_HAVE_POPPINS_REGULAR_24
#define APP_FONT_DISPLAY_24 (&poppins_regular_24)
#elif LV_FONT_MONTSERRAT_24
#define APP_FONT_DISPLAY_24 (&lv_font_montserrat_24)
#else
#define APP_FONT_DISPLAY_24 APP_FONT_TEXT_24
#endif

#if APP_HAVE_POPPINS_REGULAR_28
#define APP_FONT_DISPLAY_28 (&poppins_regular_28)
#elif LV_FONT_MONTSERRAT_28
#define APP_FONT_DISPLAY_28 (&lv_font_montserrat_28)
#else
#define APP_FONT_DISPLAY_28 APP_FONT_TEXT_28
#endif

#if APP_HAVE_POPPINS_REGULAR_34
#define APP_FONT_DISPLAY_34 (&poppins_regular_34)
#elif LV_FONT_MONTSERRAT_34
#define APP_FONT_DISPLAY_34 (&lv_font_montserrat_34)
#else
#define APP_FONT_DISPLAY_34 APP_FONT_TEXT_34
#endif

#if APP_HAVE_POPPINS_REGULAR_36
LV_FONT_DECLARE(poppins_regular_36);
#define APP_FONT_DISPLAY_36 (&poppins_regular_36)
#elif LV_FONT_MONTSERRAT_36
#define APP_FONT_DISPLAY_36 (&lv_font_montserrat_36)
#else
#define APP_FONT_DISPLAY_36 APP_FONT_DISPLAY_34
#endif

#if APP_HAVE_POPPINS_REGULAR_38
LV_FONT_DECLARE(poppins_regular_38);
#define APP_FONT_DISPLAY_38 (&poppins_regular_38)
#elif LV_FONT_MONTSERRAT_38
#define APP_FONT_DISPLAY_38 (&lv_font_montserrat_38)
#else
#define APP_FONT_DISPLAY_38 APP_FONT_DISPLAY_36
#endif

#if APP_HAVE_POPPINS_REGULAR_40
LV_FONT_DECLARE(poppins_regular_40);
#define APP_FONT_DISPLAY_40 (&poppins_regular_40)
#elif LV_FONT_MONTSERRAT_40
#define APP_FONT_DISPLAY_40 (&lv_font_montserrat_40)
#else
#define APP_FONT_DISPLAY_40 APP_FONT_DISPLAY_38
#endif
