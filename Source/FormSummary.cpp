////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SUMMARY FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _SummaryItem
  {
    _SummaryItem *Next, *Prev;
    char *Extension;
    int Count;
    longint TotalSize;
  } _SummaryItem;

_SummaryItem* SummaryFind (_SummaryItem **SummaryList, char *Ext)
  {
    _SummaryItem *Res;
    //
    Res = *SummaryList;
    while (true)
      {
        if (Res == NULL)
          {
            Res = (_SummaryItem *) malloc (sizeof (_SummaryItem));
            *Res = {0};
            StrAssignCopy (&Res->Extension, Ext);
            Res->Count = 0;
            Res->TotalSize = 0;
            ListAddEnd (Res, SummaryList);
            return Res;
          }
        if (StrSame_ (Res->Extension, Ext))   // Found
          return Res;
        Res = Res->Next;
      }
  }

void SummaryListFree (_SummaryItem *si)
  {
    _SummaryItem *si_;
    //
    while (si)
      {
        free (si->Extension);
        si_ = si;
        ListRemove (si, &si);
        free (si_);
      }
  }

class _FormSummary: public _Form
  {
    public:
      _GridView *gFileTypes;
      _Container *Toolbar;
      //_Button *bTick, *bProceed, *bAbort, *bCompare;
      _FormSummary (_Point Pos, _SummaryItem *SummaryList_);
     ~_FormSummary ();
      //
      _SummaryItem *SummaryList = NULL;
      int SummaryIndex = -1;
      _SummaryItem *SummaryItem = NULL;
      int SummarySort = 0;
  };

//_FormSummary *fSummary = NULL;

char *SummaryTitles [] = {"\abExtension\ab", "\abCount\ab", "\abTotal Size\ab"};
int SummaryWidths [] = {100, 60, 120};

void SummaryDrawCell (_GridView *Grid, int x, int y, _Rect Rect)
  {
    _FormSummary *fs;
    const int Margin = 4;
    char St [128], *s;
    //
    fs = (_FormSummary *) Grid->Form;
    s = St;
    if (y == 0)
      StrCat (&s, SummaryTitles [x]);
    else
      if (ListGoto (fs->SummaryList, y - 1, &fs->SummaryIndex, &fs->SummaryItem))
        if (x == 0)
          StrCat (&s, fs->SummaryItem->Extension);
        else if (x == 1)
          IntToStr (&s, fs->SummaryItem->Count, DigitsCommas);
        else
          IntToStr (&s, fs->SummaryItem->TotalSize, DigitsCommas);
    *s = 0;
    Rect.x += Margin;
    Rect.Width -= 2 * Margin;
    if (*St)
      if (x == 0)
        Grid->TextOutAligned (Rect, St, aLeft, aCenter);
      else
        Grid->TextOutAligned (Rect, St, aRight, aCenter);
  }

int SummarySortCopy;

int SummaryItemsCompare (_SummaryItem *Data1, _SummaryItem *Data2)
  {
    longint delta;
    //
    delta = 0;
    if (SummarySortCopy == 1)   // Sort by Count
      delta = Data1->Count - Data2->Count;
    else if (SummarySortCopy == 2)   // Sort by Total Size
      delta = Sign (Data1->TotalSize - Data2->TotalSize);
    if (delta == 0)
      delta = StrCmp (Data1->Extension, Data2->Extension);
    return delta;
  }

void ActionSummaryColumnHighlight (_GridView *Grid, int x)
  {
    _FormSummary *fs;
    //
    fs = (_FormSummary *) Grid->Form;
    fs->SummarySort = x;
    DebugAddInt ("Summary Sort", fs->SummarySort);  //####
    SummarySortCopy = fs->SummarySort;
    fs->SummaryList = ListSort (fs->SummaryList, SummaryItemsCompare);
    fs->SummaryIndex = -1;
    fs->gFileTypes->CellOffset = {0, 0};
    fs->gFileTypes->Selected0 = fs->gFileTypes->Selected1 = {0, 1};
    fs->gFileTypes->ScrollToSelected ();
    fs->gFileTypes->Invalidate ();
  }

char *SummaryCellRead (_GridView *Grid, int x, int y)
  {
    _FormSummary *fs;
    _DirItem *di;
    int Row;
    //
    fs = (_FormSummary *) Grid->Form;
    if (ListGoto (fs->SummaryList, y - 1, &fs->SummaryIndex, &fs->SummaryItem))
      {
        di = Dir;
        Row = 1;
        while (true)
          {
            if (di == NULL)
              {
                Beep ();
                break;
              }
            if (!di->Directory)
              if (StrSame_ (StrFindFileExtension (di->Name), fs->SummaryItem->Extension))
                {
                  fMain->gFiles->Selected0.y = fMain->gFiles->Selected1.y = Row;
                  fMain->gFiles->ScrollToSelected ();
                  fMain->gFiles->Invalidate ();
                  break;
                }
            di = di->Next;
            Row++;
          }
      }
    return NULL;
  }

bool FormSummaryEventPreview  (_Form *Form, _Event *Event)
  {
    if (Event->Type == etKeyDown && Event->Key == esc)
      {
        Form->Die ();
        return true;
      }
    return false;
  }

_FormSummary::_FormSummary (_Point Pos, _SummaryItem *SummaryList_): _Form ("Summary of File Types", {Pos.x, Pos.y, 280, 300}, waResizable | waAlwaysOnTop)
  {
    //const int th = 32;
    //const int bw = 64;
    int x;
    //
    // Grab (and sort) the data
    SummarySortCopy = SummarySort;
    SummaryList = ListSort (SummaryList_, SummaryItemsCompare);
    // Set up everything
    Container->FontSet (NULL);
    //Container->Bitmap = BitmapLoadResource (Window, "FileBrowser.bmp"); not used yet
    gFileTypes = new _GridView (Container, {0, 0, 0, 0}, NULL, 3, ListLength (SummaryList) + 1, SummaryDrawCell);
    gFileTypes->Colour = cAquaLight; //0xFFF0F0; //0xC0FFE0;
    gFileTypes->Options = goGridLines | goSelectMulti | goSelectLine | goAdjustColumn | goColumnHighlight;
    gFileTypes->Title = gFileTypes->Locked = {0, 1};
    gFileTypes->ColumnHighlight = 0;
    for (x = 0; x < SIZEARRAY (SummaryWidths); x++)
      gFileTypes->CellWidthSet (x, SummaryWidths [x]);
    gFileTypes->ActionCellRead = SummaryCellRead;
    gFileTypes->ActionColumnHighlight = ActionSummaryColumnHighlight;
    //
    //Toolbar = new _Container (Container, {0, -th, 0, 0});
    //Toolbar->RectLock |= rlTop;
    EventPreview = FormSummaryEventPreview;
  }

_FormSummary::~_FormSummary (void)
  {
    SummaryListFree (SummaryList);
  }
