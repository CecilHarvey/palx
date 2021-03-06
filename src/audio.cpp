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
#include "allegdef.h"
#include "internal.h"
#include "config.h"

#include "adplug/realopl.h"
#include "adplug/emuopl.h"
#include "adplug/kemuopl.h"

#define BUFFER_SIZE 2520

#define SAMPLE_RATE	44100
#define CHANNELS	1

#define MAX_VOICES 10

bool mask_use_CD=false;
boost::shared_ptr<player> musicplayer,cdplayer;
bool begin=false,once=false;
int voices[MAX_VOICES];int vocs=0;

int leaving=0;
	static short *buf;
void update_cache(playrix *plr)
{
	static int slen_buf=0,slen=630,v_scale=1;

	if(leaving<BUFFER_SIZE*CHANNELS && running)
	{
		slen_buf=0;
		int rel=BUFFER_SIZE*CHANNELS-leaving;
		while(slen_buf<rel)
		{
			if(!plr->rix.update())
			{
				if(once)
					plr->stop();
				plr->rix.rewind(plr->subsong);
				continue;
			}
			if(global->get<std::string>("music","opltype")=="real")
				break;//direct output to opl2;not fully cached forever, so every timer interrupt should get its own task done.
			plr->opl->update(buf, slen);
			for(int t=0;t<slen * CHANNELS;t++)
			{
				if (buf[t] >= 32767/v_scale)
					buf[t] = 32767;
				else if (buf[t] <= -32768/v_scale)
					buf[t] = -32768;
				else
					buf[t] *= v_scale;
				buf[t]^=0x8000;
			}
			buf+=slen * CHANNELS;
			slen_buf+=slen * CHANNELS;
		}
		buf=plr->Buffer;
		leaving+=slen_buf;
		buf+=leaving%(BUFFER_SIZE*CHANNELS);
	}
}
void playrix_timer(void *param)
{
	playrix * const plr=reinterpret_cast<playrix*>(param);
	if(voice_get_volume(plr->stream->voice)==0){
		begin=false;
		//memset(plr->stream->samp->data,0,plr->stream->samp->len*plr->stream->samp->bits/8);
	}
	voc::stop();
	short *p = (short*)get_audio_stream_buffer(plr->stream);
	if (running && begin && p)
	{
		update_cache(plr);
		if(global && global->get<std::string>("music","opltype")!="real")
		{
			leaving-=BUFFER_SIZE*CHANNELS;
			memcpy(p,plr->Buffer,BUFFER_SIZE*CHANNELS*2);
			memcpy(plr->Buffer,plr->Buffer+BUFFER_SIZE*CHANNELS,leaving*2);
			free_audio_stream_buffer(plr->stream);
		}

	}
	rest(0);
}
END_OF_FUNCTION(playrix_timer);

char playrix::mus[80];
Copl *getopl()
{
	if(global->get<std::string>("music","opltype")=="real")
		return new CRealopl(global->get<int>("music","oplport"));
	else if(global->get<std::string>("music","opltype")=="mame")
		return new CEmuopl(SAMPLE_RATE, true, CHANNELS == 2);
	else if(global->get<std::string>("music","opltype")=="ken")
		return new CKemuopl(SAMPLE_RATE, true, CHANNELS == 2);
	else
		running=false;
	return NULL;
}
playrix::playrix():opl(getopl()),rix(opl.get()),Buffer(0),stream(0),max_vol(global->get<int>("music","volume"))
{
	int BufferLength=SAMPLE_RATE*CHANNELS*10;
	if(!rix.load(mus, CProvider_Filesystem())){
		allegro_message("%s is missing",mus);
		throw std::exception();
	};
	stream = play_audio_stream(BUFFER_SIZE, 16, CHANNELS == 2, SAMPLE_RATE, max_vol, 128);
	LOCK_VARIABLE(Buffer);
	LOCK_VARIABLE(stream);
	LOCK_VARIABLE(opl);
	LOCK_VARIABLE(rix);
	LOCK_VARIABLE(subsong);
	LOCK_FUNCTION(playrix_timer);
	install_param_int(playrix_timer,this,14);

	Buffer = new short [BufferLength];
	memset(Buffer, 0, sizeof(short) * BufferLength);

	voice_set_volume(stream->voice,0);
}
playrix::~playrix()
{
	remove_param_int(playrix_timer,this);
	stop();
	stop_audio_stream(stream);
	delete []Buffer;
}
void playrix::play(int sub_song,int times)
{
	begin=false;
	once=(times==1);
	if(!sub_song){
		subsong=sub_song;
		stop();
		return;
	}
	subsong=sub_song;

	rix.rewind(subsong);
	//opl->init();
	//memset(Buffer, 0, sizeof(short) * SAMPLE_RATE * CHANNELS *10);

	leaving=0;buf=Buffer;
	update_cache(this);

	voice_set_volume(stream->voice,1);
	voice_ramp_volume(stream->voice, ((times==3)?2:0)*3000, max_vol);
	begin=true;

}
void playrix::stop(int gap)
{
	voice_ramp_volume(stream->voice, gap*3000, 0);
}

