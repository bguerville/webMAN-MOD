static void cpu_rsx_stats(char *buffer, char *templn, char *param, u8 is_ps3_http)
{
	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	u32 t1=0, t2=0, t1f, t2f;
	get_temperature(0, &t1); t1>>=24; // 3E030000 -> 3E.03°C -> 62.(03/256)°C
	get_temperature(1, &t2); t2>>=24;

	t1f=(1.8f*(float)t1+32.f);
	t2f=(1.8f*(float)t2+32.f);

	_meminfo meminfo;
	{system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}

	if((webman_config->fanc == 0) && (get_fan_policy_offset>0))
	{
		u8 st, mode, unknown;
		backup[2]=peekq(get_fan_policy_offset);
		lv2poke32(get_fan_policy_offset, 0x38600001); // sys 409 get_fan_policy
		sys_sm_get_fan_policy(0, &st, &mode, &fan_speed, &unknown);
		pokeq(get_fan_policy_offset, backup[2]);

		if(strstr(param, "?u")) enable_fan_control(3, templn);
	}

	uint64_t eid0_idps[2], buffr[0x40], start_sector;
	uint32_t read;
	sys_device_handle_t source;
	if(sys_storage_open(0x100000000000004ULL, 0, &source, 0)!=0)
	{
		start_sector = 0x204;
		sys_storage_close(source);
		sys_storage_open(0x100000000000001ULL, 0, &source, 0);
	}
	else start_sector = 0x178;
	sys_storage_read(source, 0, start_sector, 1, buffr, &read, 0);
	sys_storage_close(source);

	eid0_idps[0]=buffr[0x0E];
	eid0_idps[1]=buffr[0x0F];

	get_idps_psid();

	uint32_t blockSize;
	uint64_t freeSize;
	cellFsGetFreeSize((char*)"/dev_hdd0", &blockSize, &freeSize);

	sprintf(templn, " [<a href=\"/shutdown.ps3\">%s</a>] [<a href=\"/restart.ps3\">%s</a>]", STR_SHUTDOWN, STR_RESTART ); strcat(buffer, templn);

	if(View_Find("game_plugin"))
	{
		get_game_info();

		if(strlen(_game_TitleID)==9)
		{
#ifdef GET_KLICENSEE
			strcat(buffer, " [<a href=\"/klic.ps3\">KLIC</a>]");
#endif
#ifdef SYS_BGM
			strcat(buffer, " [<a href=\"/sysbgm.ps3\">BGM</a>]");
#endif
#ifdef VIDEO_REC
			strcat(buffer, " [<a href=\"/videorec.ps3\">REC</a>]");
#endif
			char path[MAX_PATH_LEN], version[8] = "01.00", *app_ver = version;

			sprintf(templn, "<hr><H2><a href=\"https://a0.ww.np.dl.playstation.net/tpl/np/%s/%s-ver.xml\" target=\"_blank\">%s</a>", _game_TitleID, _game_TitleID, _game_TitleID); strcat(buffer, templn);

			sprintf(path, "%s%s/PARAM.SFO", HDD0_GAME_DIR, _game_TitleID);
			if(file_exists(path)==false) sprintf(path, "/dev_bdvd/PS3_GAME/PARAM.SFO");

			getTitleID(path, app_ver, GET_VERSION); if(app_ver[0] == '0') app_ver[0]='v'; if(strstr(_game_Title, app_ver)) app_ver[0] = NULL;

			sprintf(templn, " <a href=\"%s%s\">%s %s</a> &nbsp; ", search_url, _game_Title, _game_Title, app_ver); strcat(buffer, templn);

			sprintf(path, "%s%s", HDD0_GAME_DIR, _game_TitleID);
			if(file_exists(path)==false) sprintf(path, "/dev_bdvd/PS3_GAME");

			sprintf(templn, "<a href=\"%s\"><img src=\"%s/ICON0.PNG\" border=0 %s></a>", path, path, "height=\"60\" style=\"position:absolute;top:65px;\""); strcat(buffer, templn);

			strcat(buffer, "</H2>");
		}
	}

#ifdef COPY_PS3
	if(copy_in_progress)
	{
		sprintf(templn, "<hr><font size=2><a href=\"/copy.ps3$abort\">&#9746 %s</a> %s (%i %s)</font>", STR_COPYING, current_file, copied_count, STR_FILES); strcat(buffer, templn);
	}
	else if(fix_in_progress)
	{
		sprintf(templn, "<hr><font size=2><a href=\"/fixgame.ps3$abort\">&#9746 %s</a> %s</font>", STR_FIXING, current_file); strcat(buffer, templn);
	}
#endif

	if(strstr(param, "?"))
	{
		char *pos = strstr(param, "fan=");
		if(pos) {read = get_valuen(param, "fan=", 0, 99); max_temp = 0; if(!read) enable_fan_control(0, templn); else {webman_config->manu = read; if(webman_config->fanc == 0) enable_fan_control(1, templn);}}
		else {pos = strstr(param, "max=");
		if(pos) {max_temp = get_valuen(param, "max=", 40, MAX_TEMPERATURE);}
		else
		if(strstr(param, "?m")) {if((max_temp && !strstr(param, "dyn")) || strstr(param, "man")) max_temp=0; else {max_temp=webman_config->temp1;} if(webman_config->fanc == 0) enable_fan_control(1, templn);}}

		if(max_temp) //auto mode
		{
			if(strstr(param, "?u")) max_temp++;
			if(strstr(param, "?d")) max_temp--;
			webman_config->temp1=RANGE(max_temp, 40, MAX_TEMPERATURE); // dynamic fan max temperature in °C
			webman_config->temp0=FAN_AUTO;

			fan_ps2_mode=false;
		}
		else
		{
			if(strstr(param, "?u")) webman_config->manu++;
			if(strstr(param, "?d")) webman_config->manu--;
			webman_config->manu=RANGE(webman_config->manu, 20, 95); //%

			reset_fan_mode();
		}
		save_settings();
	}

	{ PS3MAPI_ENABLE_ACCESS_SYSCALL8 }

	char max_temp1[128], max_temp2[64]; max_temp2[0] = NULL;

	if(fan_ps2_mode)
	{
		sprintf(max_temp1, " (PS2 Mode: %i%%)", webman_config->ps2temp);
	}
	else if((webman_config->fanc == 0) || (!webman_config->temp0 && !max_temp))
		sprintf(max_temp1, " <small>[%s %s]</small>", STR_FANCTRL3, STR_DISABLED);
	else if(max_temp)
	{
		sprintf(max_temp1, " (MAX: %i°C)", max_temp);
		sprintf(max_temp2, " (MAX: %i°F)", (int)(1.8f*(float)max_temp+32.f));
	}
	else
		sprintf(max_temp1, " <small>[FAN: %i%% %s]</small>", webman_config->manu, STR_MANUAL);

	sprintf( templn, "<hr><font size=\"42px\"><b><a class=\"s\" href=\"/cpursx.ps3?up\">"
											"CPU: %i°C%s<br>"
											"RSX: %i°C</a><hr>"
											"<a class=\"s\" href=\"/cpursx.ps3?dn\">"
											"CPU: %i°F%s<br>"
											"RSX: %i°F</a><hr>"
											"<a class=\"s\" href=\"/games.ps3\">"
											"MEM: %'d KB<br>"
											"HDD: %'d %s</a><hr>"
											"<a class=\"s\" href=\"/cpursx.ps3?mode\">"
											"FAN SPEED: %i%% (0x%X)</a><br>",
					t1, max_temp1, t2,
					t1f, max_temp2, t2f,
					(meminfo.avail>>10), (int)((blockSize*freeSize)>>20), STR_MBFREE,
					(int)((int)fan_speed*100)/255, fan_speed); strcat(buffer, templn);

	if( !max_temp && !is_ps3_http)
		sprintf( templn, "<input type=\"range\" value=\"%i\" min=\"%i\" max=\"95\" style=\"width:600px\" onchange=\"window.location='/cpursx.ps3?fan='+this.value\"><hr>", webman_config->manu, DEFAULT_MIN_FANSPEED);
	else
		sprintf( templn, "<hr>");

	strcat(buffer, templn);

	CellRtcTick pTick; cellRtcGetCurrentTick(&pTick); u32 dd, hh, mm, ss;

	// detect aprox. time when a game is launched
	if(View_Find("game_plugin")==0) gTick=rTick; else if(gTick.tick==rTick.tick) cellRtcGetCurrentTick(&gTick);

	////// play time //////
	if(gTick.tick>rTick.tick)
	{
		ss = (u32)((pTick.tick-gTick.tick)/1000000);
		dd = (u32)(ss / 86400); ss = ss % 86400; hh = (u32)(ss / 3600); ss = ss % 3600; mm = (u32)(ss / 60); ss = ss % 60;
		if(dd<100) {sprintf( templn, "<label title=\"Play\">&#9737;</label> %id %02d:%02d:%02d<br>", dd, hh, mm, ss); strcat(buffer, templn);}
	}
	///////////////////////

	//// startup time /////
	ss = (u32)((pTick.tick-rTick.tick)/1000000);
	dd = (u32)(ss / 86400); ss = ss % 86400; hh = (u32)(ss / 3600); ss = ss % 3600; mm = (u32)(ss / 60); ss = ss % 60;
	sprintf( templn, "<label title=\"Startup\">&#8986;</label> %id %02d:%02d:%02d", dd, hh, mm, ss); strcat(buffer, templn);
	///////////////////////

	if(file_exists("/dev_bdvd") && file_exists(WMTMP "/last_game.txt"))
	{
		int fd=0;

		if(cellFsOpen(WMTMP "/last_game.txt", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			cellFsRead(fd, (void *)param, MAX_PATH_LEN, NULL);
			cellFsClose(fd);

			if(strlen(param) > 10) {sprintf( templn, "<hr><font size=\"3\"><a href=\"%s\">%s</a> -> ", View_Find("game_plugin") ? "/dev_bdvd" : "/play.ps3", "/dev_bdvd"); strcat(buffer, templn); add_breadcrumb_trail(buffer, param); strcat(buffer, "</font>");}
		}
	}

	// Get mac address [0xD-0x12]
	u8 mac_address[0x13];
	{system_call_3(SYS_NET_EURUS_POST_COMMAND, CMD_GET_MAC_ADDRESS, (u64)(u32)mac_address, 0x13);}

#ifdef COBRA_ONLY
	if(!is_mamba && !syscalls_removed) {system_call_1(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_GET_MAMBA); is_mamba = ((int)p1 ==0x666);}

	if(!cobra_version) sys_get_version2(&cobra_version);

	char cobra_ver[8];
	if((cobra_version & 0x0F) == 0)
		sprintf(cobra_ver, "%X.%X", cobra_version>>8, (cobra_version & 0xFF) >> 4);
	else
		sprintf(cobra_ver, "%X.%02X", cobra_version>>8, (cobra_version & 0xFF));

 #if defined(DECR_SUPPORT)
	sprintf(param, "%s %s: %s", (dex_mode == 1) ? "DECR" : dex_mode ? "DEX" : "CEX", is_mamba ? "Mamba" : "Cobra", cobra_ver);
 #elif defined(DEX_SUPPORT)
	sprintf(param, "%s %s: %s", dex_mode ? "DEX" : "CEX", is_mamba ? "Mamba" : "Cobra", cobra_ver);
 #else
	sprintf(param, "%s %s: %s", "CEX", is_mamba ? "Mamba" : "Cobra", cobra_ver);
 #endif
#elif DEX_SUPPORT
	sprintf(param, "%s", (dex_mode == 1) ? "DECR" : dex_mode ? "DEX" : "CEX");
#else
	sprintf(param, "CEX");
#endif

	//net_info info;
	//memset(&info, 0, sizeof(net_info));
	//xsetting_F48C0548()->sub_44A47C(&info); //info.ipAddress

	net_info info;
	memset(&info, 0, sizeof(net_info));
	xsetting_F48C0548()->sub_44A47C(&info);

	char net_type[8] = "";
	if (info.device == 0) strcpy(net_type, "LAN"); else
	if (info.device == 1) strcpy(net_type, "WLAN");

	char ip[ip_size];
	netctl_main_9A528B81(ip_size, ip);

	sprintf( templn, "<hr></font><h2><a class=\"s\" href=\"/setup.ps3\">"
						"Firmware : %i.%02i %s<br>"
						"%s<br>"
						"PSID LV2 : %016llX%016llX<hr>"
						"IDPS EID0: %016llX%016llX<br>"
						"IDPS LV2 : %016llX%016llX<br>"
						"MAC Addr : %02X:%02X:%02X:%02X:%02X:%02X - %s %s</h2></a></b>",
					(int)c_firmware, ((u32)(c_firmware * 1000.0f) % 1000) / 10, param,
					(syscalls_removed) ? STR_CFWSYSALRD : "",
					PSID[0], PSID[1],
					eid0_idps[0], eid0_idps[1],
					IDPS[0], IDPS[1],
					mac_address[13], mac_address[14], mac_address[15], mac_address[16], mac_address[17], mac_address[18], ip, net_type); strcat(buffer, templn);

	/////////////////////////////
#ifdef COPY_PS3
	if(copy_in_progress)
	{
		sprintf( templn, "<hr>%s %s (%i %s)", STR_COPYING, current_file, copied_count, STR_FILES); strcat(buffer, templn);
	}
	else
	if(fix_in_progress)
	{
		strcat(buffer, "<hr>"); sprintf( templn, "%s %s", STR_FIXING, current_file); strcat(buffer, templn);
	}
#endif
	/////////////////////////////

	strcat(buffer,  HTML_BLU_SEPARATOR
					"webMAN - Simple Web Server" EDITION "<br>");

	{ PS3MAPI_DISABLE_ACCESS_SYSCALL8 }
}

static void get_cpursx(char *cpursx)
{
	u32 t1=0, t2=0;
	get_temperature(0, &t1); t1>>=24; // 3E030000 -> 3E.03°C -> 62.(03/256)°C
	get_temperature(1, &t2); t2>>=24;

	sprintf(cpursx, "CPU: %i°C | RSX: %i°C", t1, t2);
}