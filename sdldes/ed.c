/********************************************************
*							*
*		Mark Utility Functions			*
*		==== ======= =========			*
*                                                       *
* Ed_SetUpMarkCoords - initialises all the variables    *
*		associated with a mark, according to    *
*		the type of mark. The function should	*
*		be called before calling any of the	*
*		other three.				*
* 		Called from:	Ed_WinRefresh	 	*
*				Ed_BlockOp	 	*
*							*
* Ed_MarkStart - returns, for a given line, the         *
*		starting column of the current mark     *
*		within that line.                       *
*		Called from:	Ed_WinRefresh		*
*				Ed_BlockOp		*
*                                                       *
* Ed_MarkStop - returns, for a given line, the ending   *
*		column of the current mark within that  *
*		line, or -1 if the mark extends to the	*
*		end of the line.			*
*		Called from:	Ed_WinRefresh		*
*				Ed_BlockOp		*
*                                                       *
* Ed_IsMarked - returns TRUE or FALSE depending on      *
*		whether the specified line is           *
*		marked or not.				*
*		Called from:	Ed_WinRefresh		*
*							*
*********************************************************/

LOCAL void NEAR Ed_SetUpMarkCoords()
{
	if (Ed_MarkType!=ED_NOMARK) {
		if (Ed_LineNow>Ed_MarkLine) {
			Ed_MarkLBegin=Ed_MarkLine;
			Ed_MarkLEnd=Ed_LineNow;
			Ed_MarkCBegin=Ed_MarkCol;
			Ed_MarkCEnd=Ed_ColNow;
		} else	{
			Ed_MarkLBegin=Ed_LineNow;
			Ed_MarkLEnd=Ed_MarkLine;
			Ed_MarkCBegin=Ed_ColNow;
			Ed_MarkCEnd=Ed_MarkCol;
		}
		if (Ed_MarkType==ED_COLMARK || Ed_MarkLBegin==Ed_MarkLEnd) {
			if (Ed_MarkCBegin>Ed_MarkCEnd) {
				short tmp=Ed_MarkCBegin;
				Ed_MarkCBegin=Ed_MarkCEnd;
				Ed_MarkCEnd=tmp;
			}
		}
	} else	{
		Ed_MarkLBegin = Ed_MarkLEnd = (-1L);
	}
}

#ifdef ANSI_C
  LOCAL short NEAR Ed_MarkStart(short line)
#else
  LOCAL short NEAR Ed_MarkStart(line)
  short line;
#endif
{
	if (line==Ed_MarkLBegin || Ed_MarkType==ED_COLMARK)
		return Ed_MarkCBegin;
	else return 0;
}

#ifdef ANSI_C
  LOCAL short NEAR Ed_MarkStop(short line)
#else
  LOCAL short NEAR Ed_MarkStop(line)
  short line;
#endif
{
	if (line==Ed_MarkLEnd || Ed_MarkType==ED_COLMARK)
		return Ed_MarkCEnd;
	else return (-1);	/* -1 => mark to end of line */
}

#ifdef ANSI_C
  LOCAL BOOLEAN NEAR Ed_IsMarked(short line)
#else
  LOCAL BOOLEAN NEAR Ed_IsMarked(line)
  short line;
#endif
{
	if (Ed_MarkType!=ED_NOMARK)
		if (line>=Ed_MarkLBegin && line<=Ed_MarkLEnd)
			return TRUE;
	return FALSE;
}

/****************************************
* Bracket, brace and parenthesis moves	*
*****************************************/

GLOBAL char LeftSearch[ ED_SEARCHSETLEN]={ '[',']','{','}','(',')',0 };
GLOBAL char RightSearch[ED_SEARCHSETLEN]={ '[',']','{','}','(',')',0 };

GLOBAL void Ed_LeftSearch()
{
	Ed_SavePosition();
	Ed_Left(); /* To prevent matching at current cursor position */

	while (Ed_LineNow!=0 || Ed_ColNow!=0) {
		if (ED_TEXT[Ed_ColNow])
			if (strrchr(LeftSearch,ED_TEXT[Ed_ColNow])!=NULL)
				break;
		if ( --Ed_ColNow < 0) {
			(void)Ed_Point(Ed_LineNow-1,FALSE);
			Ed_ColNow = (short)ED_LEN;
		}
	}

	/* Check line 0, col 0 if necessary */

	if (strrchr(LeftSearch,ED_TEXT[Ed_ColNow])!=NULL) {
		Ed_PrefCol = Ed_ColNow;
		Ed_ReCursor();
	} else Ed_RestorePosition();
}

