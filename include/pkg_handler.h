#ifdef PKG_HANDLER

// /install.ps3<pkg-path>
// /download.ps3?url=<url>
// /download.ps3?to=<path>&url=<url>

#include <cell/http.h>

#include "../vsh/xmb_plugin.h"
#include "../vsh/game_ext_plugin.h"
#include "../vsh/download_plugin.h"
#include "../vsh/stdc.h"

#define MAX_URL_LEN    360
#define MAX_DLPATH_LEN 240

#define MAX_PKGPATH_LEN 240
static char pkg_path[MAX_PKGPATH_LEN];

static wchar_t pkg_durl[MAX_URL_LEN];
static wchar_t pkg_dpath[MAX_DLPATH_LEN];

static u16 pkg_dcount = 0;
static u8 pkg_auto_install = 0;

#define INT_HDD_ROOT_PATH		"/dev_hdd0/"
#define DEFAULT_PKG_PATH		"/dev_hdd0/packages/"
#define TEMP_DOWNLOAD_PATH		"/dev_hdd0/tmp/downloader/"

typedef struct {
   u32 magic; // 0x53434500//
   u32 version;
   u16 sdk_type;
   u16 SCE_header_type;
   u32 meta_offset;
   u64 size; // size of sce_hdr + sizeof meta_hdr
   u64 pkg_size;
 } _pkg_header;

static int LoadPluginById(int id, void *handler)
{
	if(xmm0_interface == 0) // getting xmb_plugin xmm0 interface for loading plugin sprx
	{
		xmm0_interface = (xmb_plugin_xmm0 *)plugin_GetInterface(View_Find("xmb_plugin"), 'XMM0');
	}
	return xmm0_interface->LoadPlugin3(id, handler,0);
}

static int UnloadPluginById(int id, void *handler)
{
	if(xmm0_interface == 0) // getting xmb_plugin xmm0 interface for loading plugin sprx
	{
		xmm0_interface = (xmb_plugin_xmm0 *)plugin_GetInterface(View_Find("xmb_plugin"), 'XMM0');
	}
	return xmm0_interface->Shutdown(id, handler,1);
}

static void unloadSysPluginCallback(void)
{
	//Add potential callback process
	//show_msg((char *)"plugin shutdown via xmb call launched");
}

static void unload_web_plugins(void)
{
	if (View_Find("webrender_plugin"))
	{
		UnloadPluginById(0x1C, (void *)unloadSysPluginCallback);
		sys_timer_usleep(5);
	}
	if (View_Find("webbrowser_plugin"))
	{
		UnloadPluginById(0x1B, (void *)unloadSysPluginCallback);
		sys_timer_usleep(5);
	}

#ifdef VIRTUAL_PAD
	if(IS_ON_XMB)
	{
		int enter_button = 0;
		xsetting_0AF1F161()->GetEnterButtonAssign(&enter_button);

		if(enter_button)
			parse_pad_command("circle", 0);
		else
			parse_pad_command("cross", 0);
	}
#endif
}

static void downloadPKG_thread(void)
{

	if(download_interface == 0) // test if download_interface is loaded for interface access
	{
		download_interface = (download_plugin_interface *)plugin_GetInterface(View_Find("download_plugin"),1);
	}
	download_interface->DownloadURL(0, pkg_durl, pkg_dpath);
}

static void installPKG_thread(void)
{
	if(game_ext_interface == 0) // test if game_ext_plugin is loaded for interface access
	{
		game_ext_interface = (game_ext_plugin_interface *)plugin_GetInterface(View_Find("game_ext_plugin"),1);
	}
	game_ext_interface->LoadPage();
	game_ext_interface->installPKG(pkg_path);
}

