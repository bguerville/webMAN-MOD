#ifdef PKG_HANDLER

// /install.ps3<pkg-path>
// /download.ps3?url=<url>
// /download.ps3?to=<path>&url=<url>

#include <cell/http.h>

#include "../vsh/xmb_plugin.h"
#include "../vsh/game_ext_plugin.h"

#include "../vsh/webbrowser_plugin.h"
#include "../vsh/webrender_plugin.h"
#include "../vsh/download_plugin.h"
#include "../vsh/stdc.h"

static char pkg_path[MAX_PATH_LEN];
static wchar_t pkg_dpath[MAX_PATH_LEN];
static wchar_t pkg_durl[MAX_PATH_LEN];

static bool wmget = false;

#define DEFAULT_PKG_PATH 		"/dev_hdd0/packages/"
#define INT_HDD_ROOT_PATH 		"/dev_hdd0/"

static int LoadPluginById(int id, void *handler)
{
	if(xmm0_interface == 0) // getting xmb_plugin xmm0 interface for loading plugin sprx
	{
		xmm0_interface = (xmb_plugin_xmm0 *)plugin_GetInterface(View_Find("xmb_plugin"),'XMM0');
	}
	return xmm0_interface->LoadPlugin3(id, handler,0);
}

static int UnloadPluginById(int id, void *handler)
{
	if(xmm0_interface == 0) // getting xmb_plugin xmm0 interface for loading plugin sprx
	{
		xmm0_interface = (xmb_plugin_xmm0 *)plugin_GetInterface(View_Find("xmb_plugin"),'XMM0');
	}
	return xmm0_interface->Shutdown(id, handler,1);
}

static void unloadSysPluginCallback(void)
{
	//Add potential callback process
	//show_msg((char *)"plugin shutdown via xmb call launched");
}

static void downloadPKG_thread(void)
{

	if(download_interface == 0) // test if download_interface is loaded for interface access
	{
		download_interface = (download_plugin_interface *)plugin_GetInterface(View_Find("download_plugin"),1);
	}

	download_interface->DoUnk5(0, pkg_durl, pkg_dpath);
}

static void installPKG_thread(void)
{
	if(game_ext_interface == 0) // test if game_ext_plugin is loaded for interface access
	{
		game_ext_interface = (game_ext_plugin_interface *)plugin_GetInterface(View_Find("game_ext_plugin"),1);
	}
	game_ext_interface->DoUnk0(); // Load Page
	game_ext_interface->DoUnk34(pkg_path); // install PKG from path
}

