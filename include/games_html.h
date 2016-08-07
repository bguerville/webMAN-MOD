#define SUFIX(a)	((a==1)? "_1" :(a==2)? "_2" :(a==3)? "_3" :(a==4)?"_4":"")
#define SUFIX2(a)	((a==1)?" (1)":(a==2)?" (2)":(a==3)?" (3)":(a==4)?" (4)":"")
#define SUFIX3(a)	((a==1)?" (1).ntfs[":(a==2)?" (2).ntfs[":(a==3)?" (3).ntfs[":(a==4)?" (4).ntfs[":"")

#define IS_ISO_FOLDER ((f1>1) && (f1<10))
#define IS_PS3_TYPE   ((f1<3) || (f1>=10))
#define IS_VID_FOLDER ((f1==3) || (f1==4))

#define IS_JB_FOLDER  (f1<2)
#define IS_PS3_FOLDER (f1==2)
#define IS_BLU_FOLDER (f1==3)
#define IS_DVD_FOLDER (f1==4)
#define IS_PS2_FOLDER (f1==5)
#define IS_PSX_FOLDER ((f1==6) || (f1==7))
#define IS_PSP_FOLDER ((f1==8) || (f1==9))

#define PS3 (1<<0)
#define PS2 (1<<1)
#define PS1 (1<<2)
#define PSP (1<<3)
#define BLU (1<<4)
#define DVD (1<<5)

#define PS3_ '3'

#define HTML_KEY_LEN  6

static void get_name(char *name, char *filename, u8 cache)
{
	int pos = 0;
	if(cache) {pos=strlen(filename); while(pos>0 && filename[pos-1]!='/') pos--;}
	if(cache==2) cache=0;

	if(cache)
		sprintf(name, "%s/%s", WMTMP, filename+pos);
	else
		sprintf(name, "%s", filename+pos);

	int flen=strlen(name);
	if(flen>2 && name[flen-2]=='.' ) {name[flen-2] = NULL; flen-=2;}
	if(flen>4 && name[flen-4]=='.' )  name[flen-4] = NULL;
	else
	if(strstr(filename+pos, ".ntfs["))
	{
		while(name[flen]!='.') flen--; name[flen] = NULL;
		if(flen>4 && name[flen-4]=='.' && (strcasestr(ISO_EXTENSIONS, &name[flen-4]))) name[flen-4] = NULL; else
		if(!extcmp(name, ".BIN.ENC", 8)) name[flen-8] = NULL;
	}
	if(cache) return;

	if(name[4] == '_' && name[8] == '.' && (name[0] == 'B' || name[0] == 'N' || name[0] == 'S' || name[0] == 'U') && ISDIGIT(name[9]) && ISDIGIT(name[10])) strcpy(&name[0], &name[12]);
	if(name[9]== '-' && name[10]=='[') {strcpy(&name[0], &name[11]); name[strlen(name)-1] = NULL;}
	if(name[10]=='-' && name[11]=='[') {strcpy(&name[0], &name[12]); name[strlen(name)-1] = NULL;}
	if(!webman_config->tid && strstr(name, " [")) *strstr(name, " [") = NULL;
}

static bool get_cover(char *icon, char *titleid)
{
	if(!titleid[0]) return false;

	const char ext[4][4] = {"JPG", "PNG", "jpg", "png"};

#ifndef ENGLISH_ONLY
	if(covers_exist[0])
	{
		for(u8 e = 0; e < 4; e++)
		{
			sprintf(icon, "%s/%s.%s", COVERS_PATH, titleid, ext[e]);
			if(file_exists(icon)) return true;
		}
	}
#endif

	if(webman_config->nocov<2)
	{
		const char cpath[5][32] = {MM_ROOT_STD, MM_ROOT_STL, MM_ROOT_SSTL, "/dev_hdd0/GAMES", "/dev_hdd0/GAMEZ"};

		if(covers_exist[1] && titleid[0]=='S')
		{
			for(u8 p = 0; p < 3; p++)
				for(u8 e = 0; e < 4; e++)
				{
					sprintf(icon, "%s/covers_retro/psx/%c%c%c%c_%c%c%c.%c%c_COV.%s", cpath[p],
							titleid[0], titleid[1], titleid[2], titleid[3],
							titleid[4], titleid[5], titleid[6], titleid[7], titleid[8], ext[e]);

					if(file_exists(icon)) return true;
				}
		}

		for(u8 p = 1; p < 6; p++)
			if(covers_exist[p])
			{

				for(u8 e = 0; e < 4; e++)
				{
					sprintf(icon, "%s/covers/%s.%s", cpath[p - 1], titleid, ext[e]);
					if(file_exists(icon)) return true;
				}
			}

		if(covers_exist[6])
		{
			for(u8 e = 0; e < 4; e++)
			{
				sprintf(icon, "%s/%s.%s", WMTMP, titleid, ext[e]);
				if(file_exists(icon)) return true;
			}
		}
	}

#ifndef ENGLISH_ONLY
	if(use_custom_icon_path)
	{
		if(use_icon_region) sprintf(icon, COVERS_PATH,  (titleid[2] == 'U') ? "US" :
														(titleid[2] == 'J') ? "JA" : "EN", titleid);
		else
							sprintf(icon, COVERS_PATH, titleid);
		return true;
	}
#endif

	icon[0] = NULL;
	return false;
}