static int download_file(const char *param, char *msg)
{
	int ret = FAILED;

	if(IS_INGAME)
	{
		sprintf(msg, "ERROR: download from XMB");
		return ret;
	}

	char *msg_durl = msg;
	char *msg_dpath = msg + MAX_URL_LEN + 16;

	char pdurl[MAX_URL_LEN];
	char pdpath[MAX_DLPATH_LEN];

	size_t conv_num_durl = 0;
	size_t conv_num_dpath = 0;

	int pdurl_len;

	size_t dparam_len;
	int pdpath_len;

	wmemset(pkg_durl, 0, MAX_URL_LEN); // Use wmemset from stdc.h instead of reinitialising wchar_t with a loop.
	wmemset(pkg_dpath, 0, MAX_DLPATH_LEN);

	memset(pdurl, 0, MAX_URL_LEN);
	memset(pdpath, 0, MAX_DLPATH_LEN);

	sprintf(msg_durl,  "ERROR: Invalid URL");
	sprintf(msg_dpath, "Download canceled");

	if(islike(param, "?to="))  //Use of the optional parameter
	{
		char *ptemp = strstr((char*)param + 4, "&url=");

		if(ptemp != NULL)
		{
			pdurl_len = strlen(ptemp + 5);

			if((pdurl_len > 0) && (pdurl_len < MAX_URL_LEN))
			{
				strncpy(pdurl, ptemp + 5, pdurl_len);
			}
			else
				goto end_download_process;
		}
		else
			goto end_download_process;

		dparam_len = strlen(param);
		pdpath_len = dparam_len - pdurl_len - 5 - 4;

		if(pdpath_len > 0) strncpy(pdpath, param + 4, pdpath_len);

		conv_num_durl = mbstowcs((wchar_t *)pkg_durl, (const char *)pdurl, pdurl_len + 1);  //size_t stdc_FCAC2E8E(wchar_t *dest, const char *src, size_t max)

	}
	else if(islike(param, "?url="))
	{
		pdurl_len = strlen(param + 5);
		if((pdurl_len > 0) && (pdurl_len < MAX_URL_LEN))
		{
			pdpath_len = strlen(DEFAULT_PKG_PATH);
			strncpy(pdpath, DEFAULT_PKG_PATH, pdpath_len);
			strncpy(pdurl, param + 5, pdurl_len);
			conv_num_durl = mbstowcs((wchar_t *)pkg_durl,(const char *)pdurl, pdurl_len + 1);  //size_t stdc_FCAC2E8E(wchar_t *dest, const char *src, size_t max)
		}
	}

	if(conv_num_durl > 0)
	{
		mkdir_tree(pdpath);

		if((pdpath_len > 0) && (pdpath_len < MAX_DLPATH_LEN) && (isDir(pdpath) || cellFsMkdir(pdpath, DMODE) == CELL_FS_SUCCEEDED)) ;

		else if(isDir(DEFAULT_PKG_PATH) || cellFsMkdir(pdpath, DMODE) == CELL_FS_SUCCEEDED)
		{
			pdpath_len = sprintf(pdpath, DEFAULT_PKG_PATH);
		}
		else
		{
			pdpath_len = sprintf(pdpath, INT_HDD_ROOT_PATH);
		}

		sprintf(msg_dpath, "To: %s", pdpath, pdpath_len);
		if(IS(pdpath, DEFAULT_PKG_PATH) && (strstr(pdurl, ".pkg") != NULL))
		{
			pdpath_len = sprintf(pdpath, TEMP_DOWNLOAD_PATH); pkg_dcount++;
		}

		conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath, (const char *)pdpath, pdpath_len + 1);

		if(conv_num_dpath > 0)
		{
			unload_web_plugins();

			sprintf(msg_durl, "%s%s", "Downloading ", pdurl);

			LoadPluginById(0x29, (void *)downloadPKG_thread);
			ret = CELL_OK;
		}
		else
			sprintf(msg_durl, "ERROR: Setting storage location");
	}

end_download_process:
	sprintf(msg, "%s\n%s", msg_durl, msg_dpath);
	return ret;
}

