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
#ifndef MAINLOOP_H
#define MAINLOOP_H

#include <list>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/multi_array.hpp>

#include "resource.h"
#include "allegdef.h"

struct scene_map:public bitmap
{
	virtual void change(int p)=0;
	scene_map(const uint8_t *a=NULL,int b=SCREEN_W,int c=SCREEN_H):bitmap(a,b,c){}
protected:
	long len;
};
class fbp:public scene_map{
public:
	fbp():scene_map(0,320,160){}
	fbp(int p):scene_map(Pal::FBP.decode(p),320,200){}
	void change(int p){
		memcpy(bmp->dat,Pal::FBP.decode(p),bmp->w*bmp->h);
	}
};
struct tile{
	boost::shared_ptr<sprite> image;
	bool blocked;
	int layer;
	bool valid;
	tile():image((sprite*)0),blocked(0),layer(0),valid(false){}
};
class palmap:public scene_map{
public:
	void make_onescreen(BITMAP *dest,int source_x,int source_y,int dest_x,int dest_y);
	void blit_to(BITMAP *dest,int sx,int sy,int dx,int dy);
	tile &gettile(int x,int y,int h,int l);
	palmap();
	void change(int p);
	int curr_map;
private:
	boost::multi_array<tile,4> sprites;
	sprite &getsprite(int x,int y,int h,int l,uint8_t *src,bool throu,int layer);
	void make_tile(uint8_t*,int,int,int,BITMAP*);
};

class sprite_queue;
struct Scene{
	palmap scenemap;
	bitmap scene_buf;
	EVENT_OBJECT *sprites_begin,*sprites_end;
	position team_pos;
	Scene();
	void clear_scanlines();
	void move_usable_screen();
	void get_sprites();
	void produce_one_screen();
	void scanline_draw_normal_scene(sprite_queue&,int,BITMAP * =screen);
};

extern Scene *scene;
class sprite_queue{
public:
	void clear_active();
	void calc_team_walking();
	void our_team_setdraw();
	void visible_NPC_movment_setdraw();
	void Redraw_Tiles_or_Fade_to_pic(bitmap &dst=scene->scene_buf,BITMAP *src=backbuf);
	void flush(bitmap &);
	typedef std::vector<boost::shared_ptr<sprite> > s_list;
	typedef std::list<boost::shared_ptr<sprite> > s_set;
	void push(s_list::value_type it){
		active_list.push_back(it);
	}
private:
	s_list active_list;
	s_set brick_list;
};

#endif