GLOBAL void Ed_RightSearch()
{
	BOOLEAN	found = FALSE;
	short	len;
	Ed_SavePosition();
	Ed_Right();

	len = (short)ED_LEN;
	while (Ed_LineNow!=Ed_LastLine || Ed_ColNow!=len) {
		if (ED_TEXT[Ed_ColNow])
			if (strrchr(RightSearch,ED_TEXT[Ed_ColNow])!=NULL) {
				found=TRUE;
				break;
			}
		if (++Ed_ColNow > len) {
			Ed_ColNow = 0;
			(void)Ed_Point(Ed_LineNow+1,FALSE);
			len = (short)ED_LEN;
		}
	}
	if (!found) Ed_RestorePosition();
	else	{
		Ed_PrefCol = Ed_ColNow;
		Ed_ReCursor();
	}
}

/************************
* Search and Translate	*
************************/

char	GLOBAL	Ed_SearchText[ED_SEARCHTEXTLEN];
char	GLOBAL	Ed_ReplaceText[ED_SEARCHTEXTLEN];
char	GLOBAL	Ed_RevSearchText[ED_SEARCHTEXTLEN];
GLOBAL	BOOLEAN	Ed_Reversed		=FALSE;
GLOBAL	BOOLEAN	Ed_MadeUpperCase	=FALSE;
GLOBAL 	BOOLEAN	Ed_CaseSensitive	=TRUE;
GLOBAL	BOOLEAN	Ed_IdentsOnly		=FALSE;
LOCAL	BOOLEAN	Ed_ForwardFound		=FALSE;

GLOBAL void Ed_SearchForward()
{
	short startpos = (short)strlen(Ed_SearchText)-1;
	short matchpos = Ed_ColNow+startpos+1;
	Ed_ForwardFound = FALSE;
	if (!Ed_CaseSensitive && !Ed_MadeUpperCase) {
		(void)strupr(Ed_SearchText);
		Ed_MadeUpperCase=TRUE;
	}
	Ed_SavePosition();
	while (!Ed_ForwardFound) {
		while (matchpos>=ED_LEN && Ed_LineNow<Ed_LastLine) {
			(void)Ed_Point(Ed_LineNow+1,FALSE);
			matchpos=startpos;
		}
		matchpos = Gen_IsMatch(ED_TEXT,Ed_SearchText,matchpos,
					Ed_CaseSensitive, Ed_IdentsOnly);
		if (matchpos==-1)
			if (Ed_LineNow==Ed_LastLine) break;
			else matchpos=ED_LEN;
		else Ed_ForwardFound=TRUE;
	}
	if (!Ed_ForwardFound) {
		Ed_RestorePosition();
		Ed_ShowMessage("No Match");
		beep();
	} else Ed_ChgCol(matchpos-startpos);
}

GLOBAL void Ed_SearchBackward()
{
	BOOLEAN found=FALSE;
	short startpos=(short)strlen(Ed_SearchText)-1;
	short matchpos=ED_LEN-Ed_ColNow+startpos;
	char RevLine[256];
	Ed_SavePosition();
	strcpy(RevLine,ED_TEXT);
	if (!Ed_Reversed) {
		strcpy(Ed_RevSearchText, Ed_SearchText);
		(void)strrev(Ed_RevSearchText);
		Ed_Reversed=TRUE;
	}
	if (!Ed_CaseSensitive && !Ed_MadeUpperCase) {
		(void)strupr(Ed_RevSearchText);
		Ed_MadeUpperCase=TRUE;
	}
	while (!found) {
		while (matchpos<startpos && Ed_LineNow>0) {
			(void)Ed_Point(Ed_LineNow-1,FALSE);
			strcpy(RevLine,ED_TEXT);
			matchpos=startpos;
		}
		(void)strrev(RevLine);
		matchpos = Gen_IsMatch(RevLine,Ed_RevSearchText,matchpos,
					Ed_CaseSensitive, Ed_IdentsOnly);
		if (matchpos==-1) {
			if (Ed_LineNow==0) break;
		} else found=TRUE;
	}
	if (!found) {
		Ed_RestorePosition();
		Ed_ShowMessage("No Match");
		beep();
	} else Ed_ChgCol(ED_LEN-1-matchpos);
}


