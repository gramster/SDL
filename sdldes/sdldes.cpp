//----------------------------------------------------------------------
// SDL*Design front end
// written by Graham Wheeler, December 1994
// (c) 1994 by Graham Wheeler. All Rights Reserved.
//----------------------------------------------------------------------

#include "sdldes.h"

MenuFrame *mainWnd = NULL;

// indices used for generating unique file names

int projnum = 0;
int filenum = 0;

//-----------------------------------------------------------------------
// File type lists for spec and project files

char *specTypes[] =
{
	"(DEBUG) Any file",	"*.*",	// remove this later
	"System specification",	"*.sdl",
	"Remote definitions",	"*.rem",
        "All files",		"*.*",
	0,			0
};

char *projTypes[] =
{
	"SDL*Design project files","*.spf",
        "All files", "*.*",
	0,		0
};

//-----------------------------------------------------------------------
// Some useful utility functions

int getConfirm(char *title, char *fmt, char *arg)
{
	char buff[256];
        sprintf(buff, fmt, arg);
	zMessage mess(app->rootWindow(), buff, title, MB_YESNO|MB_ICONHAND|MB_SYSTEMMODAL);
	return mess.isYes();
}

int FindSection(FILE *fp, char *sec)
{
	char buff[256];
        fseek(fp, 0, 0);
        for (;;)
	{
		fgets(buff, 256, fp);
		if (feof(fp))
                	break;
		if (strchr(buff, ']'))
			*(strchr(buff, ']')) = 0;
		if (strcmp(buff+1, sec)==0)
			return 1;
	}
        return 0;
}