void playrix::setvolume(int vol)
{
	max_vol=vol;
	if(global->get<std::string>("music","opltype")!="real")
		voice_set_volume(stream->voice,vol);
	else
		((CRealopl*)opl.get())->setvolume(int((255-vol)/255.0*64));
	if(vol)
		begin=true;
}

void voc::init(uint8_t *f)
{
	spl=load_voc_mem(f);
	max_vol=global->get<int>("music","volume_sfx");
}

voc::voc(uint8_t *f)
{
	init(f);
}
voc::voc(int id)
{
	init(Pal::SFX.decode(id));
}

bool not_voc=false;
SAMPLE *voc::load_voc_mem(uint8_t *src)
{
   char buffer[30];
   int freq = 22050;
   int bits = 8;
   uint8_t *f=src;
   SAMPLE *spl = NULL;
   int len;
   int x, ver;
   int s;
   ASSERT(f);

   memset(buffer, 0, sizeof buffer);

   //pack_fread(buffer, 0x16, f);
   memcpy(buffer,f,0x16);f+=0x16;

   if (memcmp(buffer, "Creative Voice File", 0x13)){
	   not_voc=true;
      goto getout;
   }
   not_voc=false;

   ver = ((uint16_t*)f)[0];f+=2;
   if (ver != 0x010A && ver != 0x0114) /* version: should be 0x010A or 0x0114 */
      goto getout;

   ver = ((uint16_t*)f)[0];f+=2;
   if (ver != 0x1129 && ver != 0x111f) /* subversion: should be 0x1129 or 0x111f */
      goto getout;

   ver = f[0];f++;
   if (ver != 0x01 && ver != 0x09)     /* sound data: should be 0x01 or 0x09 */
      goto getout;

   len = ((uint16_t*)f)[0];f+=2;                /* length is three bytes long: two */
   x = f[0];f++;                   /* .. and one byte */
   x <<= 16;
   len += x;

   if (ver == 0x01) {                  /* block type 1 */
      len -= 2;                        /* sub. size of the rest of block header */
      x = f[0];f++;                /* one byte of frequency */
      freq = 1000000 / (256-x);

      x = f[0];f++;                /* skip one byte */

      spl = create_sample(8, FALSE, freq, len);

      if (spl) {
	 //if (pack_fread(spl->data, len, f) < len) {
	  memcpy(spl->data,f,len);f+=len;
	 //   destroy_sample(spl);
	 //   spl = NULL;
	 //}
      }
   }
   else {                              /* block type 9 */
      len -= 12;                       /* sub. size of the rest of block header */
      freq = ((uint16_t*)f)[0];f+=2;            /* two bytes of frequency */

      x = ((uint16_t*)f)[0];f+=2;               /* skip two bytes */

      bits = f[0];f++;             /* # of bits per sample */
      if (bits != 8 && bits != 16)
	 goto getout;

      x = f[0];f++;
      if (x != 1)                      /* # of channels: should be mono */
	 goto getout;

      //pack_fread(buffer, 0x6, f);      /* skip 6 bytes of unknown data */
	  f+=6;

      spl = create_sample(bits, FALSE, freq, len*8/bits);

      if (spl) {
	 if (bits == 8) {
	 //   if (pack_fread(spl->data, len, f) < len) {
		  memcpy(spl->data,f,len);f+=len;
	 //      destroy_sample(spl);
	 //      spl = NULL;
	 //   }
	 }
	 else {
	    len /= 2;
	    for (x=0; x<len; x++) {
	 //      if ((s = pack_igetw(f)) == EOF) {
			s=((uint16_t*)f)[0];f+=2;
	 //	  destroy_sample(spl);
	//	  spl = NULL;
	//	  break;
	//       }
	       ((signed short *)spl->data)[x] = (signed short)s^0x8000;
	    }
	 }
     }
   }

getout:
   return spl;
}

