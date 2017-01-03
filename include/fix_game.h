#define LATEST_CFW	4.81f
#define CFW_420		0xA410ULL /* 42000 */
#define MIN_CFW		0x2710ULL /* 10000 */
#define MAX_CFW		0xD6D8ULL /* 55000 */

static bool fix_in_progress = false;
static bool fix_aborted = false;

#ifdef COPY_PS3
static u32 fixed_count = 0;
#endif

enum FIX_GAME_MODES
{
	FIX_GAME_AUTO,
	FIX_GAME_QUICK,
	FIX_GAME_FORCED,
	FIX_GAME_DISABLED
};

enum SFO_Operation_Codes
{
	FIX_SFO,
	SHOW_WARNING,
	GET_TITLE_AND_ID,
	GET_VERSION,
	GET_TITLE_ID_ONLY
};

static bool is_sfo(unsigned char *mem)
{
	return (mem[1]=='P' && mem[2]=='S' && mem[3]=='F');
}

#define READ_SFO_HEADER(ret) \
	if(is_sfo(mem) == false) return ret; \
	u16 pos, str, fld, dat, indx = 0; \
	fld=str=(mem[0x8]+(mem[0x9]<<8)); \
	dat=pos=(mem[0xc]+(mem[0xd]<<8));

#define FOR_EACH_SFO_FIELD() \
	while(pos < sfo_size) \
	{ \
		if((str>=dat) || (mem[str]==0)) break;

#define READ_NEXT_SFO_FIELD() \
		while((str < dat) && mem[str]) str++;str++; \
		pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8)); \
		indx+=0x10; if(indx>=fld) break; \
	}

static void parse_param_sfo(unsigned char *mem, char *titleID, char *title, u16 sfo_size)
{
	READ_SFO_HEADER()

	memset(titleID, 0, 10);
	memset(title, 0, 64);

	u8 fcount = 0;

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "TITLE_ID", 8))
		{
			strncpy(titleID, (char *)mem + pos, 9);
#ifndef ENGLISH_ONLY
			if(*TITLE_XX == NULL)
#endif
				if(++fcount >= 2) break;
		}
		else
		if(!memcmp((char *) &mem[str], "TITLE", 6))
		{
			strncpy(title, (char *)mem + pos, 63);
#ifndef ENGLISH_ONLY
			if(*TITLE_XX == NULL)
#endif
				if(++fcount >= 2) break;
		}
#ifndef ENGLISH_ONLY
		else
		if(!memcmp((char *) &mem[str], TITLE_XX, 9))
		{
			strncpy(title, (char *)mem + pos, 63);
			if(++fcount > 2) break;
		}
#endif
		READ_NEXT_SFO_FIELD()
	}

	if(webman_config->tid && (titleID[8] != NULL) && (*titleID == 'B' || *titleID == 'N'))
	{
		strcat(title, " ["); strcat(title, titleID); strcat(title, "]");
	}
}

static bool fix_param_sfo(unsigned char *mem, char *titleID, u8 opcode, u16 sfo_size)
{
	READ_SFO_HEADER(false)

	memset(titleID, 0, 10);

#ifdef FIX_GAME
	u8 fcount = 0;
#endif

	bool ret = false;

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "TITLE_ID", 8))
		{
			strncpy(titleID, (char *) &mem[pos], 9);
#ifdef FIX_GAME
			if(opcode == GET_TITLE_ID_ONLY) break;
			fcount++; if(fcount >= 2) break;
#else
			break;
#endif
		}
#ifdef FIX_GAME
		else
		if(!memcmp((char *) &mem[str], "PS3_SYSTEM_VER", 14))
		{
			char version[8];
			strncpy(version, (char *) &mem[pos], 7);
			int fw_ver = 10000 * ((version[1] & 0xFF) - '0') + 1000 * ((version[3] & 0xFF) - '0') + 100 * ((version[4] & 0xFF) - '0');
			if((c_firmware >= 4.20f && c_firmware < LATEST_CFW) && (fw_ver > (int)(c_firmware * 10000.0f)))
			{
				if(opcode == SHOW_WARNING) {char text[64]; sprintf(text, "WARNING: Game requires firmware version %i.%i", (fw_ver / 10000), (fw_ver - 10000*(fw_ver / 10000)) / 100); show_msg((char*)text); break;}

				mem[pos + 1] = '4', mem[pos + 3] = '2', mem[pos + 4] = '0'; ret = true;
			}
			fcount++; if(fcount>=2) break;
		}
