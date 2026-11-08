#include "xengine.h"
#include "game.h"
#include "train_base.h"
#include <dc/pvr.h>
#include <dc/maple/controller.h>
#include <dc/sound/sfxmgr.h>
#include <dc/sound/sound.h>
#include <dc/sound/stream.h>
#include <array>
#include <map>
#include <string>
#include <cstring>
#include <stdexcept>
#include <cctype>

CXEngine*XEngine=new CXEngine;
namespace {
bool running=true,scene=false,additive=false,audioPaused=false;
float alpha=1;
unsigned frameQuads=0;bool uiDraw=false;
std::map<std::string,std::unique_ptr<IDirect3DTexture8>> textures;
std::map<std::string,std::shared_ptr<SoundData>> sounds;
std::vector<std::unique_ptr<IDirectMusicSegmentState>> tokens;
IDirect3DTexture8*font=nullptr;
FILE*music=nullptr;
snd_stream_hnd_t musicStream=-1;
bool musicPlaying=false,musicPaused=false;
alignas(32) int16_t musicBuffer[65536];
std::string path(const char*name) {
 std::string p=name;if(p.size()>2&&p[1]==':')p=p.substr(3);
 for(char&c:p){if(c=='\\')c='/';c=char(std::tolower(static_cast<unsigned char>(c)));}
 if(p.rfind("/cd/",0)==0)return p;
 if(p.rfind("game/",0)==0)return "/cd/"+p;
 return "/cd/game/"+p;
}
void begin(){frameQuads=0;pvr_wait_ready();pvr_scene_begin();pvr_list_begin(PVR_LIST_TR_POLY);scene=true;}
void finish(){if(scene){pvr_list_finish();pvr_scene_finish();scene=false;}}
void header(IDirect3DTexture8*t,uint32_t color){
 (void)color;pvr_poly_cxt_t c;
 if(t)pvr_poly_cxt_txr(&c,PVR_LIST_TR_POLY,PVR_TXRFMT_ARGB4444|PVR_TXRFMT_NONTWIDDLED,t->width,t->height,reinterpret_cast<pvr_ptr_t>(uintptr_t(t->id)),PVR_FILTER_BILINEAR);
 else pvr_poly_cxt_col(&c,PVR_LIST_TR_POLY);
 c.gen.culling=PVR_CULLING_NONE;c.depth.comparison=PVR_DEPTHCMP_ALWAYS;c.depth.write=PVR_DEPTHWRITE_DISABLE;
 c.blend.src=PVR_BLEND_SRCALPHA;c.blend.dst=additive?PVR_BLEND_ONE:PVR_BLEND_INVSRCALPHA;
 if(t)c.txr.env=PVR_TXRENV_MODULATEALPHA;
 pvr_poly_hdr_t h;pvr_poly_compile(&h,&c);pvr_prim(&h,sizeof(h));
}
void vertex(float x,float y,float u,float v,uint32_t color,bool end){
 pvr_vertex_t p{};p.flags=end?PVR_CMD_VERTEX_EOL:PVR_CMD_VERTEX;
 p.x=(x+20.f)*640.f/680.f;p.y=(y+20.f)*480.f/520.f;p.z=1.f;p.u=u;p.v=v;p.argb=color;
 pvr_prim(&p,sizeof(p));
}
void quad(IDirect3DTexture8*t,const float*x,const float*y,float u0,float v0,float u1,float v1,uint32_t color){
 if(frameQuads>=(uiDraw?3000u:2500u))return;++frameQuads;
 header(t,color);vertex(x[0],y[0],u0,v0,color,false);vertex(x[1],y[1],u1,v0,color,false);
 vertex(x[2],y[2],u0,v1,color,false);vertex(x[3],y[3],u1,v1,color,true);
}
void sprite(IDirect3DTexture8*t,float x,float y,float w,float h,float rot,bool offset){
 if(!t||t->empty)return;
 float xs[4],ys[4];float angle=-rot*.01745329252f,c=std::cos(angle),s=std::sin(angle);
 const float left=offset?-.85f:-.5f,top=offset?-.15f:-.5f;
 float us[]={t->u0,t->u1,t->u0,t->u1},vs[]={t->v0,t->v0,t->v1,t->v1};
 for(int i=0;i<4;++i){float px=left+us[i],py=top+vs[i];
  if(offset){xs[i]=x+(px*c-py*s)*w;ys[i]=y+(px*s+py*c)*h;}
  else{px*=w;py*=h;xs[i]=x+px*c-py*s;ys[i]=y+px*s+py*c;}}
 quad(t,xs,ys,t->u0,t->v0,t->u1,t->v1,PVR_PACK_COLOR(alpha,1,1,1));
}
void* musicCallback(snd_stream_hnd_t,int requested,int*received){
 int count=std::min(requested,int(sizeof(musicBuffer)));size_t bytes=size_t(count),got=0;
 if(!music){*received=0;return nullptr;}int wraps=0;
 while(got<bytes){size_t n=fread(reinterpret_cast<char*>(musicBuffer)+got,1,bytes-got,music);got+=n;
  if(!n){if(ferror(music)){clearerr(music);break;}if(++wraps>1)break;rewind(music);}
 }
 if(got<bytes)std::memset(reinterpret_cast<char*>(musicBuffer)+got,0,bytes-got);
 *received=count;return musicBuffer;
}
void pollPads(){
 for(int i=0;i<4;++i){auto&p=XEngine->GamePad[i];auto*d=maple_enum_type(i,MAPLE_FUNC_CONTROLLER);
  auto*s=d?static_cast<cont_state_t*>(maple_dev_status(d)):nullptr;WORD buttons=0;BYTE a[8]{};
  p.hDevice=s?reinterpret_cast<void*>(uintptr_t(i+1)):nullptr;p.fX1=p.fY1=p.fX2=p.fY2=0;
  if(s){p.fX1=std::abs(int(s->joyx))>20?s->joyx/127.f:0;p.fY1=std::abs(int(s->joyy))>20?-s->joyy/127.f:0;
   if(s->buttons&CONT_DPAD_UP)buttons|=1;if(s->buttons&CONT_DPAD_DOWN)buttons|=2;
   if(s->buttons&CONT_DPAD_LEFT)buttons|=4;if(s->buttons&CONT_DPAD_RIGHT)buttons|=8;
   if(s->buttons&CONT_START)buttons|=16;if(s->buttons&CONT_B)buttons|=32;
   p.fX2=((buttons&8)?1.f:0.f)-((buttons&4)?1.f:0.f);p.fY2=((buttons&1)?1.f:0.f)-((buttons&2)?1.f:0.f);
   a[0]=(s->buttons&CONT_A)?255:0;a[1]=(s->buttons&CONT_B)?255:0;
   a[4]=(s->buttons&CONT_X)?255:0;a[5]=(s->buttons&CONT_Y)?255:0;a[6]=s->ltrig;a[7]=s->rtrig;
   const unsigned reset=CONT_START|CONT_A|CONT_B|CONT_X|CONT_Y;if((s->buttons&reset)==reset)running=false;
  }
  p.wPressedButtons=buttons&~p.wLastButtons;p.wLastButtons=p.wButtons=buttons;
  for(int k=0;k<8;++k){p.bPressedAnalogButtons[k]=a[k]>75&&!p.bLastAnalogButtons[k];p.bLastAnalogButtons[k]=a[k]>75;p.bAnalogButtons[k]=a[k];}
 }
}
}

