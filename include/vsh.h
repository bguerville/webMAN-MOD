//int (*_cellGcmIoOffsetToAddress)(uint32_t, void**) = NULL;
int (*vshtask_notify)(int, const char *) = NULL;
int (*View_Find)(const char *) = NULL;
int (*plugin_GetInterface)(int,int) = NULL;

#ifdef SYS_BGM
uint32_t (*BgmPlaybackDisable)(int, void *) = NULL;
uint32_t (*BgmPlaybackEnable)(int, void *) = NULL;
#endif

int (*vshmain_is_ss_enabled)(void) = NULL;
int (*set_SSHT_)(int) = NULL;

int opd[2] = {0, 0};

static void * getNIDfunc(const char * vsh_module, uint32_t fnid, int32_t offset)
{
	// 0x10000 = ELF
	// 0x10080 = segment 2 start
	// 0x10200 = code start

	uint32_t table = (*(uint32_t*)0x1008C) + 0x984; // vsh table address

	while(((uint32_t)*(uint32_t*)table) != 0)
	{
		uint32_t* export_stru_ptr = (uint32_t*)*(uint32_t*)table; // ptr to export stub, size 2C, "sys_io" usually... Exports:0000000000635BC0 stru_635BC0:    ExportStub_s <0x1C00, 1, 9, 0x39, 0, 0x2000000, aSys_io, ExportFNIDTable_sys_io, ExportStubTable_sys_io>

		const char* lib_name_ptr =  (const char*)*(uint32_t*)((char*)export_stru_ptr + 0x10);

		if(strncmp(vsh_module, lib_name_ptr, strlen(lib_name_ptr))==0)
		{
			// we got the proper export struct
			uint32_t lib_fnid_ptr = *(uint32_t*)((char*)export_stru_ptr + 0x14);
			uint32_t lib_func_ptr = *(uint32_t*)((char*)export_stru_ptr + 0x18);
			uint16_t count = *(uint16_t*)((char*)export_stru_ptr + 6); // number of exports
			for(int i = 0; i < count; i++)
			{
				if(fnid == *(uint32_t*)((char*)lib_fnid_ptr + i*4))
				{
					// take address from OPD
					return (void**)*((uint32_t*)(lib_func_ptr) + i) + offset;
				}
			}
		}
		table += 4;
	}
	return 0;
}

static void show_msg(char* msg)
{
	if(!vshtask_notify)
		vshtask_notify = getNIDfunc("vshtask", 0xA02D46E7, 0);

	if(strlen(msg) > 200) msg[200] = NULL; // truncate on-screen message

	if(vshtask_notify)
		vshtask_notify(0, msg);
}

static int get_game_info(void)
{
	int h = View_Find("game_plugin");

	if(h)
	{
		game_interface = (game_plugin_interface *)plugin_GetInterface(h, 1);
		game_interface->gameInfo(_game_info);
	}

	return h;
}

#ifndef LITE_EDITION
static void enable_ingame_screenshot(void)
{
	vshmain_is_ss_enabled = getNIDfunc("vshmain", 0x981D7E9F, 0); //is screenshot enabled?

	if(vshmain_is_ss_enabled() == 0)
	{
		set_SSHT_ = (uint32_t*)&opd;
		memcpy(set_SSHT_, vshmain_is_ss_enabled, 8);
		opd[0] -= 0x2C; // Sub before vshmain_981D7E9F sets Screenshot Flag
		set_SSHT_(1);	// enable screenshot

		show_msg((char*)"Screenshot enabled");
		sys_timer_sleep(2);
	}
}
#endif

static void launch_disc(char *category, char *seg_name)
{
	u8 n;
	for(n = 0; n < 15; n++) {if(View_Find("explore_plugin") == 0) sys_timer_sleep(2); else break;}

	if(IS(seg_name, "seg_device")) waitfor("/dev_bdvd", 10); if(n) sys_timer_sleep(3);

	int view = View_Find("explore_plugin");

	if(view)
	{
		char explore_command[128]; // info: http://www.psdevwiki.com/ps3/explore_plugin

		// default category
		if(!category[0]) sprintf(category, "game");

		// default segment
		if(!seg_name[0]) sprintf(seg_name, "seg_device");

		if(!IS(seg_name, "seg_device") || isDir("/dev_bdvd"))
		{
			// use segment for media type
			if(IS(category, "game") && IS(seg_name, "seg_device"))
			{
				if(isDir("/dev_bdvd/PS3_GAME") || file_exists("/dev_bdvd/SYSTEM.CNF")) ; else
				if(isDir("/dev_bdvd/BDMV") )    {sprintf(category, "video"); sprintf(seg_name, "seg_bdmav_device");} else
				if(isDir("/dev_bdvd/VIDEO_TS")) {sprintf(category, "video"); sprintf(seg_name, "seg_dvdv_device" );} else
				if(isDir("/dev_bdvd/AVCHD"))    {sprintf(category, "video"); sprintf(seg_name, "seg_avchd_device");} else
				{return;}
			}

			explore_interface = (explore_plugin_interface *)plugin_GetInterface(view,1);
			explore_interface->ExecXMBcommand("close_all_list",0,0);
			sys_timer_usleep(200000);
			{sprintf(explore_command, "focus_category %s", category); explore_interface->ExecXMBcommand((char*)explore_command,0,0);}
			sys_timer_usleep(500000);
			explore_interface->ExecXMBcommand("close_all_list",0,0);
			sys_timer_usleep(200000);
			sprintf(explore_command, "focus_segment_index %s", seg_name);
			explore_interface->ExecXMBcommand((char*)explore_command,0,0);
			sys_timer_usleep(500000);
			explore_interface->ExecXMBcommand("exec_push",0,0);
		}
		else {BEEP3}
	}
}


/*
#include "vsh/xmb_plugin.h"

static void show_msg2(char* msg) // usage: show_msg2(L"text");
{
	if(View_Find("xmb_plugin") != 0)
	{
		xmb2_interface = (xmb_plugin_xmb2 *)plugin_GetInterface(View_Find("xmb_plugin"),'XMB2');
		xmb2_interface->showMsg(msg);
	}
}
*/
