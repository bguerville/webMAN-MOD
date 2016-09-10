#define SC_FS_DISK_FREE		840

#define ICON_STYLE			" style=\"position:fixed;top:118px;right:10px;max-height:176px;z-index:-1;display:none\" onerror=\"this.style.display='none';\""

u32 _LINELEN = LINELEN;
u32 _MAX_PATH_LEN = MAX_PATH_LEN;
u32 _MAX_LINE_LEN = MAX_LINE_LEN;

#define _2MB_	0x200000ULL
#define _48GB_	0xC00000000ULL

#define TABLE_ITEM_PREFIX  "<tr><td><a class=\""
#define TABLE_ITEM_SUFIX   "</td></tr>"

#define TABLE_ITEM_SIZE  28  // strlen(TABLE_ITEM_PREFIX + TABLE_ITEM_SUFIX) = (18 + 10)

#define FILE_MGR_KEY_LEN	6

#define ext5   name + MAX(flen - 5, 0)
#define ext8   name + MAX(flen - 8, 0)
#define ext13  name + MAX(flen - 13, 0)

static int add_list_entry(char *param, int plen, char *tempstr, bool is_dir, char *ename, char *templn, char *name, char *fsize, CellRtcDateTime rDate, unsigned long long sz, char *sf, u8 is_net, u8 show_icon0, u8 is_ps3_http, u8 skip_cmd)
{
	if(plen < 4) sz = 0, is_dir = true; // force folders in root -> fix: host_root, app_home

	unsigned long long sbytes = sz; bool is_root = false; u16 flen;

	if(!is_dir)
	{
		if(sz < 10240)	{sprintf(sf, "%s", STR_BYTE);} else
		if(sz < _2MB_)	{sprintf(sf, "%s", STR_KILOBYTE); sz >>= 10;} else
		if(sz < _48GB_) {sprintf(sf, "%s", STR_MEGABYTE); sz >>= 20;} else
						{sprintf(sf, "%s", STR_GIGABYTE); sz >>= 30;}
	}

	// encode file name for html
	if(is_net)
		flen = htmlenc(tempstr, name, 1);
	else
		flen = strlen(name);

	char *ext = name + MAX(flen - 4, 0); *fsize = NULL;

#ifndef LITE_EDITION
	// get title & title ID from PARAM.SFO
	if( !is_dir && IS(name, "PARAM.SFO") )
	{
		char titleid[10], version[8], title[64]; strcpy(title, templn);

		//get title & app version from PARAM.SFO
		getTitleID(templn, version, GET_VERSION);
		getTitleID(title, titleid, GET_TITLE_AND_ID); char *p = strstr(title, " ["); if(p) *p = NULL; //p = strstr(title, "\n"); if(p) *p = NULL;
		get_cover_from_name(tempstr, strrchr(param, '/') + 1, titleid); // get title id from path (if title ID was not found in PARAM.SFO)
		if(*version >= '0') {strcat(title, " v"); strcat(title, version);}
		sprintf(tempstr, "%s%s", HDD0_GAME_DIR, titleid); bool has_updates_dir = file_exists(tempstr);

		sprintf(tempstr, "<label title=\"%s\">%s</label></a>", title, name); strcpy(name, tempstr);

		// show title & link to patches folder
		if(has_updates_dir)
			sprintf(fsize, HTML_URL2, HDD0_GAME_DIR, titleid, title);
		else
			sprintf(fsize, "%s", title);

		// show title id & link to updates
		sprintf(tempstr, "<div class='sfo'>%s [<a href=\"%s/%s/%s-ver.xml\">%s</a>]</div>", fsize, "https://a0.ww.np.dl.playstation.net/tpl/np", titleid, titleid, titleid);

		sprintf(fsize, "%'llu %s%s", sz, sf, tempstr);

		#ifdef FIX_GAME
		if(has_updates_dir) sprintf(fsize, "<a href=\"/fixgame.ps3%s%s\">%'llu %s</a>%s", HDD0_GAME_DIR, titleid, sz, sf, tempstr);
		#endif
	}
#endif

	// encode url for html
	if(urlenc(tempstr, templn)) {tempstr[_MAX_LINE_LEN] = NULL; sprintf(templn, "%s", tempstr);}

#ifndef LITE_EDITION
	// is image?
	u8 show_img = !is_ps3_http && (!is_dir && (!strcasecmp(ext, ".png") || !strcasecmp(ext, ".jpg") || !strcasecmp(ext, ".bmp")));
#endif

	char dclass = 'w'; // file class

	// build size column
	if(is_dir)
	{
		dclass = 'd'; // dir class

		bool show_play = ((flen == 8) && IS(templn, "/dev_bdvd") && IS_ON_XMB);

		if(flen == 9 && IS(templn, "/host_root")) strcpy(templn, "/dev_hdd0/packages"); // replace host_root with a shortcut to /dev_hdd0/packages

		if(*name == '.')
			sprintf(fsize, HTML_URL, templn, HTML_DIR);
		else if(flen == 9 && IS(templn, "/dev_blind"))
			sprintf(fsize, HTML_URL2, templn, "?0", HTML_DIR);
		else if(show_play && (isDir("/dev_bdvd/PS3_GAME") || file_exists("/dev_bdvd/SYSTEM.CNF")))
			sprintf(fsize, HTML_URL, "/play.ps3", "&lt;Play>");
		else if(show_play && isDir("/dev_bdvd/BDMV"))
			sprintf(fsize, HTML_URL, "/play.ps3", "&lt;BDV>");
		else if(show_play && isDir("/dev_bdvd/VIDEO_TS"))
			sprintf(fsize, HTML_URL, "/play.ps3", "&lt;DVD>");
		else if(show_play && isDir("/dev_bdvd/AVCHD"))
			sprintf(fsize, HTML_URL, "/play.ps3", "&lt;AVCHD>");
#ifdef FIX_GAME
		else if(islike(templn, HDD0_GAME_DIR) || (strstr(templn + 10, "/PS3_GAME" ) != NULL))
			sprintf(fsize, HTML_URL2, "/fixgame.ps3", templn, HTML_DIR);
#endif
#ifdef COPY_PS3
		else if(!is_net && ( (flen == 5 && (!strcasecmp(name, "VIDEO") || strcasestr(name, "music"))) || (flen == 6 && !strcasecmp(name, "covers")) || IS(name, "savedata") || IS(name, "exdata") || IS(name, "home") ))
			sprintf(fsize, "<a href=\"/copy.ps3%s\" title=\"copy to %s\">%s</a>", islike(templn, param) ? templn + plen : templn, islike(templn, "/dev_hdd0") ? "/dev_usb000" : "/dev_hdd0", HTML_DIR);
#endif
#ifndef LITE_EDITION
		else
		if(plen < 4 && islike(templn, "/dev_"))
		{
			uint64_t freeSize = 0, devSize = 0; is_root = true;
			system_call_3(SC_FS_DISK_FREE, (uint64_t)(uint32_t)templn, (uint64_t)(uint32_t)&devSize, (uint64_t)(uint32_t)&freeSize);

			unsigned long long	free_mb    = (unsigned long long)(freeSize>>20),
								free_kb    = (unsigned long long)(freeSize>>10),
								devsize_mb = (unsigned long long)(devSize>>20);

			// show graphic of device size & free space
			sprintf(fsize,  "<div class='bf' style='height:18px;text-align:left;overflow:hidden;'><div class='bu' style='height:18px;width:%i%%'></div><div style='position:relative;top:-%ipx;text-align:right'>"
							"<a href=\"/mount.ps3%s\" title=\"%'llu %s (%'llu %s) / %'llu %s (%'llu %s)\">&nbsp; %'8llu %s &nbsp;</a>"
							"</div></div>", (int)(100.0f * (float)(devSize - freeSize) / (float)devSize), is_ps3_http ? 20 : 18, templn, free_mb, STR_MBFREE, freeSize, STR_BYTE, devsize_mb, STR_MEGABYTE, devSize, STR_BYTE, (freeSize < _2MB_) ? free_kb : free_mb, (freeSize < _2MB_) ? STR_KILOBYTE : STR_MEGABYTE);
		}
#endif
		else
#ifdef PS2_DISC
			sprintf(fsize, "<a href=\"/mount%s%s\">%s</a>", strstr(name, "[PS2")?".ps2":".ps3", templn, HTML_DIR);
#else
			sprintf(fsize, "<a href=\"/mount.ps3%s\">%s</a>", templn, HTML_DIR);
#endif
	}

	else if(*fsize) ;

	else if(skip_cmd)
		sprintf(fsize, "%'llu %s", sz, sf);

#ifdef COBRA_ONLY
	else if( ((flen > 4) && strcasestr(ISO_EXTENSIONS, ext) != NULL && !islike(templn, HDD0_GAME_DIR)) || (!is_net && ( strstr(ext13, ".ntfs[") || IS(ext8, ".BIN.ENC") )) )
	{
		if( (strcasestr(name, ".iso.") != NULL) && extcasecmp(name, ".iso.0", 6) )
			sprintf(fsize, "<label title=\"%'llu %s\"> %'llu %s</label>", sbytes, STR_BYTE, sz, sf);
		else
			sprintf(fsize, "<a href=\"/mount.ps3%s\" title=\"%'llu %s\">%'llu %s</a>", templn, sbytes, STR_BYTE, sz, sf);
	}
#endif

#ifdef PKG_HANDLER
	else if( IS(ext, ".pkg") || IS(ext, ".PKG") )
			sprintf(fsize, "<a href=\"/install.ps3%s\">%'llu %s</a>", templn, sz, sf);
#endif

#ifdef COPY_PS3
	else if( IS(ext, ".bak") )
			sprintf(fsize, "<a href=\"/rename.ps3%s|\">%'llu %s</a>", templn, sz, sf);
	else if(   show_img
 #ifndef PKG_HANDLER
			|| IS(ext, ".pkg")
 #endif
			|| IS(ext, ".p3t") || IS(ext5, ".edat")
			|| IS(ext, ".rco") || IS(ext, ".qrc")
			|| !strcasecmp(ext, ".mp4") || !strcasecmp(ext, ".mkv") || !strcasecmp(ext, ".avi")
			|| !strcasecmp(ext, ".mp3")
			|| !memcmp(name, "webftp_server", 13) || !memcmp(name, "boot_plugins_", 13) || !memcmp(name, "coldboot", 8)
 #ifdef SWAP_KERNEL
			|| !memcmp(name, "lv2_kernel", 10)
 #endif
			)
				sprintf(fsize, "<a href=\"/copy.ps3%s\" title=\"%'llu %s copy to %s\">%'llu %s</a>", islike(templn, param) ? templn + plen : templn, sbytes, STR_BYTE, islike(templn, "/dev_hdd0") ? "/dev_usb000" : "/dev_hdd0", sz, sf);
#endif //#ifdef COPY_PS3

#ifdef LOAD_PRX
	else if(!is_net && ( IS(ext5, ".sprx")))
		sprintf(fsize, "<a href=\"/loadprx.ps3?slot=6&prx=%s\">%'llu %s</a>", templn, sz, sf);
#endif
#ifndef LITE_EDITION
	else if( (sz <= MAX_TEXT_LEN) && (strcasestr(".txt|.ini|.log|.sfx|.xml|.cfg|.his|.hip|.bup|.css|.html|conf|name", ext)!=NULL || strstr(templn, "wm_custom")!=NULL ) )
			sprintf(fsize, "<a href=\"/edit.ps3%s\">%'llu %s</a>", templn, sz, sf);
#endif
	else if(sbytes < 10240)
		sprintf(fsize, "%'llu %s", sz, sf);
	else
		sprintf(fsize, "<label title=\"%'llu %s\"> %'llu %s</label>", sbytes, STR_BYTE, sz, sf);

	snprintf(ename, FILE_MGR_KEY_LEN, "%s     ", name);
	if(flen > 4) {char c = name[flen - 1]; if(ISDIGIT(c)) ename[4] = c;} to_upper(ename);

	if((plen > 1) && memcmp(templn, param, plen) == 0) sprintf(templn, "%s", templn + plen + 1);

	flen = sprintf(tempstr, "%c%c%c%c%c%c"
							 "%c\" href=\"%s\"%s>%s</a></td>",
							dclass, ename[0], ename[1], ename[2], ename[3], ename[4],
							dclass, templn,
#ifndef LITE_EDITION
							show_img ? " onmouseover=\"s(this,0);\"" : (is_dir && show_icon0) ? " onmouseover=\"s(this,1);\"" :
#endif
							"", name);

	if(flen >= _LINELEN)
	{
		if(is_dir) sprintf(fsize, HTML_DIR); else sprintf(fsize, "%llu %s", sz, sf);

		flen = sprintf(tempstr, "%c%c%c%c%c%c"
								 "%c\" href=\"%s\">%s</a></td>",
						 		dclass, ename[0], ename[1], ename[2], ename[3], ename[4],
						 		dclass, templn, name);

		if(flen >= _LINELEN)
		{
			if(is_dir) sprintf(fsize, HTML_DIR); else sprintf(fsize, "%llu %s", sz, sf);

			flen = sprintf(tempstr, "%c%c%c%c%c%c"
									 "%c\">%s</a></td>",
									dclass, ename[0], ename[1], ename[2], ename[3], ename[4],
									dclass, name);
		}
	}

	sprintf(templn, "<td> %s%s</td>"
					"<td>%02i-%s-%04i %02i:%02i",
					fsize, is_root ? "" : " &nbsp; ",
					rDate.day, smonth[rDate.month-1], rDate.year, rDate.hour, rDate.minute);

	flen += concat(tempstr, templn);

	if(flen >= _LINELEN) {flen = 0; *tempstr = NULL;} //ignore file if it is still too long

	return flen;
}

