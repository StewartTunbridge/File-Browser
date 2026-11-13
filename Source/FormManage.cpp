////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ORGANIZE FILES FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct
  {
    char *Mask, *Code;
  } _DateMask;

_DateMask DateMask [] =
  {
    {"YYYY", "%4y"},
    {"YY", "%2y"},
    {"MMM", "%3m"},
    {"MM", "%2m"},
    {"DD", "%2d"},
    {"HH", "%2h"},
    {"NN", "%2n"},
    {"SS", "%2s"},
    {NULL, NULL}
  };

void MaskToDateTime (char *Mask, tm *DateTime)   // Overwrite Date Mask with actual Date/Time
  {
    _DateMask *dm;
    char *m;
    //
    dm = DateMask;
    while (dm->Mask)
      {
        m = Mask;
        while (true)
          {
            m = StrPos (m, dm->Mask);
            if (m == NULL)
              break;
            DateTimeToStr (&m, DateTime, dm->Code);
          }
        dm++;
      }
  }

class _FormOrganizeFiles: public _Form
  {
    public:
      _Label *lHelp;
      _Label *lDirMask;
      _EditPopup *eDirMask;
      _Button *bOK, *bCancel;
      _FormOrganizeFiles (_Point Pos);
      ~_FormOrganizeFiles (void);
  };

_FormOrganizeFiles *fOrganizeFiles = NULL;

const char PathDelimiterS [] = {PathDelimiter, '\0'};

void ActionOrganizeFilesOK (_Container *Container)
  {
    char *Mask;
    int a, b;
    tm *tmp;
    char *Dest;
    char *Name, *Ext;
    //
    Mask = fOrganizeFiles->eDirMask->TextGet ();
    //Dest = (char *) malloc (MaxPath);
    a = fMain->gFiles->Selected0.y;
    b = fMain->gFiles->Selected1.y;
    while (true)
      {
        if (ListGoto (Dir, a - 1, &GridIndex, &GridItem))
          {
            // Make Destination
            Dest = NULL;
            StrAssignCopy (&Dest, Mask);   // Start with the Mask
            Ext = StrFindFileExtension (GridItem->Name);
            if (Ext)
              Ext++;   // Step past dot
            tmp = localtime (&GridItem->DateTime);
            // Overwrite Mask with actual date/time
            MaskToDateTime (Dest, tmp);
            // Replace X with actual Extension
            StrReplace (&Dest, "X", Ext);
            // Split Name into Path and Name & replace * with local path
            Name = StrFindFilename (GridItem->Name);
            if (Name != GridItem->Name)   // Path exists
              {
                *(Name - 1) = 0;
                StrReplace (&Dest, "*", GridItem->Name);   // Replace * with local path
                *(Name - 1) = PathDelimiter;
              }
            else
              StrReplace (&Dest, "*", "");   // remove *
            // Ensure Destination path is terminated with a PathDelimeter
            if (Dest && *Dest)
              if (StrLast (Dest) != PathDelimiter)
                StrAdd (&Dest, PathDelimiterS);
            // Ensure Destination path does not start with a PathDelimeter
            while (*Dest == PathDelimiter)
              StrDelete (Dest);
            // Finally add the file name
            StrAdd (&Dest, Name);
            // And do the move
            if (!FileMove (GridItem->Name, Dest, 0))
              {
                fMain->ErrorAdd ("File Move to", Dest);
                break;
              }
            free (Dest);
          }
        if (a < b) a++; else if (a > b) a--; else break;   // next selection or exit
      }
    fOrganizeFiles->Die ();
    Reread = true;
  }

void ActionOrganizeFilesCancel (_Container *Container)
  {
    fOrganizeFiles->Die ();
  }

bool FormOrganizeFilesEventPreview (_Form *Form, _Event *Event)
  {
    if (Event->Type == etWindowClose)
      delete fOrganizeFiles;
    else if (Event->Type == etKeyDown && Event->Key == esc)
      delete fOrganizeFiles;
    else if (Event->Type == etKeyDown && Event->Key == KeyEnter)
      ActionOrganizeFilesOK (NULL);
    else
      return false;
    return true;
  }