static int installPKG(const char *pkgpath, char *msg)
{
	int ret = FAILED;
	if(IS_INGAME)
	{
		sprintf(msg, "ERROR: install from XMB");
		return ret;
	}

	size_t pkg_path_len = strlen(pkgpath);

	if (pkg_path_len < MAX_PKGPATH_LEN)
	{
		if(islike(pkgpath, "/net"))
		{
			cache_file_to_hdd((char*)pkgpath, pkg_path, "/tmp/downloader", msg); pkg_dcount++;
		}
		else
		if(*pkgpath == '?')
		{
			pkg_auto_install = 1;
			download_file(pkgpath, msg);
			ret = CELL_OK;
		}
		else
			snprintf(pkg_path, MAX_PKGPATH_LEN, "%s", pkgpath);

		if( file_exists(pkg_path) )
		{
			if(!extcasecmp(pkg_path, ".pkg", 4)) //check if file has a .pkg extension or not and treat accordingly
			{
				unload_web_plugins();

				sprintf(msg, "%s%s", "Installing ", pkg_path);

				LoadPluginById(0x16, (void *)installPKG_thread);
				ret = CELL_OK;
			}
		}
	}

	if(ret) sprintf(msg, "ERROR: %s", pkgpath);
	return ret;
}

static int installPKG_combo(char *msg)
{
	int fd, ret = FAILED;

	if(cellFsOpendir(DEFAULT_PKG_PATH, &fd) == CELL_FS_SUCCEEDED)
	{
		char pkgfile[MAX_PKGPATH_LEN];

		CellFsDirent dir; u64 read_e;

		if(file_exists(pkg_path)) {sprintf(pkgfile, "%s.bak", pkg_path); cellFsRename(pkg_path, pkgfile); pkg_path[0] = NULL;}

		while((cellFsReaddir(fd, &dir, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
		{
			if(!extcasecmp(dir.d_name, ".pkg", 4))
			{
				sprintf(pkgfile, "%s%s", DEFAULT_PKG_PATH, dir.d_name); ret = 0; { BEEP1 }

				installPKG(pkgfile, msg); show_msg(msg);
				break;
			}
		}
		cellFsClosedir(fd);

		if(ret == FAILED) { BEEP2 } else sys_timer_sleep(2);
	}

	return ret;
}

static void poll_downloaded_pkg_files(char *msg)
{
	if(pkg_dcount)
	{
		CellFsDirent entry; u64 read_e; int fd; u16 pkg_count = 0;

		if(cellFsOpendir(TEMP_DOWNLOAD_PATH, &fd) == CELL_FS_SUCCEEDED)
		{
			while((cellFsReaddir(fd, &entry, &read_e) == CELL_FS_SUCCEEDED) && (read_e > 0))
			{
				if(!extcmp(entry.d_name, ".pkg", 4))
				{
					int fdl = 0; char *dlfile = msg; _pkg_header pkg_header;
					sprintf(dlfile, "%s%s", TEMP_DOWNLOAD_PATH, entry.d_name); pkg_count++;
					cellFsChmod(dlfile, MODE);

					if(cellFsOpen(dlfile, CELL_FS_O_RDONLY, &fdl, NULL, 0) == CELL_FS_SUCCEEDED)
					{
						if(cellFsRead(fdl, (void *)&pkg_header, sizeof(pkg_header), NULL) == CELL_FS_SUCCEEDED)
						{
							cellFsClose(fdl);

							struct CellFsStat s;
							if(cellFsStat(dlfile, &s) == CELL_FS_SUCCEEDED && pkg_header.pkg_size == s.st_size)
							{
								char pkgfile[MAX_PATH_LEN]; u16 pkg_len, retry = 0;
								pkg_len = sprintf(pkgfile, "%s%s", DEFAULT_PKG_PATH, dlfile + strlen(TEMP_DOWNLOAD_PATH));
								while(cellFsRename(dlfile, pkgfile) != CELL_FS_SUCCEEDED && retry < 100)
								{
									sprintf(pkgfile + pkg_len - 4, " (%i).pkg", retry); retry++;
								}
								pkg_dcount--;

								if(pkg_auto_install) installPKG(pkgfile, msg);
							}
						}
						else
							cellFsClose(fdl);
					}
				}
			}
			cellFsClosedir(fd);

			if(pkg_count == 0) pkg_auto_install = pkg_dcount = 0; // disable polling if no pkg files were found (e.g. changed to background download)
		}
	}
}
#endif // #ifdef PKG_HANDLER