GLOBAL void Ed_Translate()
{
	BOOLEAN global=FALSE;
	short numreps=0;
	BOOLEAN doreplace;
	short cut_end = (int)strlen(Ed_SearchText)-1;
	short rep_len = (int)strlen(Ed_ReplaceText);
	ushort reply;
	char msg[80];
	short origline = Ed_LineNow;
	short origcol=Ed_ColNow;
	while (Ed_SearchForward(), Ed_ForwardFound) {
		doreplace=FALSE;
		if (!global) {
			Win_PutAttribute(ED_INVRS_COLOR,Ed_Win->c,Ed_Win->r,cut_end+1,1);
			reply=Gen_GetResponse(Msg_Win,"Replace?","(YNG)",TRUE,5,HLP_TRANS);
			Win_PutAttribute(ED_INSIDE_COLOR,Ed_Win->c,Ed_Win->r,cut_end+1,1);
			if (reply==KB_ESC) break;
			if (reply=='g' || reply=='G') global=TRUE;
			if (reply=='y' || reply=='Y') doreplace=TRUE;
		} else doreplace=TRUE;
		if (doreplace) {
			numreps++;
			Ed_CutCols(Ed_ColNow, Ed_ColNow+cut_end,FALSE);
			Ed_InsertStr(Ed_ReplaceText);
			Ed_ChgCol(Ed_ColNow+rep_len);
		}
	}
	sprintf(msg,"%d replacements ( %s / %s ) made",numreps,Ed_SearchText,
					Ed_ReplaceText);
	Ed_ShowMessage(msg);
	Ed_FixCursor(origline,origcol);
}

/********
* Undo	*
********/

GLOBAL void Ed_Undo()
{
	char *stmp;
	short slen = (int)strlen(Ed_UndoBuff);
	Ed_FileSize += (long)(slen - ED_LEN);
	Ed_Allocate(STRING,stmp,slen+1,51);
	strcpy(stmp,Ed_UndoBuff);
	Ed_ReplaceLine(Ed_BufNow,stmp,slen);
	Ed_ChgCol(Ed_UndoCol);
	Ed_ShowMessage("Undone");
}

/********************************
*   Low-level Scrap Operations	*
*********************************/

#ifdef ANSI_C
  LOCAL void NEAR Ed_CopyCols(short from, short to)
#else
  LOCAL void NEAR Ed_CopyCols(from, to)
  short from, to;
#endif
{
	short len;
	char tmp[256];
	if (to==(-1) || to>=ED_LEN) to=ED_LEN-1;
	len = to-from+1;
	if (from>=ED_LEN) len=0;
	strncpy(tmp,ED_TEXT+from,(unsigned int)len);
	tmp[len]=NUL;
	(void)Ed_AddLine(&Ed_ScrapBuf,tmp);
}

/********************************
*    Main editing functions	*
********************************/

/********************************
* Column/block mark operations	*
********************************/

GLOBAL void Ed_BlockOp()
{
	short line;

	if (Ed_MarkType==ED_NOMARK) {
		if (Ed_ColNow<ED_LEN)
			if (ed_op!=_Ed_Copy) Ed_CutCols(Ed_ColNow,Ed_ColNow,FALSE);
			else 	{
				Ed_ShowMessage("No mark set");
				beep();
			}
	} else	{
		Ed_ShowMessage(ed_op!=_Ed_Copy ?
	  		"Cut text to scrap" :
	  		"Copied text to scrap");
		Ed_FreeAll(&Ed_ScrapBuf);
		Ed_SetUpMarkCoords();
		Ed_QuietMode=TRUE; /* Prevent WinRefresh from corrupting
					mark coordinates */
		line = Ed_MarkLBegin;
		while (line<=Ed_MarkLEnd) {
			(void)Ed_Point(line,FALSE);
			if (ed_op!=_Ed_Copy)
				Ed_CutCols(Ed_MarkStart(line),Ed_MarkStop(line),TRUE);
			else Ed_CopyCols(Ed_MarkStart(line),Ed_MarkStop(line));
			line++;
		}
		if (ed_op!=_Ed_Copy && Ed_MarkType==ED_BLOKMARK && Ed_MarkLBegin<Ed_MarkLEnd) {
			line=Ed_MarkLBegin;
			while (line<Ed_MarkLEnd) {
				(void)Ed_Point(line++,FALSE);
				if (ED_LEN==0) {
					Ed_JoinLines();
					Ed_MarkLEnd--;
				}
			}
			(void)Ed_Point(Ed_MarkLEnd,FALSE);
			Ed_JoinLines();
		}
		Ed_MarkType=ED_NOMARK;
		Ed_QuietMode=FALSE;
		Ed_RefreshAll=TRUE;
		Ed_FixCursor(Ed_MarkLBegin, Ed_PrefCol = Ed_MarkCBegin);
	}
	if (ed_op!=_Ed_Copy) {
		Ed_FileChanged = TRUE;
		Ed_IsCompiled = FALSE;
	}
}