void voc::play()
{
	if(!not_voc && max_vol && vocs<MAX_VOICES-1)
		voices[vocs++]=play_sample(spl,max_vol,128,1000,0);
}
void voc::stop()
{
	for(int i=0;i<vocs;i++)
		if(!voice_check(voices[i])){
			destroy_sample(voice_check(voices[i]));
			std::copy(voices+i+1,voices+MAX_VOICES,voices+i);
			vocs--;
		}
}
bool not_midi=false;

void b2l_memcpy(uint8_t *dst,const uint8_t *src,size_t size)
{
	for(size_t i=0;i<size;i++)
		dst[i]=src[size-1-i];
}
MIDI *load_midi_mem(int midiseq)
{
	long len,sek=0;
	uint8_t *midibuf=Pal::MIDI.decode(midiseq,len);
   int c;
   char buf[4];
   long data=0;

   MIDI *midi;
   int num_tracks=0;

   midi = (MIDI*)malloc(sizeof(MIDI));              /* get some memory */
   if (!midi) {
	   not_midi=true;
      return NULL;
   }
   not_midi=false;

   for (c=0; c<MIDI_TRACKS; c++) {
      midi->track[c].data = NULL;
      midi->track[c].len = 0;
   }

   memcpy(buf,midibuf+sek,4);sek+=4;//pack_fread(buf, 4, fp); /* read midi header */

   /* Is the midi inside a .rmi file? */
   if (memcmp(buf, "RIFF", 4) == 0) { /* check for RIFF header */
      sek+=4;//pack_mgetl(fp);


	   while (sek<=len){//!pack_feof(fp)) {
         memcpy(buf,midibuf+sek,4);sek+=4;//pack_fread(buf, 4, fp); /* RMID chunk? */
         if (memcmp(buf, "RMID", 4) == 0) break;

         sek+=4;sek+=*(long*)(midibuf+sek);//pack_fseek(fp, pack_igetl(fp)); /* skip to next chunk */
      }

      if (sek>=len) goto err;

      sek+=4;//pack_mgetl(fp);
      sek+=4;//pack_mgetl(fp);
      memcpy(buf,midibuf+sek,4);sek+=4;//pack_fread(buf, 4, fp); /* read midi header */
   }

   if (memcmp(buf, "MThd", 4))
      goto err;

   sek+=4;//pack_mgetl(fp);                           /* skip header chunk length */

   b2l_memcpy((uint8_t*)&data,midibuf+sek,2);sek+=2;//pack_mgetw(fp);                    /* MIDI file type */
   if ((data != 0) && (data != 1))
      goto err;

   b2l_memcpy((uint8_t*)&num_tracks,midibuf+sek,2);sek+=2;//pack_mgetw(fp);              /* number of tracks */
   if ((num_tracks < 1) || (num_tracks > MIDI_TRACKS))
      goto err;

   b2l_memcpy((uint8_t*)&data,midibuf+sek,2);sek+=2;//pack_mgetw(fp);                    /* beat divisions */
   midi->divisions = ABS(data);

   for (c=0; c<num_tracks; c++) {            /* read each track */
      memcpy(buf,midibuf+sek,4);sek+=4;//pack_fread(buf, 4, fp);                /* read track header */
      if (memcmp(buf, "MTrk", 4))
	 goto err;

      b2l_memcpy((uint8_t*)&data,midibuf+sek,4);sek+=4;//pack_mgetl(fp);                 /* length of track chunk */
      midi->track[c].len = data;

      midi->track[c].data = (uint8_t*)malloc(data); /* allocate memory */
      if (!midi->track[c].data)
	 goto err;
					     /* finally, read track data */
	  if(len-sek>=data){
		  memcpy(midi->track[c].data,midibuf+sek,data);
		  sek+=data;
	  }
	  else
      //if (pack_fread(midi->track[c].data, data, fp) != data)
	 goto err;
   }

   //pack_fclose(fp);
   lock_midi(midi);
   return midi;

   /* oh dear... */
   err:
   //pack_fclose(fp);
   destroy_midi(midi);
   return NULL;
}
playmidi::playmidi():pmidi(NULL)
{
	subsong=-1;
}
playmidi::~playmidi()
{}
void playmidi::play(int sub_song,int times)
{
	if(global->get<int>("music","volume")){
		if(!sub_song){
			stop();
			return;
		}
		subsong=sub_song;
		if(pmidi)
			stop();
		pmidi=load_midi_mem(sub_song);
		play_midi(pmidi,(times!=1)?1:0);
	}
}
void playmidi::stop(int)
{
	stop_midi();
	//destroy_midi(pmidi);
	pmidi=NULL;
	play_midi(pmidi,0);
}
void playmidi::setvolume(int vol)
{
	if(vol<=0)
		stop();
	else if(!pmidi)
		play(subsong);
}
