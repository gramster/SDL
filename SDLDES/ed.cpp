#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <dir.h>
#include <io.h>

#include "ed.h"

#ifdef TEST
#define VK_TAB		0x0009
#define VK_RETURN	0x000D
#define VK_BACKSPACE	0x0008
#define VK_UP	   	0x4800
#define VK_DOWN		0x5000
#define VK_LEFT		0x4B00
#define VK_RIGHT   	0x4D00
#define VK_HOME		0x4700
#define VK_END		0x4F00
#define VK_PAGEUP   	0x4900
#define VK_PAGEDOWN	0x5100
#define VK_DELETE	0x5300

#define VK_ALT_D	0x2000
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifndef max
#define min(a, b)	( ((a) < (b)) ? (a) : (b) )
#define max(a, b)	( ((a) > (b)) ? (a) : (b) )
#endif

//------------------------------------------------------------------
// option flags

int makebackups = 0;	// if this flag is set .BAK files are made upon saving
int softcursor = 1;	// indicates that the cursor is software-based and thus
			// cursor movement requires screen updating. It should
			// be zero only when hardware-based cursors are in use
int fancytext = 1;	// use bold font to distinguish reserved words, etc

//------------------------------------------------------------------
// Implicit `pad' characters at end of line

#ifdef __MSDOS__
#define PAD	2		// CR+LF
#else
#define PAD	1		// LF
#endif

//-------------------------------------------------------------------
// Each line in an edit buffer is held in a TextLine

void TextLine::Dump()
{
	printf("  %3d %2d %s\n", id, length, text);
}

int TextLine::newID = 1;

void TextLine::Text(char *text_in, int length_in)
{
	if (text) delete [] text;
	if (text_in == NULL)
	{
		text = new char[1];
		assert(text);
		text[0] = 0;
		length = 0;
	}
	else
	{
		text = text_in;
		length = length_in;
		if (text && text[0] && length_in == 0)
			length = strlen(text);
	}
	NewID();
}

//--------------------------------------------------------------
// Each file being edited is held within a doubly-linked list
// of TextLines, managed by a TextBuffer
// Note: Undo must be implemented here (or at least via here).
// Undo is per-file, not per-window. The current implementation
// is not very undo-friendly - an audit-trail mechanism will
// have to be done independently, giving the cursor positions
// and keys necessary to do the undo's.

TextBuffer::TextBuffer(char *fname)
	: root(NULL, &root, &root)
{
	filesize = 0l;
	numlines = 1;
	filename = fname ? fname : "NONAME.XXX";
	if (fname==NULL || Load(fname)<0)
	{
		// create an empty line
		TextLine *first = new TextLine(NULL, &root, &root);
		root.Next(first);
		root.Prev(first);
	}
	haschanged = 0;
}

TextBuffer::~TextBuffer()
{
	TextLine *ln = root.Next();
	while (ln)
	{
		TextLine *tmp = ln;
		ln = Next(ln);
		Delete(tmp);
	}
}

int TextBuffer::Load(char *fname)
{
	FILE *fp = fopen(fname, "r");
	if (fp)
	{
		char buff[256];
		for (;;)
		{
			fgets(buff, 256, fp);
			if (feof(fp)) break;
			int l = strlen(buff);
			buff[l-1] = 0; // remove \n
			char *newtext = new char[l];
			strcpy(newtext, buff);
			Append(Last(), newtext, l-1);
		}
		fclose(fp);
		return 0;
	}
	return -1;
}

void TextBuffer::Save()
{
	if (makebackups)
	{
		if (access(filename,0)==0)
		{
			char backupname[80], drive[MAXDRIVE],
				directory[MAXDIR], basename[MAXFILE],
				extension[MAXEXT];
			(void)fnsplit(filename,drive,directory,basename,extension);
			fnmerge(backupname,drive,directory,basename,".BAK");
			(void)rename(filename,backupname);
		}
	}
	FILE *fp = fopen(filename, "w");
	if (fp)
	{
		TextLine *ln = root.Next();
		fprintf(fp, "%s", ln->Text());
		ln = Next(ln);
		while (ln)
		{
			fprintf(fp, "\n%s", ln->Text());
			ln = Next(ln);
		}
		fclose(fp);
	}
}

void TextBuffer::Insert(TextLine *now, char *text, int len)
{
	TextLine *nt = new TextLine(text, now->Prev(), now, len);
	assert(nt);
	nt->Prev()->Next(nt);
	now->Prev(nt);
	filesize += nt->Length() + PAD;
	numlines++;
	haschanged = 1;
}