const char *OrganizeFilesHelp =
  "Move the selected file(s) into sub-directories based on the file's properties: "
  "The Mask below controls the destination:\n"
  "\n"
  "  \abYY\ab, \abYYYY\ab => file year. \abMM\ab, \abMMM\ab => file month.\n"
  "  \abDD\ab => file day. \abHH\ab, \abMM\ab and \abSS\ab => file time.\n"
  "  \abX\ab => file extension. \ab*\ab => file Path. (after search)\n"
  "\n"
  "Examples: \abYYYYMMDD-HH    YYYYMM/DD    X/*\ab";

_FormOrganizeFiles::_FormOrganizeFiles (_Point Pos): _Form ("Organize Files", {Pos.x, Pos.y, 400, 230}, waAlwaysOnTop | waResizable)
  {
    int y;
    const int gap = 4;
    const int col = 108;
    const int bh = 32;
    const int bw = 64;
    int lh;
    //
    Container->Colour = cAquaLight;
    Container->FontSet (NULL);   // default
    lh = Container->FontFind ()->YAdvance;
    y = gap;
    //
    lHelp = new _Label (Container, {gap, y, -gap, 9*lh}, (char *) OrganizeFilesHelp);
    lHelp->AlignVert = aTop;
    y += lHelp->Rect.Height;
    lDirMask = new _Label (Container, {gap, y, col - gap, bh}, "Directory Mask");
    eDirMask = new _EditPopup (Container, {col, y, -gap, bh}, "YYYYMMDD");
    y += bh;
    bOK = new _Button (Container, {0, -bh, bw, bh}, "\a2OK", ActionOrganizeFilesOK);
    bCancel = new _Button (Container, {-bw, -bh, bw, bh}, "\a1Cancel", ActionOrganizeFilesCancel);
    EventPreview = FormOrganizeFilesEventPreview;
    //fMain->Container->EnabledSet (false);   // Disable main form
  }

_FormOrganizeFiles::~_FormOrganizeFiles (void)
  {
    fOrganizeFiles = NULL;
    //fMain->Container->EnabledSet (true);
  }

void OrganizeFiles (void)
  {
    if (fOrganizeFiles == NULL)
      fOrganizeFiles = new _FormOrganizeFiles (MousePos ());
  }

bool ReadDirCallBackDED (_DirItem *Item)
  {
    return true;
  }

bool DirectoryEmptyDelete (char *Path)   // If this directory (or any sub directories) are empty, delete them
  {
    _DirItem di;
    _DirItem *Dir, *d;
    char Path_ [MaxPath], *p;
    char *Nam;
    bool Res;
    //
    //DebugAddS ("DirectoryEmptyDelete", Path); // ####
    di = {0};
    if (!DirItemFromFilename (Path, &di))
      return false;
    if (di.SymLink)   // Symbolic Link
      return false;
    if (!di.Directory)   // check directory, recursively
      return false;
    Res = true;
    Dir = NULL;
    DirRead (Path, &Dir, ReadDirCallBackDED);
    d = Dir;
    while (d)
      {
        Nam = StrFindFilename (d->Name);
        if (!StrSame (Nam, "."))
          if (!StrSame (Nam, ".."))
            {
              if (!d->Directory)   // file
                Res = false;
              else
                {
                  p = Path_;
                  StrCat (&p, Path);
                  StrCat (&p, PathDelimiter);
                  StrCat (&p, Nam);
                  *p = 0;
                  if (!DirectoryEmptyDelete (Path_))   // check this sub directory
                    Res = false;
                }
            }
        d = d->Next;
      }
    DirFree (Dir);
    if (Res)
      if (DirectoryDelete (Path))
        {
          StatDelete++;
          StatChange = true;
        }
      else
        Res = false;
    return Res;
  }