static void add_breadcrumb_trail(char *pbuffer, char *param)
{
	int tlen = 0;

	char swap[_MAX_PATH_LEN], templn[_MAX_PATH_LEN], url[_MAX_PATH_LEN], *slash, *buffer = pbuffer;

	sprintf(templn, "%s", param);

	while((slash = strchr(templn + 1, '/')))
	{
		*slash = NULL;
		tlen+=strlen(templn) + 1;

		strcpy(swap, param);
		swap[tlen] = NULL;

		buffer += concat(buffer, "<a class=\"f\" href=\"");
		urlenc(url, swap);
		buffer += concat(buffer, url);

		htmlenc(url, templn, 1);
		sprintf(swap, "\">%s</a>/", templn);
		buffer += concat(buffer, swap);

		strcpy(templn, param + tlen);
	}

	if(!param[1]) sprintf(swap, "/");
	else if(param[1] != 'n' && file_exists(param) == false) strcpy(swap, strrchr(param, '/') + 1);
	else
	{
		char label[_MAX_PATH_LEN]; tlen = strlen(param) - 4; if(tlen < 0) tlen = 0;

		urlenc(url, param); if(islike(param, "/net")) htmlenc(label, templn, 0); else strcpy(label, templn);

		sprintf(swap, HTML_URL2,
#ifdef FIX_GAME
						islike(param, HDD0_GAME_DIR) ? "/fixgame.ps3" :
#endif
#ifdef PKG_HANDLER
						!extcmp(param + tlen, ".pkg", 4) ? "/install.ps3" :
#endif
						islike(param, "/dev_hdd0/GAMES/covers") ? "" :
						((isDir(param) || strcasestr(ISO_EXTENSIONS, param + tlen) != NULL) || (strstr(param, "/GAME") != NULL) || (strstr(param, ".ntfs[") != NULL) || (strstr(param, "/GAME") != NULL) || islike(param, "/net") || !extcmp(param + MAX(tlen - 4, 0), ".BIN.ENC", 8)) ? "/mount.ps3" :
						"", url, label);
	}
	strcat(buffer, swap);
}