GLOBAL void Ed_BlockPaste()
{
	Ed_Rewind(&Ed_ScrapBuf);
	if (Ed_ScrapBuf!=NULL) {
		short startline=Ed_LineNow+1;
		BUFPTR scrapstart = Ed_ScrapBuf;
		Ed_QuietMode=TRUE;
		Ed_InsertCR();
		Ed_AllocBuff(&Ed_BufNow,Ed_BufNow->prev,Ed_BufNow);
		while (Ed_ScrapBuf != NULL && Ed_ScrapBuf->next!=NULL) {
			Ed_FileSize+= (long)(2l+Ed_AddLine(&Ed_BufNow,Ed_ScrapBuf->text));
			Ed_LastLine++;
			Ed_LineNow++;
			Ed_ScrapBuf = Ed_ScrapBuf->next;
		}
		Ed_BufNow=Ed_RemoveLine(Ed_BufNow);
		Ed_JoinLines();
		(void)Ed_Point(startline,FALSE);
		Ed_JoinLines();
		Ed_ScrapBuf = scrapstart; /* Can't rewind as currently NULL */
		Ed_QuietMode=FALSE;
		Ed_RefreshAll=TRUE;
		Ed_ShowMessage("Block pasted");
		Ed_WinRefresh();
	} else 	{
		Ed_ShowMessage("No scrap to paste!");
		beep();
	}
}

GLOBAL void Ed_ColumnPaste()
{
	Ed_Rewind(&Ed_ScrapBuf);
	if (Ed_ScrapBuf!=NULL) {
		BUFPTR scrapstart=Ed_ScrapBuf;
		short pastecol=Ed_ColNow;
		char spaceline[256];
		Ed_SavePosition();
		Ed_QuietMode=TRUE;
		while (Ed_ScrapBuf != NULL && Ed_ScrapBuf->next!=NULL /* Skip extra one at end */) {
			register short i=pastecol-ED_LEN;
			if (i>0) {
				spaceline[i]=NUL;
				while (i>0) spaceline[--i]=' ';
				Ed_ColNow=ED_LEN;
				Ed_InsertStr(spaceline);
			}
			Ed_ColNow=pastecol;
			Ed_InsertStr(Ed_ScrapBuf->text);
			if (Ed_LineNow==Ed_LastLine) {
				Ed_AllocBuff(&Ed_BufNow->next,Ed_BufNow,NULL);
				Ed_FileSize+=2;
				Ed_LastLine++;
			}
			(void)Ed_Point(Ed_LineNow+1,FALSE);
			Ed_ScrapBuf = Ed_ScrapBuf->next;
		}
		Ed_RestorePosition();
		Ed_ScrapBuf=scrapstart; /* Restore scrap pointer */
		Ed_QuietMode=FALSE;
		Ed_RefreshAll=TRUE;
		Ed_ShowMessage("Columns pasted");
		Ed_WinRefresh();
	} else 	{
		Ed_ShowMessage("No scrap to paste!");
		beep();
	}
}

/************************
* File Operations	*
*************************/

/* Merge with Ed_LoadFile????? */


#ifdef ANSI_C
  LOCAL void NEAR Ed_LoadList(BUFPTR *b, STRING fname, BOOLEAN mustexist)
#else
  LOCAL void NEAR Ed_LoadList(b, fname, mustexist)
  BUFPTR *b;
  STRING fname;
  BOOLEAN mustexist;
#endif
{
	FILE *fp;
	char buff[256];
	if ((fp=fopen(fname,"rt"))!=NULL) {
		Ed_FreeAll(b);
		while (!feof(fp)) {
			(void)fgets(buff,256,fp);
			if (!feof(fp)) {
				(void)Ed_AddLine(b,buff);
			}
		}
		*b = (*b)->prev;
		Mem_Free((*b)->next);
		(*b)->next = NULL;
		fclose(fp);
	} else if (mustexist) Error(TRUE,ERR_NOOPENFL,fname);
}


#ifdef ANSI_C
  GLOBAL void Ed_LoadScrap(STRING name)
#else
  GLOBAL void Ed_LoadScrap(name)
  STRING name;
#endif
{
	Ed_ShowMessage("Loading scrap");
	Ed_LoadList(&Ed_ScrapBuf, name, TRUE);
}

#ifdef ANSI_C
  GLOBAL void Ed_WriteScrap(STRING name)
#else
  GLOBAL void Ed_WriteScrap(name)
  STRING name;
#endif
{
	FILE *fp;
	STRING line;
	if (Ed_ScrapBuf==NULL) return;
	if ((fp=fopen(name,"wt"))!=NULL) {
		Ed_Rewind(&Ed_ScrapBuf);
		while ((line=Ed_ScrapBuf->text) != NULL) {
			Ed_ScrapBuf = Ed_ScrapBuf->next;
			fputs(line,fp);
			fputs("\n",fp);
			}
		fclose(fp);
		Ed_ShowMessage("Written scrap");
	} else Error(TRUE,ERR_NOOPENFL,name);
}


