////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SEARCH FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Grid Data Search
int GridIndex = -1;
_DirItem *GridItem = NULL;
int DirSort = 0;   // Sort by: Name, Ext, Size, Attributes
bool DirSortDirFirst = true;
char *SearchTargetHistory = NULL;

class _FormSearch: public _Form
  {
    public:
      _Label *lFilename;
      _EditSearch  *eFilename;
      _Label *lContaining;
      _EditSearch  *eContaining;
      _CheckBox *cbSubFolders;
      _DropList *dlRepeats;
      _Button *bGo, *bAbort;
      _Wait *wSearching;
      _FormSearch (_Point Pos);
  };

_FormSearch *fSearch = NULL;

bool ReadDirCallBackSearch (_DirItem *Item)
  {
    bool Res;
    char *Con;
    int ConLen;
    int File;
    byte *Buffer;
    int Size;
    int a, b;
    //
    Res = false;
    StrPosFlags = fSearch->eFilename->Flags ();
    if (!Item->SymLink)
      if (!StrSame (StrFindFilename (Item->Name), ".."))   //if (StrPos (Item->Name, "..") == NULL)
        if (StrPos (StrFindFilename (Item->Name), fSearch->eFilename->EditDrop->EditPopup->TextGet ()))   // Filename OK
          {
            Res = true;
            Con = fSearch->eContaining->EditDrop->EditPopup->TextGet ();
            ConLen = StrLen (Con);
            if (ConLen)   // Checking Contents?
              if (Item->Directory)   // if contents apply, then reject directories
                Res = false;
              else
                {
                  File = FileOpen (Item->Name, foRead);
                  if (File >= 0)
                    {
                      Buffer = (byte *) malloc (BufferSize);
                      Size = FileRead (File, Buffer, BufferSize);
                      a = b = 0;
                      StrPosFlags = fSearch->eContaining->Flags ();
                      while (true)
                        {
                          if (a + ConLen >= Size)   // not found
                            {
                              Res = false;
                              break;
                            }
                          if (b >= ConLen)   // matched in full
                            break;
                          if (StrPosMatch (Buffer [a + b], Con [b]))   // matching
                            b++;
                          else
                            {
                              a++;   // try next byte along
                              b = 0;   // from the start of the target
                            }
                        }
                      free (Buffer);
                      FileClose (File);
                    }
                }
          }
    StrPosFlags = 0x00;
    return Res;
  }

int DirItemsCompare2 (_DirItem *Data1, _DirItem *Data2)
  {
    _DirItem *i1, *i2;
    int delta;
    char *p1, *p2;
    //
    i1 = (_DirItem *) Data1;
    i2 = (_DirItem *) Data2;
    delta = 0;
    if (DirSortDirFirst)
      delta = (i2->Directory | i2->SymLinkDirectory) - (i1->Directory | i1->SymLinkDirectory);
    if (delta == 0)   // both files (or directories)
      if (DirSort == 0)   // Sort by name
        delta = StrCmp (StrFindFilename (i1->Name), StrFindFilename (i2->Name));
      else if (DirSort == 1)   // Sort by Extension
        delta = StrCmp (StrFindFileExtension (i1->Name), StrFindFileExtension (i2->Name));
      else if (DirSort == 2)   // Sort by Size
        {
          if (i1->Size < i2->Size)
            delta = -1;
          else if (i1->Size > i2->Size)
             delta = +1;
          else
            delta = 0;
        }
      else if (DirSort == 3)   // Sort by DateTime
        delta = i2->DateTime - i1->DateTime;
      else   // sort by Path
        {
          p1 = StrPosLast (Data1->Name, PathDelimiter);
          p2 = StrPosLast (Data2->Name, PathDelimiter);
          if (p1 && p2)
            {
              *p1 = 0;
              *p2 = 0;
              delta = StrCmp (Data1->Name, Data2->Name);
              *p1 = PathDelimiter;
              *p2 = PathDelimiter;
            }
          else
            delta = (p1 != NULL) - (p2 != NULL);
        }
        //delta = i1->Attrib - i2->Attrib;
    if (delta == 0)
      delta = StrCmp (i1->Name, i2->Name);
    return delta;
  }

void RemoveUniquesDuplicates (_DirItem **Dir)
  {
    int Sort;
    _DirItem *d1, *d2;
    //
    if (fSearch->dlRepeats->Selected > 0)
      {
        //int t = ClockMS (); // ####
        // Tag duplicates
        Sort = DirSort;   // save current sort
        DirSort = 2;   // Sort by Size first
        *Dir = ListSort (*Dir, DirItemsCompare2);
        DirSort = Sort;   // restore current sort
        d1 = *Dir;
        while (d1)
          {
            if (DirReadAbort)
              break;
            d2 = d1->Next;
            while (d2)
              {
                if (d2->Size != d1->Size)
                  d2 = NULL;   // size does not match so abort
                else   // Size matches
                  {
                    if (d2->DateTime == d1->DateTime)   // so check date time
                      if (StrSame (StrFindFilename (d2->Name), StrFindFilename (d1->Name)))   // and Name
                        {
                          d1->Tagged = true;
                          d2->Tagged = true;
                        }
                    d2 = d2->Next;
                  }
              }
            d1 = d1->Next;
          }
        // Remove unwanted
        d1 = *Dir;
        while (d1)
          {
            if (DirReadAbort)
              break;
            d2 = d1->Next;
            if (d1->Directory ||   // remove Directories AND
                d1->Tagged == (fSearch->dlRepeats->Selected == 1))   // Unwanted: FileDuplicate == UniqueWanted?
              ListRemove (d1, Dir);
            d1 = d2;
          }
        //DebugAddInt ("RemoveUniquesDuplicates time ", ClockMS () - t); // ####
      }
  }

