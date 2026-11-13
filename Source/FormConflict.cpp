////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CONFLICT FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class _FormConflict: public _Form
  {
    public:
      _GridView *gFiles;
      _Container *Toolbar;
      _Button *bTick, *bProceed, *bAbort, *bCompare;
      _Label *lHelp;
      //
      //void Update (void);
      _FormConflict (_Point Pos);
     ~_FormConflict ();
  };

_FormConflict *fConflict;

int ConflictSort = 0;

char *ConflictTitles [] = {"\e8", "\abFile (by name)\ab", "\abDestination File\ab"};
int ConflictWidths [] = {24, 200, 200};

void ConflictCellDraw (_GridView *Grid, int x, int y, _Rect Rect)
  {
    const int Margin = 4;
    char Issue [128], *i;
    char *St;
    int h;
    //
    //DebugAddP ("ConflictDrawCell", {x, y});
    St = NULL;
    if (y == 0)
      if (x == 1 && ConflictSort == 2)   // Name column & sorting by extension
        St = "\abFile (by ext)\ab";
      else
        St = ConflictTitles [x];
    else
      if (ListGoto (ConflictList, y - 1, &ConflictIndex, &ConflictItem))
        if (x == 0)
          {
            if (ConflictItem->Ticked)
              St = ConflictTitles [0];
          }
        else
          {
            if (x == 1)
              St = &ConflictItem->DestPath [ConflictItem->CommonDestPath];
            else if (x == 2)
              {
                i = Issue;
                if (ConflictItem->Op & OpError)
                  StrCat (&i, "Error Opening file");
                else if (ConflictItem->Op & OpDelete)
                  StrCat (&i, "Error Deleting item");
                else   // Copy / Move
                  {
                    if (ConflictItem->DeltaTime)
                      {
                        if (ConflictItem->DeltaTime > 0)
                          StrCat (&i, "Newer by ");
                        else
                          StrCat (&i, "Older by ");
                        TimeDeltaToStr (&i, Abs (ConflictItem->DeltaTime));
                        StrCat (&i, ' ');
                      }
                    if (ConflictItem->DeltaSize)
                      {
                        if (ConflictItem->DeltaSize < 0)
                          StrCat (&i, "Smaller by ");
                        else
                          StrCat (&i, "Larger by ");
                        IntToStr (&i, Abs (ConflictItem->DeltaSize), DigitsCommas);
                      }
                  }
                *i = 0;
                St = Issue;
              }
            if  (ConflictItem->Op & OpError)
              Grid->FontFind ()->Colour = cRed;
          }
    Rect.x += Margin;
    Rect.Width -= 2 * Margin;
    if (St)
      {
        h = Grid->TextOutWrap (Rect, St, aLeft, aCenter) + 8;
        if (h > Grid->CellHeightGet (y))
          Grid->CellHeightSet (y, h);
      }
      //Grid->TextOutAligned (Rect, St, aLeft, aCenter);
  }

char *ConflictCellRead (_GridView *Grid, int x, int y)
  {
    if (ListGoto (ConflictList, y - 1, &ConflictIndex, &ConflictItem))
      if (~ConflictItem->Op & OpError)
        {
          ConflictItem->Ticked ^= 1;
          Grid->DrawCellsAdd ({0, y});
        }
    return NULL;
  }

void ActionTick (_Container *Button)
  {
    int a, b, c;
    int Pass;
    bool AnyTicked;
    //
    a = fConflict->gFiles->Selected0.y;
    b = fConflict->gFiles->Selected1.y;
    AnyTicked = false;
    for (Pass = 0; Pass <= 1; Pass++)
      {
        c = a;
        while (true)
          {
            if (ListGoto (ConflictList, c - 1, &ConflictIndex, &ConflictItem))
              if (Pass == 0)
                {
                  if (ConflictItem->Ticked)
                    AnyTicked = true;
                }
              else   // Pass == 1
                if (~ConflictItem->Op & OpError)
                  {
                    ConflictItem->Ticked = !AnyTicked;
                    fConflict->gFiles->DrawCellsAdd ({0, c});
                  }
            if (c < b) c++; else if (c > b) c--; else break;
          }
      }
  }

int ConflictItemsCompare (_ConflictItem *Data1, _ConflictItem *Data2)
  {
    longint delta;
    //
    if (ConflictSort == 0)   // Sort by "Tick
      delta = Data2->Ticked - Data1->Ticked;
    else if (ConflictSort == 1)   // Sort by Name
      delta = StrCmp (Data1->DestPath, Data2->DestPath);
    else if (ConflictSort == 2)   // Sort by Extension
      delta = StrCmp (StrFindFileExtension (Data1->DestPath), StrFindFileExtension (Data2->DestPath));
    else if (ConflictSort == 3)   // Sort by Delta Time
      delta = Data1->DeltaTime - Data2->DeltaTime;
    else   // ConflictSort == 4   Sort by Delta Size
      delta = Data1->DeltaSize - Data2->DeltaSize;
      //delta = StrCmp (i1->Issue, i2->Issue);
    if (delta == 0)
      delta = StrCmp (Data1->DestPath, Data2->DestPath);
    return delta;
  }

