#define SC_FS_LINK						(810)

#define SC_STORAGE_OPEN 				(600)
#define SC_STORAGE_CLOSE 				(601)
#define SC_STORAGE_INSERT_EJECT			(616)

#define NO_MSG							NULL

int file_copy(const char *file1, char *file2, uint64_t maxbytes);

static bool copy_in_progress = false;
static bool dont_copy_same_size = true; // skip copy the file if it already exists in the destination folder with the same file size

static u32 copied_count = 0;

#define COPY_WHOLE_FILE		0
#define SAVE_ALL			0
#define APPEND_TEXT			(-0xADD0ADD0ADD000ALL)
#define DONT_CLEAR_DATA		-1
#define RECURSIVE_DELETE	2

static int sysLv2FsLink(const char *oldpath, const char *newpath)
{
	system_call_2(SC_FS_LINK, (u64)(u32)oldpath, (u64)(u32)newpath);
	return_to_user_prog(int);
}

static uint64_t get_free_space(const char *dev_name)
{
	uint32_t blockSize;
	uint64_t freeSize;
	if(cellFsGetFreeSize(dev_name, &blockSize, &freeSize)  == CELL_FS_SUCCEEDED) return (freeSize * blockSize);
	return 0;
}

static int isDir(const char* path)
{
	struct CellFsStat s;
	if(cellFsStat(path, &s) == CELL_FS_SUCCEEDED)
		return ((s.st_mode & CELL_FS_S_IFDIR) != 0);
	else
		return 0;
}

static bool file_exists(const char* path)
{
	struct CellFsStat s;
	return (cellFsStat(path, &s) == CELL_FS_SUCCEEDED);
}

static void mkdir_tree(char *path)
{
	size_t path_len = strlen(path);
	for(u16 p = 12; p < path_len; p++)
		if(path[p] == '/') {path[p] = NULL; cellFsMkdir((char*)path, MODE); path[p] = '/';}
}

