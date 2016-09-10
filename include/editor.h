#ifdef EDITOR

//#define EDITOR_PATH "/dev_hdd0/tmp/editor.ps3"
#define EDITOR_HTML_HEADER_SRC " <!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">" \
								"<html xmlns=\"http://www.w3.org/1999/xhtml\">" \
								"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">" \
								"<meta http-equiv=\"Cache-Control\" content=\"no-cache\">" \
								"<meta name=\"viewport\" content=\"width=device-width,initial-scale=0.6,maximum-scale=1.0\">"  /* size: 369 */

#define EDITOR_HTML_BF_SRC "<h1>Edit your file in the Text Area below then press the Save button.</h1><p>This feature only works with an appropriate version of wMM.</p></div><form action=\"/savefile.ps3\" method=\"get\" name=\"formEdit\" target=\"_self\" id=\"formEdit\"><p align=\"center\"><input type=\"hidden\" name=\"path\" value=\"%s\"><textarea name=\"txt\" id=\"txt\" cols=\"80\" rows=\"30\">%s</textarea></p><p align=\"center\"><input name=\"btnSave\" type=\"submit\" value=\"Save\" /></p></form></body></html>"


static char * make_editor(char * param)
{

    //uint64_t lSize;
    char buffer[1792];
	char retstring[2048]="";
    //size_t result;
	//char * ed_html=NULL;
	int fd;
	
	struct CellFsStat status;
    CellFsErrno check_status = cellFsStat(param, &status);
	
	if (CELL_OK == check_status) 
	{
		if(cellFsOpen(param, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			
			uint64_t read_e = 0;
			uint64_t pos;
			cellFsLseek(fd, 0, CELL_FS_SEEK_SET, &pos);
			//show_msg((char *)"cellFsLseek");
			//memset(buffer, 0, 1792);
			//memset(retstring, 0, 2048);
			//show_msg((char *)"memset done");
			while(working)
			{
				sys_timer_usleep(500);
				
				if(cellFsRead(fd, (void *)buffer, status.st_size, &read_e) == CELL_FS_SUCCEEDED)
				{
					if(read_e>0)
					{
						//show_msg((char *)buffer);
						//if(send(conn_s, buffer, (size_t)read_e, 0)<0) break;
						//show_msg((char *)"cellFsRead");
								//sprintf(retstring, param, (char *)buffer);
						
						
						if (strlen((char *)buffer)>0)
						{
							sprintf(retstring, (char *)EDITOR_HTML_BF_SRC, param, (char *)buffer);
						}
						else {sprintf(retstring, (const char *)"Editor Error");break;}
					}
					else {sprintf(retstring, (const char *)"Editor Error");break;}					
				}
				else {sprintf(retstring, (const char *)"Editor Error");break;}
			}		
		}
		else {sprintf(retstring, (const char *)"Editor Error");}
	}
	else {sprintf(retstring, (const char *)"Editor Error");}
	
	cellFsClose(fd);
	show_msg((char *)retstring);
	return retstring;		
}					
					
/*	
						if(strlen(retstring)>strlen((char *)buffer)) 
						{
							show_msg((char *)retstring);
							
							//save the html file
							//then create a webbrowser instance with the url
							//or create web browser in vsh_menu with edit.ps3 url & filepath parameter
							return retstring;
						}
						else 
						{
							show_msg((char *)"Editor Error3");
							return (char *)"Editor Error!";
						}
					}
					
					
					else 
					{
						show_msg((char *)"Editor Error1");
						return (char *)"Editor Error!";
					}
				}
				else 
				{
					show_msg((char *)"Editor Error2");
					return (char *)"Editor Error!";
				}
			}
		}
		else
		{
			show_msg((char *)"Editor Error4");
			return (char *)"Editor Error!";
		}
	}
	else
	{
		show_msg((char *)"Editor Error5");
		return (char *)"Editor Error!";
	}	
}
*/

static bool save_file(char * param, char * msg)
{
	uint64_t lSize;
    char * buffer=NULL;
	char * retstring=NULL;
    size_t result;
	char * ed_html=NULL;
	int fd;
	char ptxt[2048-MAX_PATH_LEN-11+1]="";
	char ppath[MAX_PATH_LEN+1]="";
	int ptemp_len=0;
	int ppath_len=0;
	int ptxt_len=0;
	bool file_saved = false;
	// /savefile.ps3?path=<path>&txt=<text> 
	if(islike(param, "?path="))  //Use of the optional parameter
	{
		char *ptemp = strstr(param,(const char *)"&txt=");
		if(ptemp != NULL)
		{
				ptemp_len = strlen((const char *)ptemp);
				ptxt_len = ptemp_len - 5;
				ppath_len = strlen(param+6)-ptemp_len;
				if((ptxt_len > 0) && (ptxt_len < 2048-MAX_PATH_LEN-11+1) && (ppath_len > 0))  //modify 2048  remove delimiters+ppath_len
				{
					strncpy(ptxt, ptemp + 5, ptxt_len);
					strncpy(ppath, param+6, ppath_len);
				}
				else
				{
					sprintf(msg, (const char *)"Problem retrieving file data");
					goto end_edit_process;
				}
		}
		else
		{
			sprintf(msg, (const char *)"Problem retrieving text data");
			goto end_edit_process;
		}
	}
	else
		{
			sprintf(msg, (const char *)"Problem retrieving path data");
			goto end_edit_process;
		}
	
	
	//Check file path, save text file then return.

	// if save file is successful, set file_saved to true & set msg.
	
	
end_edit_process:
return file_saved;
	
	
}

#endif //#ifdef EDITOR