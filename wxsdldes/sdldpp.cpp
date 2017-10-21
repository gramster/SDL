/*
 * File:	mdi.cc
 * Purpose:	
 * Author:	Julian Smart
 * Created:	1994
 * Updated:	
 * Copyright:	(c) 1994, AIAI, University of Edinburgh
 */

static const char sccsid[] = "%W% %G%";

#ifdef __GNUG__
#pragma implementation
#endif

/*
 * Purpose:  MDI demo for wxWindows class library
 *           Run with no arguments for SDI style, with -mdi argument
 *           for MDI style windows.
 */


#include "wx.h"
#include "sdldpp.h"

MyFrame *frame = NULL;
wxList my_children;
Bool isMDI = FALSE;

// This statement initialises the whole application

MyApp myApp;

const  top_frame_type = wxMDI_PARENT | wxDEFAULT_FRAME;
const  child_frame_type = wxMDI_CHILD | wxDEFAULT_FRAME;
int winNumber = 1;

wxFont *small_font;

// The `main program' equivalent, creating the windows and returning the
// main frame

wxFrame *MyApp::OnInit(void)
{
  if (!ParseCommandLine())
    return NULL;
  isMDI = TRUE;

  // Create a small font
  small_font = new wxFont(10, wxSWISS, wxNORMAL, wxNORMAL);

  // Create the main frame window
  frame = new MyFrame(NULL, "SDL/Designer++", 0, 0, 300, 400, top_frame_type);
  return frame;
}

// Parses the command line

Bool MyApp::ParseCommandLine(void)
{
  return TRUE;
}

// Define my frame constructor

MyFrame::MyFrame(wxFrame *frame, char *title, int x, int y, int w, int h, int type):
  wxFrame(frame, title, x, y, w, h, type)
{
  // Make a menubar
  wxMenuBar *menu_bar = new wxMenuBar;
  wxMenu *file_menu = new wxMenu;
  file_menu->Append(ID_NEWFILE, "&New");
  file_menu->Append(ID_OPENFILE, "&Open..");
  file_menu->AppendSeparator();
  file_menu->Append(ID_SAVEFILE, "&Save");
  file_menu->Append(ID_SAVEFILEAS, "Save &as..");
  file_menu->AppendSeparator();
  file_menu->Append(ID_EXIT, "&Exit");
  menu_bar->Append(file_menu, "&File");

  wxMenu *edit_menu = new wxMenu;
  edit_menu->Append(ID_UNDO, "Undo");
  edit_menu->Append(ID_CUT, "Cu&t");
  edit_menu->Append(ID_COPY, "&Copy");
  edit_menu->Append(ID_PASTE, "&Paste");
  edit_menu->Append(ID_CLEAR, "C&lear");
  menu_bar->Append(edit_menu, "&Edit");

  wxMenu *search_menu = new wxMenu;
  search_menu->Append(ID_FIND, "&Find..");
  search_menu->Append(ID_REPLACE, "&Replace..");
  search_menu->Append(ID_REREPLACE, "Replace &again");
  search_menu->AppendSeparator();
  search_menu->Append(ID_GOTOLINE, "&Go to line...");
  search_menu->Append(ID_GOTONEXT, "&Next error");
  search_menu->Append(ID_GOTOPREV, "&Previous error");
  menu_bar->Append(search_menu, "&Search");

  wxMenu *project_menu = new wxMenu;
  project_menu->Append(ID_NEWPROJ, "&New");
  project_menu->Append(ID_OPENPROJ, "&Open..");
  project_menu->Append(ID_CLOSEPROJ, "&Close");
  project_menu->AppendSeparator();
  project_menu->Append(ID_SAVEPROJ, "&Save");
  project_menu->Append(ID_SAVEPROJAS, "Save &as..");
  project_menu->AppendSeparator();
  project_menu->Append(ID_ADDFILE, "&Add File..");
  project_menu->Append(ID_REMOVEFILE, "&Remove File..");
  menu_bar->Append(project_menu, "&Project");

  wxMenu *compile_menu = new wxMenu;
  compile_menu->Append(ID_DEFINENAME, "Define...");
  compile_menu->Append(ID_UNDEFINENAME, "Undefine...");
  compile_menu->Append(ID_COMPILE, "&Compile");
  compile_menu->Append(ID_RUN, "&Run");
  compile_menu->AppendSeparator();
  compile_menu->Append(ID_EXPORT, "&Export C++");
  menu_bar->Append(compile_menu, "&Compile");

  wxMenu *option_menu = new wxMenu;
  option_menu->Append(ID_EDITOPT, "&Edit Options...");
  option_menu->Append(ID_SAVEOPT, "&Save Options");
  menu_bar->Append(option_menu, "&Options");

  wxMenu *help_menu = new wxMenu;
  help_menu->Append(ID_CONTENTS, "&Contents");
  help_menu->Append(ID_HELPSEARCH, "&Search for Help on..");
  help_menu->AppendSeparator();
  help_menu->Append(ID_ABOUT, "&About..");
  menu_bar->Append(help_menu, "&Help");

  // Associate the menu bar with the frame
  SetMenuBar(menu_bar);
  // Centre frame on the screen.
  Centre(wxBOTH);
  Show(TRUE);
}