#define MASKCHECK	0x12348765L

GLOBAL void Ed_SaveConfig()
{
	FILE *cfp;
	short i;
	long Ed_MaskChk = MASKCHECK;
	if ((cfp=fopen("PEW.CFG","wb"))==NULL) return;
	for (i=0;i<(int)_Ed_NumKeyDefs;i++)
		fwrite(&Ed_Key[i],sizeof(Ed_Key[0]),1,cfp);
	fwrite(&Ed_BackUp,sizeof(int),1,cfp);
	fwrite(&Ed_FillTabs,sizeof(int),1,cfp);
	fwrite(&Ed_WrapLines,sizeof(int),1,cfp);
	fwrite(&Ed_TabStop,sizeof(ushort),1,cfp);
	fwrite(&Ed_ConfigSave,sizeof(int),1,cfp);
	fwrite(LeftSearch,sizeof(char),ED_SEARCHSETLEN,cfp);
	fwrite(RightSearch,sizeof(char),ED_SEARCHSETLEN,cfp);
	fwrite(Ed_SearchText,ED_SEARCHTEXTLEN,1,cfp);
	fwrite(Ed_ReplaceText,ED_SEARCHTEXTLEN,1,cfp);
	fwrite(&Ed_CaseSensitive,sizeof(int),1,cfp);
	fwrite(&Ed_IdentsOnly,sizeof(int),1,cfp);
	fwrite(&Ed_MaskChk,sizeof(long),1,cfp);
	fwrite(&Ed_IDCnt,sizeof(int),1,cfp);
	fwrite(Ed_FileName,MAXFILENAMELEN,1,cfp);
	fwrite(Ed_LoadName,MAXFILENAMELEN,1,cfp);
	fwrite(&Ed_LineNow,sizeof(short),1,cfp);
	fwrite(&Ed_ColNow,sizeof(short),1,cfp);
	fclose(cfp);
}

#ifdef ANSI_C
LOCAL BOOLEAN NEAR Ed_LoadConfig(void)
#else
LOCAL BOOLEAN NEAR Ed_LoadConfig()
#endif
{
	FILE *cfp;
	short i;
	long Ed_MaskChk = MASKCHECK;
	if ((cfp=fopen("PEW.CFG","rb"))==NULL) return FALSE;
	for (i=0;i<(int)_Ed_NumKeyDefs;i++)
		(void)fread(&Ed_Key[i],sizeof(Ed_Key[0]),1,cfp);
	(void)fread(&Ed_BackUp,sizeof(int),1,cfp);
	(void)fread(&Ed_FillTabs,sizeof(int),1,cfp);
	(void)fread(&Ed_WrapLines,sizeof(int),1,cfp);
	(void)fread(&Ed_TabStop,sizeof(ushort),1,cfp);
	if (Ed_TabStop<=0 || Ed_TabStop>80) Ed_TabStop = 8;
	Ed_Win->tabsize=(ushort)Ed_TabStop;
	(void)fread(&Ed_ConfigSave,sizeof(int),1,cfp);
	(void)fread(Ed_LeftSearch,sizeof(char),ED_SEARCHSETLEN,cfp);
	(void)fread(Ed_RightSearch,sizeof(char),ED_SEARCHSETLEN,cfp);
	(void)fread(Ed_SearchText,ED_SEARCHTEXTLEN,1,cfp);
	(void)fread(Ed_ReplaceText,ED_SEARCHTEXTLEN,1,cfp);
	(void)fread(&Ed_CaseSensitive,sizeof(int),1,cfp);
	(void)fread(&Ed_IdentsOnly,sizeof(int),1,cfp);
	(void)fread(&Ed_MaskChk,sizeof(long),1,cfp);
	if (Ed_MaskChk!=MASKCHECK) {
		(void)Win_Error(NULL,"","Error in configuration file - delete it.",FALSE,TRUE);
		Fatal(NULL);
	}
	if (*Ed_FileName==NUL) {
		(void)fread(&Ed_IDCnt,sizeof(int),1,cfp);
		(void)fread(Ed_FileName,MAXFILENAMELEN,1,cfp);
		(void)fread(Ed_LoadName,MAXFILENAMELEN,1,cfp);
		(void)fread(&Ed_LineNow,sizeof(short),1,cfp);
		(void)fread(&Ed_ColNow,sizeof(short),1,cfp);
	}
	fclose(cfp);
	return TRUE;
}

/****************************************
* Execute the command specified by the	*
* control characters c1 and c2.		*
*****************************************/

#ifdef ANSI_C
  LOCAL ushort NEAR Ed_Command(ushort c, uchar keybd_status)
#else
  LOCAL ushort NEAR Ed_Command(c, keybd_status)
  ushort c;
  uchar keybd_status;