void TextBuffer::Append(TextLine *now, char *text, int len)
{
	TextLine *nt = new TextLine(text, now, now->Next(), len);
	assert(nt);
	nt->Next()->Prev(nt);
	now->Next(nt);
	filesize += nt->Length() + PAD;
	numlines++;
	haschanged = 1;
}

void TextBuffer::Delete(TextLine *now)
{
	now->Prev()->Next(now->Next());
	now->Next()->Prev(now->Prev());
	filesize -= now->Length() + PAD;
	delete now;
	numlines--;
	haschanged = 1;
}

void TextBuffer::Replace(TextLine *now, char *text, int length)
{
	if (text && length == 0)
		length = strlen(text);
	filesize += length - now->Length();
	now->Text(text, length);
	haschanged = 1;
}

void TextBuffer::Dump()
{
	TextLine *f = First();
	while (f)
	{
		f->Dump();
		f = Next(f);
	}
}

//------------------------------------------------------------------
// The textbuffers are managed by a tbufmanager, which ensures
// that two tbufs for the same file never exist together.

TBufManager::TBufManager()
{
	for (int i = 0; i < MAX_TBUFS; i++)
	{
		tbufs[i] = NULL;
		refs[i] = 0;
	}
}

TextBuffer *TBufManager::GetTextBuffer(char *fname)
{
	for (int i = 0; i < MAX_TBUFS; i++)
		if (tbufs[i] && strcmp(tbufs[i]->Name(), fname)==0)
		{
			refs[i]++;
			return tbufs[i];
		}
	for (i = 0; i < MAX_TBUFS; i++)
		if (tbufs[i] == NULL)
		{
			refs[i] = 1;
			return tbufs[i] = new TextBuffer(fname);
		}
	return NULL; // failed
}

int TBufManager::ReleaseTextBuffer(char *fname)
{
	for (int i = 0; i < MAX_TBUFS; i++)
		if (tbufs[i] && strcmp(tbufs[i]->Name(), fname)==0)
		{
			if (--refs[i] == 0)
			{
				tbufs[i] = NULL;
				return 1;
			}
			break;
		}
	return 0;
}

TBufManager buffermanager;

//------------------------------------------------------------------
// Each editor window maintains a pointer to a TextBuffer for the
// file being edited, and a TextLine within that.

TextEditor::TextEditor(char *fname, int width_in, int height_in)
{
	// attach to existing TextBuffer if there is one
	buffer = buffermanager.GetTextBuffer(fname);
	line = buffer->First();
	first = linenow = 0;
	colnow = prefcol = left = lastleft = 0;
	width = width_in;
	height = height_in;
	cursormoved = 0;
	for (int i = 0; i < MAX_HEIGHT; i++)
        	IDs[i] = 0;
	//ShowCursor();
}

TextEditor::~TextEditor()
{
	if (buffermanager.ReleaseTextBuffer(buffer->Name())) // last instance?
	{
		if (buffer->Changed())
		{
			// ask if we must save!!
#ifdef TEST
			// hax for now
			printf("File %s has been changed. Save? (y/n) ",
				buffer->Name());
			fflush(stdout);
			for (;;)
			{
				char c = getchar();
				if (c=='Y' || c=='y')
				{
					Save();
					break;
				}
				if (c=='N' || c=='n')
					break;
			}
#endif
		}
		delete buffer;
	}
}

//------------------------------------------------------------------
// Cursor movement around file

void TextEditor::Point(int ln, int canUndo) // move within list
{
	(void) canUndo;
	if (ln > linenow)
		while (ln != linenow)
		{
			line = line->Next();
			linenow++;
		}
	else
		while (ln != linenow)
		{
			line = line->Prev();
			linenow--;
		}
}

void TextEditor::SetCursor(int ln, int col) // normalise location and view
{
	cursormoved = 0;
	ln = max(0, min(ln, Lines()-1));
	if (ln != Line())
        {
		if (softcursor)
			line->NewID();
		Point(ln, TRUE);
		if (softcursor)
			line->NewID();
                cursormoved = 1;
        }
	col = max(0, min(col, Length()));
	if (col != colnow)
	{
		colnow = col;
		if (softcursor) line->NewID();
                cursormoved = 1;
        }

	// make sure window view includes line

	if (Line() < first)
		first = Line();
	else if (Line() >= (first + height))
		first = Line() - height + 1;

	// make sure column is in view

	if (Column() < left)
		left = Column();
	else if (Column() >= (left + width))
		left = Column() - width + 1;
}