Bool MyFrame::OnClose(void)
{
  // Must delete children
  wxNode *node = my_children.First();
  while (node)
  {
//  MyChild *child = (MyChild *)node->Data();
    wxNode *next = node->Next();
//  child->OnClose();
//  delete child;
    node = next;
  }
  return TRUE;
}

// Intercept menu commands
void MyFrame::OnMenuCommand(int id)
{
  switch (id)
  {
    // &New
    case ID_NEWFILE:
    {
      break;
    }
    // &Open..
    case ID_OPENFILE:
    {
      // Show file selector.
      char *f = wxFileSelector("Open SDL File...", NULL, "noname.sdl", ".sdl", "*..sdl");
      if (!f)
        return;
      break;
    }
    // &Save
    case ID_SAVEFILE:
    {
      break;
    }
    // Save &as..
    case ID_SAVEFILEAS:
    {
      break;
    }
    // &Exit
    case ID_EXIT:
    {
      OnClose();
      delete this;
      break;
    }
    // Undo
    case ID_UNDO:
    {
      break;
    }
    // Cu&t
    case ID_CUT:
    {
      break;
    }
    // &Copy
    case ID_COPY:
    {
      break;
    }
    // &Paste
    case ID_PASTE:
    {
      break;
    }
    // C&lear
    case ID_CLEAR:
    {
      break;
    }
    // &Find..
    case ID_FIND:
    {
      break;
    }
    // &Replace..
    case ID_REPLACE:
    {
      break;
    }
    // Replace &again
    case ID_REREPLACE:
    {
      break;
    }
    // &Go to line...
    case ID_GOTOLINE:
    {
      break;
    }
    // &Next error
    case ID_GOTONEXT:
    {
      break;
    }
    // &Previous error
    case ID_GOTOPREV:
    {
      break;
    }
    // &New
    case ID_NEWPROJ:
    {
      break;
    }
    // &Open..
    case ID_OPENPROJ:
    {
      // Show file selector.
      char *f = wxFileSelector("Open Project...", NULL, "noname.spf", ".spf", "*..spf");
      if (!f)
        return;
      break;
    }
    // &Close
    case ID_CLOSEPROJ:
    {
      break;
    }
    // &Save
    case ID_SAVEPROJ:
    {
      break;
    }
    // Save &as..
    case ID_SAVEPROJAS:
    {
      break;
    }
    // &Add File..
    case ID_ADDFILE:
    {
      break;
    }
    // &Remove File..
    case ID_REMOVEFILE:
    {
      break;
    }
    // Define...
    case ID_DEFINENAME:
    {
      break;
    }
    // Undefine...
    case ID_UNDEFINENAME:
    {
      break;
    }
    // &Compile
    case ID_COMPILE:
    {
      break;
    }
    // &Run
    case ID_RUN:
    {
      break;
    }
    // &Export C++
    case ID_EXPORT:
    {
      break;
    }
    // &Edit Options...
    case ID_EDITOPT:
    {
      break;
    }
    // &Save Options
    case ID_SAVEOPT:
    {
      break;
    }
    // &Contents
    case ID_CONTENTS:
    {
      break;
    }
    // &Search for Help on..
    case ID_HELPSEARCH:
    {
      break;
    }
    // &About..
    case ID_ABOUT:
    {
      (void)wxMessageBox("SDL/Designer++ 0.1\nby Open Mind Solutions\n(c) 1994", "About SDL/Designer++");
      break;
    }
  default:
    break;
  }
}
#if 0
    case MDI_NEW_WINDOW:
    {
      // Make another frame, containing a canvas
      MyChild *subframe = new MyChild(frame, "Canvas Frame", 10, 10, 300, 300,
                             child_frame_type);

      char titleBuf[100];
      sprintf(titleBuf, "Canvas Frame %d", winNumber);
      subframe->SetTitle(titleBuf);
      winNumber ++;
      // Give it a status line
      subframe->CreateStatusLine();
      int width, height;
      subframe->GetClientSize(&width, &height);
      MyCanvas *canvas = new MyCanvas(subframe, 0, 0, width, height);
      wxCursor *cursor = new wxCursor(wxCURSOR_PENCIL);
      canvas->SetCursor(cursor);
      subframe->canvas = canvas;
      // Give it scrollbars
      canvas->SetScrollbars(20, 20, 50, 50, 4, 4);
      canvas->SetPen(red_pen);
      subframe->Show(TRUE);
      break;
    }
  }

MyChild::MyChild(wxFrame *frame, char *title, int x, int y, int w, int h, int type):
  wxFrame(frame, title, x, y, w, h, type)
{
  canvas = NULL;
  my_children.Append(this);
}

MyChild::~MyChild(void)
{
  my_children.DeleteObject(this);
}

// Intercept menu commands
void MyChild::OnMenuCommand(int id)
{
  switch (id)
  {
    case MDI_CHILD_QUIT:
    {
      OnClose();
      delete this;
      break;
    }
    default:
    {
      frame->OnMenuCommand(id);
      break;
    }
  }
}

void MyChild::OnActivate(Bool active)
{
  if (active && canvas)
    canvas->SetFocus();
}

Bool MyChild::OnClose(void)
{
  return TRUE;
}
#endif

void GenericOk(wxButton& but, wxCommandEvent& event)
{
  wxDialogBox *dialog = (wxDialogBox *)but.GetParent();
  dialog->Show(FALSE);
}