static void get_iso_icon(char *icon, char *param, char *file, int isdir, int ns, int abort_connection)
{
	//this function is called only from get_default_icon

	int flen;

	if(!extcmp(file, ".BIN.ENC", 8))
	{
		sprintf(icon, "%s/%s.png", param, file);
		if(file_exists(icon)) return;
		sprintf(icon, "%s/%s.PNG", param, file);
		if(file_exists(icon)) return;

		flen=strlen(icon)-12;
		if(flen>0 && icon[flen]=='.')
		{
			icon[flen] = NULL; strcat(icon, ".png");
			if(file_exists(icon)) return;
			icon[flen] = NULL; strcat(icon, ".PNG");
			if(file_exists(icon)) return;
		}
	}

	const char ext[4][5] = {".jpg", ".png", ".PNG", ".JPG"};

	if(isdir || ns>=0)
		{get_name(icon, file, 1); strcat(icon, ".PNG");} //wmtmp
	else
	{
		sprintf(icon, "%s/%s", param, file);
		flen=strlen(icon);

		if(strstr(file, ".ntfs["))
		{
			while(icon[flen]!='.') flen--; icon[flen] = NULL;
		}

		if(flen>2 && icon[flen-2]=='.' ) {flen-=2; icon[flen] = NULL;} // remove file extension (split iso)
		if(flen>4 && icon[flen-4]=='.' ) {flen-=4; icon[flen] = NULL;} // remove file extension

		//file name + ext
		for(u8 e = 0; e < 4; e++)
		{
			icon[flen] = NULL; // remove file extension
			strcat(icon, ext[e]);
			if(file_exists(icon)) return;
		}
	}

	//copy remote file
	if(file_exists(icon)==false)
	{
#ifdef COBRA_ONLY
		if(ns<0) {icon[0] = NULL; return;}

		char tempstr[_4KB_];

		if(isdir)
		{
			if(webman_config->nocov>1) return; // no icon0
			sprintf(tempstr, "%s/%s/PS3_GAME/ICON0.PNG", param, file);
		}
		else
		{
			get_name(icon, file, 0);
			sprintf(tempstr, "%s/%s.jpg", param, icon);

			get_name(icon, file, 1); strcat(icon, ".jpg"); //wmtmp
			if(file_exists(icon)) return;
		}

#ifndef LITE_EDITION
		copy_net_file(icon, tempstr, ns, COPY_WHOLE_FILE);
		if(file_exists(icon)) return;

		for(u8 e = 1; e < 4; e++)
		{
			icon[strlen(icon)-4] = NULL; strcat(icon, ext[e]);
			if(file_exists(icon)) return;

			tempstr[strlen(tempstr)-4] = NULL; strcat(tempstr, ext[e]);

			//Copy remote icon locally
			copy_net_file(icon, tempstr, ns, COPY_WHOLE_FILE);
			if(file_exists(icon)) return;
		}

#endif //#ifndef LITE_EDITION

#endif //#ifdef COBRA_ONLY
		icon[0] = NULL;
	}
}

static bool get_cover_from_name(char *icon, char *name, char *titleid)
{
	if(icon[0]!=0 && file_exists(icon)) return true;

	// get cover from titleid in PARAM.SFO
	if(get_cover(icon, titleid)) return true;

	// get titleid from file name
	if(titleid[0]==0 && (strstr(name, "-[") || strstr(name, " [B") || strstr(name, " [N") || strstr(name, " [S")))
	{
		if(strstr(name, "-["))
			{char *p=name; while(*p && p[9]!='-' && p[10]!='[' && p[9]!=0) p++; strncpy(titleid, p, 10);}
		else if(strstr(name, " [B"))
			strncpy(titleid, strstr(name, " [B") + 2, 10); //BCES/BLES/BCUS/BLUS/etc.
		else if(strstr(name, " [N"))
			strncpy(titleid, strstr(name, " [N") + 2, 10); //NP*
		else
			strncpy(titleid, strstr(name, " [S") + 2, 10); //SLES/SCES/SCUS/SLUS/etc.
	}
	else if(titleid[0]==0 && name[4] == '_' && name[8] == '.' && (name[0] == 'B' || name[0] == 'N' || name[0] == 'S' || name[0] == 'U') && ISDIGIT(name[9]) && ISDIGIT(name[10]))
	{
		sprintf(titleid, "%c%c%c%c%c%c%c%c%c", name[0], name[1], name[2], name[3], name[5], name[6], name[7], name[9], name[10]); //SCUS_999.99.filename.iso
	}

	if(titleid[4]=='-') strncpy(&titleid[4], &titleid[5], 5); titleid[9] = NULL;

	// get cover from titleid in file name
	if(get_cover(icon, titleid)) return true;

	return false;
}

static void get_default_icon(char *icon, char *param, char *file, int isdir, char *titleid, int ns, int abort_connection)
{
	if(webman_config->nocov>1) {if(get_cover_from_name(icon, file, titleid)) return; goto no_icon0;}

	// continue using cover or default icon0.png
	if(icon[0]>0 && file_exists(icon))
	{
		if(!extcasecmp(icon, ".png", 4) || !extcasecmp(icon, ".jpg", 4)) return;
		icon[0] = NULL;
	}

	if((webman_config->nocov==0) && get_cover_from_name(icon, file, titleid)) return; // show mm cover

	// get icon from folder && copy remote icon
	get_iso_icon(icon, param, file, isdir, ns, abort_connection);

	if(icon[0]>0 && file_exists(icon)) return;

	//use the cached PNG from wmtmp if available
	get_name(icon, file, 1);
	strcat(icon, ".PNG");

no_icon0:
	if(icon[0]>0 && file_exists(icon)) return;

	if((webman_config->nocov==1) && get_cover_from_name(icon, file, titleid)) return; // show mm cover as last option (if it's disabled)

	//show the default icon by type
	if(strstr(param, "/PS2ISO") || !extcmp(param, ".BIN.ENC", 8))
		strcpy(icon, wm_icons[7]);
	else if(strstr(param, "/PSX") || !extcmp(file, ".ntfs[PSXISO]", 13))
		strcpy(icon, wm_icons[6]);
	else if(strstr(param, "/PSPISO") || strstr(param, "/ISO/"))
		strcpy(icon, wm_icons[8]);
	else if(strstr(param, "/DVDISO") || !extcmp(file, ".ntfs[DVDISO]", 13))
		strcpy(icon, wm_icons[9]);
	else //if(strstr(param, "/BDISO") || !extcmp(file, ".ntfs[BDISO]", 12) || || !extcmp(file, ".ntfs[BDFILE]", 13))
		strcpy(icon, wm_icons[5]);
}

