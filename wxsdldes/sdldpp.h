#ifdef __GNUG__
#pragma interface
#endif

// Define a new application
class MyApp: public wxApp
{
  public:
    Bool ParseCommandLine(void);
    wxFrame *OnInit(void);
};

// Define a new frame
class MyFrame: public wxFrame
{
  public:
    MyFrame(wxFrame *frame, char *title, int x, int y, int w, int h, int type);
    Bool OnClose(void);
    void OnMenuCommand(int id);
};
#if 0
class MyChild: public wxFrame
{
  public:
    MyCanvas *canvas;
    MyChild(wxFrame *frame, char *title, int x, int y, int w, int h, int type);
    ~MyChild(void);
    Bool OnClose(void);
    void OnMenuCommand(int id);
    void OnActivate(Bool active);
};
#endif

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

void GenericOk(wxButton& but, wxCommandEvent& event);