void DeleteEmptyDirectories (void)
  {
    int a, b;
    char *s;
    //
    if (fMain->Focused == fMain->gFiles)   // File Grid has focus
      {
        b = fMain->gFiles->Selected1.y;
        a = fMain->gFiles->Selected0.y;
        while (true)
          {
            if (ListGoto (Dir, a - 1, &GridIndex, &GridItem))
              DirectoryEmptyDelete (GridItem->Name);
            if (a < b) a++; else if (a > b) a--; else break;
          }
      }
    else   // tree has focus. Select from Tree
      {
        s = GetCurrentPath ();
        DirectoryEmptyDelete (s);
        free (s);
      }
    Reread = true;
  }

bool ReadDirCallBackLUF (_DirItem *Item)
  {
    if (Item->Directory || Item->SymLink)
      return false;
    return true;
  }

void ListUnmatchedFiles (void)
  {
    int irl;
    char *p1, *p2;
    char Source [MaxPath], *s;
    _DirItem di;
    _DirItem *DirS, *DirD, *ds, *dd, *dd_;
    char c;
    //
    if (!Items)
      MessageBoxError (/*{300, 80},*/ "Hint\vCopy directory/ies before Comparing");
    else if (StrSame (ItemsRootPath, fMain->lPath->TextGet ()))
      MessageBoxError (/*{300, 80},*/ "Hint\vGo to a Destination before Comparing");
    else
      {
        DirFree (Dir);
        Dir = NULL;
        irl = StrLen (ItemsRootPath);
        // Parse Source list
        p1 = Items;
        while (p1 && *p1)
          {
            // Break off a single line
            p2 = StrPos (p1, '\n');
            if (p2 == NULL)   // Last Item
              p2 = StrPos (p1, '\0');
            c = *p2;
            *p2 = 0;
            // Make Source path
            s = Source;
            StrCat (&s, ItemsRootPath);
            StrCat (&s, p1);
            *s = 0;
            // Is it a Directory
            if (DirItemFromFilename (Source, &di))
              if (di.Directory)
                {
                  // Read Source Directory recursively
                  DirS = NULL;
                  DirRead (Source, &DirS, ReadDirCallBackLUF, true);
                  // Read Local equivalent Directory recursively
                  DirD = NULL;
                  DirRead (StrFindFilename (p1), &DirD, ReadDirCallBackLUF, true);
                  // Compare DirS and DirD
                  DebugAddP ("Dir lengths", {ListLength (DirS), ListLength (DirD)}); //####
                  dd = DirD;
                  while (true)   // For all DirD items
                    {
                      if (!dd)
                        break;
                      ds = DirS;   // Look for a match in DirS
                      while (true)
                        {
                          if (!ds)   // End of list -> not found, add to Dir, exit
                            break;
                          if (StrSame (&ds->Name [irl], dd->Name))   // Found -> exit
                            break;
                          ds = ds->Next;
                        }
                      if (ds)   // Found -> delete from DirS (for speed)
                        {
                          DirItemFree (ds);
                          ListRemove (ds, &DirS);
                          dd = dd->Next;
                        }
                      else   // not found -> add to Dir, remove from DirD
                        {
                          DebugAddS ("List Unmatched Files", dd->Name);   // add to list
                          dd_ = dd->Next;
                          ListRemove (dd, &DirD);
                          ListAddStart (dd, &Dir);
                          dd = dd_;
                        }
                    }
                  DirFree (DirS);   // Free DirS
                  DirFree (DirD);
                }
            *p2 = c;
            if (c)
              p2++;   // step past \n (if its there)
            p1 = p2;
          }
        if (!ListCheck (Dir))
          DebugAdd ("**ERROR List Unmatched Files");
        Dir = ListSort (Dir, DirItemsCompare2);   // Sort
        GridIndex = -1;
        fMain->gFiles->Columns = 5;
        fMain->gFiles->Rows = Max (ListLength (Dir) + 1, 2);
        fMain->gFiles->CellOffset = {0, 0};
        fMain->gFiles->Selected0 = fMain->gFiles->Selected1 = fMain->gFiles->Title;
        fMain->gFiles->DrawCellsClear ();
        fMain->FindFileSelected ();
        fMain->gFiles->Invalidate ();
        ShowNumberOfItems ();
      }
  }