void CXEngine::XInit(){
 vid_set_mode(DM_640x480,PM_RGB565);pvr_init_params_t p=pvr_default_params;
 p.opb_sizes[0]=PVR_BINSIZE_0;p.autosort_disabled=1;p.opb_overflow_count=2;pvr_init(&p);pvr_set_bg_color(0,0,0);
 snd_init();snd_stream_init_ex(2,65536);musicStream=snd_stream_alloc(musicCallback,65536);
 font=LoadTexture("/cd/font.dct");pollPads();begin();
 printf("DBX Dreamcast platform ready; PVR free=%u\n",unsigned(pvr_mem_available()));
}
void CXEngine::XUpdate(){finish();if(musicPlaying&&!musicPaused&&!audioPaused)snd_stream_poll(musicStream);
 pollPads();m_Ticks+=m_GameSpeed;m_Millisecs.QuadPart=GetTickCount64();m_SoundsPerFrame=16;begin();}
void CXEngine::RequestExit(){running=false;}bool CXEngine::Running()const{return running;}
bool CXEngine::TakeMenuBack(){return false;}
void CXEngine::ClearInputEdges(){for(auto&p:GamePad){p.wPressedButtons=0;std::fill(std::begin(p.bPressedAnalogButtons),std::end(p.bPressedAnalogButtons),0);}}
void CXEngine::Shutdown(){finish();pvr_wait_ready();StopAllSounds();if(musicPlaying)snd_stream_stop(musicStream);
 if(musicStream>=0)snd_stream_destroy(musicStream);if(music)fclose(music);music=nullptr;snd_stream_shutdown();snd_sfx_unload_all();sounds.clear();
 for(auto&t:textures)pvr_mem_free(reinterpret_cast<pvr_ptr_t>(uintptr_t(t.second->id)));textures.clear();pvr_shutdown();}
