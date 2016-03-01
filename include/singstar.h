#ifdef NOSINGSTAR
static void no_singstar_icon(void);

static void no_singstar_icon(void)
{
	int fd;

	if(cellFsOpendir("/dev_hdd0/tmp/explore/xil2/game", &fd) == CELL_FS_SUCCEEDED)
	{
		u64 read; CellFsDirent dir; char xmlpath[64];
		read = sizeof(CellFsDirent);
		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read) break;
			if(dir.d_name[0]=='.') continue;
			if(strlen(dir.d_name)==2)
			{
				sprintf(xmlpath, "/dev_hdd0/tmp/explore/xil2/game/%s/c/db.xml", dir.d_name);
				cellFsUnlink(xmlpath);
			}
		}
		cellFsClosedir(fd);
	}
}
#endif
