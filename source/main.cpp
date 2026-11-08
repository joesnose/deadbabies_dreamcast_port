#include "game.h"
#include "ui_style.h"
#include <exception>
#include <stdexcept>
#include <string>
KOS_INIT_FLAGS(INIT_DEFAULT);
extern void ResetNativeRound();
extern void ReleaseNativeRound();
extern int NativeSelfTest();

int main(int argc,char**argv){
 arch_set_exit_path(ARCH_EXIT_MENU);
 printf("DeadBabiesX Dreamcast original-art test build\n");
 if(argc>1&&std::string(argv[1])=="--self-test")return NativeSelfTest();
 try{
  XEngine->XInit();
  if(FILE*f=fopen("/cd/selftest.flag","rb")){fclose(f);if(NativeSelfTest())throw std::runtime_error("Native self-test failed");XEngine->StopAllSounds();}
  XEngine->PlayMusic(nullptr);
  enum Scene{Title,Playing,Paused};Scene scene=Title;
  bool round=false,armed=false;int selection=0,players=1,held=0;Difficulty difficulty=Difficulty::Normal;
  auto enter=[&](Scene s){scene=s;selection=0;held=0;armed=false;XEngine->ClearInputEdges();XEngine->SetAudioPaused(s==Paused);};
  while(XEngine->Running()){
   bool confirm=false,back=false,anyHeld=false,start=false;int direction=0,horizontal=0;
   for(auto&p:XEngine->GamePad){if(!p.hDevice)continue;
    confirm|=p.bPressedAnalogButtons[0]||(p.wPressedButtons&16);back|=p.bPressedAnalogButtons[1];start|=(p.wPressedButtons&16)!=0;
    anyHeld|=(p.wButtons&63)||p.bAnalogButtons[0]||p.bAnalogButtons[1];
    if((p.wButtons&1)||p.fY1>.55f)direction=-1;if((p.wButtons&2)||p.fY1<-.55f)direction=1;
    if(p.wPressedButtons&4)horizontal=-1;if(p.wPressedButtons&8)horizontal=1;
   }
   if(scene==Playing){
    if(!Game.m_ResultsReady&&(start||back)){
     for(auto*p:Game.m_PlayerList)p->m_Weapon->Stop();enter(Paused);continue;
    }
    if(!Game.Update()){XEngine->StopAllSounds(false);enter(Title);}continue;
   }
   if(!armed){if(!anyHeld&&!direction)armed=true;confirm=false;horizontal=0;}
   int count=scene==Title?4:2;
   if(armed&&direction&&held!=direction)selection=(selection+direction+count)%count;held=direction;
   if(scene==Paused&&back){enter(Playing);continue;}
   if(scene==Title&&selection==1&&armed&&(horizontal||confirm)){players=(players-1+(horizontal?horizontal:1)+4)%4+1;confirm=false;}
   if(scene==Title&&selection==2&&armed&&(horizontal||confirm)){difficulty=CycleDifficulty(difficulty,horizontal?horizontal:1);confirm=false;}
   if(armed&&confirm){
    if(scene==Paused){if(selection==0)enter(Playing);else{XEngine->StopAllSounds(false);enter(Title);}continue;}
    if(selection==3){XEngine->RequestExit();continue;}
    if(selection==0){SetStartingPlayers(players);SetGameDifficulty(difficulty);XEngine->m_Ticks=0;XEngine->SetGameSpeed(1);
     XEngine->DrawUiText("LOADING ORIGINAL ART...",320,120,22,255,230,200,true);XEngine->XUpdate();
     if(round)ResetNativeRound();else{XEngine->StopAllSounds();Game.Init();round=true;}
     printf("ROUND start players=%d difficulty=%s PVR-free=%u\n",players,DifficultyLabel(difficulty),unsigned(pvr_mem_available()));enter(Playing);continue;}
   }
   if(scene==Paused)Game.Refresh();else{
    XEngine->SetCameraPos(0,0);XEngine->SetSpriteMode(SM_ALPHA_BLEND);XEngine->SetMaterial(1,1,1,1);
    XEngine->DrawSprite(XEngine->LoadTexture("d:\\data\\backgrounds\\train_bg3.png"),320,240,680,680,0);
   }
   XEngine->SetCameraPos(0,0);XEngine->SetSpriteMode(SM_ALPHA_BLEND);XEngine->SetMaterial(1,1,1,1);
   XEngine->FillRect(-20,-20,680,520,.02f,.02f,.02f,.72f);NativeUiPanel(105,150,430,scene==Title?225:140);
   XEngine->DrawUiText("DEAD BABIES X",320,55,32,245,225,195,true);
   XEngine->DrawUiText(scene==Paused?"PAUSED":"DREAMCAST - ORIGINAL ART",320,105,14,215,150,100,true);
   char pLabel[40],dLabel[60];snprintf(pLabel,sizeof(pLabel),"PLAYERS: < %d >",players);snprintf(dLabel,sizeof(dLabel),"DIFFICULTY: < %s >",DifficultyLabel(difficulty));
   const char*items[]={"START GAME",pLabel,dLabel,"EXIT TO BIOS"};const char*pause[]={"RESUME","MAIN MENU"};
   for(int i=0;i<count;++i){float y=175+i*43;if(i==selection)XEngine->FillRect(120,y-4,400,36,.3f,.08f,.04f,1);
    XEngine->DrawUiText(scene==Title?items[i]:pause[i],320,y,18,i==selection?255:170,i==selection?230:160,i==selection?200:140,true);}
   XEngine->DrawUiText("STICK: MOVE   D-PAD: AIM   R/L: FIRE",320,407,12,205,195,175,true);
   XEngine->DrawUiText("A: PICKUP / CONFIRM   START: PAUSE   B: BACK",320,435,10,185,175,155,true);
   XEngine->XUpdate();
  }
  if(round)ReleaseNativeRound();XEngine->Shutdown();return 0;
 }catch(const std::exception&e){printf("FATAL: %s\n",e.what());fflush(stdout);return 1;}
}