void ActionConflictColumnHighlight (_GridView *Grid, int x)
  {
    if (x == 0)
      ConflictSort = 0;
    else if (x == 1)
      if (ConflictSort == 1)
        ConflictSort = 2;
      else
        ConflictSort = 1;
    else   // x == 2
      if (ConflictSort == 3)
        ConflictSort = 4;
      else
        ConflictSort = 3;
    DebugAddP ("Conflict Sort: {Column, Sort}", {x, ConflictSort});
    free (fConflict->gFiles->CellHeights);
    fConflict->gFiles->CellHeights = NULL;
    ConflictList = ListSort (ConflictList, ConflictItemsCompare);
    ConflictIndex = -1;
    fConflict->gFiles->CellOffset = {0, 0};
    fConflict->gFiles->Selected0 = fConflict->gFiles->Selected1 = {0, 1};
    fConflict->gFiles->ScrollToSelected ();
    fConflict->gFiles->Invalidate ();
  }

char *OpName (byte Op)
  {
    if (Op & OpCopy)
      return "Copy";
    if (Op & OpMove)
      return "Move";
    if (Op & OpDelete)
      return "Delete";
    return "Error";
  }

int ThreadConflict (void *arg)
  {
    _ConflictItem *ConflictList;
    _ConflictItem *ci;
    bool ok;
    //
    DebugAdd ("ThreadConflict"); //####
    Threads++;
    ConflictList = (_ConflictItem *) arg;
    ci = ConflictList;
    while (ci)
      {
        if (ci->Ticked)
          {
            if (ci->Op & OpMove)
              ok = FileMove (ci->SourcePath, ci->DestPath, 0, true);
            else if (ci->Op & OpCopy)
              ok = FileCopy (ci->SourcePath, ci->DestPath, 0, true);
            else if (ci->Op & OpDelete)
              {
                chmod (ci->DestPath, S_IWUSR | S_IWGRP | S_IWOTH);
                ok = DeleteItem (ci->DestPath, 0);
              }
            if (!ok)
              {
                fMain->ErrorAdd (OpName (ci->Op), ci->DestPath);
                break;
              }
          }
        ci = ci->Next;
      }
    Reread = true;
    ConflictListFree (ConflictList);
    Threads--;
    return 0;
  }

void ActionProceed (_Container *Button)
  {
    _ConflictItem *ConflictList_;
    //
    ConflictList_ = ConflictList;
    ConflictList = NULL;   // steal conflict list;
    StartThread (ThreadConflict, ConflictList_);
    fConflict->Die ();
  }

void ActionAbort (_Container *Button)
  {
    Button->Form->Die ();
  }

void ShowResult (char *Result)
  {
    if (Result)
      {
        MessageBox ({800, 400}, Result, 14);
        fMessageBox->lMessage->Colour = cTextBgnd;
        fMessageBox->lMessage->AlignVert = aTop;
        fMessageBox->lMessage->Align = aLeft;
        fMessageBox->lMessage->Margin = 4;
        free (Result);
      }
  }

void ActionCompare (_Container *Button)
  {
    char *TextSource;
    char *TextDest;
    int Size;
    bool Hex;
    //
    if (ListGoto (ConflictList, fConflict->gFiles->Selected0.y - 1, &ConflictIndex, &ConflictItem))
      {
        Hex = false;
        TextSource = FileTextReadAll (ConflictItem->SourcePath, IntMax, &Hex, &Size);
        TextDest = FileTextReadAll (ConflictItem->DestPath, IntMax, &Hex, &Size);
        if (TextSource && TextDest)
          {
            if (fCompare)
              delete fCompare;
            fCompare = new _FormCompare (MousePos (), TextSource, TextDest, "Source", "Destination", 16);
          }
      }
  }

_FormConflict::_FormConflict (_Point Pos): _Form ("Action Conflict", {Pos.x, Pos.y, 550, 300}, waResizable | waAlwaysOnTop)
  {
    const int th = 32;
    const int bw = 64;
    int x;
    //
    Container->FontSet (NULL);
    Container->Bitmap = BitmapLoad (Window, "FileBrowser.bmp");
    gFiles = new _GridView (Container, {0, 0, 0, -th}, NULL, 3, ListLength (ConflictList) + 1, ConflictCellDraw);
    gFiles->Colour = ColourAdjust (cPink, 190);
    gFiles->Options = goGridLines | goSelectMulti | goSelectLine | goAdjustColumn | goColumnHighlight;
    gFiles->Title = gFiles->Locked = {0, 1};
    ConflictSort = 1;   // Name
    gFiles->ColumnHighlight = 1;
    ConflictList = ListSort (ConflictList, ConflictItemsCompare);
    for (x = 0; x < SIZEARRAY (ConflictWidths); x++)
      gFiles->CellWidthSet (x, ConflictWidths [x]);
    gFiles->ActionCellRead = ConflictCellRead;
    gFiles->ActionColumnHighlight = ActionConflictColumnHighlight;
    //
    Toolbar = new _Container (Container, {0, -th, 0, 0});
    Toolbar->RectLock |= rlTop;
    x = 0;
    bTick = new _Button (Toolbar, {x, 0, bw / 2, 0}, "\e8", ActionTick); x += bw / 2;
    bProceed = new _Button (Toolbar, {x, 0, bw, 0}, "\a2Proceed", ActionProceed); x += bw;
    bAbort = new _Button (Toolbar, {x, 0, bw, 0}, "\a1Abort", ActionAbort); x += bw;
    bCompare = new _Button (Toolbar, {x, 0, bw, 0}, "Compare", ActionCompare); x += bw;
    x += th;
    lHelp = new _Label (Toolbar, {x, 0, 0, 0}, "\e8 the files to \abdelete\ab");
  }

_FormConflict::~_FormConflict (void)
  {
    ConflictListFree (ConflictList);
    ConflictList = NULL;
    fConflict = NULL;
  }