void TextEditor::GotoLineAbs(int ln, int canUndo)
{
	(void)canUndo;
	SetCursor(ln, prefcol);
}

void TextEditor::GotoColAbs(int col)
{
	if (col<0 && Line()>0L)
	{
		GotoLineRel(-1,TRUE);
		col = Length();
	}
	else if (col>Length() && Line()<(Lines()-1))
	{
		GotoLineRel(+1,TRUE);
		col = 0;
	}
	SetCursor(Line(), prefcol=col);
}

#define is_letter(c)	((c >= 'a' && c <='z') || (c>='A' && c<='Z'))
#define is_digit(c)	((c) >= '0' && (c) <= '9')

void TextEditor::LeftWord()
{
	char c;
	Left();
	do
	{
		Left();
		c = Text()[Column()];
	}
	while (is_letter(c) && !AtStart());
	if (!AtStart())
		Right();
}

void TextEditor::RightWord()
{
	char c;
	Right();
	while (c = Text()[Column()], is_letter(c) && !AtEnd())
		Right();
}

//-----------------------------------------------------------
// Character insertion

#define TABSTOP		8

void TextEditor::InsertChar(char c)
{
	int oldlen = Length();
	char *oldtext = Text();
	char *s = new char [oldlen+2];
	assert(s);
	strncpy(s, oldtext, (unsigned)Column());
	s[Column()]=c;
	strncpy(s+Column()+1, oldtext+Column(),(unsigned)(oldlen-Column()));
	s[oldlen+1] = 0;
	Replace(s, oldlen+1);
	Right();
}

void TextEditor::Tab()	// make spaces for now
{
	int i = TABSTOP - ( Column() % TABSTOP);
	while (i--)
		InsertChar(' ');
}

void TextEditor::InsertCR() // splits a line at current column
{
	char *oldtext = Text();
	int tlen = Length() - Column();
	assert(tlen >= 0);
	char *newtext1 = new char[Column()+1];
	assert(newtext1);
	strncpy(newtext1, oldtext, Column());
	newtext1[Column()] = 0;
	char *newtext2 = new char[tlen+1];
	assert(newtext2);
	strncpy(newtext2, oldtext+Column(), tlen);
	newtext2[tlen] = 0;
	Replace(newtext1, Column());
	Append(newtext2, tlen);
	Down();
	GotoColAbs(0);
}

void TextEditor::InsertStr(char *str)
{
	int slen = strlen(str);
	int len = Length() + slen;
	char *newtext = new char[len+1];
	strncpy(newtext, Text(), Column());
	strcpy(newtext+Column(), str);
	strcpy(newtext+Column()+slen, Text()+Column());
	Replace(newtext, len);
}

//-------------------------------------------------------------------
// Character deletion

void TextEditor::Join() // deletion of a newline
{
	if (Line() > 0)
	{
		char *txt2 = Text();
		int len = Length(), len2;
		Up();
		len += (len2 = Length());
		char *newtext = new char[len+1];
		assert(newtext);
		strcpy(newtext, Text());
		strcat(newtext, txt2);
		Replace(newtext, len);
		buffer->Delete(line->Next());
		SetCursor(Line(), prefcol = len2);
	}
}

void TextEditor::CutCols(int startcol, int endcol, int cutToClipboard)
{
	if (startcol>=Length())
	{
//		if (cutToClipboard)
//			CopyCols(0,0); /* Create an empty scrap line */
		return;
	}
	if (endcol==(-1) || endcol>=Length())
		endcol=Length()-1;
	int len = endcol-startcol+1;
//	if (cutToClipboard)
//		CopyCols(startcol,endcol);
	char *newtext = new char[Length() - len + 1];
	assert(newtext);
	strncpy(newtext, Text(), (unsigned)startcol);
	strncpy(newtext+startcol, Text()+endcol+1, (unsigned)(Length()-endcol-1));
	newtext[Length()-len] = 0;
	Replace(newtext, Length() - len);
	GotoColAbs(startcol);
}

void TextEditor::DeleteLine()
{
	if (Lines() > 1)
	{
		TextLine *tmp = line;
		// move to destination line
		if (Line())
			Up();
		else Down();
		// and delete old line
		buffer->Delete(tmp);
	}
	else // only line in file; just empty it
	{
		line->Text()[0] = 0; // hax
		prefcol = colnow = 0;
	}
}