#endif
{
	ushort i;
	ushort rtn=c;
	BOOLEAN done=TRUE;
	enum editorcommand cmd;
	if ((char)rtn) switch((char)rtn) {
		case KB_TAB:	i=Ed_Win->tabsize-(ushort)Ed_ColNow%Ed_Win->tabsize;
				while (i--) Ed_InsertCh(' ');
				break;
		case KB_CRGRTN:	Ed_InsertCR();			break;
		case KB_BACKSPC:Ed_BackSpace(keybd_status); 	break;
		default:	if (rtn>=' ' && rtn<='~')
					Ed_InsertCh((char)rtn);
				else done=FALSE;
				break;
		}
	else switch (rtn)
		{
		case KB_UP:	Ed_Up();		break;
		case KB_DOWN:	Ed_Down();		break;
		case KB_LEFT:	Ed_Left();		break;
		case KB_RIGHT:	Ed_Right();		break;
		case KB_HOME:	Ed_Home();		break;
		case KB_END:	Ed_End();		break;
		case KB_PGUP:	Ed_PageUp();		break;
		case KB_PGDN:	Ed_PageDown();		break;
		case KB_DEL:	if (Ed_ColNow>=ED_LEN && Ed_LineNow<Ed_LastLine
						&& Ed_MarkType==ED_NOMARK) {
					Ed_PrefCol = Ed_ColNow;
					Ed_Down();
					Ed_JoinLines();
				} else	{
					ed_op=_Ed_DelBlock;
					Ed_BlockOp();
				}
				break;
		case KB_F1:
		case KB_ALT_H:	(void)Mnu_Process(&Help_Menu,Msg_Win,0);
				Ed_RefreshAll = TRUE;
				Ed_ReCursor();
		default:        done=FALSE;
				break;
		}
	if (!done)
		if (rtn==Ed_Key[(int)_Ed_Quit])
			rtn=Ed_Quit();
		else for (cmd=_Ed_LeftWord;cmd<_Ed_NumKeyDefs;cmd++)
		       if (rtn == Ed_Key[(int)cmd]) {
			      (*(Ed_Func[(int)(ed_op = cmd)]))();
			      break;
		      }
	return rtn;
}


/**************************
* Set up the Edit Windows *
**************************/

LOCAL void NEAR Ed_Initialise()
{
	short i;
	ushort keypress;
	uchar foreground=9;
	WINPTR tmpwin;
	Msg_Win = Win_Make(WIN_NOBORDER,0,24,80,1,0,ED_INSIDE_COLOR);
	Pew_InitMainMenu();
	Win_1LineBorderSet();
	Ed_Win = Win_Make(WIN_WRAP|WIN_CLIP|WIN_BORDER,1,2,78,21,ED_BORDER_COLOR, ED_INSIDE_COLOR);
	Pos_Win = Win_Make(WIN_NOBORDER|WIN_CURSOROFF,3,1,74,1,0,ED_BORDER_COLOR);
	Win_Activate(Ed_Win);
	if (Ed_LoadConfig()==FALSE) {
		Ed_AssignKeys();
		strcpy(Ed_LoadName,DEFAULTNAME);
		if (*Ed_FileName==NUL) strcpy(Ed_FileName,DEFAULTNAME);
	}
	Ed_SavePosition();
	Ed_LoadFile(Ed_FileName,FALSE);
	Ed_RestorePosition();
	(void)Ed_Point(Ed_LineNow,TRUE);
	for (i=0; i<(int)_Ed_NumKeyDefs; i++)
		Kb_GetKeyName(Ed_KeyNames[i], Ed_Key[i]);
	Ed_AssignFunctions();
	Ed_ReCursor();
	Win_2LineBorderSet();
	tmpwin=Win_Make(WIN_NONDESTRUCT|WIN_BORDER|WIN_CURSOROFF,23,6,32,12,SCR_NORML_ATTR,SCR_NORML_ATTR);
	Win_Activate(tmpwin);
	Win_SetAttribute(Scr_ColorAttr(LIGHTRED,BLACK));
	Win_FastPutS(1,5,"The Estelle PEW  v2.1");
	Win_FastPutS(3,7,"by Graham Wheeler");
	Win_SetAttribute(SCR_NORML_ATTR);
	Win_SetAttribute(Scr_ColorAttr(LIGHTBLUE,BLACK));
	Win_FastPutS(5,1,"Department of Computer Science");
	Win_FastPutS(6,13,"U.C.T.");
	Win_FastPutS(7,4,"(c) 1989-1992");
	while (Kb_RawLookC(&keypress)==FALSE) {
		Win_SetAttribute(Scr_ColorAttr(foreground,BLACK));
		Win_FastPutS(9,7,"Press F1 for Help");
		Win_FastPutS(10,5,"Press any key to begin");
		delay(250);
		foreground++;
		if (foreground>15) foreground=9;
	}
	if (keypress!=KB_F1) (void)Kb_RawGetC();
	Win_Free(tmpwin);
	Win_Activate(Ed_Win);
	Gen_BuildName("PEWPATH","PEW_HELP",Hlp_FileName);
}