static int get_title_and_id_from_sfo(char *templn, char *tempID, char *entry_name, char *icon, char *data, u8 f0)
{
	int fdw, ret;

	ret = cellFsOpen(templn, CELL_FS_O_RDONLY, &fdw, NULL, 0); // get titleID & title from PARAM.SFO

	if(ret == CELL_FS_SUCCEEDED)
	{
		uint64_t msiz = 0;
		cellFsLseek(fdw, 0, CELL_FS_SEEK_SET, &msiz);
		cellFsRead(fdw, (void *)data, _4KB_, &msiz);
		cellFsClose(fdw);

		if(msiz>256)
		{
			unsigned char *mem = (u8*)data;
			parse_param_sfo(mem, tempID, templn);
			if(webman_config->nocov==0) get_cover(icon, tempID);
		}
	}

	if(webman_config->use_filename)
	{
		if(!icon[0] && !tempID[0]) get_cover_from_name(icon, templn, tempID); // get titleID from name

		ret=~CELL_FS_SUCCEEDED;
	}

	if(ret != CELL_FS_SUCCEEDED)
	{
		get_name(templn, entry_name, 2); if(f0!=NTFS) utf8enc(data, templn, 1); //use file name as title
	}

	return ( (ret == CELL_FS_SUCCEEDED) ? 0 : FAILED );
}

static void get_folder_icon(char *icon, u8 f1, u8 is_iso, char *param, char *entry_name, char *tempID)
{
	if(webman_config->nocov<2)
	{
		if(!is_iso && IS_JB_FOLDER && (icon[0]==0 || webman_config->nocov)) sprintf(icon, "%s/%s/PS3_GAME/ICON0.PNG", param, entry_name);

		get_cover_from_name(icon, entry_name, tempID);

		if(icon[0]==0)
		{
			sprintf(icon, "%s/%s", param, entry_name);
			int flen = strlen(icon);
#ifdef COBRA_ONLY
			if(flen > 13 && (!extcmp(icon, ".ntfs[PS3ISO]", 13) || !extcmp(icon, ".ntfs[DVDISO]", 13) || !extcmp(icon, ".ntfs[PSXISO]", 13) || !extcmp(icon, ".ntfs[BDFILE]", 13))) {flen -= 13; icon[flen] = NULL;} else
			if(flen > 12 &&  !extcmp(icon, ".ntfs[BDISO]" , 12)) {flen -= 12; icon[flen] = NULL;}
#endif
			if(flen > 4 && icon[flen-4]=='.')
			{
				icon[flen-3]='p'; icon[flen-2]='n'; icon[flen-1]='g';
				if(file_exists(icon)==false)
				{
					icon[flen-3]='P'; icon[flen-2]='N'; icon[flen-1]='G';
				}
			}
			else
			if(flen > 5 && icon[flen-2]=='.')
			{
				icon[flen-5]='p'; icon[flen-4]='n'; icon[flen-3]='g'; flen -= 2; icon[flen] = NULL;
			}

			if(file_exists(icon)) return;

			icon[flen-3]='j'; icon[flen-2]='p'; icon[flen-1]='g';
			if(file_exists(icon)) return;

			get_name(icon, entry_name, 1); strcat(icon, ".jpg");
			if(file_exists(icon)) return;

			get_name(icon, entry_name, 1); strcat(icon, ".png");
			if(file_exists(icon)) return;

			get_name(icon, entry_name, 1); strcat(icon, ".PNG");
			if(file_exists(icon)) return;

			get_name(icon, entry_name, 1); strcat(icon, ".JPG");
			if(file_exists(icon)==false) icon[0] = NULL;
		}
	}
}

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
static int add_net_game(int ns, netiso_read_dir_result_data *data, int v3_entry, char *neth, char *param, char *templn, char *tempstr, char *enc_dir_name, char *icon, char *tempID, u8 f1, u8 is_html)
{
	int abort_connection=0, is_directory=0; int64_t file_size; u64 mtime, ctime, atime;

	if(!data[v3_entry].is_directory)
	{
		int flen = strlen(data[v3_entry].name)-4;
		if(flen<0) return FAILED;
		if(!strcasestr(".iso.0|.img|.mdf|.bin", data[v3_entry].name + flen)) return FAILED;
	}
	else
	{
		if(data[v3_entry].name[0]=='.') return FAILED;
		//if(!strstr(param, "/GAME")) return FAILED;
	}

	icon[0] = tempID[0] = NULL;


	if(IS_PS3_TYPE) //PS3 games only (0="GAMES", 1="GAMEZ", 2="PS3ISO", 10="video")
	{
		if(data[v3_entry].is_directory)
			sprintf(templn, WMTMP "/%s.SFO", data[v3_entry].name);
		else
			{get_name(templn, data[v3_entry].name, 1); strcat(templn, ".SFO\0");}

		if(file_exists(templn)==false)
		{
			sprintf(enc_dir_name, "%s/%s/PS3_GAME/PARAM.SFO", param, data[v3_entry].name);
			copy_net_file(templn, enc_dir_name, ns, COPY_WHOLE_FILE);
		}

		get_title_and_id_from_sfo(templn, tempID, data[v3_entry].name, icon, tempstr, 0);
	}
	else
		{get_name(enc_dir_name, data[v3_entry].name, 0); utf8enc(templn, enc_dir_name, 1);}

	if(data[v3_entry].is_directory && IS_ISO_FOLDER)
	{
		char iso_ext[8][4] = {"iso", "ISO", "bin", "BIN", "mdf", "MDF", "img", "IMG"};
		for(u8 e = 0; e < 10; e++)
		{
			if(e >= 8) return FAILED;

			sprintf(tempstr, "%s/%s/%s.%s", param, data[v3_entry].name, data[v3_entry].name, iso_ext[e]);
			if(remote_stat(ns, tempstr, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection)==0) break;
		}

		u8 index = 4;

		// cover: folder/filename.jpg
		char img_ext[4][5] = {".jpg\0", ".png\0", ".PNG\0", ".JPG\0"};
		for(u8 e = 0; e < 4; e++)
		{
			sprintf(enc_dir_name, "%s/%s/%s%s", param, data[v3_entry].name, data[v3_entry].name, img_ext[e]);
			if(remote_stat(ns, enc_dir_name, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection)==0) {index = e; break;}
		}

		if(index < 4)
		{
			get_name(icon, data[v3_entry].name, 1); strcat(icon, img_ext[index]);
			copy_net_file(icon, enc_dir_name, ns, COPY_WHOLE_FILE);

			if(file_exists(icon)==false) icon[0] = NULL;
		}

		sprintf(data[v3_entry].name, "%s", tempstr + strlen(param) + 1);
	}

	urlenc(enc_dir_name, data[v3_entry].name);
	get_default_icon(icon, param, data[v3_entry].name, data[v3_entry].is_directory, tempID, ns, abort_connection);

	if(webman_config->nocov<2 && (icon[0]==0 || webman_config->nocov)) {get_name(tempstr, data[v3_entry].name, 1); strcat(tempstr, ".PNG"); if(file_exists(tempstr)) strcpy(icon, tempstr);}

	if(webman_config->tid && tempID[0]>'@' && strlen(templn) < 50 && strstr(templn, " [")==NULL) {strcat(templn, " ["); strcat(templn, tempID); strcat(templn, "]");}

	return 0;
}
 #endif //#ifndef LITE_EDITION