static void download_file(char *param, bool wmget, char *msg)
{
	char msg_durl[MAX_PATH_LEN] = "";   //////Conversion Debug msg
	char msg_dpath[MAX_PATH_LEN] = "";  //////Conversion Debug msg
	char pdpath[MAX_PATH_LEN] = "";
	char pdurl[MAX_PATH_LEN] = "";

	size_t conv_num_durl = 0;
	size_t conv_num_dpath = 0;
	size_t pdpath_len;
	size_t pdurl_len;
	size_t ptemp_len;
	size_t dparam_len;

	wmemset(pkg_dpath, 0, MAX_PATH_LEN);
	wmemset(pkg_durl, 0, MAX_PATH_LEN); // Use wmemset from stdc.h instead of reinitialising wchar_t with a loop.

	memset(pdurl, 0, MAX_PATH_LEN);
	memset(pdpath, 0, MAX_PATH_LEN);

	sprintf(msg_durl,  (const char *)"ERROR: Invalid URL\n");
	sprintf(msg_dpath, (const char *)"Download canceled\n");

	if(islike(param + 13, "?to="))  //Use of the optional parameter
	{
		char *ptemp = strstr(param,(const char *)"&url=");

		if(ptemp != NULL)
		{
				ptemp_len = strlen((const char *)ptemp);
				pdurl_len = ptemp_len - 5;
				if((pdurl_len > 0) && (pdurl_len < MAX_PATH_LEN))
				{
					strncpy(pdurl, ptemp + 5, pdurl_len);
				}
				else
				{
					goto end_download_process;
				}
		}
		else
		{
			sprintf(msg_durl, (const char *)"ERROR: No URL given\n");
			goto end_download_process;
		}

		dparam_len = strlen((const char *)param + 13);
		pdpath_len = dparam_len - ptemp_len - 4;

		if(pdpath_len > 0) strncpy(pdpath, (const char *)param + 17, pdpath_len);

		conv_num_durl = mbstowcs((wchar_t *)pkg_durl,(const char *)pdurl,pdurl_len+1);  //size_t stdc_FCAC2E8E(wchar_t *dest, const char *src, size_t max)

	}
	else if(islike(param+13, "?url=")) //
	{
		pdurl_len=strlen(param)-18;
		if((pdurl_len>0) && (pdurl_len<MAX_PATH_LEN))
		{
			pdpath_len=strlen((const char *)DEFAULT_PKG_PATH);
			strncpy(pdpath,(const char *)DEFAULT_PKG_PATH,pdpath_len);
			strncpy(pdurl,param+18,pdurl_len);
			conv_num_durl = mbstowcs((wchar_t *)pkg_durl,(const char *)pdurl,pdurl_len+1);  //size_t stdc_FCAC2E8E(wchar_t *dest, const char *src, size_t max)
		}
		else
		{
			goto end_download_process;
		}
	}
	else
	{
		goto end_download_process;
	}

	if(conv_num_durl > 0)
	{
		if((pdpath_len > 0) && (pdpath_len < MAX_PATH_LEN) && isDir((const char *)pdpath))
		{
			conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath,(const char *)pdpath,pdpath_len+1);
			sprintf(msg_dpath, (const char *)"To: %s\n", (const char *)pdpath);
		}
		else if((pdpath_len > 0) && (pdpath_len < MAX_PATH_LEN))
		{
			//Try to create the folder in proposed path
			//if success set given path if failure set default path
			if(cellFsMkdir(pdpath, DMODE) == CELL_FS_SUCCEEDED)
			{
				conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath,(const char *)pdpath,pdpath_len+1);
				sprintf(msg_dpath, (const char *)"To: %s\n", (const char *)pdpath);
			}
			else
			{
				if(isDir((const char *)DEFAULT_PKG_PATH))
				{
					conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath, (const char *)DEFAULT_PKG_PATH, strlen((const char *)DEFAULT_PKG_PATH)+1);
					sprintf(msg_dpath, (const char *)"To: %s\n", (const char *)DEFAULT_PKG_PATH);
				}
				else
				{
					//create default dir
					//if success set default path if failure use /dev_hdd0 & show message explaining error + download will be in /dev_hdd0/tmp/downloader
					if(cellFsMkdir((const char *)DEFAULT_PKG_PATH, DMODE) == CELL_FS_SUCCEEDED)
					{

						conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath, (const char *)DEFAULT_PKG_PATH, strlen((const char *)DEFAULT_PKG_PATH)+1);
						sprintf(msg_dpath, (const char *)"To: %s\n", (const char *)DEFAULT_PKG_PATH);
					}
					else
					{
						conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath, (const char *)INT_HDD_ROOT_PATH, strlen((const char *)INT_HDD_ROOT_PATH) + 1);
						sprintf(msg_dpath, (const char *)"To: %s\n", (const char *)INT_HDD_ROOT_PATH);
					}
				}
			}
		}
		else
		{
			if(isDir((const char *)DEFAULT_PKG_PATH))
			{
				conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath, (const char *)DEFAULT_PKG_PATH, strlen((const char *)DEFAULT_PKG_PATH)+1);
				sprintf(msg_dpath, (const char *)"To: %s\n", (const char *)DEFAULT_PKG_PATH);
			}
			else
			{
				//create default dir
				//if success set default path if failure use /dev_hdd0 & show message explaining error + download will be in /dev_hdd0/tmp/downloader
				if(cellFsMkdir((char *)DEFAULT_PKG_PATH, DMODE) == CELL_FS_SUCCEEDED)
				{
					conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath, (const char *)DEFAULT_PKG_PATH, strlen((const char *)DEFAULT_PKG_PATH)+1);
					sprintf(msg_dpath, (const char *)"To: %s\n", (const char *)DEFAULT_PKG_PATH);
				}
				else
				{
					conv_num_dpath = mbstowcs((wchar_t *)pkg_dpath, (const char *)INT_HDD_ROOT_PATH, strlen((const char *)INT_HDD_ROOT_PATH)+1);
					sprintf(msg_dpath, (const char *)"To: %s\n", (const char *)INT_HDD_ROOT_PATH);
				}
			}
		}

		if(conv_num_dpath > 0)
		{
			int ret=-1;
			if (View_Find("webrender_plugin"))
			{
				ret = UnloadPluginById(0x1C,(void *)unloadSysPluginCallback);
				sys_timer_usleep(5);
			}
			if (View_Find("webbrowser_plugin"))
			{
				ret = UnloadPluginById(0x1B,(void *)unloadSysPluginCallback);
				sys_timer_usleep(5);
			}

			sprintf(msg_durl, (const char *)"Downloading: %s\n", (const char *)pdurl);

			ret = LoadPluginById(0x29, (void *)downloadPKG_thread);
		}
		else
		{
			sprintf(msg_durl, (const char *)"ERROR: Setting storage location\n");
		}
	}
	else
	{
		sprintf(msg_durl, (const char *)"ERROR: Invalid URL\n");
	}

end_download_process:
	sprintf(msg, "%s%s", msg_durl, msg_dpath);
}

static int installPKG(char *pkgpath, char *msg)
{
	int ret = FAILED;

	if(View_Find("game_plugin"))
	{
		sprintf(msg, (const char *)"ERROR: install from XMB");
		return ret;
	}

	sprintf(msg, (const char *)"ERROR: invalid pkg path");

	size_t pkg_path_len = strlen(pkgpath);

	if (pkg_path_len < MAX_PATH_LEN)
	{
		memset(pkg_path, 0, MAX_PATH_LEN);
		strcpy(pkg_path, pkgpath);

		if( file_exists(pkg_path) )
		{
			if(!extcasecmp(pkg_path, ".pkg", 4)) //check if file has a .pkg extension or not and treat accordingly
			{
				if (View_Find("webrender_plugin"))
				{
					ret = UnloadPluginById(0x1C, (void *)unloadSysPluginCallback);
					sys_timer_usleep(5);
				}
				if (View_Find("webbrowser_plugin"))
				{
					ret = UnloadPluginById(0x1B, (void *)unloadSysPluginCallback);
					sys_timer_usleep(5);
				}
				ret = LoadPluginById(0x16, (void *)installPKG_thread);
				sprintf(msg,(const char *)"Installing %s", pkg_path);
			}
		}
	}

	return ret;
}

#endif // #ifdef PKG_HANDLER