static bool folder_listing(char *buffer, u32 BUFFER_SIZE_HTML, char *templn, char *param, int conn_s, char *tempstr, char *header, u8 is_ps3_http)
{
	struct CellFsStat buf;
	int fd;

	CellRtcDateTime rDate;

	if(islike(param, "/dev_blind?"))
	{
		if(strstr(param, "?1")) enable_dev_blind(NO_MSG);
		if(strstr(param, "?0")) disable_dev_blind();

		sprintf(templn, "/dev_blind: %s", isDir("/dev_blind")?STR_ENABLED:STR_DISABLED); strcat(buffer, templn); return true; //goto send_response;
	}

	absPath(templn, param, "/"); // auto mount /dev_blind

	u8 is_net = (param[1] == 'n'), skip_cmd = 0;

#if defined(FIX_GAME) || defined(COPY_PS3)
	if(copy_aborted) strcat(buffer, STR_CPYABORT);    //  /copy.ps3$abort
	else
	if(fix_aborted)  strcat(buffer, "Fix aborted!");  //  /fixgame.ps3$abort

	if(copy_aborted | fix_aborted) {strcat(buffer, "<p>"); sys_timer_usleep(100000); copy_aborted = fix_aborted = false;}
#endif

	_LINELEN = LINELEN;
	_MAX_PATH_LEN = MAX_PATH_LEN;
	_MAX_LINE_LEN = MAX_LINE_LEN;

	if(is_net || cellFsOpendir(param, &fd) == CELL_FS_SUCCEEDED)
	{
		int plen = strlen(param);

		if(!extcmp(param + MAX(plen - 7, 0), "/exdata", 7)) {_LINELEN = _MAX_LINE_LEN = _MAX_PATH_LEN = 200; skip_cmd = 1;}

		unsigned long long sz = 0, dir_size = 0;
		char ename[16], sf[8];
		char fsize[MAX_PATH_LEN], *swap = fsize;
		u16 idx = 0, dirs = 0, flen; bool is_dir;
		u32 tlen = 0, buf_len = 0;
		char *sysmem_html = buffer + _6KB_;

		typedef struct
		{
			char path[_LINELEN];
		}
		t_line_entries;
		t_line_entries *line_entry = (t_line_entries *)sysmem_html;
		u16 max_entries = ((BUFFER_SIZE_HTML - _12KB_) / _MAX_LINE_LEN) - 1;

		BUFFER_SIZE_HTML -= _2KB_;

		u8 jb_games = (strstr(param, "/GAMES") || strstr(param, "/GAMEZ"));
		u8 show_icon0 = jb_games || (islike(param, "/dev_hdd0/game") || islike(param, "/dev_hdd0/home/"));

#ifndef LITE_EDITION
		sprintf(templn, "<img id=\"icon\"%s>"
						"<script>"
						// show icon of item pointed with mouse
						"function s(o,d){u=o.href;p=u.indexOf('.ps3');if(p>0)u=u.substring(p+4);if(d){p=u.indexOf('/PS3_');if(p<0)p=u.indexOf('/USRDIR');if(p>0)u=u.substring(0,p);u+='%s/ICON0.PNG';}icon.src=u;icon.style.display='block';}"
						"</script>", ICON_STYLE, (jb_games ? "/PS3_GAME" : "")); strcat(buffer, templn);
#endif

		// breadcrumb trail //
		add_breadcrumb_trail(buffer, param);

		if((param[7] == 'v' || param[1] == 'a') && IS_ON_XMB && (isDir("/dev_bdvd/PS3_GAME") || file_exists("/dev_bdvd/SYSTEM.CNF") || isDir("/dev_bdvd/BDMV") || isDir("/dev_bdvd/VIDEO_TS") || isDir("/dev_bdvd/AVCHD")))
			strcat(buffer, ": <a href=\"/play.ps3\">&lt;Play>&nbsp;</a><br>");
		else
			strcat(buffer, ":<br>");

#ifdef COPY_PS3
		if(cp_mode) {sprintf(tempstr, "<font size=2><a href=\"/paste.ps3%s\">&#128203;</a> ", is_net ? "/dev_hdd0/packages" : param); add_breadcrumb_trail(tempstr, cp_path); strcat(buffer, tempstr); strcat(buffer, "</font><p>"); }
#endif
		tlen = strlen(buffer);

		tlen += concat(buffer, "<style>.sfo{position:absolute;top:300px;right:10px;font-size:14px}</style><table class=\"propfont\"><tr><td colspan=3><col width=\"220\"><col width=\"98\">");

		buf_len = tlen;

 #ifdef COBRA_ONLY
  #ifndef LITE_EDITION
		if(is_net)
		{
			int ns = FAILED, abort_connection = 0; char netid = param[4];

			if(netid >= '0' && netid <= '4') ns = connect_to_remote_server((netid  & 0xFF) - '0');

			if(ns >= 0)
			{
				strcat(param, "/");
				if(open_remote_dir(ns, param + 5, &abort_connection) >= 0)
				{
					strcpy(templn, param); while(templn[plen] == '/') templn[plen--] = NULL; plen++;
					char *p = strrchr(templn, '/'); if(p) *p = NULL; if(strlen(templn) < 6 && strlen(param) < 8) {templn[0] = '/', templn[1] = NULL;}

					urlenc(swap, templn);
					flen = sprintf(line_entry[idx].path, "!00000"
												  "f\" href=\"%s\">..</a></td>"
												  "<td> " HTML_URL " &nbsp; </td>"
												  "<td>11-Nov-2006 11:11"
												, swap, swap, HTML_DIR);

					if(flen >= _MAX_LINE_LEN) return false; //ignore lines too long
					idx++, dirs++;
					tlen += flen;

					sys_addr_t data = 0;
					netiso_read_dir_result_data *dir_items = NULL;
					int v3_entries = read_remote_dir(ns, &data, &abort_connection);
					if(data != NULL)
					{
						dir_items = (netiso_read_dir_result_data*)data;

						for(int n = 0; n < v3_entries; n++)
						{
							if(dir_items[n].name[0] == '.' && dir_items[n].name[1] == 0) continue;
							if(tlen > BUFFER_SIZE_HTML) break;
							if(idx >= (max_entries-3)) break;

							if(param[1] == 0)
								flen = sprintf(templn, "/%s", dir_items[n].name);
							else
							{
								flen = sprintf(templn, "%s%s", param, dir_items[n].name);
							}
							if(templn[flen - 1] == '/') templn[flen--] = NULL;

							cellRtcSetTime_t(&rDate, dir_items[n].mtime);

							sz = (unsigned long long)dir_items[n].file_size; dir_size += sz;

							is_dir = dir_items[n].is_directory; if(is_dir) dirs++;

							flen = add_list_entry(param, plen, tempstr, is_dir, ename, templn, dir_items[n].name, fsize, rDate, sz, sf, true, show_icon0, is_ps3_http, skip_cmd);

							if((flen == 0) || (flen > _MAX_LINE_LEN)) continue; //ignore lines too long
							strcpy(line_entry[idx].path, tempstr); idx++;
							tlen += (flen + TABLE_ITEM_SIZE);

							if(!working) break;
						}
						sys_memory_free(data);
					}
				}
				else //may be a file
				{
					flen = strlen(param) - 1; if(param[flen] == '/') param[flen] = NULL;

					int is_directory = 0;
					int64_t file_size;
					u64 mtime, ctime, atime;
					if(remote_stat(ns, param+5, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection) == 0)
					{
						if(file_size && !is_directory)
						{
							if(open_remote_file(ns, param+5, &abort_connection) > 0)
							{
								size_t header_len = prepare_header(header, param, 1);
								header_len += sprintf(header + header_len, "Content-Length: %llu\r\n\r\n", (unsigned long long)file_size);

								send(conn_s, header, header_len, 0);

								int bytes_read, boff = 0;
								while(boff < file_size)
								{
									bytes_read = read_remote_file(ns, (char*)buffer, boff, _64KB_, &abort_connection);
									if(bytes_read)
									{
										if(send(conn_s, buffer, bytes_read, 0) < 0) break;
									}
									boff+=bytes_read;
									if((uint32_t)bytes_read < _64KB_ || boff >= file_size) break;
								}
								open_remote_file(ns, (char*)"/CLOSEFILE", &abort_connection);
								shutdown(ns, SHUT_RDWR); socketclose(ns);
								sclose(&conn_s);
								return false;
							}
						}
					}
				}
				shutdown(ns, SHUT_RDWR); socketclose(ns);
			}
		}
		else
  #endif
 #endif
		{
			CellFsDirent entry; u64 read_e;

			while((cellFsReaddir(fd, &entry, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
			{
				if(entry.d_name[0] == '.' && entry.d_name[1] == 0) continue;
				if(tlen > BUFFER_SIZE_HTML) break;
				if(idx >= (max_entries-3)) break;

				if(plen < 4)
					flen = sprintf(templn, "/%s", entry.d_name);
				else
				{
					flen = sprintf(templn, "%s/%s", param, entry.d_name);
				}
				if(templn[flen - 1] == '/') templn[flen--] = NULL;

				cellFsStat(templn, &buf);
				cellRtcSetTime_t(&rDate, buf.st_mtime);

				sz = (unsigned long long)buf.st_size; dir_size += sz;

				is_dir = (buf.st_mode & S_IFDIR); if(is_dir) dirs++;

				flen = add_list_entry(param, plen, tempstr, is_dir, ename, templn, entry.d_name, fsize, rDate, sz, sf, false, show_icon0, is_ps3_http, skip_cmd);

				if((flen == 0) || (flen > _MAX_LINE_LEN)) continue; //ignore lines too long
				strcpy(line_entry[idx].path, tempstr); idx++;
				tlen += (flen + TABLE_ITEM_SIZE);

				if(!working) break;
			}
			cellFsClosedir(fd);
		}

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
		if(plen < 4)
		{
			for(u8 n = 0; n < 5; n++)
			if( (n == 0 && (webman_config->netd0 && webman_config->neth0[0] && webman_config->netp0))
			||	(n == 1 && (webman_config->netd1 && webman_config->neth1[0] && webman_config->netp1))
			||	(n == 2 && (webman_config->netd2 && webman_config->neth2[0] && webman_config->netp2))
#ifdef NET3NET4
			||	(n == 3 && (webman_config->netd3 && webman_config->neth3[0] && webman_config->netp3))
			||	(n == 4 && (webman_config->netd4 && webman_config->neth4[0] && webman_config->netp4))
#endif
			  )
			{
				sprintf(line_entry[idx].path, "dnet%i "
											  "d\" href=\"/net%i\">net%i (%s:%i)</a></td>"
											  "<td> <a href=\"/mount.ps3/net%i\">%s</a> &nbsp; </td><td>11-Nov-2006 11:11"
											,  n, n, n,	n == 1 ? webman_config->neth1 :
														n == 2 ? webman_config->neth2 :
#ifdef NET3NET4
														n == 3 ? webman_config->neth3 :
														n == 4 ? webman_config->neth4 :
#endif
														webman_config->neth0,
														n == 1 ? webman_config->netp1 :
														n == 2 ? webman_config->netp2 :
#ifdef NET3NET4
														n == 3 ? webman_config->netp3 :
														n == 4 ? webman_config->netp4 :
#endif
														webman_config->netp0, n, HTML_DIR); idx++;
			}
		}
 #endif
#endif
		if(idx)
		{   // sort html file entries
			u16 n, m;
			for(n = 0; n < (idx - 1); n++)
				for(m = (n + 1); m < idx; m++)
					if(strncmp(line_entry[n].path, line_entry[m].path, FILE_MGR_KEY_LEN) > 0)
					{
						strcpy(swap, line_entry[n].path);
						strcpy(line_entry[n].path, line_entry[m].path);
						strcpy(line_entry[m].path, swap);
					}
		}

		tlen = buf_len;

		for(u16 m = 0; m < idx; m++)
		{
			if(tlen > BUFFER_SIZE_HTML) break;

			tlen += concat(buffer + tlen, TABLE_ITEM_PREFIX);
			tlen += concat(buffer + tlen, (line_entry[m].path) + FILE_MGR_KEY_LEN);
			tlen += concat(buffer + tlen, TABLE_ITEM_SUFIX);
		}

		buffer += tlen;

		buffer += concat(buffer, "</table>");

		if(plen > 4)
		{
			///////////
			unsigned int effective_disctype = 1;

			if(!is_ps3_http)
			{
				bool show_icon = false;
				if(is_net && jb_games)
				{
					char *p = strchr(param + 12, '/'); if(p) *p = NULL; sprintf(templn, "%s/PS3_GAME/ICON0.PNG", param); show_icon = true;
				}

				if(!show_icon)
				{
					sprintf(templn, "%s/ICON0.PNG", param); show_icon = file_exists(templn);                    // current folder
					if(!show_icon) sprintf(templn, "%s/ICON2.PNG", param); show_icon = file_exists(templn);     // ps3_extra folder
					if(!show_icon)
					{
						char *p = strchr(param + 18, '/'); if(p) *p = NULL;
						sprintf(templn, "%s/PS3_GAME/ICON0.PNG", param); show_icon = file_exists(templn);       // dev_bdvd or jb folder
						if(!show_icon) sprintf(templn, "%s/ICON0.PNG", param); show_icon = file_exists(templn); // game dir
					}
				}

#ifdef COBRA_ONLY
				unsigned int real_disctype, iso_disctype;
#endif

				if(!show_icon && islike(param, "/dev_bdvd"))
				{
#ifdef COBRA_ONLY
					cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);
					if(iso_disctype == DISC_TYPE_PSX_CD) sprintf(templn, "%s", wm_icons[6]); else
					if(iso_disctype == DISC_TYPE_PS2_DVD || iso_disctype == DISC_TYPE_PS2_CD) sprintf(templn, "%s", wm_icons[7]); else
					if(iso_disctype == DISC_TYPE_DVD) sprintf(templn, "%s", wm_icons[9]); else
#endif
					sprintf(templn, "%s", wm_icons[5]); show_icon = true;
				}

				for(u16 m = idx; m < 8; m++) buffer += concat(buffer, "<BR>");

				if(show_icon || show_icon0)
					{urlenc(swap, templn); sprintf(templn, "<script>icon.src=\"%s\";icon.style.display='block';</script>", swap); buffer += concat(buffer, templn);}
			}
			///////////

#ifdef EMBED_JS
			sprintf(tempstr, // popup menu
							"<div id='mnu' style='position:fixed;width:140px;background:#333;display:none;padding:5px;'>"
							"<a id='m0'>%s</a><a id='m1'>%s</a><a id='m2'>%s</a><hr><a id='m3'>%s<br></a><a href=\"javascript:t=prompt('%s',self.location.pathname);if(t.indexOf('/dev_')==0)self.location='/mkdir.ps3'+t\">%s</a><hr><a id='m4'>%s<br></a><a id='m5'>%s<br></a><a id='m6'>%s</a><hr><a id='m7'>%s<br></a><a id='m8'>%s</a></div>"
							"<script>var s,m;window.addEventListener('contextmenu',function(e){if(s)s.color='#ccc';t=e.target,s=t.style,c=t.className,m=mnu.style,p=t.pathname;if(c=='w'||c=='d'){e.preventDefault();s.color='#fff',b='block',n='none';m.display=b;m.left=(e.clientX+12)+'px';y=e.clientY;w=window.innerHeight;m.top=(((y+220)<w)?(y+12):(w-220))+'px';m0.href='/install.ps3'+p;m0.style.display=(p.indexOf('.pkg')>0)?b:n;m1.href='/mount.ps3'+p;m1.style.display=(p.toLowerCase().indexOf('.iso')>0||c=='d'||p.indexOf('/GAME')>0)?b:n;m2.href=p;m2.text=(c=='w')?'Download':'Open';m3.href='/delete.ps3'+p;m4.href='/cut.ps3'+p;m5.href='/cpy.ps3'+p;m6.href='/paste.ps3'+self.location.pathname;m7.href='javascript:rn(\"'+p+'\")';m7.style.display=(p.substring(0,5)=='/dev_')?b:n;m8.href='/copy.ps3'+p}},false);window.onclick=function(e){if(m)m.display=n;};"

							// F2 = rename/move item pointed with mouse
							"document.addEventListener('keyup',ku,false);"
							"function rn(f){if(f.substring(0,5)=='/dev_'){f=unescape(f);t=prompt('Rename to:',f);if(t&&t!=f)self.location='/rename.ps3'+f+'|'+escape(t)}}"
							"function ku(e){e=e||window.event;if(e.keyCode==113){var a=document.querySelectorAll('a:hover')[0].pathname;rn(a);}}"

						 	"</script>",
							"Install PKG", "Mount", "Open", "Delete", "New Folder", "New Folder", "Cut", "Copy", "Paste", "Rename", "Copy To"); buffer += concat(buffer, tempstr);
#else
			// add fm.js script
			if(file_exists(FM_SCRIPT_JS))
			{
				sprintf(templn, SCRIPT_SRC_FMT, FM_SCRIPT_JS); buffer += concat(buffer, templn);
			}
#endif

			// show last mounted game
			memset(tempstr, 0, _4KB_); memset(templn, 0, _MAX_PATH_LEN);
			if(effective_disctype != DISC_TYPE_NONE && IS(param, "/dev_bdvd") && file_exists(WMTMP "/last_game.txt"))
			{
				int fd = 0;

				if(cellFsOpen(WMTMP "/last_game.txt", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
				{
					u64 bytes_read = 0;
					cellFsRead(fd, (void *)templn, _MAX_PATH_LEN, &bytes_read);
					cellFsClose(fd);

					if(bytes_read > 10) {sprintf(tempstr, "<span style=\"position:absolute;right:8px\"><font size=2>"); add_breadcrumb_trail(tempstr, templn); strcat(tempstr, "</font></span>");}
				}
			}

			///////////
			char *slash = strchr(param + 1, '/');
			if(slash) *slash = NULL;

			sprintf(templn, "<hr>"
							"<b>" HTML_URL ":", param, param);

			buffer += concat(buffer, templn);

			if(param[1] != 'n')
			{
				sprintf(templn, " %'d %s", (int)(get_free_space(param)>>20), STR_MBFREE);
				buffer += concat(buffer, templn);
			}

			// summary
			sprintf(templn, "</b> &nbsp; <font color=\"#707070\">%'i Dir(s) %'d %s %'d %s</font>%s",
							MAX(dirs - 1, 0), (idx-dirs), STR_FILES,
							dir_size<(_1MB_) ? (int)(dir_size>>10):(int)(dir_size>>20),
							dir_size<(_1MB_) ? STR_KILOBYTE:STR_MEGABYTE, tempstr);

			strcat(buffer, templn);
			///////////
		}
		else
		{

#ifndef LITE_EDITION
			strcat(buffer, "<a onclick=\"o=lg.style,o.display=(o.display=='none')?'block':'none';\" style=\"cursor:pointer;\">");
#endif
			buffer += concat(buffer, HTML_BLU_SEPARATOR
									 "webMAN - Simple Web Server" EDITION "<p>");

#ifndef LITE_EDITION
			if(cellFsOpen(WMTMP "/last_games.bin", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
			{
				buffer += concat(buffer, "<div id=\"lg\" style=\"display:none\">");

				_lastgames lastgames; memset(&lastgames, 0, sizeof(_lastgames));

				cellFsRead(fd, (void *)&lastgames, sizeof(_lastgames), NULL);
				cellFsClose(fd);

				u8 n, m;
				for(n = 0; n < MAX_LAST_GAMES; n++)
				{
					if(file_exists(lastgames.game[n]) == false) *lastgames.game[n] = NULL;
				}

				for(n = 0; n < (MAX_LAST_GAMES - 1); n++)
					for(m = (n + 1); m < MAX_LAST_GAMES; m++)
						if(*lastgames.game[n] == '/' && *lastgames.game[m] == '/' && (strcasecmp(strrchr(lastgames.game[n], '/'), strrchr(lastgames.game[m], '/')) > 0))
						{
							strncpy(swap, lastgames.game[n], MAX_PATH_LEN);
							strncpy(lastgames.game[n], lastgames.game[m], MAX_PATH_LEN);
							strncpy(lastgames.game[m], swap, MAX_PATH_LEN);
						}

				for(n = 0; n < MAX_LAST_GAMES; n++)
				{
					if(*lastgames.game[n]) {sprintf(tempstr, "<a class=\"%c\" href=\"/mount.ps3%s\">%s</a><br>", isDir(lastgames.game[n]) ? 'd' : 'w', lastgames.game[n], strrchr(lastgames.game[n], '/') + 1); buffer += concat(buffer, tempstr);}
				}

				strcat(buffer, "</div>");
			}
#endif
		}
	}
	return true;
}

#undef ext5
#undef ext8
#undef ext13