#endif //#ifdef COBRA_ONLY

static void add_query_html(char *buffer, char *param, char *label)
{
	char templn[64];
	sprintf(templn, "[<a href=\"/index.ps3?%s\">%s</a>] ", param, label); strcat(buffer, templn);
}

static void check_cover_folders(char *buffer)
{
#ifndef ENGLISH_ONLY
												covers_exist[0]=isDir(COVERS_PATH);
#endif
	sprintf(buffer, "%s/covers", MM_ROOT_STD) ; covers_exist[1]=isDir(buffer);
	sprintf(buffer, "%s/covers", MM_ROOT_STL) ; covers_exist[2]=isDir(buffer);
	sprintf(buffer, "%s/covers", MM_ROOT_SSTL); covers_exist[3]=isDir(buffer);
												covers_exist[4]=isDir("/dev_hdd0/GAMES/covers");
												covers_exist[5]=isDir("/dev_hdd0/GAMEZ/covers");
												covers_exist[6]=isDir(WMTMP) && (webman_config->nocov!=2);

#ifndef ENGLISH_ONLY
	if(!covers_exist[0]) {use_custom_icon_path = strstr(COVERS_PATH, "%s"); use_icon_region = strstr(COVERS_PATH, "%s/%s");} else {use_icon_region = use_custom_icon_path = false;}

	// disable custom icon from web repository if network is disabled //
	if(use_custom_icon_path && islike(COVERS_PATH, "http"))
	{
		char ip[ip_size] = "";
		netctl_main_9A528B81(ip_size, ip);
		if(ip[0] == NULL) use_custom_icon_path = false;
	}
#endif
}

static bool game_listing(char *buffer, char *templn, char *param, char *tempstr, bool mobile_mode)
{
	u64 c_len = 0;
	CellRtcTick pTick;

	struct CellFsStat buf;
	int fd;

	gmobile_mode = mobile_mode;

	if(!mobile_mode && strstr(param, "/index.ps3"))
	{
		strcat(buffer, "<font style=\"font-size:18px\">");
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS3)) { add_query_html(buffer, (char*)"ps3", (char*)"PS3");
											add_query_html(buffer, (char*)"games", (char*)"GAMES");
											add_query_html(buffer, (char*)"PS3ISO", (char*)"PS3ISO");}

		if(!(webman_config->cmask & PS2))   add_query_html(buffer, (char*)"PS2ISO", (char*)"PS2ISO");
		if(!(webman_config->cmask & PSP))   add_query_html(buffer, (char*)"PSPISO", (char*)"PSPISO");
		if(!(webman_config->cmask & PS1))   add_query_html(buffer, (char*)"PSXISO", (char*)"PSXISO");
		if(!(webman_config->cmask & BLU))   add_query_html(buffer, (char*)"BDISO" , (char*)"BDISO" );
		if(!(webman_config->cmask & DVD))   add_query_html(buffer, (char*)"DVDISO", (char*)"DVDISO");
 #ifndef LITE_EDITION
		if(webman_config->netd0 || webman_config->netd1 || webman_config->netd2 || webman_config->netd3 || webman_config->netd4) add_query_html(buffer, (char*)"net", (char*)"NET");
 #endif
		add_query_html(buffer, (char*)"hdd", (char*)"HDD");
		add_query_html(buffer, (char*)"usb", (char*)"USB");
		add_query_html(buffer, (char*)"ntfs", (char*)"NTFS");
#else
		if(!(webman_config->cmask & PS3)) add_query_html(buffer, (char*)"games", (char*)"GAMES");
		if(!(webman_config->cmask & PS2)) add_query_html(buffer, (char*)"PS2ISO", (char*)"PS2ISO");

		add_query_html(buffer, (char*)"hdd", (char*)"HDD");
		add_query_html(buffer, (char*)"usb", (char*)"USB");
#endif //#ifdef COBRA_ONLY
	}
	else
		strcat(buffer, " <br>");

	c_len = 0; while(loading_games && working && (c_len < 500)) {sys_timer_usleep(200000); c_len++;}

	if(c_len >= 500 || !working) {strcat(buffer, "503 Server is busy"); return false;}

	u32 buf_len = strlen(buffer);