/************************************************************************

	The main program simply sets up the screen, then repeatedly
	reads key presses and acts on them by calling the 'command'
	keystroke interpreter. If the character read is a null, then
	a special key has been pressed, and a second character must
	be read as well.

*************************************************************************/

GLOBAL ushort Ed_Quit()
{
	if (Ed_ConfigSave) Ed_SaveConfig();
	if (Ed_CleanUp()==KB_ESC) return KB_ESC;
	Win_Free(Ed_Win);
	Win_Free(Pos_Win);
	Win_Free(Msg_Win);
	Win_Free(Menu_Win);
	return (ushort)0;
}

#ifdef ANSI_C
  GLOBAL void Ed_Abort(short sig)
#else
  GLOBAL void Ed_Abort(sig)
  short sig;
#endif
{
#ifdef ANSI_C
	extern void FAR Pew_EndSystem(short);
#else
	extern void FAR Pew_EndSystem();
#endif
	if (Ed_Quit()==KB_ESC) return;
	Pew_EndSystem(sig);
}

#ifdef ANSI_C
  LOCAL void NEAR Ed_WinResize(short lines, short cols)
#else
  LOCAL void NEAR Ed_WinResize(lines, cols)
  short lines, cols;
#endif
{
	if (lines>=0 && lines<22 && cols>=0 && cols<=78) {
		Ed_SavePosition();
		Win_Free(Ed_Win);
		Win_1LineBorderSet();
		Ed_Win = Win_Make(WIN_WRAP|WIN_CLIP|WIN_BORDER,1,2,cols,lines,ED_BORDER_COLOR,ED_INSIDE_COLOR);
		Win_Activate(Ed_Win);
		Ed_RefreshAll = TRUE;
		Ed_ScrImage.firstln = Ed_ScrImage.lastln = Ed_ScrImage.lastlnrow = 0;
		Ed_RestorePosition();
		Ed_ShowPos(TRUE);
	}
}

GLOBAL void Ed_ToggleSize()
{
	if (Ed_Win->h == 21) Ed_WinResize(5,78);
	else Ed_WinResize(21,78);
}

#ifdef ANSI_C
  GLOBAL void Fatal(char *msg)
#else
  GLOBAL void Fatal(msg)
  char *msg;
#endif
{
	Scr_EndSystem();
	if (msg) { puts("PEW Fatal Error : "); puts(msg); }
	if (Ed_FileChanged) {
		puts("Saving editor buffer to RECOVER.EST\n");
		Ed_BackUp = FALSE;
		Ed_WriteFile("RECOVER.EST");
	}
	exit(-1);
}

GLOBAL void Ed_Main()
{
	ushort c;
	Ed_Initialise();
	Scr_InstallFatal(Fatal);
	(void)signal(SIGINT,Ed_Abort);
	while (TRUE) {
		Ed_ShowPos(FALSE);
		c=Kb_GetCh();
		if (c==Ed_Key[(int)_Ed_Quit]) {
			if (Ed_Quit()!=KB_ESC) break;
		} else c = Ed_Command(c, (uchar)*((uchar far *)0x417));
	}
}




/**********************************/
/* Set Up Default Key Assignments */
/**********************************/