IDirect3DTexture8*CXEngine::LoadTexture(const char*name){
 auto file=path(name);if(file.size()>4&&file.substr(file.size()-4)==".png")file.replace(file.size()-4,4,".dct");
 auto hit=textures.find(file);if(hit!=textures.end())return hit->second.get();
 FILE*f=fopen(file.c_str(),"rb");if(!f)throw std::runtime_error("Missing texture: "+file);
 unsigned h[3]{};if(fread(h,1,12,f)!=12||h[0]!=0x58544344u||h[1]<8||h[2]<8||h[1]>1024||h[2]>1024){fclose(f);throw std::runtime_error("Invalid texture: "+file);}
 std::vector<uint16_t>pixels(size_t(h[1])*h[2]);if(fread(pixels.data(),2,pixels.size(),f)!=pixels.size()){fclose(f);throw std::runtime_error("Truncated texture: "+file);}fclose(f);
 auto t=std::make_unique<IDirect3DTexture8>();t->width=h[1];t->height=h[2];unsigned l=h[1],top=h[2],r=0,b=0;
 for(unsigned y=0;y<h[2];++y)for(unsigned x=0;x<h[1];++x)if(pixels[size_t(y)*h[1]+x]&0xf000){l=std::min(l,x);r=std::max(r,x+1);top=std::min(top,y);b=std::max(b,y+1);}
 t->empty=r==0;if(!t->empty){t->u0=float(l>2?l-2:0)/h[1];t->u1=float(std::min(r+2,h[1]))/h[1];t->v0=float(top>2?top-2:0)/h[2];t->v1=float(std::min(b+2,h[2]))/h[2];}
 auto mem=pvr_mem_malloc(pixels.size()*2);if(!mem)throw std::runtime_error("Dreamcast texture memory exhausted: "+file);
 pvr_txr_load(pixels.data(),mem,pixels.size()*2);t->id=uint32_t(uintptr_t(mem));auto*out=t.get();textures[file]=std::move(t);return out;
}
void CXEngine::SetMaterial(float,float,float,float a){alpha=std::clamp(a,0.f,1.f);}
void CXEngine::SetSpriteMode(int m){additive=m==SM_ALPHA_ADD;}
void CXEngine::SetCameraPos(float x,float y){cameraX=x;cameraY=-y;}
void CXEngine::DrawSprite(IDirect3DTexture8*t,float x,float y,float w,float h,float r){sprite(t,x+cameraX,y+cameraY,w,h,r,false);}
void CXEngine::DrawSpriteOC(IDirect3DTexture8*t,float x,float y,float w,float h,float r){sprite(t,x+cameraX,y+cameraY,w,h,r,true);}
void CXEngine::DrawTrainBase(IDirect3DTexture8*t,float x,float y,float w,float h){
 if(!t||t->empty)return;float end=y-h/2+h*std::max(t->v0,t->v1-2.f/t->height);TrainBaseStretch s(end,cameraY);
 float cut=std::clamp((460-y)/h+.5f,t->v0,t->v1);float u0=t->u0,u1=t->u1;
 for(auto band: {std::pair<float,float>{t->v0,cut},{cut,t->v1}}){if(band.second<=band.first)continue;
  float xs[]={x-w/2+w*u0+cameraX,x-w/2+w*u1+cameraX,x-w/2+w*u0+cameraX,x-w/2+w*u1+cameraX};
  float top=s.map(y-h/2+h*band.first)+cameraY,bottom=s.map(y-h/2+h*band.second)+cameraY;float ys[]={top,top,bottom,bottom};
  quad(t,xs,ys,u0,band.first,u1,band.second,PVR_PACK_COLOR(alpha,1,1,1));}
}
void CXEngine::DrawTracer(IDirect3DTexture8*t,float x1,float y1,float x2,float y2,float w){
 if(!t)return;float a=ATan2(x2-x1,y2-y1),dx=Sin(a-90)*w,dy=Cos(a-90)*w;
 float xs[]={x1-dx+cameraX,x1+dx+cameraX,x2-dx+cameraX,x2+dx+cameraX};float ys[]={y1-dy+cameraY,y1+dy+cameraY,y2-dy+cameraY,y2+dy+cameraY};
 quad(t,xs,ys,0,1,1,0,PVR_PACK_COLOR(alpha,1,1,1));
}
void CXEngine::FillRect(float x,float y,float w,float h,float r,float g,float b,float a){float xs[]={x,x+w,x,x+w},ys[]={y,y,y+h,y+h};bool old=additive,oldUI=uiDraw;uiDraw=true;additive=false;quad(nullptr,xs,ys,0,0,1,1,PVR_PACK_COLOR(a,r,g,b));additive=old;uiDraw=oldUI;}
void CXEngine::DrawUiText(const char*text,float x,float y,float size,int r,int g,int b,bool center){
 if(!font)return;float cw=size*.60f;if(center)x-=std::strlen(text)*cw/2;
 bool old=additive,oldUI=uiDraw;uiDraw=true;additive=false;for(const unsigned char*c=reinterpret_cast<const unsigned char*>(text);*c;++c){
  unsigned k=*c<128?*c:'?';float xs[]={x,x+size,x,x+size},ys[]={y,y,y+size,y+size};
  quad(font,xs,ys,(k%16)/16.f,(k/16)/16.f,(k%16+1)/16.f,(k/16+1)/16.f,PVR_PACK_COLOR(1,r/255.f,g/255.f,b/255.f));x+=cw;}
 additive=old;uiDraw=oldUI;
}
void CXEngine::DrawText(const char*s,int x,int y,int r,int g,int b){DrawUiText(s,x,y,20,r,g,b);}
void CXEngine::DrawScoreText(const char*s,int x,int y,int r,int g,int b){DrawUiText(s,x,y,14,r,g,b);}
void CXEngine::DrawMenuText(const char*s,int x,int y,int r,int g,int b){DrawUiText(s,x,y,28,r,g,b);}
void CXEngine::DrawHudScore(const char*s,float x,int r,int g,int b){float size=std::min(18.f,140.f/std::max(1.f,float(std::strlen(s))*.6f));DrawUiText(s,x,12,size,r,g,b,true);}
void CXEngine::EnableMotionBlur(){}void CXEngine::DisableMotionBlur(){}void CXEngine::RenderMotionBlur(float){}
void CXEngine::SetGameSpeed(float f){m_GameSpeed=std::max(0.f,f);}void NativeRumble(HANDLE,PadFeedback*){}
void CSound::Create(const char*name){auto p=path(name);auto i=sounds.find(p);if(i!=sounds.end()){sound=i->second;return;}sound=std::make_shared<SoundData>();sound->handle=snd_sfx_load(p.c_str());if(!sound->handle)printf("SFX unavailable: %s\n",p.c_str());sounds[p]=sound;}
void CSound::PlaySound(){if(!sound||!sound->handle||audioPaused||XEngine->m_SoundsPerFrame<=0)return;--XEngine->m_SoundsPerFrame;sfx_play_data_t p{};p.chn=-1;p.idx=sound->handle;p.vol=180;p.pan=128;p.loop=loop;p.freq=0;snd_sfx_play_ex(&p);}
void CSound::PlaySound(IDirectMusicSegmentState**channel){if(!channel)return;auto t=std::make_unique<IDirectMusicSegmentState>();
 if(sound&&sound->handle&&!audioPaused){sfx_play_data_t p{};int reserved=snd_sfx_chn_alloc();if(reserved>=0){p.chn=reserved;p.idx=sound->handle;p.vol=180;p.pan=128;p.loop=loop;p.freq=0;int n=snd_sfx_play_ex(&p);if(n>=0)t->id=uint64_t(reserved)+1;else snd_sfx_chn_free(reserved);}}
 *channel=t.get();tokens.push_back(std::move(t));}
