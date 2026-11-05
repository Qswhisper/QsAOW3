// Stable local identities for original lobby formats. Training modes carry no
// formal-match rewards; original team counts are retained in UI and history.
#pragma once
namespace local_battle_format {
inline bool free_for_all(int mode){return mode>=7&&mode<=10;}
inline bool control_points(int mode){return mode==5||mode==6;}
inline bool valid(int mode){return mode==1||mode==2||mode==4||control_points(mode)||free_for_all(mode);}
inline int seats(int mode){return free_for_all(mode)?mode-4:mode==4?6:mode==2?4:2;}
inline int teams(int mode){return free_for_all(mode)?seats(mode):2;}
inline int team(int mode,int side){return side>=1&&side<=seats(mode)?free_for_all(mode)?side-1:(side-1)%2:-1;}
inline int team_format(int mode){return free_for_all(mode)?mode==7?111:mode==8?1111:mode==9?11111:111111:mode==4?33:mode==2?22:11;}
inline int lobby_mode(int format,int teams){
    if(format>=6&&format<=9){int mode=format+1;return teams==team_format(mode)?mode:0;}
    if(format==2&&teams==11)return 5;if(format==3&&teams==11)return 6;
    return format==1&&teams==11?1:format==4&&teams==22?2:format==5&&teams==33?4:0;
}
// Returns the actual surviving seat, 0 for simultaneous extinction, -1 while
// multiple independent armies survive, -2 for invalid observations.
inline int ffa_winner(const int* buildings,const int* units,int count){
    if(!buildings||!units||count<3||count>6)return -2;int alive=0,winner=0;
    for(int i=0;i<count;++i){if(buildings[i]<0||units[i]<0)return -2;
        if(buildings[i]>0||units[i]>0){++alive;winner=i+1;}}
    return alive>1?-1:winner;
}
}
