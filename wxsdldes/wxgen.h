/*
 * File:    wxsdldes.h
 */

#ifndef _INC_WXSDLDES_H
#define _INC_WXSDLDES_H

/*
 * Forward declarations of all top-level window classes.
 */

class  AboutDialog;
class  DefineDialog;
class  UndefineDialog;
class  FrameClass36;

/*
 * Class representing the entire Application
 */

class AppClass: public wxApp
{
 public:
  AboutDialog	*about_dialog;
  DefineDialog	*define_dialog;
  UndefineDialog *undefine_dialog;
  FrameClass36	*frame35;

  wxFrame *OnInit(void);
  Bool ParseCommandLine(void);
};

class AboutDialog: public wxDialogBox
{
 private:
 protected:
 public:
  // Panel items for reference within the program.
  wxMessage *message7;
  wxMessage *message8;
  wxMessage *message9;
  wxButton *button10;

  // Possible bitmaps for buttons.

  // Constructor and destructor
  AboutDialog(wxFrame *parent, char *title, Bool modal, int x, int y, int width, int height, long style, char *name);
  ~AboutDialog(void);

 void OnSize(int w, int h);
};

void ButtonProc9(wxButton& but, wxCommandEvent& event);

class DefineDialog: public wxDialogBox
{
 private:
 protected:
 public:
  // Panel items for reference within the program.
  wxButton *button16;
  wxButton *button18;
  wxText *text20;

  // Possible bitmaps for buttons.

  // Constructor and destructor
  DefineDialog(wxFrame *parent, char *title, Bool modal, int x, int y, int width, int height, long style, char *name);
  ~DefineDialog(void);

 void OnSize(int w, int h);
};

void ButtonProc15(wxButton& but, wxCommandEvent& event);

void ButtonProc17(wxButton& but, wxCommandEvent& event);

void TextProc19(wxText& text, wxCommandEvent& event);

class UndefineDialog: public wxDialogBox
{
 private:
 protected:
 public:
  // Panel items for reference within the program.
  wxButton *button26;
  wxButton *button28;
  wxMessage *message30;
  wxListBox *listbox31;

  // Possible bitmaps for buttons.

  // Constructor and destructor
  UndefineDialog(wxFrame *parent, char *title, Bool modal, int x, int y, int width, int height, long style, char *name);
  ~UndefineDialog(void);

 void OnSize(int w, int h);
};

void ButtonProc25(wxButton& but, wxCommandEvent& event);

void ButtonProc27(wxButton& but, wxCommandEvent& event);

void ListBoxProc30(wxListBox& lbox, wxCommandEvent& event);

class FrameClass36: public wxFrame
{
 private:
 protected:
 public:
  // Constructor and destructor
  FrameClass36(wxFrame *parent, char *title, int x, int y, int width, int height, long style, char *name);
  ~FrameClass36(void);

 Bool OnClose(void);
 void OnSize(int w, int h);
 void OnMenuCommand(int commandId);
};

/* Menu identifiers
 */
#define FILE 1
#define ID_NEWFILE 2
#define ID_OPENFILE 3
#define ID_SAVEFILE 4
#define ID_SAVEFILEAS 5
#define ID_EXIT 6

#define EDIT 7
#define ID_UNDO 8
#define ID_CUT 9
#define ID_COPY 10
#define ID_PASTE 11
#define ID_CLEAR 12

#define SEARCH 13
#define ID_FIND 14
#define ID_REPLACE 15
#define ID_REREPLACE 16
#define ID_GOTOLINE 17
#define ID_GOTONEXT 18
#define ID_GOTOPREV 19

#define PROJECT 20
#define ID_NEWPROJ 21
#define ID_OPENPROJ 22
#define ID_CLOSEPROJ 23
#define ID_SAVEPROJ 24
#define ID_SAVEPROJAS 25
#define ID_ADDFILE 26
#define ID_REMOVEFILE 27

#define COMPILE 28
#define ID_DEFINENAME 29
#define ID_UNDEFINENAME 30
#define ID_COMPILE 31
#define ID_RUN 32
#define ID_EXPORT 33

#define OPTIONS 34
#define ID_EDITOPT 35
#define ID_SAVEOPT 36

#define HELP 37
#define ID_CONTENTS 38
#define ID_HELPSEARCH 39
#define ID_ABOUT 40

/*
 * Toolbar identifiers
 */

#endif