size_t read_file(const char *file, char *data, size_t size, int32_t offset)
{
	int fd = 0; uint64_t pos, read_e = 0;

	if(offset < 0) offset = 0; else memset(data, 0, size);

	if(cellFsOpen(file, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		if(cellFsLseek(fd, offset, CELL_FS_SEEK_SET, &pos) == CELL_FS_SUCCEEDED)
		{
			if(cellFsRead(fd, (void *)data, size, &read_e) != CELL_FS_SUCCEEDED) read_e = 0;
		}
		cellFsClose(fd);
	}

	return read_e;
}

int save_file(const char *file, const char *mem, int64_t size)
{
	int fd = 0; u32 flags = CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY;
	cellFsChmod(file, MODE);

	if( size < 0 )  {flags = CELL_FS_O_APPEND | CELL_FS_O_CREAT | CELL_FS_O_WRONLY; size = (size == APPEND_TEXT) ? 0 : -size;} else
	if(!extcmp(file, "/PARAM.SFO", 10)) flags = CELL_FS_O_CREAT | CELL_FS_O_WRONLY;

	if(cellFsOpen(file, flags, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		if((size <= SAVE_ALL) && mem) size = strlen(mem);

		if(size) cellFsWrite(fd, (void *)mem, size, NULL);
		cellFsClose(fd);

		cellFsChmod(file, MODE);

		return CELL_FS_SUCCEEDED;
	}

	return FAILED;
}

static void filepath_check(char *file)
{
	if(file[5] == 'u' && islike(file, "/dev_usb"))
	{
		u16 n = 8, c = 8;
		// remove invalid chars
		while(true)
		{
			if(file[c] == '\\') file[c] = '/';
			if(strchr("\"<|>:*?", file[c]) == NULL) file[n++] = file[c];
			if(!file[c++]) break;
		}
	}
}

/*
static int file_concat(const char *file1, char *file2)
{
	struct CellFsStat buf;
	int fd1, fd2;
	int ret = FAILED;

	filepath_check(file2);

	if(islike(file1, "/dvd_bdvd"))
		{system_call_1(36, (uint64_t) "/dev_bdvd");} // decrypt dev_bdvd files

	if(cellFsStat(file1, &buf) != CELL_FS_SUCCEEDED) return ret;

	if(cellFsOpen(file1, CELL_FS_O_RDONLY, &fd1, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		uint64_t size = buf.st_size;

		sys_addr_t sysmem = 0; uint64_t chunk_size = _64KB_;

		if(sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
		{
			// append
			if(cellFsOpen(file2, CELL_FS_O_CREAT | CELL_FS_O_RDWR | CELL_FS_O_APPEND, &fd2, 0, 0) == CELL_FS_SUCCEEDED)
			{
				char *chunk = (char*)sysmem;
				uint64_t read = 0, written = 0, pos=0;
				copy_aborted = false;

				while(size > 0)
				{
					if(copy_aborted) break;

					cellFsLseek(fd1, pos, CELL_FS_SEEK_SET, &read);
					cellFsRead(fd1, chunk, chunk_size, &read);

					cellFsWrite(fd2, chunk, read, &written);
					if(!written) {break;}

					pos+=written;
					size-=written;
					if(chunk_size>size) chunk_size=(int) size;
				}
				cellFsClose(fd2);

				if(copy_aborted)
					cellFsUnlink(file2); //remove incomplete file
				else
					{cellFsChmod(file2, MODE); copied_count++;}

				ret=size;
			}
			sys_memory_free(sysmem);
		}
		cellFsClose(fd1);
	}

	return ret;
}
*/

int file_copy(const char *file1, char *file2, uint64_t maxbytes)
{
	struct CellFsStat buf, buf2;
	int fd1, fd2;
	int ret = FAILED;
	copy_aborted = false;

	filepath_check(file2);

	if(IS(file1, file2)) return FAILED;

#ifdef COPY_PS3
	sprintf(current_file, "%s", file2);
#endif

	if(cellFsStat(file1, &buf) != CELL_FS_SUCCEEDED)
	{
#ifndef LITE_EDITION
#ifdef COBRA_ONLY
		if(islike(file1, "/net"))
		{
			int ns = connect_to_remote_server((file1[4] & 0xFF) - '0');
			copy_net_file(file2, (char*)file1 + 5, ns, maxbytes);
			if(ns>=0) {shutdown(ns, SHUT_RDWR); socketclose(ns);}

			if(file_exists(file2)) return 0;
		}
#endif
#endif
		return FAILED;
	}

	if(islike(file1, "/dev_hdd0/") && islike(file2, "/dev_hdd0/"))
	{
		cellFsUnlink(file2); copied_count++;
		return sysLv2FsLink(file1, file2);
	}

	if(islike(file1, "/dvd_bdvd"))
		{system_call_1(36, (uint64_t) "/dev_bdvd");} // decrypt dev_bdvd files

	// skip if file already exists with same size
	if(dont_copy_same_size && (cellFsStat(file2, &buf2) == CELL_FS_SUCCEEDED) && (buf2.st_size == buf.st_size))
	{
		copied_count++;
		return buf.st_size;
	}

	if(buf.st_size > get_free_space("/dev_hdd0")) return FAILED;

	if(cellFsOpen(file1, CELL_FS_O_RDONLY, &fd1, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		sys_addr_t sysmem = 0; uint64_t chunk_size = _64KB_;

		if(sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == CELL_OK)
		{
			uint64_t size = buf.st_size, part_size = buf.st_size; u8 part = 0;
			if(maxbytes > 0 && size > maxbytes) size = maxbytes;

			if((part_size > 0xFFFFFFFFULL) && islike(file2, "/dev_usb"))
			{
				if(!extcasecmp(file2, ".iso", 4)) strcat(file2, ".0"); else strcat(file2, ".66600");
				part++; part_size = 0xFFFF0000ULL; //4Gb - 64kb
			}

			uint64_t read = 0, written = 0, pos = 0;
			char *chunk = (char*)sysmem;
			u16 flen = strlen(file2);
next_part:
			// copy_file
			if(cellFsOpen(file2, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd2, 0, 0) == CELL_FS_SUCCEEDED)
			{
				while(size > 0)
				{
					if(copy_aborted) break;

					cellFsLseek(fd1, pos, CELL_FS_SEEK_SET, &read);
					cellFsRead(fd1, chunk, chunk_size, &read);
					if(!read) break;

					cellFsWrite(fd2, chunk, read, &written);
					if(!written) break;

					pos  += written;
					size -= written;

					if(chunk_size > size) chunk_size = (int) size;

					part_size -= written;
					if(part_size == 0) break;

					sys_timer_usleep(1000);
				}
				cellFsClose(fd2);

				if(copy_aborted)
					cellFsUnlink(file2); //remove incomplete file
				else if((part > 0) && (size > 0))
				{
					if(part < 10)
						file2[flen-1] = '0' + part;
					else if(file2[flen-2] == '.')
					{
						file2[flen-1] = '0' + (u8)(part / 10);
						file2[flen  ] = '0' + (u8)(part % 10);
						file2[flen+1] = 0;
					}
					else
					{
						file2[flen-2] = '0' + (u8)(part / 10);
						file2[flen-1] = '0' + (u8)(part % 10);
					}
					part++; part_size = 0xFFFF0000ULL;
					goto next_part;
				}
				else
					{cellFsChmod(file2, MODE); copied_count++;}

				ret = size;
			}
			sys_memory_free(sysmem);
		}
		cellFsClose(fd1);
	}

	return ret;
}

#ifdef COPY_PS3
static int folder_copy(const char *path1, char *path2)
{
	filepath_check(path2);

	cellFsChmod(path1, DMODE);
	cellFsMkdir(path2, DMODE);

	int fd;

	copy_aborted = false;

	if(working && cellFsOpendir(path1, &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirent dir; u64 read_e;

		char source[MAX_PATH_LEN];
		char target[MAX_PATH_LEN];

		while(working && (cellFsReaddir(fd, &dir, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
		{
			if(copy_aborted) break;
			if(dir.d_name[0] == '.' && (dir.d_name[1] == '.' || dir.d_name[1] == NULL)) continue;

			sprintf(source, "%s/%s", path1, dir.d_name);
			sprintf(target, "%s/%s", path2, dir.d_name);

			if(isDir(source))
			{
				if(IS(source, "/dev_bdvd/PS3_UPDATE")) {cellFsMkdir(target, DMODE); continue;} // just create /PS3_UPDATE without its content
				folder_copy(source, target);
			}
			else
				file_copy(source, target, COPY_WHOLE_FILE);
		}
		cellFsClosedir(fd);

		if(copy_aborted) return FAILED;
	}
	else
		return FAILED;

	return CELL_FS_SUCCEEDED;
}
#endif

#ifndef LITE_EDITION
static int del(const char *path, u8 recursive)
{
	if(recursive == RECURSIVE_DELETE) ; else
	if(!sys_admin) return FAILED;

	if(!isDir(path)) return cellFsUnlink(path);

	if(strlen(path) < 11 || islike(path, "/dev_bdvd") || islike(path, "/dev_flash") || islike(path, "/dev_blind")) return FAILED;

	int fd;

	copy_aborted = false;

	if(working && cellFsOpendir(path, &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirent dir; u64 read_e;

		char entry[MAX_PATH_LEN];

		while(working && (cellFsReaddir(fd, &dir, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
		{
			if(copy_aborted) break;
			if(dir.d_name[0] == '.' && (dir.d_name[1] == '.' || dir.d_name[1] == NULL)) continue;

			sprintf(entry, "%s/%s", path, dir.d_name);

			if(isDir(entry))
				{if(recursive) del(entry, recursive);}
			else
				cellFsUnlink(entry);
		}
		cellFsClosedir(fd);

		if(copy_aborted) return FAILED;
	}
	else
		return FAILED;

	if(recursive) cellFsRmdir(path);

	return CELL_FS_SUCCEEDED;
}
#endif

int waitfor(const char *path, uint8_t timeout)
{
	struct CellFsStat s;
	for(uint8_t n = 0; n < (timeout * 4); n++)
	{
		if(*path && cellFsStat(path, &s) == CELL_FS_SUCCEEDED) return CELL_FS_SUCCEEDED;
		if(!working) break; sys_timer_usleep(250000);
	}
	return FAILED;
}

static void enable_dev_blind(const char *msg)
{
	if(!sys_admin) return;

	if(!isDir("/dev_blind"))
		{system_call_8(SC_FS_MOUNT, (u64)(char*)"CELL_FS_IOS:BUILTIN_FLSH1", (u64)(char*)"CELL_FS_FAT", (u64)(char*)"/dev_blind", 0, 0, 0, 0, 0);}

	if(!msg) return;

	show_msg((char*)msg);
	sys_timer_sleep(2);
}

static void disable_dev_blind(void)
{
	system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_blind", 0, 1);
}

static void unlink_file(const char *drive, const char *path, const char *file)
{
	char filename[64];
	sprintf(filename, "%s/%s%s", drive, path, file); cellFsUnlink(filename);
}

#if defined(WM_CUSTOM_COMBO) || defined(WM_REQUEST)
static bool do_custom_combo(const char *filename)
{
 #if defined(WM_CUSTOM_COMBO)
	char combo_file[MAX_PATH_LEN];

	if(*filename == '/')
		sprintf(combo_file, "%s", filename);
	else
		sprintf(combo_file, "%s%s", WM_CUSTOM_COMBO, filename); // use default path
 #else
	const char *combo_file = filename;
 #endif

	if(file_exists(combo_file))
	{
		file_copy(combo_file, (char*)WMREQUEST_FILE, COPY_WHOLE_FILE);

		loading_html++;
		sys_ppu_thread_t t_id;
		if(working) sys_ppu_thread_create(&t_id, handleclient, WM_FILE_REQUEST, THREAD_PRIO, THREAD_STACK_SIZE_64KB, SYS_PPU_THREAD_CREATE_NORMAL, THREAD_NAME_WEB);

		return true;
	}
	return false;
}
#endif

static void delete_history(bool delete_folders)
{
	int fd; char path[64];

	if(cellFsOpendir("/dev_hdd0/home", &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirent dir; u64 read_e;

		while(working && (cellFsReaddir(fd, &dir, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
		{
			unlink_file("/dev_hdd0/home", dir.d_name, "/etc/boot_history.dat");
			unlink_file("/dev_hdd0/home", dir.d_name, "/etc/community/CI.TMP");
			unlink_file("/dev_hdd0/home", dir.d_name, "/community/MI.TMP");
			unlink_file("/dev_hdd0/home", dir.d_name, "/community/PTL.TMP");
		}
		cellFsClosedir(fd);
	}

	unlink_file("/dev_hdd0", "vsh/pushlist/", "game.dat");
	unlink_file("/dev_hdd0", "vsh/pushlist/", "patch.dat");

	if(!delete_folders || !working) return;

	for(u8 p = 0; p < 10; p++)
	{
		sprintf(path, "%s/%s", drives[0], paths[p]); cellFsRmdir(path);
		strcat(path, AUTOPLAY_TAG); 				 cellFsRmdir(path);
	}
	cellFsRmdir("/dev_hdd0/PKG");
}

#ifdef COPY_PS3
static void import_edats(const char *path1, const char *path2)
{
	cellFsMkdir(path2, DMODE);

	if(!isDir(path2)) return;

	int fd; bool from_usb;

	copy_aborted = false;
	from_usb = islike(path1, "/dev_usb");

	if(cellFsOpendir(path1, &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirent dir; u64 read_e;

		char source[MAX_PATH_LEN];
		char target[MAX_PATH_LEN];

		while(working && (cellFsReaddir(fd, &dir, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
		{
			if(copy_aborted) break;
			if((strstr(dir.d_name, ".edat") == NULL) || !extcmp(dir.d_name, ".bak", 4)) continue;

			sprintf(source, "%s/%s", path1, dir.d_name);
			sprintf(target, "%s/%s", path2, dir.d_name);

			if(file_exists(target) == false)
				file_copy(source, target, COPY_WHOLE_FILE);

			if(from_usb && file_exists(target))
				{sprintf(target, "%s.bak", source); cellFsRename(source, target);}
		}
		cellFsClosedir(fd);
	}
	else
		return;

	return;
}
#endif
