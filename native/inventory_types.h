#pragma once
namespace inventory_types {
inline bool pre_mod_stock(int type){return type==4||type==7||type==8||type==9||type==10||type==11;}
inline bool stock(int type){return pre_mod_stock(type)||type==17;}
inline bool legacy_stock(int type){return type==4||type==8||type==9;}
inline bool reward(int type){return type==0||type==1||type==2||stock(type);}
}
