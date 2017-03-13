#ifndef __NET_H__
#define __NET_H__

#define ACTIVE_REQUEST_TYPE		0
#define ACTIVE_RESPONSE_TYPE	1
#define GUN_STATUS_TYPE			2
#define CLOTHES_STATUS_TYPE		3
#define STATUS_RESPONSE_TYPE	4
#define HEART_BEAT_TYPE			5
#define MESSAGE_TYPE			6
#define MESSAGE_RESPONSE_TYPE	7


#define STOP_WORK				6
#define START_WORK				7
#define LCD_MSG					8

struct ActiveRequestData {
	char transMod [1];
	char packTye[1];
	char keySN[16];
	char packageID [4];
};

struct ActiveAskData  {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char characCode [4];
	char curTime [8];
};

struct WorkFlahgDipatch {
	char transMod [1];
	char packTye[1];
};

struct GunActiveAskData  {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char characCode [4];
};

struct LcdActiveAskData {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char rtc[8];	
};

struct ClothesStatusData  {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char deviceType [1];
	char deviceSubType [1];
	char lifeLeft [3];
	char keySN [16];
	char characCode [10][4];
	char attachTime [10][8];
	char ifHeadShot[10][1];
	char PowerLeft [2];
};

struct GunStatusData  {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char deviceType [1];
	char deviceSubType [1];
	char deviceSN [16];
	char bulletLeft [3];
	char keySN [16];
	char characCode [4];
	char PowerLeft [2];
};

#define LCD_GUN_BULLET	0
#define LCD_LIFE		1


struct LcdStatusData {
	char transMod [1];
	char packTye[1];
	char msg_type[2];
	char msg_value[4];
};

struct StatusRespData {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char errorNum [2];	
};

struct HeartBeat {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char deviceType [1];
	char deviceSubType [1];
	char deviceSN [16];
};

struct MsgPkg {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char msg_type[1];
};

struct MsgPkgResp {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char msg_type[1];
	char rx_ok[1];
};

struct sepcial_key {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char keySN[16];
	char AkeySN [16];
};

void main_loop(void);
s8 get_actived_state(void);


#endif