void TextEditor::DeleteBlock()
{
	if (Column() < Length())
		CutCols(Column(), Column(), FALSE);
}

void TextEditor::Backspace()
{
	if (Column() == 0)
		Join();
	else
		CutCols(Column()-1,Column()-1,FALSE);
}

void TextEditor::Delete()
{
	if (Column() >= Length() && Line() < (Lines()-1)
		/* && MarkType==ED_NOMARK */)
	{
		Down();
		Join();
	}
	else
		DeleteBlock();
}

//---------------------------------------------------------------

void TextEditor::Undo()
{
}

void TextEditor::Redo()
{
}

//---------------------------------------------------------------
// hooks to get the stuff to display

void TextEditor::Prepare(int redoall)
{
	int cnt = Line() - first;
	fetchline = line;
	while (cnt-- > 0)
		fetchline = buffer->Prev(fetchline);
	fetchnum = 0;
	mustredo = (redoall || (lastleft != left));
        lastleft = left;
}

char *TextEditor::Fetch(int &cursorpos)
{
	char *rtn = NULL;
        cursorpos = -1;
	if (fetchline == NULL)
	{
		if (mustredo || IDs[fetchnum])
		{
			IDs[fetchnum] = 0;
			rtn = "";
		}
	}
	else
	{
		if (mustredo || IDs[fetchnum] != fetchline->ID())
                {
			rtn = fetchline->Text() + left;
			IDs[fetchnum] = fetchline->ID();
		}
		fetchline = buffer->Next(fetchline);
	}
	if ((first + fetchnum) == Line())
        	cursorpos = Column() - left;
	fetchnum++;
	return rtn;
}

//---------------------------------------------------------------

#ifdef TEST
int TextEditor::HandleKey(int ke)
#else
int TextEditor::HandleKey(zKeyEvt *ke)
#endif
{
	SetCursor(Line(), Column());	// normalise; needed if editing
					// has occurred in other windows
#ifdef TEST
	switch(ke)
#else
	switch(ke->ch())
#endif
	{
	// Hard-coded keys
	case VK_TAB:		Tab();		break;
	case VK_RETURN:		InsertCR();	break;
	case VK_BACKSPACE:	Backspace();	break;
	case VK_UP:		Up();		break;
	case VK_DOWN:		Down();		break;
	case VK_LEFT:		Left();		break;
	case VK_RIGHT:		Right();	break;
	case VK_HOME:		Home();		break;
	case VK_END:		End();		break;
	case VK_PAGEUP:		PageUp();	break;
	case VK_PAGEDOWN:	PageDown();	break;
	case VK_DELETE:		Delete();	break;

	// insertable and redefinable keys

//	case VK_ALT_D:		DeleteLine();	break;
	default:
#ifdef TEST
		if (ke>=' ' && ke<='~')
			InsertChar(ke);
#else
		if (!ke->extended() && ke->ch()>=' ' && ke->ch()<='~')
		{
			InsertChar(ke->ch());
                }
#endif
//		else for (cmd=_LeftWord;cmd<_NumKeyDefs;cmd++)
//		       if (rtn == Key[(int)cmd])
//		{
//			      (*(Func[(int)(ed_op = cmd)]))();
//			      break;
//		}
	}
	return 0;
}

void TextEditor::Dump()
{
	printf("line = %d   col = %d   prefcol = %d  first = %d  left = %d\n",
		linenow, colnow, prefcol, first, left);
	printf("lines = %d   size = %ld   length = %d\n",
		Lines(), Size(), Length());
	printf("text = %.*s@%s\n\nBuffer:\n", Column(), Text(),
		Text()+Column());
	buffer->Dump();
}

//--------------------------------------------------------------
#ifdef TEST

int getkeycode()
{
	int rtn;
	_AH=0;
	geninterrupt(0x16);
	rtn = _AX;
	if (rtn & 0x007F) return (rtn & 0x007F);
	else return rtn;
}

int main(int argc, char *argv[])
{
	char *fname = (argc > 1) ? argv[1] : NULL;
	TextEditor *te = new TextEditor(fname, 60, 20);
	TextEditor *te2 = new TextEditor(fname, 60, 20);
	assert(te);
	int key;
	for (;;)
	{
		printf("Key for window 1?"); fflush(stdout);
		if ((key = getkeycode()) == 27)
			break;
		te->HandleKey(key);
		printf("\nWindow 1\n");
		te->Dump();
#if 0
		printf("\nWindow 2\n");
		te2->Dump();
		printf("Key for window 2?"); fflush(stdout);
		if ((key = getkeycode()) == 27)
			break;
		te2->HandleKey(key);
		printf("\nWindow 1\n");
		te->Dump();
		printf("\nWindow 2\n");
		te2->Dump();
#endif
	}
	delete te;
	delete te2;
	return 0;
}

