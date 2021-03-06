/***************************************************************************
 *   PALx: A platform independent port of classic RPG PAL                  *
 *   Copyleft (C) 2006 by Pal Lockheart                                    *
 *   palxex@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, If not, see                          *
 *   <http://www.gnu.org/licenses/>.                                       *
 ***************************************************************************/
 #include <allegro.h>
/* XPM */
static char * celestial_xpm[] = {
"32 32 3 1",
" 	c None",
".	c #808080",
"+	c #000000",
"                                ",
"        ..                      ",
"      ..++.                     ",
"     ..+++.          ...        ",
"     .++++.         .++..       ",
"     .++++.         .++++       ",
"     +++++.          ++++       ",
"    .+++++.          ++++       ",
"   ..+++++.          ++++       ",
"   .+++++.          .++++       ",
"   .+++++.          .++++       ",
"  .++++++.          .++++       ",
"  .+++++.           .++++       ",
"  ++++++            .++++ ....  ",
" .++++++      ...   .++++..+++. ",
".+++++++.     .++. .++++++..+++ ",
".++++++++.   .+++. .+++++. .+++ ",
"..++++++++.  .++++  .++++  .+++ ",
" .+++.++++. ..++++  .++++. .+++ ",
" .++. .+++. .+++++   .++++..+++ ",
"  ....++++. ++++++...++++...+++ ",
"     .++++. ++++++++++..   .++. ",
"     +++++. .++++++...     .++. ",
"     +++++. ..++...         +.  ",
"     +++++.  ...                ",
"     .++++.                     ",
"     .++++.                     ",
"     .++++.                     ",
"      .+++                      ",
"      ....                      ",
"                                ",
"                                "};

#if defined ALLEGRO_WITH_XWINDOWS && defined ALLEGRO_USE_CONSTRUCTOR
extern void *allegro_icon;
CONSTRUCTOR_FUNCTION(static void _set_allegro_icon(void));
static void _set_allegro_icon(void)
{
    allegro_icon = celestial_xpm;
}
#endif
int __just_for_eliminating_the_warning_gcc_reported=celestial_xpm[0][0];
