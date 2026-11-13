////////////////////////////////////////////////////////////////////////////////////////////////////
//
// EDIT FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "SyntaxHighlight.cpp"

int *LineColours = NULL;
char *LineStart = NULL;
int LineColoursLength = 0;
int ColourDefault;

int *MacroRecording;   // Pointer to Macro being recorded
int MacroRecordingIndex;

const char Brakets [] = "()[]{}<>";

int IsBracket (char Ch)
  {
    char *p;
    //
    if (Ch)
      {
        p = StrPos ((char *) Brakets, Ch);
        if (p)
          return p - Brakets;
      }
    return -1;
  }

int FindMatchingBracket (char *Text, int Pos)
  {
    int Brac;
    int Count;
    //
    Count = 1;
    Brac = IsBracket (Text [Pos]);
    if (Brac >= 0)
      while (true)
        {
          if ((Brac & 0x01) == 0)   // opening brace
            Pos++;
          else
            Pos--;
          if (Pos <= 0 || Text [Pos] == '\0')
            break;
          if (Text [Pos] == Brakets [Brac])   // another bracket
            Count++;
          else if (Text [Pos] == Brakets [Brac ^ 0x01])   // matching
            if (--Count == 0)
              return Pos;
        }
    return -1;
  }

bool Within (int Val, int Min, int Max)
  {
    if (Val >= Min && Val <= Max)
      return true;
    return false;
  }

bool WhiteSpace (char c)
  {
    if (c != ' ')
      if (c != '\t')
        if (c != '\n')
          if (c != '\r')
            return false;
    return true;
  }

_Point EditSize = {700, 500};
int CurrentLine;

class _FormEdit: public _Form
  {
    public:
      _Container *Toolbar;
      _Button *bBookmark, /**bMacro,*/ *bCut, *bCopy, *bPaste, *bUndo, *bSave;
      _Button *bFont;
      _ButtonArrow *bSizeUp, *bSizeDn;
      //_Button *bFind;   // Never enabled, indicator only
      _ButtonArrow *bFindPrev, *bFindNext;
      _EditSearch *eTarget;
      _Button *bReplace;
      //_SelectionList *slMacros;
      _EditMultiline *eContent;
      _MenuPopup *pmEdit;
      _Container *Status;
      _Label *lStatus;
      _Edit *eLineNumber;
      _FormEdit (char *Filename, _Point Pos, bool Hex, char **Text = NULL);
     ~_FormEdit ();
      void StatusFill (void);
      //
      char *Filename;
      char **TextResident;
      //bool CompareClipboard;
      int SyntaxID;
      int Attrib;
  };


//////////////////////////////////////////////////////////////////////
//
// Replace sub-form

class _FormReplace: public _Form
  {
    public:
      _FormEdit *fEdit;
      _EditPopup *eReplacement;
      _Button *bGo;
      _FormReplace (_Point Pos, _FormEdit *fEdit_);
     ~_FormReplace ();
  };

_FormReplace *fReplace = NULL;

void ActionReplaceGo (_Button *Button)
  {
    _EditMultiline *Edit;
    int Sel0, Sel1;
    char *Text, *Target;
    char *SelNew;
    int SelNewLen;
    //
    Edit = fReplace->fEdit->eContent;
    Sel0 = Min (Edit->Selected [0], Edit->Selected [1]);
    Sel1 = Max (Edit->Selected [0], Edit->Selected [1]);
    Target = fReplace->fEdit->eTarget->EditDrop->EditPopup->TextGet ();
    if (Sel0 == Sel1 || *Target == '\0')   // no selection OR no Target
      MessageBoxError ("Enter a Target AND Select Text Region First");
    else
      {
        Edit->MoveToIndex (Sel0);
        Text = Edit->TextGet ();
        SelNew = StrMake (&Text [Sel0], Sel1 - Sel0);
        StrPosFlags = fReplace->fEdit->eTarget->Flags ();
        StrReplace (&SelNew, Target, fReplace->eReplacement->TextGet ());
        StrPosFlags = 0x00;
        SelNewLen = StrLen (SelNew);
        Edit->Delete (Sel0, Sel1 - Sel0);
        Edit->Insert (Sel0, SelNewLen, SelNew);
        Edit->Selected [0] = Sel0;
        Edit->Selected [1] = Sel0 + SelNewLen;
        free (SelNew);
        Edit->Invalidate ();
        //fReplace->Die ();
      }
  }

bool FormReplaceEventPreview (_Form *Form, _Event *Event)
  {
    if (Event->Type == etWindowClose || Event->Type == etKeyDown && Event->Key == esc)
      {
        if (fMessageBox)
          fMessageBox->Die ();
        Form->Die ();
      }
    else
      {
        //if (fMessageBox)
        //  return true;
        if (Event->Type == etKeyDown && Event->Key == KeyEnter)
          ActionReplaceGo (NULL);
        else
          return false;
      }
    return true;
  }

_FormReplace::_FormReplace (_Point Pos, _FormEdit *fEdit_) : _Form ("Replace with", {Pos.x, Pos.y, 400, 28}, waAlwaysOnTop | waResizable) // | waBorderless)
  {
    const int bh = 24;
    //
    fEdit = fEdit_;
    Container->FontSet (NULL, 0);
    eReplacement = new _EditPopup (Container, {0, 0, -bh, 0}, NULL);
    bGo = new _Button (Container, {-bh, 0, 0, 0}, "\a2Go", (_Action) ActionReplaceGo);
    _Form::EventPreview = FormReplaceEventPreview;
  }