#endif

//---------------------------------------------------------------------------------
#ifndef TEST

#define MAXLINES	64

class TextEditWin : public zPane, public TextEditor
{
	int IDs[MAXLINES];
        zCaret *caret;
public:
	TextEditWin(zWindow *w);
        void Draw(int redoall = 0);
	~TextEditWin()
	{
		delete caret;
        }
	int KeyDown(zKeyEvt *ke);
	virtual int draw(zDrawEvt *de)
	{
		Draw(1);
		return 1;
        }
	virtual int ch(zKeyEvt *ke)
	{
		if (!ke->extended())
		{
			HandleKey(ke);
			Draw();
		}
		return 1;
        }
};

typedef int (zEvH::*KeyProc)(zKeyEvt *);

TextEditWin::TextEditWin(zWindow *w)
	: zPane(w, new zSizeWithParent), TextEditor(NULL, 20, 20)
{
	for (int i = 0; i < MAXLINES; i++)
		IDs[i] = 0;
	setHandler(this, (NotifyProc)(KeyProc)&TextEditWin::KeyDown, WM_KEYDOWN);
	caret = new zCaret(this, SolidBlob, zDimension(2,14));
	caret->pos(zPoint(1,2));
        caret->show();
	setFocus();
        show();
}

int TextEditWin::KeyDown(zKeyEvt *ke)
{
	if (ke->extended())
        {
		HandleKey(ke);
		Draw();
        }
       	return 1;
}

//-----------------------------------------------------------------------------------
// Language dependent fancy formatting

char *reswrds[] =
{
	"BLOCK",
	"CHANNEL",
	"INPUT",
	"PROCESS",
	"SIGNAL",
        "STATE",
	"SYSTEM",
        NULL
};

int IsReserved(int len, char *txt)
{
	char buf[100];
	strncpy(buf, txt, len);
	buf[len] = 0;
        strupr(buf);
        // should do a binary search eventually!!
	int i = 0;
	while (reswrds[i])
		if (strcmp(buf, reswrds[i])==0)
			return 1;
		else i++;
        return 0;
}

void DrawSlice(zDisplay *cv, int &xpos, int ypos, char *txt, int len, zColor &col,
	int weight, int style, int &curs, int &cpos)
{
	char buff[256];
	strncpy(buff, txt, len);
	buff[len] = 0;
	cv->pushFont(new zFont("Courier", zPrPoint(90,130, cv), weight,
		ffModern, FixedPitch, style));
	cv->textColor(col);
	cv->text(xpos, ypos, buff);
	if (curs>=0)
	{
        	int cnt = min(curs, len); // letters left of cursor in this chunk
		if (cnt)
			cpos += cv->getTextDim(buff, cnt).width();
		curs -= len;
        }
	xpos += cv->getTextDim(buff, len).width();
	delete cv->popFont();
}

void DrawNormal(zDisplay *cv, int &xpos, int ypos, char *txt, int len, zColor &col,
	int &curs, int &cpos)
{
	DrawSlice(cv, xpos, ypos, txt, len, col, FW_NORMAL, 0, curs, cpos);
}

void DrawItalic(zDisplay *cv, int &xpos, int ypos, char *txt, int len, zColor &col,
	int &curs, int &cpos)
{
	DrawSlice(cv, xpos, ypos, txt, len, col, FW_NORMAL, FS_ITALIC, curs, cpos);
}

void DrawBold(zDisplay *cv, int &xpos, int ypos, char *txt, int len, zColor &col,
	int &curs, int &cpos)
{
	DrawSlice(cv, xpos, ypos, txt, len, col, FW_BOLD, 0, curs, cpos);
}