#endif

		READ_NEXT_SFO_FIELD()
	}

	return ret;
}

static void get_app_ver(unsigned char *mem, char *version, u16 sfo_size)
{
	READ_SFO_HEADER()

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "APP_VER", 7))
		{
			strncpy(version, (char *) &mem[pos], 6);
			break;
		}

		READ_NEXT_SFO_FIELD()
	}
}

static bool getTitleID(char *filename, char *titleID, u8 opcode)
{
	bool ret = false;

	memset(titleID, 0, 10);
	char paramsfo[_4KB_]; unsigned char *mem = (u8*)paramsfo;

	uint64_t sfo_size = read_file(filename, paramsfo, _4KB_, 0);

	if(sfo_size)
	{
		// get titleid
		if(opcode == GET_VERSION)
			get_app_ver(mem, titleID, (u16)sfo_size);                 // get game version (app_ver)
		else
		if(opcode == GET_TITLE_AND_ID)
			parse_param_sfo(mem, titleID, filename, (u16)sfo_size);   // get titleid & return title in the file name (used to backup games in _mount.h)
		else
		{
			ret = fix_param_sfo(mem, titleID, opcode, (u16)sfo_size); // get titleid & show warning if game needs to fix PS3_SYSTEM_VER

			if(ret && opcode == FIX_SFO) save_file(filename, paramsfo, sfo_size);
		}
	}
	return ret;
}

#ifdef FIX_GAME
static bool fix_sfo_attribute(unsigned char *mem, u16 sfo_size)
{
	READ_SFO_HEADER(false)

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "ATTRIBUTE", 9))
		{
			if(webman_config->fixgame==FIX_GAME_FORCED) {mem[pos+2]|=0x2; mem[pos+4]|=0xA5; return true;} // PS3_EXTRA + BGM + RemotePlay
			if(!(mem[pos+2] & 2)) {mem[pos+2]|=0x2; return true;}                                         // PS3_EXTRA
			break;
		}

		READ_NEXT_SFO_FIELD()
	}

	return false;
}

char fix_game_path[7][256]; int plevel = NONE;