_FormReplace::~_FormReplace ()
  {
    fReplace = NULL;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Syntax Highlighting Callbacks

#define ColTarget  0x80000000   // This Search Target matches

bool TextCallBackEditHighlight (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size)
  {
    int i;
    //
    //Font->Style &= ~(fsBold | fsUnderline);
    if (LineColours)
      {
        i = *Text - LineStart;
        if (i >= 0 && i < LineColoursLength)
          {
            Font->Colour = ColourDefault;
            Font->Style = LineColours [i] >> 24;
            if (LineColours [i] & 0xFFFFFF)   // Colour specified //ColChange)
              Font->Colour = LineColours [i] & 0xFFFFFF;
            if (LineColours [i] & ColTarget)
              Font->ColourBG = ColourAdjust (cRed, 185); //ColourAverage (cSelected, cTextBgnd);
          }
      }
    return TextCallBackEdit (Window, Text, Font, Posn, Size);
  }

void EditDrawLineCallBackSyntax (_EditCommon *EditCommon, char *Line, int LineLength)
  {
    _FormEdit *fEdit;
    _EditMultiline *eContent;
    char *Text;
    char *t1, *t2;
    int BracPos;
    char ln [16];
    //
    fEdit = (_FormEdit *) EditCommon->Form;
    // Calc colour for every character on the line
    if (LineColours)
      free (LineColours);
    LineColours = SyntaxHighlight (Syntax [fEdit->SyntaxID].Syntax, Line, LineLength, Syntax [fEdit->SyntaxID].UpCase);
    LineStart = Line;
    LineColoursLength = LineLength;
    // Add Flags for Matching Bracket highlighting and Search matching
    if (LineColours)
      {
        // Look for Search Targets
        t1 = Line;
        while (true)
          {
            t2 = fEdit->eTarget->EditDrop->EditPopup->TextGet ();
            if (t2 == NULL || *t2 == '\0')   // no Target
              break;
            t1 = StrPos (t1, t2, 0, LineLength - (t1 - Line));
            if (!t1)   // no (more) matches
              break;
            while (*t2++)
              LineColours [t1++ - Line] |= ColTarget;
          }
        // Look for matching brackets
        eContent = fEdit->eContent;
        Text = eContent->TextGet ();
        if (Line == &Text [eContent->IndexLine])   // Is this the cursor line?
          {
            if (IsBracket (Line [eContent->Posn.x]) >= 0)
              LineColours [eContent->Posn.x] |= fsBold << 24;
            BracPos = FindMatchingBracket (Line, eContent->Posn.x);   // Is cursor on a bracket
            if (BracPos >= 0 && BracPos < LineLength)
              LineColours [BracPos] |= fsBold << 24;
          }
      }
    // Update Status:Line Number
    if (EditCommon->Posn.y != CurrentLine)
      {
        CurrentLine = EditCommon->Posn.y;
        t1 = ln;
        NumToStr (&t1, CurrentLine + 1);
        *t1 = 0;
        ((_FormEdit *) EditCommon->Form)->eLineNumber->TextSet (ln);
      }
  }

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SUPPORT

void Beep_ (_FormEdit *fEdit)
  {
    Beep ();
    fEdit->KeySeries = NULL;   // Cancel running Macro
    if (MacroRecording)   // Cancel recording Macro
      {
        MacroRecording [0] = 0;   // empty Macro
        MacroRecording = NULL;   // Stop recording
      }

  }

void MessageNoSave (void)
  {
    MessageBoxError (/*{300, 64},*/ "Error\v\a1Could NOT save file");
  }

void TextCompare (_FormEdit *fEdit, char *Clip)
  {
    _EditMultiline *eContent;
    int SelStart, SelEnd;
    char c;
    //
    //fEdit->CompareClipboard = false;
    eContent = fEdit->eContent;
    if (Clip == NULL || Clip [0] == '\0' || eContent->Selected [0] == eContent->Selected [1])
      MessageBoxError (/*{200, 50},*/ "Hint\vCompare: Copy text to Clipboard and make a Selection FIRST");
    else
      {
        SelStart = eContent->Selected [0];
        SelEnd = eContent->Selected [1];
        if (SelEnd < SelStart)
          Swap (SelStart, SelEnd);
        c = eContent->TextGet () [SelEnd];
        eContent->TextGet () [SelEnd] = '\0';
        if (fCompare)
          delete fCompare;
        fCompare = new _FormCompare (MousePos (), Clip, &eContent->TextGet () [SelStart], "Clipboard", "Selection", 10);
        eContent->TextGet () [SelEnd] = c;
      }
  }

void ActionChange (_EditMultiline *Edit)
  {
    _FormEdit *fe;
    //
    fe = (_FormEdit *) Edit->Form;
    fe->bSave->EnabledSet (true);
  }

void ActionFindFocus (_FormEdit *fEdit)
  {
    char *Text, *s;
    int *Sel;
    //
    Text = fEdit->eContent->TextGet ();
    Sel = &(fEdit->eContent->Selected [0]);
    if (Sel [0] == Sel [1])
      fEdit->eContent->SelectWord ();
    if (Sel [0] < Sel [1])
      s = StrMake (&Text [Sel [0]], Sel [1] - Sel [0]);
    else
      s = StrMake (&Text [Sel [1]], Sel [0] - Sel [1]);
    fEdit->eTarget->EditDrop->EditPopup->TextSet (s);
    free (s);
    fEdit->eTarget->EditDrop->EditPopup->Clipboard (coSelectAll);
    fEdit->eTarget->EditDrop->EditPopup->FocusSet ();
  }

void ActionGotoFocus (_FormEdit *fEdit)
  {
    fEdit->eLineNumber->Clipboard (coSelectAll);
    fEdit->eLineNumber->FocusSet ();
  }

void ActionGotoPathFile (_FormEdit *fEdit)
  {
    _EditMultiline *eContent;
    char *Text;
    int s0, s1;
    char *PathFile;
    //
    eContent = fEdit->eContent;
    Text = eContent->TextGet ();
    s0 = Min (eContent->Selected [0], eContent->Selected [1]);
    s1 = Max (eContent->Selected [0], eContent->Selected [1]);
    if (s0 == s1)   // nothing selected
      {
        while (s0 > 0 && Within (Text [s0 - 1], ' ' + 1, 0x7F))
          s0--;
        while (Within (Text [s1], ' ' + 1, 0x7F))
          s1++;
      }
    else   // Selection present, trim white space
      {
        while (WhiteSpace (Text [s0]))
          s0++;
        while (s1 > 0 && WhiteSpace (Text [s1 - 1]))
          s1--;
      }
    if (s1 > s0)   // there is a string to jump to
      {
        eContent->Selected [0] = s0;
        eContent->Selected [1] = s1;
        eContent->Invalidate ();
        PathFile = StrMake (eContent->TextGet () + s0, s1 - s0);
        DebugAddS ("ActionGotoPathFile", PathFile); // ####
        if (!GotoPathFile (PathFile))
          Beep_ (fEdit);
        free (PathFile);
      }
  }

void ActionFind (_ButtonArrow *Button)
  {
    _FormEdit *fEdit;
    _EditMultiline *eCon;
    char *Text, *Target, *p;
    int tl;
    //
    fEdit = (_FormEdit *) Button->Form;
    eCon = fEdit->eContent;
    if (eCon->TextLen)
      {
        Text = eCon->TextGet ();
        Target = fEdit->eTarget->EditDrop->EditPopup->TextGet ();
        fEdit->eTarget->EditDrop->DropListAdd (Target, 8);
        StrAssignCopy (&Settings.EditTarget, fEdit->eTarget->EditDrop->TextGet ());
        tl = StrLen (Target);
        StrPosFlags = fEdit->eTarget->Flags ();
        if (Button->Direction == dRight)   // Search forward
          p = StrPos (Text + eCon->Selected [0], Target);
        else   // search backwards
          p = StrPosPrev (Text, Text + eCon->Selected [1], Target);
        StrPosFlags = 0x00;
        if (p)
          {
            eCon->MoveToIndex (p - Text + tl);
            eCon->Selected [1] = p - Text;
            eCon->Selected [0] = p - Text + tl;
            eCon->ScrollToCursor ();
            eCon->Invalidate ();
            eCon->FocusSet ();
          }
        else
          Beep_ (fEdit);
      }
  }

void ActionReplace (_Button *Button)
  {
    if (!fReplace)
      fReplace = new _FormReplace (MousePos (), (_FormEdit *) Button->Form);
  }

void ActionHexASCII (_Button *Button)
  {
    _FormEdit *fEdit;
    char *Text;
    int s0, s1;
    int ld, ls;
    byte *Data;
    bool Ascii;
    char *Str, *s;
    char c;
    //
    fEdit = (_FormEdit *) Button->Form;
    Text = fEdit->eContent->TextGet ();
    s0 = fEdit->eContent->Selected [0];
    s1 = fEdit->eContent->Selected [1];
    if (s0 == s1)
      Beep ();
    else
      {
        Data = NULL;
        if (s0 > s1)
          Swap (s0, s1);
        fEdit->eContent->MoveToIndex (s0);
        c = Text [s1];
        Text [s1] = '\0';
        ld = StrGetHexAscii (&Text [s0], NULL, 0, NULL);
        if (ld < 0)   // Error
          Beep ();
        else
          {
            Data = (byte *) malloc (ld);
            StrGetHexAscii (&Text [s0], Data, ld, &Ascii);
          }
        Text [s1] = c;
        if (Data)
          {
            s = Str = NULL;
            ls = 0;
            while (true)
              {
                if (Ascii)
                  ls = DataToHex (&s, Data, ld);
                else
                  ls = DataToHexAscii (&s, Data, ld, 80);
                if (Str)
                  break;
                s = Str = (char *) malloc (ls + 1);
                ls = 0;
              }
            // delete insert
            fEdit->eContent->Delete (s0, s1 - s0);
            fEdit->eContent->Insert (s0, ls, Str);
            //fEdit->eContent->Selected [1] = s0;
            fEdit->eContent->Selected [1] = s0 + ls;
            //fEdit->eContent->MoveToIndex (s0);
            fEdit->eContent->Invalidate ();
            free (Str);
            free (Data);
          }
      }
  }

void ActionGoto (_FormEdit *fEdit)
  {
    int l;
    char *s;
    //
    s = fEdit->eLineNumber->TextGet ();
    l = StrGetNum (&s);
    //DebugAddInt ("Going to ", l); // ####
    fEdit->eContent->MoveToPosn ({0, l - 1});
    fEdit->eContent->FocusSet ();
  }

void ActionClipboard (_Button *Button)
  {
    _FormEdit *fEdit;
    //
    fEdit = (_FormEdit *) Button->Form;
    fEdit->eContent->Clipboard ((_ClipboardOp) Button->DataInt);
  }

void ActionJumpMatching (_Container *Container)
  {
    _FormEdit *fEdit;
    char *Text;
    int Pos;
    //
    fEdit = (_FormEdit *) Container->Form;
    Text = fEdit->eContent->TextGet ();
    Pos = FindMatchingBracket (Text, fEdit->eContent->Selected [0]);
    if (Pos >= 0)   // there is a match
      {
        fEdit->eContent->MoveToIndex (Pos);
        fEdit->eContent->ScrollToCursor ();
        fEdit->eContent->Invalidate ();
      }
    else
      Beep_ (fEdit);
  }

void ActionBookmark (_Button *Button)
  {
    _FormEdit *fEdit;
    _EditMultiline *eCon;
    //
    fEdit = (_FormEdit *) (Button->Form);
    eCon = fEdit->eContent;
    eCon->BookmarkSwap ();
  }

/*void ActionMacroRecordSelection (_SelectionList *SelectionList)
  {
    int Mac;
    _FormEdit *fEdit;
    char St [64], *ps;
    //
    Mac = SelectionList->Selected;
    if (Mac >= 0)   // Start Recording
      {
        if (Settings.Macros [Mac] == NULL)
          Settings.Macros [Mac] = (int *) malloc ((MacroMax + 1) * sizeof (int));
        MacroRecording = Settings.Macros [Mac];
        MacroRecording [0] = 0;
        MacroRecordingIndex = 0;
        fEdit = (_FormEdit *) SelectionList->Form;
        ps = St;
        StrCat (&ps, "Recording Macro \a1F");
        NumToStr (&ps, Mac + 1);
        *ps = 0;
        fEdit->lStatus->TextSet (St);
      }
    SelectionList->Die ();
  }*/

void ActionMacroRecord (_FormEdit *fEdit, int Mac)
  {
    char St [64], *ps;
    //
    if (MacroRecording == NULL)   // not recording a Macro
      {
        if (Settings.Macros [Mac] == NULL)   // New Macro
          Settings.Macros [Mac] = (int *) malloc ((MacroMax + 1) * sizeof (int));
        MacroRecording = Settings.Macros [Mac];   // Start Recording
        MacroRecording [0] = 0;
        MacroRecordingIndex = 0;
        ps = St;
        StrCat (&ps, "Recording Macro \a1F");
        NumToStr (&ps, Mac + 1);
        *ps = 0;
        fEdit->lStatus->TextSet (St);
      }
    else   // already recording
      {
        if (MacroRecordingIndex == 0)   // but empty
          fEdit->lStatus->TextSet ("\a2Macro Removed");
        else
          fEdit->lStatus->TextSet ("\a2Macro Complete");
        MacroRecording = NULL;
      }
  }

/*void ActionMacroRecord (_Button *Button)
  {
    _FormEdit *fEdit;
    //
    fEdit = (_FormEdit *) (Button->Form);
    if (Button->Down)   // Select a Fn button ...
      fEdit->slMacros = new _SelectionList (fEdit->Container, {Button->Rect.x, Button->Rect.y + Button->Rect.Height, 16, 16},
                              "F1\tF2\tF3\tF4\tF5\tF6\tF7\tF8\tF9\tF10\tF11\tF12", (_Action) ActionMacroRecordSelection);
    else
      if (MacroRecording == NULL)   // No Fn button selected
        fEdit->slMacros->Die ();
      else   // finish recording
        {
          if (MacroRecording [0] == 0)   // empty macro
            fEdit->lStatus->TextSet ("\a2Macro Removed");
          else
            fEdit->lStatus->TextSet ("\a2Macro Complete");
          MacroRecording = NULL;
        }
  }*/

void ActionUndo (_Button *Button)
  {
    _FormEdit *fEdit;
    //
    fEdit = (_FormEdit *) (Button->Form);
    fEdit->eContent->Undo ();
  }

void ActionSave (_Button *Button)
  {
    _FormEdit *fEdit;
    //
    fEdit = (_FormEdit *) (Button->Form);
    if (fEdit->eContent->Changed)
      {
        if (fEdit->TextResident)   // Save to Resident text
          {
            StrAssignCopy (fEdit->TextResident, fEdit->eContent->TextGet ());
            fEdit->eContent->Changed = false;
          }
        else
          if (fEdit->eContent->FileSave (fEdit->Filename))
            if (FileAttrWrite (fEdit->Filename, fEdit->Attrib))
              Log ("Written", fEdit->Filename);
        if (fEdit->eContent->Changed)   // could not save
          MessageNoSave ();
        else
          fEdit->bSave->EnabledSet (false);
      }
  }

void ActionFontSelected (char *Filename, void *Parameter)
  {
    _FormEdit *fEdit;
    //
    fEdit = (_FormEdit *) Parameter;
    StrAssignCopy (&Settings.EditFont, Filename);
    fEdit->eContent->FontSet (Filename, Settings.EditFontSize);
    fEdit->eContent->Invalidate ();
  }

void ActionFont (_Container *Button)
  {
    FileSelect ("Select Font File", ".ttf", false, ActionFontSelected, Button->Form);
  }

void ActionSize (_ButtonArrow *bSize)
  {
    _FormEdit *fEdit;
    //
    fEdit = (_FormEdit *) bSize->Form;
    if (fEdit->eContent->Font)
      {
        Settings.EditFontSize = fEdit->eContent->Font->Size;
        if (bSize->Direction == dUp)
          Settings.EditFontSize += 1;
        else
          Settings.EditFontSize = Max (4, Settings.EditFontSize - 1);
        fEdit->eContent->FontSet (Settings.EditFont, Settings.EditFontSize);
        fEdit->eContent->Invalidate ();
      }
  }

char* SyntaxMenuString (void)
  {
    char *MenuString;
    char *s;
    int ID;
    int len;
    //
    MenuString = NULL;
    s = NULL;
    len = 0;
    ID = 0;
    while (true)
      {
        if (Syntax [ID].Name)   // not end of list
          {
            if (len)
              len += StrCat (&s, '\t');
            len += StrCat (&s, "    ");
            len += StrCat (&s, Syntax [ID].Name);
            ID++;
          }
        else   // end
          if (MenuString == NULL)
            {
              MenuString = (char *) malloc (len + 1);
              s = MenuString;
              len = 0;
              ID = 0;
            }
          else
            break;
      }
    *s = 0;
    //#### DebugAddS ("SyntaxMakeShowMenu", MenuString);
    return MenuString;
  }

void ActionPopupSyntax (_SelectionList *List)
  {
    _FormEdit *fEdit;
    //
    fEdit = (_FormEdit *) List->Form;
    //DebugAddInt ("ActionPopupSyntax", List->Selected);   //####
    if (List->Selected >= 0)
      {
        fEdit->SyntaxID = List->Selected;
        fEdit->lStatus->Colour = Syntax [fEdit->SyntaxID].Colour;
        fEdit->StatusFill ();
        fEdit->eContent->Invalidate ();
      }
    List->Die ();
  }

void ActionPopupConvert (_SelectionList *List)
  {
    _EOL NewEOL;
    _FormEdit *fEdit;
    char *Text;
    //
    if (List->Selected >= 0)
      {
        NewEOL = (_EOL) List->Selected;
        fEdit = (_FormEdit *) List->Form;
        if (fEdit->eContent->EOL != NewEOL)
          {
            Text = StrReplace (fEdit->eContent->TextGet (), EOLText [fEdit->eContent->EOL], EOLText [NewEOL]);
            fEdit->eContent->TextSet (Text);
            fEdit->eContent->EOL = NewEOL;
            fEdit->eContent->Changed = true;
            fEdit->bSave->EnabledSet (true);
            fEdit->StatusFill ();   // now redraw status
          }
      }
    List->Die ();
  }

const char MessageEditHelp [] =
  "Edit Help\v"
  "\a+\ab\auStewy's Editor\ab\au\a- \eT\n\n"
  //"  -Text and Binary files\n"
  //"  -Syntax Highlighting\n"
  "\auToolbar\au: "
  "Buttons for Bookmark Swap, Record Macros, Cut, Copy, Paste, Undo, Save, Search & Fonts.\n\n"
  //"The Font button allows font selection and the small arrow buttons change the font Size.\n\n"

  "\eN Swap with \ab^B\abookmark\n"
  "\eX Undo (\ab^Z\ab)\n"
  "\e9 \ab^S\abave.\n\n"

  "\eJ Find \ab^N\abext\n"
  "\eI Find \ab^P\abrevious.\n"
  "\ab[aA]\ab Toggle Case Sensitive\n"
  "\ab[#@?]\ab Toggle Wild Cards. This matches any digit with a \ab#\ab, "
  "any letter with a \ab@\ab and anything with a \ab?\ab.\n\n"

  "\eY \ab^R\abeplace instances of the Target anywhere within the current Selection.\n\n"

  "\ea Font change, \eK\eL size change.\n\n"

  "\auFooter\au: Current Line Number, End of Line encoding, Character Encoding, Syntax Highlight.\n\n"

  "\auKeys\au:\n\n"

  "\ab^F\abind current word.\n\n"

  "\ab^<space>\ab: Auto Complete word."
  "Searches backwards matches. "
  "Press repeatedly to find desired match.\n\n"

  "\ab^Del\ab = Delete word.\n"
  "\abSh-Del\ab = Delete Line.\n"
  "\abAlt-Del\ab = Delete to End of Line.\n\n"

  "\abEnter\ab = Insert Line Break with Auto Indenting.\n"
  "\ab^Enter\ab = as above without a line break.\n\n"

  "\ab^\eI\ab and \ab^\eJ\ab: Move by word.\n"
  "\ab^K\ab: Change case of word(s) {aaa Aaa AAA}.\n"
  "\ab^W\abord Select.\n\n"

  "\ab^D\abuplicate Line / Selection.\n"
  "\abAlt\eK\ab and \abAlt\eL\ab: Move a line up / down.\n"
  "\abAlt\eI\ab and \abAlt\eJ\ab: Outdent / Indent line(s).\n\n"

  "\ab^\eK\ab / \ab^\eL\ab: Scroll Text up / down.\n"
  "\ab^]\ab: Jump to matching bracket.\n\n"

  "\auContext Menu\au:\n\n"

  "\eQ Compare Selection with Clipboard: "
  "Opens a window showing the textual differences in colour.\n\n"

  "Convert Hex <-> ASCII: Show Hex as \"ASCII\" and the reverse.\n\n"

  "\ab^G\aboto Line number.\n\n"

  "Explore \ai<Path><File>\ai in the File Browser.\n\n"

  "\eu Syntax: Change the type of Syntax Highlighting.\n\n"

  "Convert EOL: "
  "Convert the text to a different End Of Line coding. "
  "Supports \ablf\ab, \abcr\ab and \abcrlf\ab.\n\n"

  "\auRecordable Macros\au: "
  "Assign common or complex edits to the Function Keys:\n"
  "  Record a macro on \abF<x>\ab key :\n"
  "    Press \ab^F<x>\ab.\n"
  "    It now records the Macro as you type / edit.\n"
  "    Click \ab^F<x>\ab again to complete.\n"
  "  Playback: Press \abF<x>\ab.";

void ActionEditPopup (_MenuPopup *Menu)
  {
    _FormEdit *fEdit;
    _Rect Pos;
    char *MenuString;
    _SelectionList *sl;
    //
    fEdit = (_FormEdit *) Menu->Form;
    switch (Menu->Selected)
      {
        case 0:
          fEdit->eContent->Clipboard (coCut);
          break;
        case 1:
          fEdit->eContent->Clipboard (coCopy);
          break;
        case 2:
          fEdit->eContent->Clipboard (coPaste);
          break;
        case 3:
          fEdit->eContent->Clipboard (coSelectAll);
          break;
        case 4: // text Compare
          //fEdit->CompareClipboard = true;
          if (ClipboardGet (fEdit->Window))
            TextCompare (fEdit, ClipboardData);
          break;
        case 5: // Hex <-> ASCII
          ActionHexASCII (fEdit->bBookmark);
          break;
        case 6: // Undo
          fEdit->eContent->Undo ();
          break;
        case 7: // swap Bookmark
          ActionBookmark (fEdit->bBookmark);
          break;
        case 8: // Find
          ActionFindFocus (fEdit);
          break;
        case 9: // line Goto #
          ActionGotoFocus (fEdit);
          break;
        case 10: // Goto Path / File
          ActionGotoPathFile (fEdit);
          break;
        case 11: // Syntax Highlight select
          MenuString = SyntaxMenuString ();
          Pos = {Menu->SelectionList->Rect.x, Menu->SelectionList->Rect.y, 0, 0};
          sl = new _SelectionList (Menu->SelectionList->Parent, Pos, MenuString, (_Action) ActionPopupSyntax);
          sl->Dotted = Bit [fEdit->SyntaxID];
          free (MenuString);
          break;
        case 12: // Convert EOL
          Pos = {Menu->SelectionList->Rect.x, Menu->SelectionList->Rect.y, 0, 0};
          sl = new _SelectionList (Menu->SelectionList->Parent, Pos, "    Convert to LF\t    Convert to CRLF\t    Convert to CR", (_Action) ActionPopupConvert);
          sl->Dotted = Bit [fEdit->eContent->EOL];
          break;
        case 13: // Help
          MessageBox ({500, 300}, (char *) MessageEditHelp, 14);   // Help
          fMessageBox->Container->Bitmap = BitmapLoad (fMessageBox->Window, "FileBrowser.bmp");
          fMessageBox->lMessage->AlignVert = aTop;
          fMessageBox->lMessage->Align = aLeft;
          fMessageBox->lMessage->Margin = 4;
          fMessageBox->Container->Colour = cBlueLight;
          fMessageBox->Container->ColourGrad = cWhite;
          break;
      };
  }


void ActionEditClose (int Selection, void *Parameter)
  {
    _FormEdit *fEdit;
    //
    if (Parameter)
      {
        fEdit = (_FormEdit *) (((_Container *) Parameter)->Form);
        if (Selection == 0)   // Save File & Quit
          {
            ActionSave (fEdit->bSave);
            if (!fEdit->eContent->Changed)
              fEdit->Die ();
          }
        else if (Selection == 2)   // Discard Changes
          fEdit->Die ();
        else   // Keep Editing
          ;
      }
  }

void OpenUnsavedChangesWindow (_Container *Parameter)
  {
    MessageResponseBox ({380, 80}, "Warning\vUnsaved Changes:", "Save File\tKeep Editing\t\a1Discard Changes", ActionEditClose, Parameter);
    fMessageResponseBox->Container->Colour = cBlueLight;
    fMessageResponseBox->Container->ColourGrad = -1;
  }

bool FormEditEventPreview (_Form *Form, _Event *Event)
  {
    _FormEdit *fEdit;
    bool Res;
    //
    if (fMessageResponseBox)
      return true;
    Res = false;
    fEdit = (_FormEdit *) Form;
    fEdit->bUndo->EnabledSet (fEdit->eContent->UndoAvailable ());
    if (FileSelectActive)   // Picking a Font, so ignore everything
      Res = true;
    else if (Event->Type == etWindowClose)
      {
        if (fEdit->eContent->Changed)   // unsaved changes
          {
            OpenUnsavedChangesWindow (fEdit->Container);
            Res = true;
          }
      }
    else if (Event->Type == etKeyDown)
      {
        Res = true;
        if (Event->Key == esc)   // Escapr pressed
          if (fEdit->Focused == fEdit->eContent)
            if (fEdit->eContent->Changed)   // unsaved changes
              OpenUnsavedChangesWindow (fEdit->Container);
            else
              fEdit->Die ();   // no changes so Close window
          else
            fEdit->eContent->FocusSet ();   // return focus to edit control
        else if (Event->Key == Cntrl (']'))   // Jump to matching backet/brace
          ActionJumpMatching (Form->Container);
        else if (Event->Key == Cntrl ('F'))   // Find
          ActionFindFocus (fEdit);
        else if (Event->Key == Cntrl ('G'))   // Goto
          ActionGotoFocus (fEdit);
        //else if (Event->Key == Cntrl ('J'))   // Jump to file
        //  ActionGotoPathFile (fEdit);
        else if (Event->Key == Cntrl ('N'))   // Find Next
          ActionFind (fEdit->bFindNext);
        else if (Event->Key == Cntrl ('P'))   // Find Prev
          ActionFind (fEdit->bFindPrev);
        else if (Event->Key == Cntrl ('R'))   // Replace
          ActionReplace (fEdit->bReplace);
        else if (Event->Key == Cntrl ('E'))   // Hex / ASCII Convert
          ActionHexASCII (fEdit->bFindNext);
        else if (Event->Key == Cntrl ('S'))   // Save
          ActionSave (fEdit->bSave);
        //else if (Event->Key == Cntrl ('O'))   // Compare with Clipboard
        //  {
        //    //fEdit->CompareClipboard = true;
        //    if (ClipboardGet (fEdit->Window))
        //      TextCompare (fEdit, ClipboardData);
        //  }
        else if (Event->Key == KeyEnter)   // Key Enter. Action depends on Focus
          if (fEdit->Focused == fEdit->eTarget->EditDrop->EditPopup)   // Target: Key Enter => Find Next
            ActionFind (fEdit->bFindNext);
          else if (fEdit->Focused == fEdit->eLineNumber)
            ActionGoto (fEdit);
          else   // leave for the editor
            Res = false;
        else if ((Event->Key & KeyMax) >= KeyF1 && (Event->Key & KeyMax) - KeyF1 < SIZEARRAY (Settings.Macros))   // F1.. MACROS
          {
            if (Event->Key & KeyCntl)
              ActionMacroRecord ((_FormEdit *) Form, (Event->Key & KeyMax) - KeyF1);
            else
              fEdit->KeySeries = Settings.Macros [Event->Key - KeyF1];
            Event->Key = 0;   // Don't record this
          }
        else   // record next key of current macro being recorded
          Res = false;
        if (MacroRecording && (Event->Key & KeyMax))
          {
            if (MacroRecordingIndex < MacroMax)
              MacroRecording [MacroRecordingIndex++] = Event->Key;
            MacroRecording [MacroRecordingIndex] = 0;   // terminate as we go
          }
      }
    return Res;
  }

const char *EOLName [] = {"LF", "CRLF", "CR"};
const char *UTFName [] = {"ASCII", "UTF8", "UTF16"};

_FormEdit::_FormEdit (char *Filename_, _Point Pos, bool Hex, char **Text_): _Form (Filename_, {Pos.x, Pos.y, EditSize.x, EditSize.y}, waResizable)
  {
    const int th = 28;
    const int bw = 28;//64;
    //const int tw = 160;
    const int sh = 20;
    int x1, x2;
    char *t;
    //
    Filename = NULL;
    //CompareClipboard = false;
    MacroRecording = NULL;
    StrAssignCopy (&Filename, Filename_);
    TextResident = Text_;
    Container->FontSet (NULL, 0);   // Default font and size
    Container->Bitmap = BitmapLoad (Window, "FileBrowser.bmp");
    Toolbar = new _Container (Container, {0, 0, 0, th});
    x1 = 0;
    bBookmark = new _Button (Toolbar, {x1, 0, th, 0}, "\eN", (_Action) ActionBookmark);
    x1 += th;
    //bMacro = new _Button (Toolbar, {x1, 0, th, 0}, "\eP", (_Action) ActionMacroRecord);
    //bMacro->Toggle = true;
    //x1 += th;
    x1 += 8;   // Gap
    bCut = new _Button (Toolbar, {x1, 0, bw, 0}, "\e0", (_Action) ActionClipboard);// Cut
    bCut->DataInt = coCut;
    x1 += bw;
    bCopy = new _Button (Toolbar, {x1, 0, bw, 0}, "\e1", (_Action) ActionClipboard);// Copy
    bCopy->DataInt = coCopy;
    x1 += bw;
    bPaste = new _Button (Toolbar, {x1, 0, bw, 0}, "\e2", (_Action) ActionClipboard);// Paste
    bPaste->DataInt = coPaste;
    x1 += bw;
    x1 += 8;   // Gap
    bUndo = new _Button (Toolbar, {x1, 0, bw, 0}, "\ex", (_Action) ActionUndo);// Undo
    x1 += bw;
    bSave = new _Button (Toolbar, {x1, 0, bw, 0}, "\e9", (_Action) ActionSave);// Save
    bSave->EnabledSet (false);
    x1 += bw;
    x1 += 8;   // Gap
    //
    //bFind = new _Button (Toolbar, {x1, 0, 16, 0}, "\eB");
    //bFind->Colour = -1;
    //bFind->ColourText = bFind->ColourFind (); // -1;
    //bFind->EnabledSet (false);
    //x1 += 16;
    bFindPrev = new _ButtonArrow (Toolbar, {x1, 0, th, 0}, NULL, (_Action) ActionFind, dLeft);   // Square arrow button
    x1 += th;
    bFindNext = new _ButtonArrow (Toolbar, {x1, 0, th, 0}, NULL, (_Action) ActionFind, dRight);
    //bFindNext->RectLock |= rlLeft | rlRight;
    x1 += th;
    x2 = -th / 2;   // Start from the Right
    bSizeUp = new _ButtonArrow (Toolbar, {x2, 0, th / 2, th / 2}, NULL, (_Action) ActionSize, dUp);
    bSizeDn = new _ButtonArrow (Toolbar, {x2, th / 2, th / 2, 0}, NULL, (_Action) ActionSize, dDown);
    x2 -= th;
    bFont = new _Button (Toolbar, {x2, 0, th, 0}, "\ea", ActionFont);
    x2 -= 8;   // add a Gap
    x2 -= th;
    bReplace = new _Button (Toolbar, {x2, 0, th, 0}, "\eY", (_Action) ActionReplace);
    eTarget = new _EditSearch (Toolbar, {x1, 0, x2, 0}, Settings.EditTarget);
    t = StrGetItem (Settings.EditTarget, 0);
    eTarget->EditDrop->EditPopup->TextSet (t);
    free (t);
    eTarget->EditDrop->EditPopup->Colour = ColourAdjust (Toolbar->ColourFind (), 150);
    eTarget->RectLock |= rlRight;
    //
    eContent = new _EditMultiline (Container, {0, th, 0, -sh}, NULL, (_Action) ActionChange);
    eContent->EditDrawLineCallBack = EditDrawLineCallBackSyntax;
    eContent->TextCallBack = TextCallBackEditHighlight;
    eContent->FontSet (Settings.EditFont, Settings.EditFontSize);
    eContent->FocusSet ();
    ColourDefault = eContent->ColourTextFind ();
    pmEdit = new _MenuPopup (eContent, {0},
      "\e0 Cut\t\e1 Copy\t\e2 Paste\t\eO Select All\t\eQ Compare with CB\t_Convert Hex <=> \"ASCII\"\t_\eX Undo\t\eN Bookmarks swap\t\eB Find\tGoto Line ..\t_Explore \ai<Path><File>\ai\t\eu Syntax ...\t_Convert EOL ...\tHelp",
      (_Action) ActionEditPopup);
    Status = new _Container (Container, {0, -sh, 0, 0});
    eLineNumber = new _Edit (Status, {0, 0, 80, 0}, NULL);
    eLineNumber->Colour = ColourAdjust (Status->ColourFind (), 150);
    //
    SyntaxID = 0;
    Attrib = -1;
    if (TextResident)   // Edit resident Text
      eContent->TextSet (*TextResident);
    else if (eContent->FileLoad (Filename, Hex))   // Edit File
      Attrib = FileAttrRead (Filename);
    else   // Can not load Filename
      eContent->TextSet ("********** File Unreadable OR Too big");
    //
    lStatus = new _Label (Status, {80, 0, 0, 0}, NULL, aLeft, bLowered);
    //
    // Determine Syntax Highlight set and add syntax indicator
    SyntaxID = SyntaxHighlightSelect (Filename, eContent->Hex, TextResident);
    //eContent->Colour = Syntax [SyntaxID].Colour;
    lStatus->Colour = Syntax [SyntaxID].Colour;
    //
    StatusFill ();
    EventPreview = FormEditEventPreview;
    CurrentLine = -1;
  }

_FormEdit::~_FormEdit ()
  {
    free (Filename);
    WindowGetPosSize (Window, NULL, NULL, &EditSize.x, &EditSize.y);
    free (LineColours);
    LineColours = NULL;
    if (fReplace)
      fReplace->Die ();
    if (fMessageBox)
      fMessageBox->Die ();
  }

void _FormEdit::StatusFill (void)
  {
    char St [64], *s;
    //
    s = St;
    StrCat (&s, '\v');
    //StrCat (&s, "Line ");
    //NumToStr (&s, CurrentLine + 1);
    if (!eContent->Hex)
      {
        StrCat (&s, "EOL-");
        StrCat (&s, EOLName [eContent->EOL]);
        StrCat (&s, "  ");
        StrCat (&s, UTFName [eContent->UTF]);
        StrCat (&s, "  ");
      }
    StrCat (&s, Syntax [SyntaxID].Name);
    *s = 0;
    lStatus->TextSet (St);
  }

void FileEdit (char *Filename, bool Hex, char **Text = NULL)
  {
    _Point Mouse;
    //
    Mouse = MousePos ();
    new _FormEdit (Filename, Mouse, Hex, Text);
  }