/*
	CellRtcTick pTick, pTick2;
	cellRtcGetCurrentTick(&pTick);
	int upd_time=0;

	if(file_exists(WMTMP "/games.html"))
		upd_time=buf.st_mtime;

	CellRtcDateTime rDate;
	cellRtcSetTime_t(&rDate, upd_time);
	cellRtcGetTick(&rDate, &pTick2);

	sprintf(templn, "[%ull %ull %i ]<br>", pTick2, pTick, (pTick.tick-pTick2.tick)/1000000);
	strcat(buffer, templn);

	if(strstr(param, "/index.ps3?") || ((pTick.tick-pTick2.tick)/1000000)>43200) {DELETE_CACHED_GAMES}
*/

	// use cached page
	loading_games = 1;
	if(mobile_mode) {cellFsUnlink((char*)GAMELIST_JS); buf_len = 0;}
	else
	{
		if(strstr(param, "/index.ps3?")) cellFsUnlink((char*)WMTMP "/games.html");

		if(cellFsStat((char*)WMTMP "/games.html", &buf) == CELL_FS_SUCCEEDED && buf.st_size > 10)
		{
			int fdu;
			if(cellFsOpen((char*)WMTMP "/games.html", CELL_FS_O_RDONLY, &fdu, NULL, 0) == CELL_FS_SUCCEEDED)
			{
				cellFsRead(fdu, (char*)(buffer + buf_len), buf.st_size, NULL);
				cellFsClose(fdu);

				loading_games = 0; // return
			}
		}
	}

	if(loading_games)
	{
		int abort_connection=0;
		u8 is_net = 0;

		u16 idx = 0;
		u32 tlen = buf_len; buffer[tlen] = NULL;
		char *sysmem_html=buffer+_8KB_;

		u32 BUFFER_MAXSIZE = (BUFFER_SIZE_ALL-(12*KB));

		typedef struct
		{
			char 	path[MAX_LINE_LEN];
		}
		t_line_entries;
		t_line_entries *line_entry = (t_line_entries *)sysmem_html;
		u16 max_entries = (BUFFER_MAXSIZE / MAX_LINE_LEN); tlen = 0;

#ifndef ENGLISH_ONLY
		char onerror_prefix[24]=" onerror=\"this.src='", onerror_suffix[30]="';\"";  // wm_icons[default_icon]
		if(!use_custom_icon_path) onerror_prefix[0] = onerror_suffix[0] = NULL;
#else
		#define onerror_prefix ""
		#define onerror_suffix ""
#endif
		check_cover_folders(templn);

		char icon[MAX_PATH_LEN], enc_dir_name[1024], subpath[MAX_PATH_LEN];

		// filter html content
		u8 filter0, filter1, b0, b1; char filter_name[MAX_PATH_LEN]; filter_name[0] = NULL; filter0=filter1=b0=b1=0;

#ifdef COBRA_ONLY
		if(strstr(param, "ntfs")) {filter0=NTFS; b0=1;} else
#endif
		for(u8 f0=0; f0<16; f0++) if(strstr(param, drives[f0])) {filter0=f0; b0=1; break;}
		for(u8 f1=0; f1<11; f1++) if(strstr(param, paths [f1])) {filter1=f1; b1=1; break;}
		if(!b0 && strstr(param, "hdd" ))  {filter0=0; b0=1;}
		if(!b0 && strstr(param, "usb" ))  {filter0=1; b0=2;}
		if(!b1 && strstr(param, "games")) {filter1=0; b1=2;}
		if(!b1 && strstr(param, "?ps3"))  {filter1=0; b1=3;}
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
		if(!b0 && strstr(param, "net" ))  {filter0=7;  b0=3;}
 #endif
#endif
		if(b0==0 && b1==0 && strstr(param, "?")!=NULL && strstr(param, "?html")==NULL && strstr(param, "mobile")==NULL) strcpy(filter_name, strstr(param, "?")+1);

		int ns=-2; u8 uprofile=profile, default_icon=0;

		for(u8 f0=filter0; f0<16; f0++)  // drives: 0="/dev_hdd0", 1="/dev_usb000", 2="/dev_usb001", 3="/dev_usb002", 4="/dev_usb003", 5="/dev_usb006", 6="/dev_usb007", 7="/net0", 8="/net1", 9="/net2", 10="/net3", 11="/net4", 12="/ext", 13="/dev_sd", 14="/dev_ms", 15="/dev_cf"
		{
			if(!webman_config->usb0 && (f0==1)) continue;
			if(!webman_config->usb1 && (f0==2)) continue;
			if(!webman_config->usb2 && (f0==3)) continue;
			if(!webman_config->usb3 && (f0==4)) continue;
			if(!webman_config->usb6 && (f0==5)) continue;
			if(!webman_config->usb7 && (f0==6)) continue;

			// f0 -> 7 to 11 (net), 12 ntfs/ext

			if(!webman_config->dev_sd && (f0==13)) continue;
			if(!webman_config->dev_ms && (f0==14)) continue;
			if(!webman_config->dev_cf && (f0==15)) continue;

			if( f0==NTFS && (!webman_config->usb0 && !webman_config->usb1 && !webman_config->usb2 &&
							 !webman_config->usb3 && !webman_config->usb6 && !webman_config->usb7)) continue;

			if(( f0<7 || f0>NTFS) && file_exists(drives[f0])==false) continue;
//
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
			if((ns >= 0) && (ns!=g_socket)) {shutdown(ns, SHUT_RDWR); socketclose(ns);}
 #endif
#endif
			ns=-2; uprofile=profile; default_icon=0;
			for(u8 f1=filter1; f1<11; f1++) // paths: 0="GAMES", 1="GAMEZ", 2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO", 7="PSXGAMES", 8="PSPISO", 9="ISO", 10="video"
			{
#ifndef COBRA_ONLY
				if(IS_ISO_FOLDER && !(IS_PS2_FOLDER)) continue; // 0="GAMES", 1="GAMEZ", 5="PS2ISO", 10="video"
#endif
				if(idx>=max_entries || tlen>=BUFFER_MAXSIZE) break;

				//if(IS_PS2_FOLDER && f0>0)  continue; // PS2ISO is supported only from /dev_hdd0
				if(f1>=10) {if(f0<7 || f0>NTFS) strcpy(paths[10], f0==0 ? "video" : "GAMES_DUP"); else continue;}
				if(f0==NTFS) {if(f1>6 || !cobra_mode) break; else if(IS_JB_FOLDER || IS_PS2_FOLDER) continue;}

				is_net = (f0>=7 && f0<NTFS);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				if(is_net)
				{
					if(f1>8 || !cobra_mode) break;
					if(f0==7  && !webman_config->netd0) break; //net0
					if(f0==8  && !webman_config->netd1) break; //net1
					if(f0==9  && !webman_config->netd2) break; //net2
 #ifdef NET3NET4
					if(f0==10 && !webman_config->netd3) break; //net3
					if(f0==11 && !webman_config->netd4) break; //net4
 #else
					if(f0==10 || f0==11) break;
 #endif
				}
 #endif
#endif
				if(b0) {if(b0==2 && f0<7); else if(b0==3 && f0!=NTFS); else if(filter0!=f0) continue;}
				if(b1) {if(b1>=2 && (f1<b1 || f1>=10) && filter1<3); else if(filter1!=f1) continue;}
				else
				{
					if( (webman_config->cmask & PS3) && IS_PS3_TYPE   ) continue; // 0="GAMES", 1="GAMEZ", 2="PS3ISO", 10="video"
					if( (webman_config->cmask & BLU) && IS_BLU_FOLDER ) continue;
					if( (webman_config->cmask & DVD) && IS_DVD_FOLDER ) continue;
					if( (webman_config->cmask & PS2) && IS_PS2_FOLDER ) continue;
					if( (webman_config->cmask & PS1) && IS_PSX_FOLDER ) continue;
					if( (webman_config->cmask & PSP) && IS_PSP_FOLDER ) continue;
				}

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				if(is_net && (netiso_svrid == (f0-7)) && (g_socket != -1)) ns = g_socket; /* reuse current server connection */ else
				if(is_net && (ns<0)) ns = connect_to_remote_server(f0-7);
 #endif
#endif
				if(is_net && (ns<0)) break;
//
				bool ls; u8 li, subfolder; li=subfolder=0; ls=false; // single letter folder

		subfolder_letter_html:
				subfolder = 0; uprofile = profile;
		read_folder_html:
//
#ifndef LITE_EDITION
 #ifdef COBRA_ONLY
				if(is_net)
				{
					char ll[4]; if(li) sprintf(ll, "/%c", '@'+li); else ll[0] = NULL;
					sprintf(param, "/%s%s%s",    paths[f1], SUFIX(uprofile), ll);

					if(li==99) sprintf(param, "/%s%s", paths[f1], AUTOPLAY_TAG);
				}
				else
 #endif
#endif
				{
					if(f0==NTFS) //ntfs
						sprintf(param, "%s", WMTMP);
					else
						sprintf(param, "%s/%s%s", drives[f0], paths[f1], SUFIX(uprofile));

					if(li==99) sprintf(param, "%s/%s%s", drives[f0], paths[f1], AUTOPLAY_TAG);
				}

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				if(is_net && open_remote_dir(ns, param, &abort_connection) < 0) goto continue_reading_folder_html; //continue;
 #endif
#endif


				CellFsDirent entry;
				u64 read_e;
				u8 is_iso=0;
				int fd2=0, flen;
				char tempID[12];
				cellRtcGetCurrentTick(&pTick);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				sys_addr_t data2 = 0;
				int v3_entries, v3_entry; v3_entries=v3_entry=0;
				netiso_read_dir_result_data *data=NULL; char neth[8];
				if(is_net)
				{
					v3_entries = read_remote_dir(ns, &data2, &abort_connection);
					if(data2==NULL) goto continue_reading_folder_html; //continue;
					data=(netiso_read_dir_result_data*)data2; sprintf(neth, "/net%i", (f0-7));
				}
 #endif
#endif
				if(!is_net && file_exists( param) == false) goto continue_reading_folder_html; //continue;
				if(!is_net && cellFsOpendir( param, &fd) != CELL_FS_SUCCEEDED) goto continue_reading_folder_html; //continue;

				default_icon = (f1 < 4) ? 5 : (f1 == 4) ? 9 : (f1 == 5) ? 7 : (f1 < 8 ) ? 6 : (f1 < 10 ) ? 8 : 5;

				while((!is_net && (cellFsReaddir(fd, &entry, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
					|| (is_net && (v3_entry < v3_entries))
 #endif
#endif
					)
				{
					if(idx>=max_entries || tlen>=BUFFER_MAXSIZE) break;
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
					if(is_net)
					{
						if((ls==false) && (li==0) && (f1>1) && (data[v3_entry].is_directory) && (data[v3_entry].name[1]==NULL)) ls=true; // single letter folder was found

						if(add_net_game(ns, data, v3_entry, neth, param, templn, tempstr, enc_dir_name, icon, tempID, f1, 1)==FAILED) {v3_entry++; continue;}

						if(filter_name[0]>=' ' && strcasestr(templn, filter_name)==NULL && strcasestr(param, filter_name)==NULL && strcasestr(data[v3_entry].name, filter_name)==NULL) {v3_entry++; continue;}


						strcpy(tempstr, icon); urlenc(icon, tempstr);

						snprintf(tempstr, 8, "%s      ", templn); // sort key

						if(mobile_mode)
						{
							if(strchr(enc_dir_name, '"') || strchr(icon, '"')) continue; // ignore: cause syntax error in javascript: gamelist.js
							for(size_t c=0; templn[c]!=0; c++) {if(templn[c]=='"' || templn[c]<0x20) templn[c]=0x20;} // replace invalid chars

							int w=260, h=300; if(strstr(icon, "ICON0.PNG")) {w=320; h=176;} else if(strstr(icon, "icon_wm_")) {w=280; h=280;}

							sprintf(tempstr + HTML_KEY_LEN, "{img:\"%s\",width:%i,height:%i,desc:\"%s\",url:\"%s%s/%s\"},",
									icon[0] ? icon : wm_icons[default_icon], w, h, templn, neth, param, enc_dir_name);
						}
						else
							sprintf(tempstr + HTML_KEY_LEN, "<div class=\"gc\"><div class=\"ic\"><a href=\"/mount.ps3%s%s/%s?random=%x\"><img id=\"im%i\" src=\"%s\"%s%s%s class=\"gi\"></a></div><div class=\"gn\"><a href=\"%s%s/%s\">%s</a></div></div>",
									neth, param, enc_dir_name, (u16)pTick.tick, idx,
									icon, onerror_prefix, (onerror_prefix[0]!=NULL && default_icon) ? wm_icons[default_icon] : "", onerror_suffix,
									neth, param, enc_dir_name, templn);

						v3_entry++; flen = strlen(tempstr);
						if(flen > MAX_LINE_LEN) continue; //ignore lines too long
						strcpy(line_entry[idx].path, tempstr); idx++;
						tlen += flen;
					}
					else
 #endif
#endif
					{
						if(entry.d_name[0]=='.') continue;

//////////////////////////////
						subfolder = 0;
						sprintf(subpath, "%s/%s", param, entry.d_name);
						if(IS_ISO_FOLDER && isDir(subpath) && cellFsOpendir(subpath, &fd2) == CELL_FS_SUCCEEDED)
						{
							strcpy(subpath, entry.d_name); subfolder = 1;
next_html_entry:
							cellFsReaddir(fd2, &entry, &read_e);
							if(read_e<1) {cellFsClosedir(fd2); fd2 = 0; continue;}
							if(entry.d_name[0]=='.') goto next_html_entry;
							sprintf(templn, "%s/%s", subpath, entry.d_name); strcpy(entry.d_name, templn);
						}
						flen = strlen(entry.d_name);
//////////////////////////////

						if(idx>=max_entries || tlen>=BUFFER_MAXSIZE) break;

#ifdef COBRA_ONLY
						is_iso = (f0==NTFS && flen>13 && strstr(entry.d_name + flen - 13, ".ntfs[")!=NULL) ||
								 (IS_ISO_FOLDER && flen > 4 && (
								 (            !strncasecmp(entry.d_name + flen - 4, ".iso", 4)) ||
								 (flen > 6 && !strncasecmp(entry.d_name + flen - 6, ".iso.0", 6)) ||
								 ((IS_PS2_FOLDER) && strcasestr(".bin|.img|.mdf|.enc", entry.d_name + flen - 4)) ||
								 ((IS_PSX_FOLDER || IS_DVD_FOLDER || IS_BLU_FOLDER) && strcasestr(".bin|.img|.mdf", entry.d_name + flen - 4))
								 ));
#else
						is_iso = (IS_PS2_FOLDER && flen > 8 && !strncmp(entry.d_name + flen - 8, ".BIN.ENC", 8));
#endif
						if(IS_JB_FOLDER && !is_iso)
						{
							sprintf(templn, "%s/%s/PS3_GAME/PARAM.SFO", param, entry.d_name);
						}

						if(is_iso || (IS_JB_FOLDER && file_exists(templn)))
						{
							icon[0] = tempID[0] = NULL;

							if(!is_iso)
							{
								get_title_and_id_from_sfo(templn, tempID, entry.d_name, icon, tempstr, 0);
							}
							else
							{
								get_name(templn, entry.d_name, 0);
#ifdef COBRA_ONLY
								if(f0==NTFS)
								{   // ntfs
									if(f1< 2 || f1>6) continue; //2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO"

									// skip non-matching extended content
									if((uprofile >0) && (strstr(entry.d_name, ").ntfs[")!=NULL) && !strstr(entry.d_name, SUFIX3(uprofile))) continue;

									// skip extended content of ntfs cached in /wmtmp if current user profile is 0
									if((uprofile==0 && flen>17)) {u8 u; for(u=1;u<5;u++) if(strstr(entry.d_name + flen - 17, SUFIX3(u))) break; if(u!=5) continue;}

									flen-=13; if(flen<0) continue;

									if(IS_PS3_FOLDER && strcmp(entry.d_name+flen, ".ntfs[PS3ISO]")) continue;
									if(IS_BLU_FOLDER &&!strstr(entry.d_name+flen, ".ntfs[BD"     )) continue;
									if(IS_DVD_FOLDER && strcmp(entry.d_name+flen, ".ntfs[DVDISO]")) continue;
									if(IS_PSX_FOLDER && strcmp(entry.d_name+flen, ".ntfs[PSXISO]")) continue;
								}

								if((IS_PS3_FOLDER) && ((f0!=NTFS) || (f0==NTFS && !extcmp(entry.d_name, ".ntfs[PS3ISO]", 13))))
								{
									get_name(templn, entry.d_name, 1); strcat(templn, ".SFO\0");
									if(f0!=NTFS && file_exists(templn)==false)
									{
										get_name(tempstr, entry.d_name, 0);
										sprintf(templn, "%s/%s.SFO", param, tempstr);
									}

									if(get_title_and_id_from_sfo(templn, tempID, entry.d_name, icon, tempstr, f0)==FAILED)
									{
										if( f0!=NTFS && is_iso)
										{
											int fs; char *ps3iso = icon;
											sprintf(ps3iso, "%s/%s", param, entry.d_name);
											if(cellFsOpen(ps3iso, CELL_FS_O_RDONLY, &fs, NULL, 0) == CELL_FS_SUCCEEDED)
											{
												uint64_t msiz = 0;
												if(cellFsLseek(fs, 0x810, CELL_FS_SEEK_SET, &msiz) == CELL_FS_SUCCEEDED)
												{
													if(cellFsRead(fs, (void *)&tempID, 11, &msiz) == CELL_FS_SUCCEEDED)
													{
														strncpy(&tempID[4], &tempID[5], 5); tempID[9] = NULL;
													}
												}
												cellFsClose(fs);
											}
											ps3iso[0] = NULL;
										}
									}
								}
#endif
							}

							if(filter_name[0]>=' ' && strcasestr(templn, filter_name)==NULL && strcasestr(param, filter_name)==NULL && strcasestr(entry.d_name, filter_name)==NULL)
							{if(subfolder) goto next_html_entry; else continue;}

							get_folder_icon(icon, f1, is_iso, param, entry.d_name, tempID);

							get_default_icon(icon, param, entry.d_name, 0, tempID, ns, abort_connection);

							if(webman_config->tid && tempID[0]>'@' && strlen(templn) < 50 && strstr(templn, " [")==NULL) {strcat(templn, " ["); strcat(templn, tempID); strcat(templn, "]");}

							urlenc(enc_dir_name, entry.d_name);

							templn[64] = NULL; flen = strlen(templn);

							urlenc(tempstr, icon);

							snprintf(tempstr, 8, "%s      ", templn); // sort key

							if(mobile_mode)
							{
								if(strchr(enc_dir_name, '"') || strchr(icon, '"')) continue; // ignore: cause syntax error in javascript: gamelist.js
								for(size_t c=0; templn[c]!=0; c++) {if(templn[c]=='"' || templn[c]<0x20) templn[c]=0x20;} // replace invalid chars

								int w=260, h=300; if(strstr(icon, "ICON0.PNG")) {w=320; h=176;} else if(strstr(icon, "icon_wm_")) {w=280; h=280;}

								sprintf(tempstr + HTML_KEY_LEN, "{img:\"%s\",width:%i,height:%i,desc:\"%s\",url:\"%s/%s\"},",
										icon, w, h, templn, param, enc_dir_name);
							}
							else
							{
								do
								{
									sprintf(tempstr + HTML_KEY_LEN, "<div class=\"gc\"><div class=\"ic\"><a href=\"/mount.ps3%s%s/%s?random=%x\"><img id=\"im%i\" src=\"%s\"%s%s%s class=\"gi\"></a></div><div class=\"gn\"><a href=\"%s%s/%s\">%s</a></div></div>",
											param, "", enc_dir_name, (u16)pTick.tick, idx, icon, onerror_prefix, (onerror_prefix[0]!=NULL && default_icon) ? wm_icons[default_icon] : "", onerror_suffix, param, "", enc_dir_name, templn);

									flen-=4; if(flen<32) break;
									templn[flen] = NULL;
								}
								while(strlen(templn)>MAX_LINE_LEN);
							}

							flen = strlen(tempstr);
							if(flen > MAX_LINE_LEN) continue; //ignore lines too long

							strcpy(line_entry[idx].path, tempstr); idx++;
							tlen += flen;

							cellRtcGetCurrentTick(&pTick);
						}
//////////////////////////////
						if(subfolder) goto next_html_entry;
//////////////////////////////
					}
				}

				if(!is_net) cellFsClosedir(fd);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				if(data2) sys_memory_free(data2);
 #endif
#endif

//
	continue_reading_folder_html:

				if((uprofile>0) && (f1<9)) {subfolder=uprofile=0; goto read_folder_html;}
				if(is_net && ls && (li<27)) {li++; goto subfolder_letter_html;} else if(li<99 && f1<7) {li=99; goto subfolder_letter_html;}
//
			}

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
			if(is_net && (ns >= 0) && (ns!=g_socket)) {shutdown(ns, SHUT_RDWR); socketclose(ns); ns=-2;}
 #endif
#endif
		}

		if(idx)
		{   // sort html game items
			u16 n, m;
			char *swap = tempstr;
			for(n=0; n<(idx-1); n++)
				for(m=(n+1); m<idx; m++)
					if(strncasecmp(line_entry[n].path, line_entry[m].path, HTML_KEY_LEN) > 0)
					{
						strcpy(swap, line_entry[n].path);
						strcpy(line_entry[n].path, line_entry[m].path);
						strcpy(line_entry[m].path, swap);
					}
		}

#ifndef LITE_EDITION
		bool sortable = false;
#endif

		if(mobile_mode)
			sprintf(buffer, "slides = [");
		else
		{
			sprintf(templn, // wait dialog div
							"<div id=\"wmsg\" style=\"display:none\"><H1>. . .</H1></div>"
							"<script>window.onclick=function(e){t=e.target;if(t.id.indexOf('im')==0||typeof(t.href)=='string')wmsg.style.display='block';}</script>"
							// show games count + find icon
							"<a href=\"javascript:var s=prompt('Search:','');if(s){rhtm.style.display='block';window.location='/index.ps3?'+escape(s)}\">%'i %s &#x1F50D;</a></font>"
							// separator
							"<HR><span style=\"white-space:normal;\">", idx, (strstr(param, "DI")!=NULL) ? STR_FILES : STR_GAMES); strcat(buffer, templn);

#ifndef LITE_EDITION
 #ifndef EMBED_JS
			if(file_exists(GAMES_SCRIPT_JS))
			{
				sprintf(templn, SCRIPT_SRC_FMT, GAMES_SCRIPT_JS); strcat(buffer, templn);
			}
 #endif
			sortable = file_exists(JQUERY_LIB_JS) && file_exists(JQUERY_UI_LIB_JS);
			if(sortable)
			{	// add external jquery libraries
				sprintf(templn, SCRIPT_SRC_FMT
								SCRIPT_SRC_FMT
								"<script>$(function(){$(\"#mg\").sortable();});</script><div id=\"mg\">",
								JQUERY_LIB_JS, JQUERY_UI_LIB_JS); strcat(buffer, templn);
			}
#endif
		}

		tlen = buf_len;
		for(u16 m = 0; m < idx; m++)
		{
			strcat(buffer + tlen, (line_entry[m].path) + HTML_KEY_LEN); tlen += strlen(buffer + tlen);
			if(tlen > (BUFFER_MAXSIZE)) break;
		}

#ifndef LITE_EDITION
		if(sortable) strcat(buffer + tlen, "</div>");
#endif

		loading_games = 0;

		if(mobile_mode)
		{
			strcat(buffer, "];");
			savefile((char*)GAMELIST_JS, (char*)(buffer), strlen(buffer));
		}
		else
		{
			savefile((char*)WMTMP "/games.html", (char*)(buffer + buf_len), (strlen(buffer) - buf_len));
		}
	}
	return true;
}