LOCAL void NEAR Ed_AssignKeys()
{
 	Ed_Key[(int)_Ed_Quit		] = KB_ALT_Z;
 	Ed_Key[(int)_Ed_LeftWord	] = KB_CTRL_LEFT;
 	Ed_Key[(int)_Ed_RightWord	] = KB_CTRL_RIGHT;
	Ed_Key[(int)_Ed_LeftMatch	] = KB_CTRL_PGUP;
	Ed_Key[(int)_Ed_RightMatch	] = KB_CTRL_PGDN;
 	Ed_Key[(int)_Ed_FSearch		] = KB_ALT_S;
	Ed_Key[(int)_Ed_BSearch		] = KB_CTRL_S;
 	Ed_Key[(int)_Ed_Translate	] = KB_ALT_T;
 	Ed_Key[(int)_Ed_MarkBlock	] = KB_ALT_M;
	Ed_Key[(int)_Ed_MarkColumn	] = KB_ALT_C;
	Ed_Key[(int)_Ed_MarkPlace	] = KB_CTRL_P;
	Ed_Key[(int)_Ed_GotoMark	] = KB_CTRL_G;
	Ed_Key[(int)_Ed_File		] = KB_CTRL_F;
 	Ed_Key[(int)_Ed_Undo		] = KB_ALT_U;
	Ed_Key[(int)_Ed_Compile		] = KB_CTRL_C;
	Ed_Key[(int)_Ed_Execute		] = KB_CTRL_X;
	Ed_Key[(int)_Ed_Options		] = KB_CTRL_O;
	Ed_Key[(int)_Ed_Analyze		] = KB_CTRL_A;
	Ed_Key[(int)_Ed_Copy		] = KB_ALT_MINUS;
	Ed_Key[(int)_Ed_BlokPaste	] = KB_INS;
	Ed_Key[(int)_Ed_ColPaste	] = KB_ALT_P;
	Ed_Key[(int)_Ed_Edit		] = KB_CTRL_E;
	Ed_Key[(int)_Ed_TopFile		] = KB_CTRL_HOME;
	Ed_Key[(int)_Ed_EndFile		] = KB_CTRL_END;
	Ed_Key[(int)_Ed_RecMacro	] = KB_ALT_R;
	Ed_Key[(int)_Ed_LangHelp	] = KB_ALT_F1;
	Ed_Key[(int)_Ed_Template	] = KB_CTRL_F1;
	Ed_Key[(int)_Ed_DelLine		] = KB_ALT_D;
}

LOCAL void NEAR Ed_AssignFunctions()
{
 	Ed_Func[(int)_Ed_Quit		] = NULL; /* handled explicitly */
 	Ed_Func[(int)_Ed_LeftWord	] = Ed_LeftWord;
 	Ed_Func[(int)_Ed_RightWord	] = Ed_RightWord;
	Ed_Func[(int)_Ed_LeftMatch	] = Ed_LeftSearch;
	Ed_Func[(int)_Ed_RightMatch	] = Ed_RightSearch;
 	Ed_Func[(int)_Ed_FSearch	] = Ed_SearchForward;
	Ed_Func[(int)_Ed_BSearch	] = Ed_SearchBackward;
	Ed_Func[(int)_Ed_Translate	] = Ed_Translate;
 	Ed_Func[(int)_Ed_MarkBlock	] = Ed_Mark;
	Ed_Func[(int)_Ed_MarkColumn	] = Ed_Mark;
	Ed_Func[(int)_Ed_MarkPlace	] = Ed_MarkPlace;
	Ed_Func[(int)_Ed_GotoMark	] = Ed_GotoMark;
 	Ed_Func[(int)_Ed_File		] = Pew_MainMenu;
 	Ed_Func[(int)_Ed_Undo		] = Ed_Undo;
	Ed_Func[(int)_Ed_Compile	] = Pew_MainMenu;
 	Ed_Func[(int)_Ed_Execute	] = Pew_MainMenu;
 	Ed_Func[(int)_Ed_Options	] = Pew_MainMenu;
	Ed_Func[(int)_Ed_Analyze	] = Pew_MainMenu;
 	Ed_Func[(int)_Ed_Copy		] = Ed_BlockOp;
	Ed_Func[(int)_Ed_BlokPaste	] = Ed_BlockPaste;
	Ed_Func[(int)_Ed_ColPaste	] = Ed_ColumnPaste;
	Ed_Func[(int)_Ed_Edit		] = Pew_MainMenu;
	Ed_Func[(int)_Ed_TopFile	] = Ed_TopFile;
	Ed_Func[(int)_Ed_EndFile	] = Ed_EndFile;
	Ed_Func[(int)_Ed_RecMacro	] = (edHndl)Kb_RecordOn;
	Ed_Func[(int)_Ed_LangHelp	] = Ed_LangHelp;
	Ed_Func[(int)_Ed_Template	] = Ed_Template;
	Ed_Func[(int)_Ed_DelLine	] = Ed_DeleteLine;
}


#ifdef ANSI_C
  GLOBAL char Ed_NextChar(short *lineno) /* Compiler interface */
#else
  GLOBAL char Ed_NextChar(lineno) /* Compiler interface */
  short *lineno;
#endif
{
	char ch;
        while (Ed_ColNow>ED_LEN && Ed_LineNow < Ed_LastLine) {
                (void)Ed_Point(Ed_LineNow+1,FALSE);
                Ed_ColNow = 0;
        }
        if (Ed_LineNow <= Ed_LastLine)
		if (Ed_ColNow==ED_LEN) ch='\n';
		else ch = ED_TEXT[Ed_ColNow];
        else ch = 0;
	Ed_ColNow++;
	*lineno = Ed_LineNow+1;
	return ch;
}