bool ThreadSearchRunning = false;
//bool ThreadSearchFinished = false;

int ThreadSearch (void *arg)
  {
    ThreadSearchRunning = true;
    DirFree (Dir);
    Dir = NULL;
    DirReadAbort = false;
    DirRead (NULL/*path*/, &Dir, ReadDirCallBackSearch, fSearch->cbSubFolders->Down);
    if (!DirReadAbort)
      {
        RemoveUniquesDuplicates (&Dir);
        Dir = ListSort (Dir, DirItemsCompare2);
        if (!ListCheck (Dir))
          DebugAdd ("LISTSORT BROKEN AGAIN");
      }
    DirReadAbort = false;
    GridIndex = -1;
    // Update Grid
    free (fMain->gFiles->CellHeights);
    fMain->gFiles->CellHeights = NULL;
    fMain->gFiles->Columns = 5;
    fMain->gFiles->Rows = Max (ListLength (Dir) + 1, 2);
    fMain->gFiles->CellOffset = {0, 0};
    fMain->gFiles->Selected0 = fMain->gFiles->Selected1 = fMain->gFiles->Title;
    fMain->gFiles->DrawCellsClear ();
    fMain->FindFileSelected ();
    fMain->gFiles->Invalidate ();
    ShowNumberOfItems ();
    fSearch->wSearching->VisibleSet (false);
    fSearch->bGo->EnabledSet (true);
    fSearch->bAbort->EnabledSet (false);
    ThreadSearchRunning = false;
    //ThreadSearchFinished = true;
    return 0;
  }

void ActionSearchGo (_Container *Container)
  {
    char *Target;
    //
    Target = fSearch->eFilename->EditDrop->EditPopup->TextGet ();
    if (*Target)
      {
        fSearch->eFilename->EditDrop->DropListAdd (Target, 8);
        StrAssignCopy (&SearchTargetHistory, fSearch->eFilename->EditDrop->TextGet ());
      }
    //
    Target = fSearch->eContaining->EditDrop->EditPopup->TextGet ();
    if (*Target)   // Containing target exists
      {
        //if (Dir)   // Files found containing eContaining target
        //StrAddItemStart (&Settings.EditTarget, Target, 8);
        fSearch->eContaining->EditDrop->DropListAdd (Target, 8);
        StrAssignCopy (&Settings.EditTarget, fSearch->eContaining->EditDrop->TextGet ());
      }
    //
    fSearch->wSearching->VisibleSet (true);
    fSearch->bGo->EnabledSet (false);
    fSearch->bAbort->EnabledSet (true);
    StartThread (ThreadSearch, NULL);
  }

void ActionSearchAbort (_Container *Container)
  {
    DirReadAbort = true;
  }

bool FormSearchEventPreview (_Form *Form, _Event *Event)
  {
    if (Event->Type == etWindowClose || (Event->Type == etKeyDown && Event->Key == esc))
      {
        if (!ThreadSearchRunning)
          {
            delete fSearch;
            fSearch = NULL;
          }
        return true;
      }
    else if (Event->Type == etKeyDown && Event->Key == KeyEnter)
      {
        ActionSearchGo (NULL);
        return true;
      }
    return false;
  }

_FormSearch::_FormSearch (_Point Pos): _Form ("File Search", {Pos.x, Pos.y, 400, 140}, waAlwaysOnTop | waResizable)
  {
    int x, y;
    const int gap = 4;
    const int col = 140;
    const int ht = 28;
    const int bht = 40;
    const int bwd = 64;
    const int prog = 32;
    //
    y = gap;
    Container->Colour = cYellow; //cOrangeLight; //cGreenLight;
    Container->FontSet (NULL);
    Container->Bitmap = BitmapLoad (Window, "FileBrowser.bmp");
    lFilename = new _Label (Container, {gap, y, col, ht}, "Filename (or part)");
    eFilename = new _EditSearch (Container, {col, y, -gap, ht}, SearchTargetHistory);
    //eFilename->EditDrop->EditPopup->Clipboard (coSelectAll);
    y += ht + gap;
    lContaining = new _Label (Container, {gap, y, col, ht}, "Containing");
    eContaining = new _EditSearch (Container, {col, y, -gap, ht}, Settings.EditTarget);
    y += ht + gap;
    cbSubFolders = new _CheckBox (Container, {col, y, 0, ht}, "Search Sub Folders");
    cbSubFolders->Down = true;
    y += ht + gap;
    dlRepeats = new _DropList (Container, {col, y, 120, ht}, "All Files\tUnique Files\tDuplicates");
    dlRepeats->Selected = 0;
    y += ht + gap;
    x = 0;
    bGo = new _Button (Container, {x, -bht, bwd, 0}, "\eB\n\ab\a2Search\ab", ActionSearchGo); x += bwd;
    bAbort = new _Button (Container, {x, -bht, bwd, 0}, "\eG\n\ab\a1Abort\ab", ActionSearchAbort);
    bAbort->EnabledSet (false);
    //
    wSearching = new _Wait (Container, {-prog, -prog, 0, 0});
    wSearching->VisibleSet (false);
    EventPreview = FormSearchEventPreview;
  }

void SearchFree (void)
  {
    free (SearchTargetHistory);
  }
