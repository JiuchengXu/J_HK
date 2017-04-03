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
#define SPECIAL_KEY_TYPE		8
#define IP_CHANGE_TYPE			9


#define STOP_WORK				0xf
#define START_WORK				0xe
#define LCD_MSG					0xd

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

struct attacked_info {
	char characCode [10][4];
	char attachTime [10][8];
	char ifHeadShot[10][1];	
};

struct ClothesStatusData  {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char deviceType [1];
	char deviceSubType [1];
	char lifeLeft [3];
	char keySN [16];
#if 0
	char characCode [10][4];
	char attachTime [10][8];
	char ifHeadShot[10][1];
#else
	struct attacked_info atck_info;
#endif
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
#define LCD_PWR_INFO	2

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
	char killCount[2];
	char bekillCount[2];
	char myTeamKillCount[2];
	char myTeamBeKillCount[2];
	char headShootCount[2];
	char headBeShootCount[2];
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

struct sta_ip_change_pkg {
	char transMod [1];
	char packTye[1];
	char packageID [4];
	char KeySN[16];
};

void main_loop(void);
s8 get_actived_state(void);


#endif