void CXEngine::StopChannel(IDirectMusicSegmentState*t){if(t&&t->id){snd_sfx_stop(int(t->id-1));snd_sfx_chn_free(int(t->id-1));t->id=0;}}
void CXEngine::StopAllSounds(bool release){for(auto&t:tokens)StopChannel(t.get());snd_sfx_stop_all();if(release)tokens.clear();}
void CXEngine::SetAudioPaused(bool p){if(p==audioPaused)return;audioPaused=p;if(p){StopAllSounds(false);if(musicPlaying)snd_stream_stop(musicStream);}else if(musicPlaying&&!musicPaused)snd_stream_start(musicStream,22050,1);}
bool CXEngine::SelectMusic(int choice){if(choice!=0||musicStream<0)return false;if(!music)music=fopen("/cd/music.raw","rb");if(!music)return false;
 if(!musicPlaying){printf("MUSIC start\n");snd_stream_start(musicStream,22050,1);snd_stream_volume(musicStream,90);musicPlaying=true;printf("MUSIC ready\n");}return true;}
void CXEngine::PlayMusic(const char*){SelectMusic(0);}
void CXEngine::PauseMusic(){musicPaused=!musicPaused;if(musicPlaying){if(musicPaused)snd_stream_stop(musicStream);else snd_stream_start(musicStream,22050,1);}}
void CXEngine::SetMusicVolume(int v){if(musicStream>=0)snd_stream_volume(musicStream,std::clamp(v,0,1000)*90/1000);}
void CXEngine::PlayResultCue(bool winner){CSound s;s.Create(winner?"game/audio/winner-fanfare.wav":"game/audio/game-over.wav");s.PlaySound();}
void CXEngine::BeginResultsAudio(){StopAllSounds(false);audioPaused=false;PlayResultCue(false);}
const char*NativeArtSkin(){return nullptr;}bool NativeHasArtMode(int m){return m==0;}bool NativeSetArtMode(int m){return m==0;}const char*NativeArtLabel(int){return "ORIGINAL";}
bool NativeHasHDArt(){return false;}bool NativeSetHDArt(bool b){return !b;}int NativeMusicChoice(){return 0;}const char*NativeMusicLabel(int){return "ORIGINAL";}
float Sin(float x){return std::sin(x*.01745329252f);}float Cos(float x){return std::cos(x*.01745329252f);}float ATan2(float x,float y){return std::atan2(x,y)*57.2957795f;}
float fAbs(float x){return std::abs(x);}float Rand(){return float(rand())/(float(RAND_MAX)+1.f);}float Pow(float a,float b){return std::pow(a,b);}float Sqrt(float x){return std::sqrt(x);}
COLLISIONINFO CXEngine::Collide(float x1,float y1,float r1,float dx,float dy,float x2,float y2,float r2){
 COLLISIONINFO out;D3DXVECTOR2 d1{dx,dy},d2{x2-x1,y2-y1};float l1=Sqrt(dx*dx+dy*dy),l2=Sqrt(d2.x*d2.x+d2.y*d2.y);
 d1.x/=l1;d1.y/=l1;d2.x/=l2;d2.y/=l2;float a=(d1.x*d2.x+d1.y*d2.y)*l2,o=Sqrt(l2*l2-a*a);
 if(o<r1+r2&&x2>std::min(x1,x1+dx)-r1-r2&&x2<std::max(x1,x1+dx)+r1+r2&&y2>std::min(y1,y1+dy)-r1-r2&&y2<std::max(y1,y1+dy)+r1+r2){out.Collided=true;out.Pos={d1.x*(a-((r1+r2)-o)),d1.y*(a-((r1+r2)-o))};}
 else out.Pos={dx,dy};return out;
}