static void fix_game_folder(char *path)
{
	if(plevel >= 6) return; // limit recursion up to 6 levels

	int fd;

	if(working && cellFsOpendir(path, &fd) == CELL_FS_SUCCEEDED)
	{
		plevel++;

#ifdef COPY_PS3
		sprintf(current_file, "%s", path);
#endif

		struct CellFsStat s;
		CellFsDirent dir; uint64_t read_e;

		while(working && (cellFsReaddir(fd, &dir, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
		{
			if(fix_aborted) break;
			if(dir.d_name[0] == '.') continue;

			sprintf(fix_game_path[plevel], "%s/%s", path, dir.d_name);

			if(!extcasecmp(dir.d_name, ".sprx", 5) || !extcasecmp(dir.d_name, ".self", 5) || IS(dir.d_name, "EBOOT.BIN"))
			{
				if(cellFsStat(fix_game_path[plevel], &s) != CELL_FS_SUCCEEDED || s.st_size < 0x500) continue;

				int fdw, offset; uint64_t bytes_read = 0; u64 ps3_sys_version = 0;

				cellFsChmod(fix_game_path[plevel], MODE); fixed_count++; //fix file read-write permission

				if(cellFsOpen(fix_game_path[plevel], CELL_FS_O_RDWR, &fdw, NULL, 0) == CELL_FS_SUCCEEDED)
				{
					cellFsLseek(fdw, 0xC, CELL_FS_SEEK_SET, &bytes_read);
					cellFsRead(fdw, (void *)&offset, 4, &bytes_read); offset-=0x78;

				retry_offset:
					if(offset < 0x90 || offset > 0x800) offset=!extcasecmp(dir.d_name, ".sprx", 5) ? 0x258 : 0x428;
					cellFsLseek(fdw, offset, CELL_FS_SEEK_SET, &bytes_read);
					cellFsRead(fdw, (void *)&ps3_sys_version, 8, NULL);

					if(ps3_sys_version > CFW_420 && ps3_sys_version < MAX_CFW)
					{
						if(offset != 0x278 && offset != 0x428)
						{
							offset = (offset == 0x258) ? 0x278 : 0; goto retry_offset;
						}

						ps3_sys_version = CFW_420;

						cellFsLseek(fdw, offset, CELL_FS_SEEK_SET, &bytes_read);
						cellFsWrite(fdw, (void*)(&ps3_sys_version), 8, NULL);
					}
					cellFsClose(fdw);
				}
				cellFsClose(fdw);
			}
			else if(isDir(fix_game_path[plevel]) && (webman_config->fixgame!=FIX_GAME_QUICK)) fix_game_folder(fix_game_path[plevel]);

			sys_timer_usleep(1000);
		}

		cellFsClosedir(fd);
		plevel--;
	}
}

#ifdef COBRA_ONLY
static uint64_t getlba(const char *s1, u16 n1, const char *s2, u16 n2, u16 *start)
{
	for(u16 n = *start + 0x1F; n < (n1 - n2); n++)
	{
		if(memcmp(&s1[n], s2, n2) == 0)
		{
			while(n > 0x1D && s1[n--] != 0x01); n-=0x1C, fixed_count++;
			u32 lba = (s1[n+0] & 0xFF) + ((s1[n+1] & 0xFF) << 8) + ((s1[n+2] & 0xFF) << 16) + ((s1[n+3] & 0xFF) << 24);
			*start = n + 0x1C + n2;
			return lba;
		}
	}
	return 0;
}

static void fix_iso(char *iso_file, uint64_t maxbytes, bool patch_update)
{
	struct CellFsStat buf;

	if(islike(iso_file, "/net") || strstr(iso_file, ".ntfs[")) ; else
	if(fix_aborted || cellFsStat(iso_file, &buf) != CELL_FS_SUCCEEDED) return;

	int fd; char titleID[10], update_path[MAX_PATH_LEN];

#ifdef COPY_PS3
	sprintf(current_file, "%s", iso_file);
#endif

	cellFsChmod(iso_file, MODE); //fix file read-write permission

	if(cellFsOpen(iso_file, CELL_FS_O_RDWR, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		uint64_t chunk_size=_4KB_; char chunk[chunk_size]; u64 ps3_sys_version;
		uint64_t bytes_read = 0, lba = 0, pos=0xA000ULL;

		bool fix_sfo = true, fix_eboot = true, fix_ver = false;

		uint64_t size = buf.st_size;
		if(maxbytes > 0 && size > maxbytes) size = maxbytes;
		if(size > pos) size -= pos; else size = 0;

		u16 start, offset;

		while(size > 0ULL)
		{
			if(fix_aborted) break;

			if(fix_sfo)
			{
				cellFsLseek(fd, pos, CELL_FS_SEEK_SET, &bytes_read);
				cellFsRead(fd, chunk, chunk_size, &bytes_read); if(!bytes_read) break;

				start = 0, lba = getlba(chunk, chunk_size, "PARAM.SFO;1", 11, &start);

				if(lba)
				{
					lba*=0x800ULL, fix_sfo = false;
					cellFsLseek(fd, lba, CELL_FS_SEEK_SET, &bytes_read);
					cellFsRead(fd, (void *)&chunk, chunk_size, &bytes_read); if(!bytes_read) break;

					fix_ver = fix_param_sfo((unsigned char *)chunk, titleID, FIX_SFO, (u16)bytes_read);

					if(patch_update)
					{
						sprintf(update_path, "%s%s/USRDIR/EBOOT.BIN", HDD0_GAME_DIR, titleID); // has update on hdd0?
						if(file_exists(update_path)) fix_ver = false;
					}

					if(fix_ver)
					{
						cellFsLseek(fd, lba, CELL_FS_SEEK_SET, &bytes_read);
						cellFsWrite(fd, chunk, chunk_size, &bytes_read);
					}
					else goto exit_fix; //do not fix if sfo version is ok

					if(size > lba) size = lba;

					sprintf(chunk, "%s %s", STR_FIXING, iso_file);
					show_msg(chunk);

					start = 0, lba = getlba(chunk, chunk_size, "PS3_DISC.SFB;1", 14, &start), lba *= 0x800ULL, chunk_size = 0x800; //1 sector
					if(lba > 0 && size > lba) size = lba;
				}
			}

			for(u8 t = (fix_eboot ? 0 : 1); t < 5; t++)
			{
				cellFsLseek(fd, pos, CELL_FS_SEEK_SET, &bytes_read);
				cellFsRead(fd, chunk, chunk_size, &bytes_read); if(!bytes_read) break;

				lba = start = 0;

				while(true)
				{
					sys_timer_usleep(1000);
					if(fix_aborted) goto exit_fix;

					if(t==0) lba = getlba(chunk, chunk_size, "EBOOT.BIN;1", 11, &start);
					if(t==1) lba = getlba(chunk, chunk_size, ".SELF;1", 7, &start);
					if(t==2) lba = getlba(chunk, chunk_size, ".self;1", 7, &start);
					if(t==3) lba = getlba(chunk, chunk_size, ".SPRX;1", 7, &start);
					if(t==4) lba = getlba(chunk, chunk_size, ".sprx;1", 7, &start);

					if(lba)
					{
						#define IS_EBOOT	(t == 0)
						#define IS_SPRX		(t >= 3)

						if(IS_EBOOT) fix_eboot = false;

						lba *= 0x800ULL;
						cellFsLseek(fd, lba, CELL_FS_SEEK_SET, &bytes_read);
						cellFsRead(fd, (void *)&chunk, chunk_size, &bytes_read); if(!bytes_read) break;

						memcpy(&offset, chunk + 0xC, 4); offset-=0x78;

						for(u8 retry = 0; retry < 3; retry++)
						{
							if(retry == 1) {if(IS_SPRX) offset = 0x258; else offset = 0x428;}
							if(retry == 2) {if(IS_SPRX) offset = 0x278; else break;}

							memcpy(&ps3_sys_version, chunk + offset, 8); if(ps3_sys_version >= MIN_CFW && ps3_sys_version < MAX_CFW) break;
						}

						if(ps3_sys_version > CFW_420 && ps3_sys_version < MAX_CFW)
						{
							ps3_sys_version = CFW_420;
							cellFsLseek(fd, lba+offset, CELL_FS_SEEK_SET, &bytes_read);
							cellFsWrite(fd, (void*)(&ps3_sys_version), 8, NULL);
						}
						else goto exit_fix;

						if(IS_EBOOT) break;

						#undef IS_EBOOT
						#undef IS_SPRX

					} else break;
				}
			}

			if(!bytes_read) break;

			pos  += chunk_size;
			size -= chunk_size;
			if(chunk_size > size) chunk_size = (int) size;

			sys_timer_usleep(1000);
		}
exit_fix:
		cellFsClose(fd);
	}
	else
	{
		get_name(update_path, strrchr(iso_file, '/') + 1, GET_WMTMP); strcat(update_path, ".SFO\0");
		getTitleID(update_path, titleID, GET_TITLE_ID_ONLY);
	}

	// fix update folder
	sprintf(update_path, "%s%s/PARAM.SFO", HDD0_GAME_DIR, titleID);
	if(getTitleID(update_path, titleID, FIX_SFO) || webman_config->fixgame==FIX_GAME_FORCED) {sprintf(update_path, "%s%s/USRDIR", HDD0_GAME_DIR, titleID); fix_game_folder(update_path);}
}
#endif //#ifdef COBRA_ONLY

static void fix_game(char *game_path, char *titleID, uint8_t fix_type)
{
	memset(titleID, 0, 10);

	if(file_exists(game_path) || islike(game_path, "/net") || strstr(game_path, ".ntfs["))
	{
		fix_in_progress = true, fix_aborted = false, fixed_count = 0;

#ifdef COBRA_ONLY
		if(!extcasecmp(game_path, ".iso", 4) || !extcasecmp(game_path, ".iso.0", 6))
			fix_iso(game_path, 0x100000UL, false);
		else
#endif //#ifdef COBRA_ONLY

		{
			// -- get TitleID from PARAM.SFO
			char filename[MAX_PATH_LEN];

			char *p = strstr(game_path, "/PS3_GAME"); if(!p) p = strstr(game_path, "/USRDIR"); if(p) *p = NULL;

			if(islike(game_path, "/net") || strstr(game_path, ".ntfs["))
				{get_name(filename, strrchr(game_path, '/') + 1, GET_WMTMP); strcat(filename, ".SFO\0");}
			else
				sprintf(filename, "%s/PARAM.SFO", game_path);

			if(file_exists(filename) == false) sprintf(filename, "%s/PS3_GAME/PARAM.SFO", game_path);
			if(file_exists(filename) == false) {waitfor("/dev_bdvd", 10); sprintf(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");}

			char paramsfo[_4KB_]; unsigned char	*mem = (u8*)paramsfo;
			uint64_t bytes_read = 0;

			bytes_read = read_file(filename, paramsfo, _4KB_, 0);
			if(is_sfo(mem))
			{
				// fix ps3 extra or bgm + remoteplay + ps3 extra
				char tmp_path[MAX_PATH_LEN]; sprintf(tmp_path, "%s/PS3_EXTRA", game_path); bool has_ps3_extra = isDir(tmp_path);
				if((fix_type == FIX_GAME_FORCED || (has_ps3_extra && fix_type != FIX_GAME_DISABLED)) && fix_sfo_attribute(mem, (u16)bytes_read))
				{
					save_file(filename, paramsfo, bytes_read);
				}

				tmp_path[10] = NULL;

				// get titleid & fix game folder if version is higher than cfw
				if((fix_param_sfo(mem, titleID, FIX_SFO, (u16)bytes_read) || fix_type == FIX_GAME_FORCED) && fix_type != FIX_GAME_DISABLED && !islike(tmp_path, "/net") && !islike(tmp_path, "/dev_bdvd") && !strstr(game_path, ".ntfs["))
				{
					save_file(filename, paramsfo, bytes_read);

					sprintf(filename, "%s %s", STR_FIXING, game_path);
					show_msg(filename);

					sprintf(filename, "%s/PS3_GAME/USRDIR", game_path);  // fix bdvd game

					fix_game_folder(filename);
				}

				// fix PARAM.SFO on hdd0
				for(u8 i = 0; i < 2; i++)
				{
					sprintf(filename, "%s%s%s/PARAM.SFO", HDD0_GAME_DIR, titleID, i ? "" : "/C00");

					bytes_read = read_file(filename, paramsfo, _4KB_, 0);
					if(is_sfo(mem))
					{
						// fix ps3 extra or bgm + remoteplay + ps3 extra
						if((fix_type == FIX_GAME_FORCED || (has_ps3_extra && fix_type != FIX_GAME_DISABLED)) && fix_sfo_attribute(mem, (u16)bytes_read))
						{
							save_file(filename, paramsfo, bytes_read);
						}

						if((fix_param_sfo(mem, titleID, FIX_SFO, (u16)bytes_read) || fix_type == FIX_GAME_FORCED) && fix_type!=FIX_GAME_DISABLED)
						{
							save_file(filename, paramsfo, bytes_read);

							if(i == 0) continue;

							sprintf(filename, "%s %s%s", STR_FIXING, HDD0_GAME_DIR, titleID);
							show_msg(filename);

							sprintf(filename, "%s%s/USRDIR", HDD0_GAME_DIR, titleID);  // fix update folder in /dev_hdd0/game

							fix_game_folder(filename);
						}
					}
				}
			}
			// ----

		}

		fix_in_progress = false, fixed_count = 0;

		if(webman_config->fixgame == FIX_GAME_FORCED) {webman_config->fixgame=FIX_GAME_QUICK; save_settings();}
	}
}

#endif //#ifdef FIX_GAME
