#ifndef __SDLDES_H
#define __SDLDES_H

// SDL*Design front end
// written by Graham Wheeler, December 1994
// (c) 1994 by Graham Wheeler. All Rights Reserved.

#include <zapp.hpp>
#include <zapph\mdi.hpp>
#include "constant.h"
#include <assert.h>
#include <string.h>

// Default values of soft limits

#define MAX_FILES	32	// Maximum number of files in a project
#define MAX_DEFINES	32	// Max per-proj DEFINEs for conditional compiles

// Utility name and file container classes

class ItemList
{
	int maxelts;
	int nelts;
        int iter;
        char **names;
	zWindow *parent; // needed for dialogs
public:
	ItemList(zWindow *parent_in, int maxelts_in);
	~ItemList();
	const char **Names();
	int Contains(char *name, int *idx = NULL);
	int Add();
	int Remove(char *name = NULL);
	int NumElts()
	{
		return nelts;
	}
	void First();
	char *Next();
	void Empty();
	void Save(FILE *fp, char *sec);
	void Load(FILE *fp, char *sec);

	// Dialogs

	virtual char *GetNewName() = 0;
	char *GetOldName();
        friend class NameList;
	friend class FileList;
};

// Defined names

class NameList : public ItemList
{
public:
	NameList(zWindow *parent_in, int maxelts_in);
	void Save(FILE *fp);
        void Load(FILE *fp);
	virtual char *GetNewName();
};

// As above but for file names

class FileList : public ItemList
{
//	char *(&types)[];
public:
	FileList(zWindow *parent_in, int maxelts_in);
	void Save(FILE *fp);
        void Load(FILE *fp);
	virtual char *GetNewName();
};

//-----------------------------------------------------------------------
// Project class

class Project; // forward decl

class ProjectList : public zListBoxUnsorted
{
	Project *proj;
public:
	ProjectList(Project *proj_in);
	int HandleKeyUp(zKeyEvt *ke);
};

class Project : public zMDIDialogFrame
{
	zString name;
	NameList *defines;	// conditional compile defines
	FileList *files;	// files in project
	int changed;	// need to save before delete?
	zWindow *parent;
	ProjectList *filelistbox;
//      int system;	// index of system file
	int Save(FILE *pfp);
	int Load(FILE *pfp);
public:
	Project(zMDIAppFrame* parent_in, int max_files_in=MAX_FILES,
		int max_defs_in=MAX_DEFINES);
	~Project();
	void UpdateWin();
	int Changed();
	void Changed(int v);
	int Add();
	int Remove(char *name = NULL);
	int Define();
	int Undefine();
	int Defined(char *name = NULL);
	int Save(char *fname = NULL);
        int SaveAs();
	int Load();
	int Compile();
	int NumFiles()
	{
		return files->NumElts();
	}
	int NumDefined()
	{
		return defines->NumElts();
        }
//	virtual int size(zSizeEvt *);
//	virtual int ch(zKeyEvt *);
	virtual int MDIActivate(zActivateEvt *);
//	virtual int kill(zEvent *);
};

//-----------------------------------------------------------------------
// A simple dialog class for messages such as the Help-About command

class MessageDlg : public zFormDialog
{
public:
	MessageDlg(zWindow *, zResId&);
};

//-----------------------------------------------------------------------
// Dialog for defining new conditional compile IDs

class DefineDlg : public zFormDialog
{
public:
	DefineDlg(zWindow *, zResId&, zString*);
};

//-----------------------------------------------------------------------
// Dialog for undefining existing conditional compile IDs

class UndefineDlg : public zFormDialog
{
	zDefPushButton *pb;
	zListBox *defined;
	char **names;
	char **res;
public:
	UndefineDlg(zWindow *, zResId&, char **names_in, char **result);
	int doOk(zNotifyEvt *ce);
};

//-----------------------------------------------------------------------
// Editor window class

class EditorWin : public zMDIChildFrame
{
	zEditor*ed;
	zString fname;
public:
	EditorWin(zMDIAppFrame *parent, char *fname_in = NULL);
	~EditorWin();
	char *Name();
	void Save(char *fname_in = NULL);
        void Undo();
	void Cut();
	void Copy();
	void Paste();
	void Search(char *str = NULL);
	void Replace(char *src = NULL, char *dest = NULL);
	void GotoLine(int ln);
	virtual int MDIActivate(zActivateEvt *ae);
	zRange& Selection();
};

//-----------------------------------------------------------------------
// Main Menu

class MenuFrame : public zMDIAppFrame
{
	NameList *defines;
	EditorWin *currentEditWin;
	zMenu	*mainmenu;
	zClipboard *clip;
        int filelistchanged;
public:
	MenuFrame(zWindow* parent,zSizer* siz,DWORD winStyle,const char* title);
	~MenuFrame();
	void Edit(char *fname = NULL);
	void CloseAll();
	void SetEditWin(EditorWin *ew);
	void SetProjectWin(Project *pw);
	void SaveList(FILE *fp);
	void LoadList(FILE *fp);
	int FileListChanged();
        void FileListChanged(int v);
        EditorWin *EditWin();
	
	virtual int command(zCommandEvt *);
        int menuSetup(zEvent *);
};

#endif