void DrawFancyText(zDisplay *cv, int &xpos, int ypos, char *txt, int curs, int &cpos)
{
	// This is very crude for now!!
	int p = 0, l = strlen(txt);
	char c;
        char buff[256];
	while (p < l)
	{
		c = txt[p];
		if (!is_letter(c) && !is_digit(c) && (c!= '/' || txt[p+1]!='*'))
		{
			p++;
			continue;
		}
		// Draw what we have skipped...
		DrawNormal(cv, xpos, ypos, txt, p, zColor(0,0,0), curs, cpos);
		txt += p; l -= p; p = 0;
		if (c=='/') // comment
		{
			for (p = 2; p < l && (txt[p]!='*' || txt[p+1]!='/'); p++);
			if (p < l) p+=2;
			DrawItalic(cv, xpos, ypos, txt, p, zColor(255,0,0), curs, cpos);
		}
		else if (is_letter(c))
		{
			for (; is_letter(txt[p]) || txt[p]=='_' || is_digit(txt[p]);  p++);
			if (IsReserved(p, txt))
				DrawBold(cv, xpos, ypos, txt, p, zColor(0,0,0), curs, cpos);
			else
				DrawNormal(cv, xpos, ypos, txt, p, zColor(0,0,0), curs, cpos);
		}
		else if (is_digit(c))
		{
			for (; is_digit(txt[p]); p++);
			DrawNormal(cv, xpos, ypos, txt, p, zColor(0,0,255), curs, cpos);
		}
		txt += p; l-= p;
		p = 0;
	}
	// still got stuff to draw?
	if (l)
		DrawNormal(cv, xpos, ypos, txt, l, zColor(0,0,0), curs, cpos);
}

void TextEditWin::Draw(int redoall)
{
	setFocus();
	Prepare(redoall);
	canvas()->lock();
        zRect r;
	canvas()->getVisible(r);
	canvas()->pushFont(new zFont("Courier", zPrPoint(90,130, canvas()), FW_NORMAL,
		 ffModern, FixedPitch));
	int charheight = canvas()->getTextDim("|", 1).height();
	int charwidth = canvas()->getTextDim("X", 1).width();
        int yskip = charheight;
	DisplaySize((r.height() / yskip)-1, r.width() / charwidth);
	int ypos = 2;
        caret->hide();
	for (int cnt = height; cnt--; )
	{
        	int cursor, cpos = 3;
        	char *txt = Fetch(cursor);
		if (txt) // line changed??
		{
			// draw new line contents
			if (fancytext)
                        {
				int xtnt = 5;
				DrawFancyText(canvas(), xtnt, ypos, txt, cursor, cpos);
				canvas()->fill(zRect(xtnt, ypos, r.width() - 1, ypos+yskip));
                        }
			else
                        {
				canvas()->text(5, ypos, txt);
				canvas()->fill(zRect(5+canvas()->getTextDim(txt, strlen(txt)).width(),
						ypos, r.width() - 1, ypos+yskip));
			}
			if (cursor>=0)
			{
				if (!fancytext)
					cpos += canvas()->getTextDim(txt, cursor).width();
				caret->pos(zPoint(cpos, ypos));
                        }
		}
                ypos += yskip;
	}
	// last line is a status line
	ypos = r.height() - yskip - 1;
	char buff[80];
	sprintf(buff, " ln %d/%d col %d/%d", Line()+1, Lines(), Column()+1, Length());
	canvas()->backColor(zColor(200,200,200));
	canvas()->textColor(zColor(0,0,0));
	canvas()->text(0, ypos, buff);
	canvas()->pushBrush(new zBrush(LiteGrayBrush));
	canvas()->fill(zRect(canvas()->getTextDim(buff, strlen(buff)).width(),
				ypos, r.width() - 1, ypos+yskip));
	delete canvas()->popBrush();
	canvas()->backColor(zColor(255,255,255));
	caret->show();
	delete canvas()->popFont();
        canvas()->unlock();
}

class TextEditFrame: public zMDIChildFrame
{
	TextEditWin *ew;
public:
	TextEditFrame(zMDIAppFrame *);
	~TextEditFrame() { delete ew; }
//	virtual int MDIActivate(zActivateEvt *ae)
//	{
//		if (ae->active())
//			ew->setFocus();
//                return 0;
//        }
};

TextEditFrame::TextEditFrame(zMDIAppFrame *parent)
	: zMDIChildFrame(parent, new zSizeWithParent, zSTDFRAME, "Editor")
{
	ew = new TextEditWin(this);
	deleteOnClose(TRUE);
        show();
}

class TopFrame: public zMDIAppFrame
{
public:
	TopFrame();
};

TopFrame::TopFrame()
	: zMDIAppFrame(0, new zSizer, zSTDFRAME, "Test")
{
	new TextEditFrame(this);
	show();
}

void zApp::main()
{
	TopFrame *p = new TopFrame;
	go();
	delete p;
}
#endif // TEST