int NextLine(FILE *fp, char *buff)
{
	for (;;)
	{
		fgets(buff, 256, fp);
		if (feof(fp))
			break;
		if (strchr(buff, '['))
			break;
		buff[strlen(buff)-1] = 0;
		if (buff[0])
                	return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------
// Dialog for defining new conditional compile IDs

DefineDlg::DefineDlg(zWindow *w, zResId &rid, zString *res)
	: zFormDialog(w, rid)
{
	zEditLine *ln = new zEditLine(this, ID_DEFINETEXT, (zString *)res, 0l);
	ln->setFocus();
	show();
	modal();
}


//-----------------------------------------------------------------------
// Dialog for undefining existing conditional compile IDs

UndefineDlg::UndefineDlg(zWindow *w, zResId &rid, char **names_in,
	char **result)
	: zFormDialog(w, rid)
{
	assert(result);
	names = names_in;
	defined = new zListBox(this, ID_DEFINED);
	assert(defined);
	defined->addCharStrings(names);
	pb = new zDefPushButton(this, IDOK);
	assert(pb);
	pb->setNotifyClicked(this, (ClickProc)&UndefineDlg::doOk);
	res = result;
	*res = NULL;
	show();
	modal();
}

int UndefineDlg::doOk(zNotifyEvt *ce)
{
	int s = defined->selection();
	if (s>=0) *res = defined->text(s);
	shutdown();
	return 1;
}

//-----------------------------------------------------------------------
// Useful item list class for lists of files in projects and list
// of conditional compile identifiers

ItemList::ItemList(zWindow *parent_in, int maxelts_in)
{
	parent = parent_in;
	maxelts = maxelts_in;
        iter = 0;
	nelts = 0;
	names = new char* [maxelts+1]; // include a sentinel
	assert(names);
	for (int i = 0; i <= maxelts; i++)
		names[i] = NULL;
}

// clear the list

void ItemList::Empty()
{
	for (int i = 0; i < maxelts; i++)
		if (names[i])
                {
			delete [] names[i];
			names[i] = NULL;
                }
}


ItemList::~ItemList()
{
	Empty();
	delete [] names;
}

// load list from file

void ItemList::Load(FILE *fp, char *sec)
{
	char buff[256];
	Empty();
	if (FindSection(fp, sec))
        	while (NextLine(fp, buff))
	        {
			names[nelts] = new char[strlen(buff)+1];
			strcpy(names[nelts], buff);
			nelts++;
		}
}

// save list to file

void ItemList::Save(FILE *fp, char *sec)
{
	fprintf(fp, "[%s]\n", sec);
	for (int i = 0; i < nelts; i++)
		fprintf(fp, "%s\n", names[i]);
}

// return the array of names for use in list box controls

const char **ItemList::Names()
{
	return names;
}

// search list for a name; return index or index of next free slot

int ItemList::Contains(char *name, int *idx)
{
	for (int i = 0; names[i] != NULL; i++)
	{
		if (strcmp(names[i], name)==0)
		{
			if (idx) *idx = i;
			return 1;
		}
	}
	if (idx)
		if (i < maxelts) // return free slot
			*idx = i;
		else
			*idx = -1;
	return 0;
}

// add a name to the list

int ItemList::Add()
{
	if (nelts >= maxelts)
		return -1;
	char *name = GetNewName(); // this is virtual
	int i;
	if (name && name[0] && !Contains(name, &i))
	{
		names[i] = name;
		nelts++;
                return 0;
	}
	else
	{
		delete [] name;
                return -1;
	}
}

// remove a name from the list

int ItemList::Remove(char *name)
{
	int i;
	if (name == NULL)
		name = GetOldName();
	if (name && Contains(name, &i))
	{
		delete [] names[i];
		nelts--;
		// swap with last to avoid fragmentation
		if (i!=nelts)
			names[i] = names[nelts];
		names[nelts] = NULL;
                return 0;
	}
	return -1;
}

// set index to first name in list (unused at present)

void ItemList::First()
{
	iter = 0;
}

// return next name in list (unused at present)
 
char *ItemList::Next()
{
	return names[iter++];
}

// get a name to remove from the list

char *ItemList::GetOldName()
{
	// Open a dialog box to remove an existing name
	char *rtn;
	UndefineDlg *dlg = new UndefineDlg(parent, zResId(UNDEFINE_DIALOG),
		names, &rtn);
	if (dlg->completed())
	{
		// caller expects a dynamic allocation
		assert(rtn);
		char *tmp = rtn;
		rtn = new char [strlen(tmp)+1];
		strcpy(rtn, tmp);
	}
	delete dlg;
	return rtn;
}

// Subclass for list of conditional compile IDs

NameList::NameList(zWindow *parent_in, int maxelts_in)
	: ItemList(parent_in, maxelts_in)
{
}

void NameList::Save(FILE *fp)
{
	ItemList::Save(fp, "Defines");
}

void NameList::Load(FILE *fp)
{
	ItemList::Load(fp, "Defines");
}

char *NameList::GetNewName()
{
	// Open a dialog box to add a new name
	char *rtn;
	zString *res = new zString;
	DefineDlg *dlg = new DefineDlg(parent, zResId(DEFINE_DIALOG), res);
	if (dlg->completed())
	{
		rtn = new char [res->length()+1];
                strcpy(rtn, *res);
	}
        else rtn = NULL;
        delete res;
	delete dlg;
	return rtn;
}

// Subclass for lists of file names

FileList::FileList(zWindow *parent_in, int maxelts_in)
	: ItemList(parent_in, maxelts_in)
{
}

void FileList::Save(FILE *fp)
{
	ItemList::Save(fp, "Files");
}

void FileList::Load(FILE *fp)
{
	ItemList::Load(fp, "Files");
}

char *FileList::GetNewName()
{
	char *rtn;
	zFileOpenForm *dlg = new zFileOpenForm(
		parent, "Add file to project", "*.sdl", specTypes);
	if (dlg->completed())
	{
		rtn = new char [strlen(dlg->name())+1];
                strcpy(rtn, dlg->name());
	}
        else rtn = NULL;
	delete dlg;
	return rtn;
}

//-----------------------------------------------------------------------
// dialog class for messages such as the Help-About command

MessageDlg::MessageDlg(zWindow *w, zResId &rid)
	: zFormDialog(w, rid)
{
	show();
	modal();
}

//--------------------------------------------------------------------
// Utility routine for generating unique filenames

char *GetUnique(char *tmplate, int nxt)
{
	static char rtn[16];

	// Generate a unique name
	for (int i = nxt; i < 1000; i++)
	{
		sprintf(rtn, tmplate, i);
		if (access(rtn, 0) < 0)
                	break;
	}
	return rtn;
}

//--------------------------------------------------------------------
// Project class

Project *project = NULL; // pointer to project, if any

Project::Project(zMDIAppFrame *parent_in, int max_files_in, int max_defs_in)
	: zMDIDialogFrame(parent_in, new zSizer, zSTDFRAME)
{
	parent = parent_in;
	name = GetUnique("PROJ_%03d.SPF", projnum++);
        caption(name);
	defines = new NameList(parent, max_defs_in); assert(defines);
	files = new FileList(parent, max_files_in);  assert(files);
	filelistbox = new ProjectList(this);
	Changed(0);
	mainWnd->SetEditWin(NULL);
	mainWnd->SetProjectWin(this);
	mainWnd->FileListChanged(0);
	deleteOnClose(1);
	show();
	setActive();
	filelistbox->setFocus();
}

Project::~Project()
{
	if (Changed())
		if (getConfirm("Project", "Project %s has changed. Save?", name))
			Save();
	if (defines)
		delete defines;
	if (files)
		delete files;
        delete filelistbox;
	mainWnd->SetProjectWin(NULL);
}

// Update the project window using the current list of files. Crude but brief.

void Project::UpdateWin()
{
	filelistbox->clear();
	filelistbox->addCharStrings((char **)files->Names());
}

// Add a file to the project

int Project::Add()
{
	if (files->Add()==0)
	{
        	UpdateWin();
		Changed(1);
		return 0;
	}
        return -1;
}

// Remove a file from the project

int Project::Remove(char *name)
{
	if (files->Remove(name)==0)
	{
        	UpdateWin();
		Changed(1);
		return 0;
	}
        return -1;
}

// Has project been changed since it was last saved?

int Project::Changed()
{
	return changed || mainWnd->FileListChanged();
}

void Project::Changed(int v)
{
	changed = v;
	if (v == 0)
        	mainWnd->FileListChanged(0);
}

// Add a new conditional compile ID to project

int Project::Define()
{
	if (defines->Add()==0)
        {
		Changed(1);
		return 0;
	}
        return -1;
}

// Remove a conditional compile ID from project

int Project::Undefine()
{
	if (defines->Remove()==0)
        {
		Changed(1);
		return 0;
	}
        return -1;
}

// Test if an ID is defined in project

int Project::Defined(char *name)
{
	return (defines->Contains(name));
}

// Save project to file

int Project::Save(FILE *fp)
{
	mainWnd->FileListChanged(0);
	mainWnd->SaveList(fp);
	files->Save(fp);
        defines->Save(fp);
	Changed(0);
        return 0;
}

// Save project given a file name

int Project::Save(char *fname)
{
	int rtn = -1;
	if (fname && name[0] == 0)
        {
		name = fname;
		caption(name);
        }
	if (fname == NULL)
        	fname = name;
	if (fname)
        {
		FILE *pfp = fopen(fname, "w");
		if (pfp)
		{
			Save(pfp);
			fclose(pfp);
		}
        }
        return rtn;
}

// Save project via a Save As dialog box

int Project::SaveAs()
{
	int rtn;
	zFileSaveAsForm *dlg = new zFileSaveAsForm(
		parent, "Save Project", "*.spf", projTypes);
	if (dlg->completed())
		rtn = Save(dlg->name());
        else rtn = -1;
	delete dlg;
	return rtn;
}

// Load project from file

int Project::Load(FILE *fp)
{
        mainWnd->LoadList(fp);
	files->Load(fp);
	defines->Load(fp);
	UpdateWin();
        Changed(0);
	return 0;
}

// Load project via an Open File dialog box

int Project::Load()
{
	int rtn = -1;
	zFileOpenForm *dlg = new zFileOpenForm(
		parent, "Open Project", "*.spf", projTypes);
	if (dlg->completed())
	{
		FILE *pfp = fopen(dlg->name(), "r");
		if (pfp)
                {
			Load(pfp);
			rtn = 0;
			fclose(pfp);
			name = dlg->name();
			caption(name);
                        UpdateWin();
		}
	}
	delete dlg;
	return rtn;
}

// Invoke the compiler

int Project::Compile()
{
	return 0;
}

// Resize the project window listbox

//int Project::size(zSizeEvt *se)
//{
//	zMDIDialogFrame::size(se);
//	return 0;	
//}

// ProjectList class; needed for overriding ch key event handler

typedef int (zEvH::*KeyProc)(zKeyEvt *);

ProjectList::ProjectList(Project *proj_in)
	: zListBoxUnsorted(proj_in, new zSizeWithParent)
{
	proj = proj_in;
	setHandler(this, (NotifyProc)(KeyProc)&ProjectList::HandleKeyUp, WM_KEYUP);
	show();
	setFocus();
}

// Handle Ins/Del keys in project window

int ProjectList::HandleKeyUp(zKeyEvt *ke)
{
	switch(ke->ch())
	{
	case VK_INSERT:
		proj->Add();
		break;
	case VK_DELETE:
		proj->Remove(text(selection()));
		break;
	case VK_RETURN:
		mainWnd->Edit(text(selection()));
                break;
	default:
		return 0;
	}
	return 1;
}

// Tell app that the active window is not an edit window

int Project::MDIActivate(zActivateEvt *ae)
{
	if (ae->active())
	{
		filelistbox->setFocus();
                mainWnd->SetEditWin(NULL);
        }
        return 1;
}

//-----------------------------------------------------------------------
// Help Routines

//-----------------------------------------------------------------------
// Editor class

EditorWin::EditorWin(zMDIAppFrame *parent, char *fname_in)
	: zMDIChildFrame(parent, new zSizeWithParent, zSTDFRAME)
{
	ed = new zEditor(this, new zSizeWithParent, 32000);
	if (fname_in)
	{
		fname = fname_in;
		ed->openFile(fname);
        }
	else
                fname = GetUnique("NONAM%03d.SDL", filenum++);
	caption(fname);
	mainWnd->SetEditWin(this);
	mainWnd->FileListChanged(1);
	deleteOnClose(1);
	ed->show();
	show();
	setActive();
	ed->setFocus();
}

// Get name of file being edited

char *EditorWin::Name()
{
	return fname;
}

// Save file

void EditorWin::Save(char *fname_in)
{
	ed->saveFile(fname_in ? fname_in : fname);
}

EditorWin::~EditorWin()
{
	if (ed->hasChanged())
		if (getConfirm("Editor", "File %s has been changed. Save?",
			fname))
			Save();
        mainWnd->FileListChanged(1);
	delete ed;
}

// Cut/Copy/Paste to clipboard

void EditorWin::Undo()
{
	ed->undo();
}

void EditorWin::Cut()
{
	ed->cut();
}

void EditorWin::Copy()
{
	ed->copy();
}

void EditorWin::Paste()
{
	ed->paste();
}

// Currently unimplemented operations in Search menu

void EditorWin::Search(char *str)
{
}

void EditorWin::Replace(char *src, char *dest)
{
}

void EditorWin::GotoLine(int ln)
{
}

// Tell app which editor window is active

int EditorWin::MDIActivate(zActivateEvt *ae)
{
	if (ae->active())
		mainWnd->SetEditWin(this);
//	else
//		mainWnd->SetEditWin(NULL);
	return 1;
}

// Get the current selection range

zRange& EditorWin::Selection()
{
	return ed->selection();
}

//-----------------------------------------------------------------------
// Main Menu

MenuFrame::MenuFrame(zWindow* parent,zSizer* siz,DWORD winStyle, const char* title)
	: zMDIAppFrame(parent,siz,winStyle,title, new zMenu(zResId(ID_WINDOWS)), 200)
{
	defines = new NameList(this, MAX_DEFINES);
        clip = new zClipboard(this);
	menu(mainmenu = new zMenu(this, zResId(MAINMENU)));
        menu()->appendDropDown(MDImenu(), "&Window");
	menu()->setSetup(this,(NotifyProc)&MenuFrame::menuSetup);
	SetEditWin(NULL);
	FILE *optfile = fopen("sdldes.ini", "r");
	if (optfile)
	{
		defines->Load(optfile);
                LoadList(optfile);
		fclose(optfile);
	}
        filelistchanged = 0;
}

MenuFrame::~MenuFrame()
{
	delete defines;
        delete clip;
}

// Toggle the gray state of menu items according to context

int MenuFrame::menuSetup(zEvent*)
{
	int nofile = (EditWin() == NULL);
	int empty = (nofile || EditWin()->Selection().isNull());
	int noproj = (project == NULL);
	int emptyproj = (noproj || project->NumFiles() == 0);
	int nodefines = !((project && project->NumDefined()) ||
			 (!project && defines->NumElts()));

	menu()->grayItem(ID_PROJECT, noproj);

        menu()->grayItem(ID_UNDO, nofile);
	menu()->grayItem(ID_COPY, empty);
	menu()->grayItem(ID_CUT, empty);
	menu()->grayItem(ID_PASTE,!clip->isTextAvail() || nofile);
	menu()->grayItem(ID_CLEAR,!clip->isTextAvail());

	menu()->grayItem(ID_FIND, nofile);
	menu()->grayItem(ID_REPLACE, nofile);
	menu()->grayItem(ID_REREPLACE, nofile);
        menu()->grayItem(ID_GOTOLINE, nofile);

	menu()->grayItem(ID_SAVEFILE, nofile);
	menu()->grayItem(ID_SAVEFILEAS, nofile);

	menu()->grayItem(ID_COMPILE, nofile && emptyproj);
	menu()->grayItem(ID_RUN, nofile && emptyproj);
	menu()->grayItem(ID_EXPORT, nofile && emptyproj);

	menu()->grayItem(ID_SAVEPROJ, emptyproj);
	menu()->grayItem(ID_SAVEPROJAS, emptyproj);
	menu()->grayItem(ID_ADDFILE, noproj);
//	menu()->grayItem(ID_NEWPROJ, !noproj);
//	menu()->grayItem(ID_OPENPROJ, !noproj);
	menu()->grayItem(ID_CLOSEPROJ, noproj);
	menu()->grayItem(ID_PROJECT, noproj);
	menu()->grayItem(ID_REMOVEFILE, emptyproj);
	menu()->grayItem(ID_UNDEFINENAME, nodefines);

        menu()->grayItem(ID_SAVEOPT, defines->NumElts()==0);
        return 1;
}

void MenuFrame::CloseAll()
{
	zWindowDlistIter trav(&(kidslist()));
	zMDIChildFrame *el;
	while ((el=(zMDIChildFrame*)trav())!=0)
	{
		trav.remove();
		delete el;
	}
}

void MenuFrame::SaveList(FILE *fp)
{
	zWindowDlistIter trav(&(kidslist()));
	zMDIChildFrame *el;
	fprintf(fp, "[%s]\n", "Edits");
        char *pn = project ? project->caption() : "";
	while ((el=(zMDIChildFrame*)trav())!=0)
        	if (strcmp(el->caption(), pn))
	        	fprintf(fp, "%s\n", el->caption());
}

void MenuFrame::LoadList(FILE *fp)
{
	char buff[256];
	FindSection(fp, "Edits");
	while (NextLine(fp, buff))
        	new EditorWin(this, buff);
}

void MenuFrame::Edit(char *fname)
{
	new EditorWin(this, fname);
}

// Handle menu commands

int MenuFrame::command(zCommandEvt *ce)
{
	switch(ce->cmd())
	{

	// MDI Window menu options

	case ID_ARRANGE:
		arrangeIcons();
		break;
	case ID_CASCADE:
		cascade();
		break;
	case ID_TILE:
		tile();
		break;
	case ID_CLOSEALL:
        	CloseAll();
		break;
	case ID_MESSAGE:
		break;
	case ID_PROJECT:
		if (project)
		{
			project->show();
			project->setActive();
		}
        	break;

	// File menu options

	case ID_NEWFILE:
		Edit();
                break;
	case ID_OPENFILE:
	{
		zFileOpenForm *dlg = new zFileOpenForm(
			this, "Open File", "*.sdl", specTypes);
		if (dlg->completed())
			Edit(dlg->name());
                delete dlg;
                break;
        }
	case ID_SAVEFILE:
		EditWin()->Save();
                break;
	case ID_SAVEFILEAS:
	{
		zFileSaveAsForm *dlg = new zFileSaveAsForm(
			this, "Save File", "*.sdl", specTypes);
		if (dlg->completed())
                       	EditWin()->Save(dlg->name());
		delete dlg;
		break;
        }
	case ID_EXIT:
		app->quit();
		break;

	// Edit menu options

	case ID_UNDO:
		EditWin()->Undo();
                break;
	case ID_CUT:
		EditWin()->Cut();
                break;
	case ID_COPY:
		EditWin()->Copy();
                break;
	case ID_PASTE:
		EditWin()->Paste();
		break;
	case ID_CLEAR:
        	clip->clear();
		break;

        // Search menu options

	case ID_FIND:
		EditWin()->Search(); // should retain last??
                break;
	case ID_REPLACE:
		EditWin()->Replace();
                break;
	case ID_REREPLACE:
		EditWin()->Replace();
                break;
	case ID_GOTOLINE:
		EditWin()->GotoLine(0);
                break;
	case ID_GOTONEXT:
        case ID_GOTOPREV:
		break;

	// Project menu options

	case ID_NEWPROJ:
        	CloseAll();
		new Project(this);
		assert(project);
		break;
	case ID_OPENPROJ:
        	CloseAll();
		new Project(this);
		assert(project);
		if (project->Load())
			delete project;
                break;
	case ID_CLOSEPROJ:
		if (project)
			delete project; // NULLs project automatically
                break;
	case ID_SAVEPROJ:
		assert(project);
                if (project->Changed())
			project->Save();
                break;
	case ID_SAVEPROJAS:
		assert(project);
		project->SaveAs();
                break;
	case ID_ADDFILE:
		assert(project);
		project->Add();
		break;
	case ID_REMOVEFILE:
		assert(project);
               	project->Remove();
		break;

	// Compile menu options

	case ID_DEFINENAME:
		if (project)
			project->Define();
		else
			defines->Add();
		break;
	case ID_UNDEFINENAME:
		if (project)
			project->Undefine();
		else
			defines->Remove();
		break;
	case ID_COMPILE:
		// Compile(project ? project->Defines() : defines->Names());
                break;
	case ID_RUN:
	case ID_EXPORT:
        	break;

	// Option menu options

	case ID_EDITOPT:
        	break;
	case ID_SAVEOPT:
	{
		FILE *optfile = fopen("sdldes.ini", "w");
		if (optfile)
		{
                	SaveList(optfile);
			defines->Save(optfile);
                        fclose(optfile);
                }
		break;
        }

        // Help menu options

	case ID_HELPSEARCH:
	case ID_CONTENTS:
		break;
	case ID_ABOUT:
	{
		MessageDlg *dlg = new MessageDlg(this, zResId(ABOUT_DIALOG));
                (void)dlg->completed();
		delete dlg;
                break;
	}
	default:
		return 0;
	}
	return 1;
}

// Note changes in which edit window is active

void MenuFrame::SetEditWin(EditorWin *ew)
{
	currentEditWin = ew;
}

void MenuFrame::SetProjectWin(Project *pw)
{
	project = pw;
}

int MenuFrame::FileListChanged()
{
	return filelistchanged;
}

void MenuFrame::FileListChanged(int v)
{
	filelistchanged = v;
}

// Get current editor window

EditorWin *MenuFrame::EditWin()
{
	return currentEditWin;
}

//int MenuFrame::ch(zKeyEvt *ke)
//{
//	
//}

//----------------------------------------------------------------------

void zApp::main()
{
	mainWnd=new MenuFrame(0,new zSizer,zSTDFRAME,"SDL*Design");
	mainWnd->show();
	go();
	delete mainWnd;
}
