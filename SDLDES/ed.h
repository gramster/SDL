#ifndef __ED_H

#define __ED_H

#include <stdlib.h> // for NULL
#include <assert.h>

#ifndef TEST
#include <zapp.hpp>
#include <zapph\mdi.hpp>
#endif

//-------------------------------------------------------------------
// Each line in an edit buffer is held in a TextLine

class TextLine	// line in a buffer
{
	static int newID;
	int id;
	int length;
	char *text;
	TextLine *next, *prev;
public:
	void Text(char *text_in, int length_in = 0);
	TextLine(char *text_in = NULL, TextLine *prev_in = NULL,
		TextLine *next_in = NULL, int length_in = 0)
	{
		text = NULL;
		prev = prev_in;
		next = next_in;
		Text(text_in, length_in);
	}
	~TextLine()
	{
		assert(text);
		delete [] text;
	}
	void Next(TextLine *next_in)
	{
		next = next_in;
	}
	void Prev(TextLine *prev_in)
	{
		prev = prev_in;
	}
	TextLine *Next()
	{
		return next;
	}
	TextLine *Prev()
	{
		return prev;
	}
	int ID()
	{
		return id;
	}
	char *Text()
	{
		return text;
	}
	int Length()
	{
		return length;
	}
	void NewID()
	{
		id = newID++;
        }
	void Dump();
};

//--------------------------------------------------------------
// Each file being edited is held within a doubly-linked list
// of TextLines, managed by a TextBuffer


class TextBuffer // buffer manager
{
	TextLine root;
	long filesize;
	int numlines;
	int haschanged;
	char *filename;
public:
	TextBuffer(char *fname = NULL);
	~TextBuffer();
	TextLine *First()
	{
		return root.Next();
	}
	TextLine *Last()
	{
		return root.Prev();
	}
	TextLine *Next(TextLine *now)
	{
		if (now->Next() == &root)
			return NULL;
		else
			return now->Next();
	}
	TextLine *Prev(TextLine *now)
	{
		if (now->Prev() == &root)
			return NULL;
		else
			return now->Prev();
	}
	void Insert(TextLine *now, char *text, int len = 0);
	void Append(TextLine *now, char *text, int len = 0);
	void Delete(TextLine *now);
	void Replace(TextLine *now, char *text, int length = 0);
	long Size()
	{
		return filesize;
	}
	int Lines()
	{
		return numlines;
	}
	int Changed()
	{
		return haschanged;
	}
	int Load(char *fname);
	void Save();
	char *Name()
	{
		return filename;
	}
	void Dump();
};

//------------------------------------------------------------------
// The textbuffers are managed by a tbufmanager, which ensures
// that two tbufs for the same file never exist together.

#define MAX_TBUFS	32

class TBufManager
{
	TextBuffer	*tbufs[MAX_TBUFS];
	int		refs[MAX_TBUFS];
public:
	TBufManager();
	TextBuffer *GetTextBuffer(char *fname);
	int ReleaseTextBuffer(char *fname);
};

//------------------------------------------------------------------
// Each editor window maintains a pointer to a TextBuffer for the
// file being edited, and a TextLine within that.

#define MAX_HEIGHT	64

class TextEditor
{
	TextBuffer     *buffer;
	TextLine       *line,
		       *fetchline;
	int		linenow,
			markline,
			colnow,
			markcol,
			marktype,
			prefcol,
			width,	// width of window
			height,	// height of window
			first,	// first line displayed in window
			left,	// leftmost column displayed in window
			lastleft,
			fetchnum,
			mustredo,
                        cursormoved,
                        IDs[MAX_HEIGHT];

	int Length()
	{
		return line->Length();
	}
	char *Text()
	{
		return line->Text();
	}
	int Lines()
	{
		return buffer->Lines();
	}
	long Size()
	{
		return buffer->Size();
	}
	void Replace(char *txt, int len = 0)
	{
		buffer->Replace(line, txt, len);
	}
	void Append(char *txt, int len = 0)
	{
		buffer->Append(line, txt, len);
	}
	void Insert(char *txt, int len = 0)
	{
		buffer->Insert(line, txt, len);
	}
	int Line()
	{
		return linenow;
	}
	int Column()
	{
		return colnow;
	}
	void Point(int ln, int canUndo);
	void SetCursor(int ln, int col);

	// We provide both relative and absolute methods; this may be a
	// possible optimisation for efficiency later.

	void GotoLineAbs(int ln, int canUndo = 1);
	void GotoLineRel(int cnt, int canUndo = 1)
	{
		GotoLineAbs(Line() + cnt, canUndo);
	}
	void GotoColAbs(int col);
	void GotoColRel(int cnt)
	{
		GotoColAbs(Column() + cnt);
	}
	int AtStart()
	{
		return Line()==0 && Column()==0;
	}
	int AtEnd()
	{
		return Line()==(Lines()-1) && Column() == Length();
	}
	void Up()
	{
		GotoLineRel(-1);
	}
	void Down()
	{
		GotoLineRel(+1);
	}
	void Left()
	{
		GotoColRel(-1);
	}
	void LeftWord();
	void Right()
	{
		GotoColRel(+1);
	}
	void RightWord();
	void Home()
	{
		if (Column() == 0)
			if (Line() == first)
				SetCursor(0,0); // go to top of file
			else
				GotoLineAbs(first);
		else
			GotoColAbs(0);
	}
	void End()
	{
		GotoColAbs(line->Length());
	}
	void PageUp()
	{
		if (Line() != first)
			GotoLineAbs(first);
		else GotoLineRel(- height + 1);
	}
	void PageDown()
	{
		if (Line() != (first + height - 1))
			GotoLineAbs(first + height - 1);
		else GotoLineRel(height-1);
	}
	void SetMark(int typ = 0)
	{
		markline = linenow;
		markcol = colnow;
		marktype = typ;
	}
	void GotoLine(int ln)
	{
		SetCursor(ln, prefcol);
	}
	void Tab();
	void InsertCR();
	void InsertChar(char c);
	void InsertStr(char *str);
	void Join();
	void CutCols(int startcol, int endcol, int canUndo);
	void DeleteBlock();
	void Backspace();
	void Delete();
	void DeleteLine();
	void Undo();
	void Redo();
	void DisplaySize(int lns, int cols)
	{
		height = lns;
		width = cols;
	}
	void Prepare(int redoall = 0);
	char *Fetch(int &cursorpos);
	int CursorMoved()
	{
		return cursormoved;
        }
public:
	TextEditor(char *fname, int width_in, int height_in);
	~TextEditor();
	void Save()
	{
		buffer->Save();
	}
#ifdef TEST
	int  HandleKey(int ke);
#else
	int HandleKey(zKeyEvt *ke);
#endif
	void Dump();
	friend class TextEditWin;
};

#endif /* __ED_H */


