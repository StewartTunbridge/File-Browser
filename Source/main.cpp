////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FILE BROWSER - Smart Filing using Widgets
// =========================================
//
// 19 Feb 2025  Genesis
// 25 Mar 2025  Ready for Beta testing
//
// TO DO
// -----
// /Search, include Directories
// Paste: Add Errors to Conflict list
// / Add bCut
// / bCut and bCopy are grouped and toggle
// / bCut:
// /   On Paste: if bCut->Down, delete file after copying to destination, abort on copy fail BEFORE deleting
// / Moving file: Destination File exists: Make list and ask
// / Implement Rename with Mask
// / Implement AddFile/Directory
// / Copy: Make list of Newer files to get zapped and get action on each in a Grid
// / Implement Edit
// / Implement Open
// / Edit: Some mechanism to distinguish binary files (and show hex)
// /  Edit: Syntax Highlighting could be done with embedded \a* commands. Each line on draw
// /    ?Add a CallBack in _Edit to present an alternative string per line. Display only
// / Edit: bug Paste then Paste loses integrity
// / Right click on Files echos options in Toolbar
// / Grid: Add ActionCellSelect
// / Grid: Add Sort Column Select & Highlight
// / Confirm before Delete
// / Add Tabs for bookmarks
// / Support Links:
//   /Display
//   /Create
//     /Allow multiple links?
//   /Go to
// / File Filter
// / Start a thread for Paste / Delete
//   /Show Progress
//   /Abort button
//   Log Window?
// ?Grid: Support Cntrl to select individually
//   Add Select as a dynamic array of bool OR use Tagged in Dir data
// / Duplicate Files / Directories
//     ?Make into a thread
// / Search:
//   / Add command to Search
//   / Add Path column (far right)
//   / Dir:
//   /  Name: keep relative path
//     / DirRead supports recursion into Directories
//   / Grid Draw:
//     / Use Filename part in Name col
//     / Use Path part in Path col
// / Remove Attribute col, add Properties command
// / New Component, _Divider: Narrow Container (with a grip) that -
//   / Locks a mouse click and moves itself and adjusts its neighbours
// Edit
//   /Duplicate indent on line break
//   /Line Duplicate
//   /Move line  ^Up ^Down
//   /Select word(s) / line(s)
//   /Insert Line Break: Cope with cr and cr+lf
//   /Syntax Highlight
//   /Auto complete
//   /Undo
//   /Macros: F1-F12
//     /Saved in Settings file
//   /Compare:
//     /Clipboard w Selection
//     /File w File on Conflict
//   /allow tab insertion on multiline
// /Duplicate: Selection should stay on selected item
// /Change to different EOL formats
//
// BUGS
//
// TO DO
//   Compare: use Edit not Label ... somehow
//
// Search List: Paste with / without Path
//
// Scale Font up/down (like in Edit)
//
// /Add Refresh
//
// Error:
//   Rename folder
//   Cut folder (uses old name I think)
//   Paste folder
//
// Edit:
//   Better way of showing matching bracket
//   Implement Insert
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#define Revision_ "2.2102"

//#define Preview

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#ifdef _Windows
  #include <fileapi.h>
#else
  #include <fcntl.h>
  #include <sys/mount.h>
  #include <errno.h>
  #include <pwd.h>
  #include <grp.h>
#endif // _Windows

#include "../Widgets/Widgets.hpp"
#include "../Widgets/WidgetsGrid.hpp"
#include "../Widgets/WidgetsTree.hpp"
#include "../Widgets/FileSelect.hpp"
#include "../Widgets/ColourSelect.hpp"
#include "../Widgets/MessageBox.hpp"
#include "../Widgets/MessageResponseBox.hpp"


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SUPPORT
//
////////////////////////////////////////////////////////////////////////////////////////////////////

int StrAddThings (char **St, int n, char *Thing)   // Number + String (space if n < 0)
  {
    int l;
    //
    l = 0;
    if (n && Thing)
      {
        if (n < 0)
          l += StrCat (St, ' ');
        n = Abs (n);
        l += NumToStr (St, n, DigitsCommas);
        l += StrCat (St, ' ');
        l += StrCat (St, Thing);
        if (n > 1)   // make plural ... because we care
          {
            if (StrPos (Thing, '\0') [-1] == 'y')   // Thing ends in 'y', so replace with "ies"
              {
                if (*St)
                  (*St)--;   // remove 'y'
                l += StrCat (St, "ies") - 1;
              }
            else
              l += StrCat (St, 's');
          }
        l += StrCat (St, '.');
      }
    return l;
  }

/*int StrAddThings_ (char **St, int n, char *Thing)
  {
    if (n && Thing)
      {
        StrCat (St, ' ');
        return StrAddThings (St, n, Thing) + 1;
      }
    return 0;
  }*/

#define OpCopy    0x01
#define OpMove    0x02
#define OpDelete  0x04
#define OpError   0x80

typedef struct _ConflictItem  // Directory Item. This is also a _List item
  {
    _ConflictItem *Next, *Prev;
    //
    byte Op;
    char *SourcePath, *DestPath;
    int CommonDestPath;
    int DeltaTime;
    longint DeltaSize;
    bool Ticked;
  } _ConflictItem;

_ConflictItem *ConflictList = NULL;

int ConflictIndex = -1;
_ConflictItem *ConflictItem = NULL;

void ConflictListFree (_ConflictItem *cl)
  {
    _ConflictItem *cl_;
    //
    while (cl)
      {
        free (cl->SourcePath);
        free (cl->DestPath);
        cl_ = cl;
        ListRemove (cl, &cl);
        free (cl_);
      }
    ConflictIndex = -1;
  }

void ConflictListAdd (byte Op, char *SourcePath, char *DestPath, int DeltaTime, longint DeltaSize, int CommonDestPath)
  {
    _ConflictItem *New;
    //
    New = (_ConflictItem *) malloc (sizeof (_ConflictItem));
    memset (New, 0, sizeof (_ConflictItem));
    New->Op = Op;
    StrAssignCopy (&New->SourcePath, SourcePath);
    StrAssignCopy (&New->DestPath, DestPath);
    New->CommonDestPath = CommonDestPath;
    New->DeltaTime = DeltaTime;
    New->DeltaSize = DeltaSize;
    //New->Ticked = true;
    ListAddEnd (New, &ConflictList);
  };

int DirFilter = 0;   // Filter Hidden and System files: 0x01 Hidden,  0x02 System

bool DirReadCallback (_DirItem *Item)
  {
    if (Item->Directory)
      if (StrSame (StrFindFilename (Item->Name), ".."))
        return false;
    return true;
  }

bool DirReadCallbackFilter (_DirItem *Item)
  {
    if (Item->Directory)
      {
        if (StrSame (StrFindFilename (Item->Name), ".."))
          return false;
      }
    else if (~DirFilter & 2)   // Don't filter out Directories
      if (Item->System)
        return false;
    if (~DirFilter & 1)
      if (Item->Hidden)
        return false;
    return true;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SETTINGS
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#define MacroMax 64

typedef struct
  {
    // Operations
    bool OverwriteOlder;
    bool PastePath;
    bool DoubleClickOpens;
    // Appearance
    int ColourDir, ColourFile;
    // System
    char *Open;
    char *OpenExe;
    char *RunCommand;
    char *Terminal;
    // Editor
    char *EditFont;
    int EditFontSize;
    char *EditTarget;
    int *Macros [12];   // Macros F1-F12
    // ...
  } _Settings;

_Settings Settings;


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FILE OPERATIONS
//
////////////////////////////////////////////////////////////////////////////////////////////////////

bool Reread = false;

int Threads = 0;
bool ThreadsAbort = false;
bool Thread = false;

const int BufferSize = 1 << 24;   // = 2^24 = 16 Meg

bool DoRename;
int StatCopy, StatMove, StatSkip, StatDelete;
bool StatChange = false;

typedef enum {aSkip, aProceed, aExists, aConflict} _FileAction;

bool DestinationInSource (char *SourcePath, char *DestPath)
  {
    bool Res;
    char *Dest;
    int ls, ld;
    //
    Res = true;
    Dest = NULL;
    StrAssignCopy (&Dest, DestPath);
    ls = StrLen (SourcePath);
    ld = StrLen (Dest);
    while (true)
      {
        if (StrSame (SourcePath, Dest))   // DestPath parent matches Source
          break;
        while (ld > 0 && Dest [ld] != PathDelimiter)
          ld--;
        Dest [ld] = 0;
        if (ld < ls)
          {
            Res = false;
            break;
          }
      }
    free (Dest);
    return Res;
  }

bool FileCopy (char *SourcePath, char *DestPath, int CommonDestPath, bool Force = false)
  {
    bool OK;
    _FileAction ac;
    char *DP, *s;
    _DirItem diS, diD;
    int fS, fD;
    byte *Buffer;
    int ds;
    _DirItem *Dir, *d;
    //
    #if DEBUG
    DebugAddSS ("FileCopy", SourcePath, DestPath);   //####
    #endif
    diS = {0};
    diD = {0};
    if (ThreadsAbort)
      return false;
    //DebugAddS (SourcePath, DestPath);  //####
    if (!DirItemFromFilename (SourcePath, &diS))
      return false;
    OK = true;
    if (!diS.SymLink)   // not a Symbolic Link
      {
        if (diS.Directory)   // copy a directory, recursively
          if (DestinationInSource (SourcePath, DestPath))
            OK = false;   // copying inside yourself. Bad idea
          else
            {
              Dir = NULL;
              DirRead (SourcePath, &Dir, DirReadCallback);
              d = Dir;
              DP = (char *) malloc (MaxPath);   // and destination path
              while (d)   // for each item in the directory
                {
                  DirMake (DestPath);   // make new target directory
                  s = DP;
                  StrCat (&s, DestPath);
                  StrCat (&s, PathDelimiter);
                  StrCat (&s, StrFindFilename (d->Name));
                  *s = 0;
                  if (!FileCopy (d->Name, DP, CommonDestPath, Force))   // copy the file (or directory)
                    OK = false;
                  d = d->Next;
                }
              free (DP);
              DirFree (Dir);
            }
        else   // copy file?
          {
            ac = aProceed;
            if (DirItemFromFilename (DestPath, &diD))
              if (!Force)
                {
                  if (diD.Size == diS.Size && Abs (diS.DateTime - diD.DateTime) <= 1)   // Same size and age
                    ac = aSkip;
                  else
                    if (diS.DateTime < diD.DateTime)   // Source is older
                      ac = aConflict;
                    else   // Dest is Older !@#!@#@!#@!#
                      if (!Settings.OverwriteOlder)
                        ac = aConflict;
                }
            if (ac == aConflict)   // can't copy file exists and we can not auto delete ####
              DebugAddInt ("  Source newer by", diS.DateTime - diD.DateTime); // ####
            if (ac == aProceed)
              {
                OK = false;
                fS = FileOpen (SourcePath, foRead);
                if (fS > 0)
                  {
                    if (ForcePath (DestPath, true))
                      {
                        fD = FileOpen (DestPath, foWrite);
                        if (fD > 0)
                          {
                            Buffer = (byte *) malloc (BufferSize);
                            while (true)
                              {
                                ds = FileRead (fS, Buffer, BufferSize);
                                if (ds == 0)
                                  break;
                                if (!FileWrite (fD, Buffer, ds))
                                  break;
                              }
                            free (Buffer);
                            FileClose (fD);
                            StatCopy++;
                            StatChange = true;
                            if (ds == 0)   // only true on final read ok and FileWrite ok
                              if (FileDateTimeWrite (DestPath, diS.DateTime))
                                if (chmod (DestPath, diS.Attrib) == 0)
                                  OK = true;
                          }
                      }
                    FileClose (fS);
                  }
                if (OK)
                  Log ("Copy", SourcePath, DestPath);
                else
                  ConflictListAdd (OpCopy | OpError, SourcePath, DestPath, 0, 0, CommonDestPath);
              }
            else if (ac == aConflict)
              ConflictListAdd (OpCopy, SourcePath, DestPath, diD.DateTime - diS.DateTime, diD.Size - diS.Size, CommonDestPath);   // add to list for post command action
            else   // acSkip
              {
                StatSkip++;
                StatChange = true;
              }
          }
      }
    DirItemFree (&diS);
    DirItemFree (&diD);
    return OK;
  }

bool DeleteItem (char *Filename, int CommonDeletePath)
  {
    bool Res;
    _DirItem di, *Dir, *d;
    //
    if (ThreadsAbort)
      return true;
    Res = false;
    di = {0};
    if (DirItemFromFilename (Filename, &di))
      {
        Res = true;
        if (di.Directory)
          {
            Dir = NULL;
            DirRead (Filename, &Dir, DirReadCallback);
            d = Dir;
            while (d)
              {
                DeleteItem (d->Name, CommonDeletePath);
                d = d->Next;
              }
            DirFree (Dir);
            DirectoryDelete (Filename);
          }
        else
          if (FileDelete (Filename))
            {
              Log ("Delete", Filename);
              StatDelete++;
              StatChange = true;
            }
          else
            Res = false;
      }
    DirItemFree (&di);
    if (!Res)
      ConflictListAdd (OpDelete, NULL, Filename, 0, 0, CommonDeletePath);
    return Res;
  }

bool FileMove (char *SourcePath, char *DestPath, int CommonDestPath, bool Force = false)
  {
    _DirItem diS, diD;
    bool Err, Go;
    //
    if (ThreadsAbort)
      return false;
    if (StrSame (SourcePath, DestPath))
      return true;
    Err = Go = false;
    // Remove any destination file
    diS = {0};
    diD = {0};
    if (DirItemFromFilename (SourcePath, &diS))   // problem with source file
      if (!DirItemFromFilename (DestPath, &diD))   // destination not exists?
        Go = true;   // no destination
      else
        if (diD.DateTime <= diS.DateTime || Force)   // destination file is older OR Forced?
          {
            if (DeleteItem (DestPath, CommonDestPath))   // delete anyway
              Go = true;
            else
              Err = true;
          }
        else   // add newer file to conflict list
          ConflictListAdd (OpMove, SourcePath, DestPath, diD.DateTime - diS.DateTime, diD.Size - diS.Size, CommonDestPath);
    DirItemFree (&diD);
    DirItemFree (&diS);
    if (Go && !Err)   // No destination exists now
      {
        ForcePath (DestPath, true);
        //if (access (DestPath, F_OK) == 0)   // Destination exists
        if (FileRename (SourcePath, DestPath))   // success
          {
            Log ("Move", SourcePath, DestPath);
            StatMove++;
            StatChange = true;
          }
        else   // Cant rename, try Copy + Delete
          {
            Err = true;
            if (FileCopy (SourcePath, DestPath, CommonDestPath, Force))
              if (DeleteItem (SourcePath, CommonDestPath))
                Err = false;
          }
      }
    if (Err)
      ConflictListAdd (OpMove | OpError, SourcePath, DestPath, diD.DateTime - diS.DateTime, diD.Size - diS.Size, CommonDestPath);
    return !Err;
  }

bool Duplicate (_DirItem *Item) // (char *SourcePath)
  {
    bool Res;
    char *DestPath;
    char *p;
    //
    DestPath = NULL;
    StrAssignCopy (&DestPath, Item->Name);
    p = StrFindFileExtension (DestPath);
    if (p == NULL)
      p = StrPos (DestPath, '\0');
    p--;
    if (!IsDigit (*p))
      *p = '0';
    while (Item->Directory ? DirExists (DestPath) : FileExists (DestPath))
      (*p)++;
    Res = FileCopy (Item->Name, DestPath, 0);
    free (DestPath);
    return Res;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MAIN FORM - header
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class _FormMain: public _Form
  {
    public:
      _Container *MenuBar;
      _Menu *mFile, *mEdit, *mView, *mManage, *mHelp;
      _Container *Toolbar;
      _Button *bPreview, *bCut, *bCopy, *bPaste, *bRename, *bOpen, *bEdit, *bDelete, /**bNewFile, *bNewDir,*/ *bSearch; //, *bSummary;
      _ButtonArrow *bUp, *bDown;
      _Button *bDirUp;
      _Button *bRefresh;
      _Tabs *tBookmarks;
      _Label *lPath, *lSize;
      _MenuPopup *pmPathCopy;
      _Container *cBody;
        _TreeView *tDir;
        _Splitter *Splitter;
        _GridView *gFiles;
      _Label *lStatus;
      _Wait *Spinner;
      _Button *bStatus;
      _MenuPopup *mFiles;
      //_MenuPopup *mStatus;
      bool FindFileSelected (void);
      void ErrorAdd (char *Err, char *File);
      _FormMain (char *Title);
     ~_FormMain ();
      //
      char *Error;
      char *Status;
      char *FileSelect;   // On reading a directory, select this filename (if it exists)
      char *RenameDefault;   // Rename command sets this to the selected filename
      bool RenameStemDiff, RenameExtDiff;
      char *Bookmarks [8];
  };

_FormMain *fMain;

_DirItem *Dir = NULL;
char *Items = NULL;
char *ItemsRootPath;

void StrDirsFileLinks (char **St, int Dirs, int Files, int Links)
  {
    StrAddThings (St, Dirs, "Directory");
    StrAddThings (St, -Files, "File");
    StrAddThings (St, -Links, "Link");
  }

void ShowNumberOfItems (void)
  {
    char St [64], *p;
    int CountDirs, CountFiles, CountLinks;
    longint SizeFiles;
    _DirItem *d;
    //
    CountFiles = CountDirs = CountLinks = SizeFiles = 0;
    d = Dir;
    while (d)
      {
        if (d->SymLink)
          CountLinks++;
        else if (d->Directory)
          CountDirs++;
        else
          {
            CountFiles++;
            SizeFiles += d->Size;
          }
        d = d->Next;
      }
    p = St;
    //StrCat (&p, "Dir:");
    if (CountDirs || CountFiles || CountLinks)
      {
        StrDirsFileLinks (&p, CountDirs, CountFiles, CountLinks);
        StrCat (&p, "   (");
        IntToStrScaled (&p, SizeFiles);
        StrCat (&p, " bytes");
        StrCat (&p, ')');
      }
    *p = 0;
    StrAssignCopy (&fMain->Status, St);
  }

int Open (char *Filename)
  {
    char *File, *Command;
    int Res;
    //
    File = StrReplace (Filename, "$", "\\$");   // Replace $ with \$
    StrReplace (&File, "\"", "\\\"");   // Replace " with \"
    if (FileIsExecutable (Filename))
      Command = StrReplace (Settings.OpenExe, "@", File);
    else
      Command = StrReplace (Settings.Open, "@", File);
    Res = system (Command);
    DebugAddS ("Opening", Command); //####
    free (File);
    free (Command);
    return Res;
  }

int RunCommand (char *Command)
  {
    //char *File, *Command;
    int Res;
    //
    //File = StrReplace (Filename, "$", "\\$");   // Replace $ with \$
    //StrReplace (&File, "\"", "\\\"");   // Replace " with \"
    Command = StrReplace (Settings.RunCommand, "@", Command);
    Log ("Execute", Command);
    Res = system (Command);
    DebugAddS ("Running", Command); //####
    //free (File);
    //free (Command);
    return Res;
  }

void PathToTree ()
  {
    char *Path;
    char *p, *q;
    _TreeViewNode *Node, *Node_;
    //
    Node = fMain->tDir->Nodes;
    Path = fMain->lPath->TextGet ();
    p = Path;
    while (true)
      {
        if (p == NULL || *p == 0)
          break;
        q = StrPos (p, PathDelimiter);
        if (q)
          *q++ = 0;   // terminate dir name and step into next dir
        Node->Expanded = true;
        Node_ = Node->NodeFind (p);
        if (!Node_)
          Node_ = Node->NodeAdd (true, p);
        Node = Node_;
        p = q;
      }
    fMain->tDir->Selected = Node;
    fMain->tDir->Invalidate ();
    Reread = true;
  }

bool GotoPathFile (char *PathFile)
  {
    int File;
    char *c;
    //
    File = FileOpen (PathFile, foRead);
    if (File >= 0)
      {
        FileClose (File);
        c = StrPosLast (PathFile, PathDelimiter);   // Look for a directory
        if (c)   // Does it contains a directory (from a deep search)? If so, go there
          {
            *c++ = 0;   // Split the folder and filename
            fMain->lPath->TextSet (PathFile);
            PathToTree ();
            StrAssignCopy (&fMain->FileSelect, c);
            return true;
          }
      }
    return false;
  }

void SettingsApply (void)
  {
    fMain->tDir->Colour = Settings.ColourDir;
    fMain->tDir->Invalidate ();
    fMain->gFiles->Colour = Settings.ColourFile;
    fMain->gFiles->Invalidate ();
  }

void MessageBoxError (const char *Error)
  {
    MessageBox ({300, 76}, Error);
    fMessageBox->Container->Colour = cRedLight;
    fMessageBox->Container->ColourGrad = -1;
    fMessageBox->Container->ColourText = cRedDark;
  }

class _EditSearch: public _Container
  {
    public:
      _EditPopupDropList *EditDrop;
      _Button *bCaps, *bWild;
      _EditSearch (_Container *Parent, _Rect Rect, char *Text = NULL);
      byte Flags (void);
      bool Match (char Ch, char Target);
  };

_EditSearch::_EditSearch (_Container *Parent, _Rect Rect_, char *Text_)
  : _Container (Parent, Rect_, NULL)
  {
    int h;
    //
    h = Rect.Height;
    EditDrop = new _EditPopupDropList (this, {0, 0, -h, 0}, Text_);
    bCaps = new _Button (this, {-h, 0, h, h/2}, "\a1aA");
    bCaps->FontSet (NULL, 10);
    bCaps->Toggle = true;
    bWild = new _Button (this, {-h, h/2, h, h/2}, "\a2#@?");
    bWild->FontSet (NULL, 10);
    bWild->Toggle = true;
  }

byte _EditSearch::Flags (void)
  {
    return bCaps->Down | (bWild->Down << 1);
  }

extern bool TextCallBackEdit (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size);

#include "FormCompare.cpp"   // Text comparison module + form
#include "FormConflict.cpp"
#include "FormProperties.cpp"
#include "FormEdit.cpp"
#include "FormSearch.cpp"
#include "FormSummary.cpp"
#include "FormExecute.cpp"
#include "FormSettings.cpp"
#include "FormManage.cpp"
#ifdef Preview
#include "FormPreview.cpp"
#endif // Preview


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// MAIN FORM - body
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// Translate between Column and DirSort
const int ColumnToSort [] = {0, 0, 2, 3, 4};
const int SortToColumn [] = {1, 1, 2, 3, 4};

void GridRefresh (void)
  {
    // Reset Dir Data Pointer
    GridIndex = -1;
    // Update Grid
    free (fMain->gFiles->CellHeights);
    fMain->gFiles->CellHeights = NULL;
    fMain->gFiles->Rows = Max (ListLength (Dir) + 1, 2);
    fMain->gFiles->CellOffset = {0, 0};
    fMain->gFiles->Selected0 = fMain->gFiles->Selected1 = fMain->gFiles->Title;
    fMain->gFiles->DrawCellsClear ();
    fMain->FindFileSelected ();
    fMain->gFiles->ColumnHighlight = SortToColumn [DirSort];
    fMain->gFiles->Invalidate ();
  }

void DirRead_ (void)
  {
    DirFree (Dir);   // Read
    Dir = NULL;
    DirRead (NULL, &Dir, DirReadCallbackFilter);
    Dir = ListSort (Dir, DirItemsCompare2);   // Sort
    fMain->gFiles->Columns = 4;
    GridRefresh ();
  }

char *StrEmbrace (char *St)   // St1 + '<' + St2 + '>'
  {
    char *Res, *r;
    //
    Res = (char *) malloc (StrLen (St) + 3);
    r = Res;
    StrCat (&r, '<');
    StrCat (&r, St);
    StrCat (&r, '>');
    *r = 0;
    return Res;
  }

bool ItemNamesUnique (void)
  {
    char *p1, *p2, *pName;
    char *Name, *Name_, *Name__;
    char *Collection;
    //
    Collection = NULL;
    p1 = Items;
    pName = NULL;
    while (p1 && *p1 && !pName)
      {
        // p1/p2 point to start/end of each filename
        p2 = StrPos (p1, '\n');
        if (p2 == NULL)   // Last Item
          p2 = StrPos (p1, '\0');
        Name = StrMake (p1, p2 - p1);
        Name_ = Name;
        if (!Settings.PastePath)
          Name_ = StrFindFilename (Name);
        // filename found in Collection?
        Name__ = StrEmbrace (Name_);
        pName = StrPos (Collection, Name__);
        if (pName == NULL)
          StrAdd (&Collection, Name__);
        free (Name);
        free (Name__);
        if (*p2)   // Step past lf
          p2++;
        p1 = p2;
      }
    free (Collection);
    return !pName;
  }

int SelectedOld = 0;

void ActionTabSelected (_Tabs *Bookmarks)
  {
    //
    StrAssignCopy (&fMain->Bookmarks [SelectedOld], fMain->lPath->TextGet ());   // Save old Tab path
    SelectedOld = Bookmarks->Selected;
    if (fMain->Bookmarks [SelectedOld])
      {
        SetCurrentPath (fMain->Bookmarks [SelectedOld]);
        fMain->lPath->TextSet (fMain->Bookmarks [SelectedOld]);
      }
    PathToTree ();
    fMain->tDir->ScrollToSelected ();   // Out of sight?
    fMain->tDir->Invalidate ();
  }

void ActionDrag (int From, int To)
  {
    SwapBytes (&fMain->Bookmarks [From], &fMain->Bookmarks [To], sizeof (fMain->Bookmarks [0]));
    SelectedOld = To;
  }

void ActionCopyPath (_Container *Container)
  {
    ClipboardSet (fMain->Window, fMain->lPath->TextGet ());
  }

void ActionNodeSelected (_TreeView *TreeView)
  {
    const int TabMax = 16;
    char *St, *p;
    _TreeViewNode *Node;
    _DirItem *d;
    longint Capacity, Free;
    char *Tabs;
    //
    // Update the current Bookmark Tab
    St = NULL;
    StrAssignCopy (&St, TreeView->Selected->Text);
    if (StrLen (St) > TabMax)   // trim if necessary
      strcpy (&St [TabMax - 3], "...");
    Tabs = NULL;
    StrAssignCopy (&Tabs, fMain->tBookmarks->TextGet ());
    StrSetItem (&Tabs, fMain->tBookmarks->Selected, St);
    fMain->tBookmarks->TextSet (Tabs);
    free (Tabs);
    free (St);
    // Determine Path of Selected Node
    St = (char *) malloc (MaxPath);
    Node = TreeView->Selected;
    St [0] = 0;
    while (Node->Parent)
      {
        StrInsert (St, PathDelimiter);
        StrInsert (St, Node->Text);
        Node = Node->Parent;
      }
    // Display Path
    fMain->lPath->TextSet (St);
    // Read Directory
    if (SetCurrentPath (St))   // this place does not exist
      {
        DirRead_ ();
        // Replace this nodes Children with any sub-directory names
        while (TreeView->Selected->Children)
          delete TreeView->Selected->Children;
        d = Dir;
        while (d)
          {
            if (d->Directory)
              TreeView->Selected->NodeAdd (true, d->Name);
            d = d->Next;
          }
        // Display Disk Space
        if (DiskSpace (St, &Capacity, &Free))
          {
            p = St;
            StrCat (&p, "Free ");
            IntToStrScaled (&p, Free);
            StrCat (&p, " / ");
            IntToStrScaled (&p, Capacity);
            *p = 0;
            fMain->lSize->TextSet (St);
          }
        ShowNumberOfItems ();
      }
    else
      fMain->ErrorAdd ("Directory Unreadable", NULL);
    TreeView->ScrollToSelected ();
    free (St);
  }

const char AttributesTab [] = "rwxrwxrwx";

void AttributesToStr (char **St, _DirItem *Item)
  {
    int m;
    char *p;
    //
    StrCat (St, Item->Directory ? 'd' : '-');
    StrCat (St, Item->SymLink   ? 'l' : '-');
    p = (char *) AttributesTab;
    for (m = 0x100; m > 0; m = m >> 1, p++)
      if (Item->Attrib & m)
        StrCat (St, *p);
      else
        StrCat (St, '-');
  }

char *GridTitle [] = {NULL, "\abName\ab", "\abSize\ab", "\abDate Time\ab", "\abPath\ab"};//"\abAttributes\ab"};

char *GridItemIcon [] = {"\er", "\es"};

void ActionGridCellDraw (_GridView *Grid, int x, int y, _Rect Rect)
  {
    char Str [MaxPath], *pStr, *p, *fn;
    tm *tmp;
    int Margin;
    int Col;
    int th;
    //
    Col = Grid->ColourTextFind ();
    Margin = 4;
    p = pStr = Str;
    if (y == 0)
      if (x == 1 && DirSort == 1)   // Name column & sorting by extension
        pStr = "\abName (by ext)\ab";
      else
        pStr = GridTitle [x];
    else
      {
        if (ListGoto (Dir, y - 1, &GridIndex, &GridItem))
          {
            if (GridItem->SymLink)
              Col = cRed;
            else if (GridItem->Directory)
              Col = ColourAdjust (cBlue, 140);
            else if (FileIsExecutable (GridItem->Name))
              Col = cGreenDark; //ColourAdjust (cGreen, 50);
            if (GridItem->Hidden || GridItem->System)
              Col = ColourAdjust (Col, 140);
            if (x == 0)   // Dir / File icon
              {
                pStr = GridItemIcon [GridItem->Directory || GridItem->SymLinkDirectory];
                Margin = 1;
              }   // Filename
            else if (x == 1)
              {
                StrCat (&p, StrFindFilename (GridItem->Name));
                if (GridItem->SymLink)
                  {
                    StrCat (&p, " => ");
                    StrCat (&p, GridItem->SymLinkTarget);
                  }
              }
            else if (x == 2)   // Size
              {
                if (!GridItem->Directory && !GridItem->SymLink)
                  IntToStr (&p, GridItem->Size, 0 | DigitsCommas);
              }
            else if (x == 3)   // DateTime
              {
                tmp = localtime (&GridItem->DateTime);
                if (!GridItem->SymLink)
                  DateTimeToStr (&p, tmp, "%4y %3m %2d  %2h%2n:%2s");
              }
            else if (x == 4) // Path
              {
                fn = StrPosLast (GridItem->Name, PathDelimiter);
                if (fn)
                  {
                    *fn = 0;
                    StrCat (&p, GridItem->Name);
                    *fn = PathDelimiter;
                  }
              }
              //AttributesToStr (&p, GridItem);
          }
      }
    *p = 0;
    Rect.x += Margin;
    Rect.Width -= 2 * Margin;
    Grid->FontFind ()->Colour = Col;
    if (x == 1 && y > 0)   // Name
      {
        th = Grid->TextOutWrap (Rect, pStr, aLeft, aCenter) + 8;
        if (th > Grid->CellHeightGet (y))
          Grid->CellHeightSet (y, th);
      }
    else
      Grid->TextOutAligned (Rect, pStr, x == 2 ? aRight : aLeft, aCenter);
  }

void ActionGridCellWrite (_GridView *Grid, int x, int y, char *Data, bool Abort)   // Rename
  {
    char *New, *n;
    int a, b;
    //
    fMain->gFiles->ActionCellWrite = NULL;
    if (!Abort)
      {
        New = (char *) malloc (MaxPath);
        // Filter Data
        n = StrFindFileExtension (Data);
        if (n)
          {
            if (fMain->RenameStemDiff)   // Group rename => change Ext
              StrDelete (Data, n - Data);   // remove any Stem
            if (fMain->RenameExtDiff)   // Group rename => change Stem
              *n = 0;   // remove Ext
          }
        else if (fMain->RenameExtDiff)   // Group rename Stem
          Data [0] = 0;
        // Go through selection
        a = fMain->gFiles->Selected0.y;
        b = fMain->gFiles->Selected1.y;
        while (true)
          {
            if (ListGoto (Dir, a - 1, &GridIndex, &GridItem))
              {
                if (fMain->RenameStemDiff || fMain->RenameExtDiff)   // Group rename => change either Stem or Ext.
                  if (fMain->RenameStemDiff)   // Differing Stems => changing the extension only
                    {
                      strcpy (New, GridItem->Name);
                      n = StrFindFileExtension (New);
                      if (n)
                        strcpy (n, Data);
                      else
                        StrAppend (New, Data);
                    }
                  else   // changing Stem only
                    {
                      strcpy (New, Data);
                      n = StrFindFileExtension (GridItem->Name);
                      if (n)
                        StrAppend (New, n);
                    }
                else
                  strcpy (New, Data);
                if (FileRename (GridItem->Name, New))   // rename OK
                  StrAssignCopy (&fMain->FileSelect, New);
                else   // Fail
                  {
                    fMain->ErrorAdd ("Rename", GridItem->Name);
                    break;
                  }
              }
            if (a == b)
              break;
            if (a < b) a++; else a--;
          }
        free (New);
        Reread = true;
      }
    StrAssign (&fMain->RenameDefault, NULL);
  }

void ActionNodeEditWrite (_TreeView *TreeView, char *Text, bool Abort)
  {
    if (!Abort)
      {
        SetCurrentPath ("..");
        if (FileRename (TreeView->Selected->Text, Text))   // rename OK
          {
            StrAssignCopy (&TreeView->Selected->Text, Text);   // Fix in Tree
            ActionNodeSelected (TreeView);   // Fix in lPath
          }
        else
          fMain->ErrorAdd ("Rename", TreeView->Selected->Text);
      }
  }

void ActionOpen (_Container *Container)
  {
    int Res;
    char *fp;
    //
    if (fMain->Focused == fMain->tDir)   // Open a Directory
      Res = Open (".");
    else if (fMain->Focused == fMain->gFiles)   // grid focussed
      if (ListGoto (Dir, fMain->gFiles->Selected1.y - 1, &GridIndex, &GridItem))
        {
          if (GridItem->SymLink)   // Go to a Symbolic Link
            GotoPathFile (GridItem->SymLinkTarget);
          else   // Open a File
            {
              Res = Open (GridItem->Name);
              if (Res != 0)   // Failes to "open"
                fMain->ErrorAdd ("We do not know how to open", GridItem->Name);
              else   // Success so Log operation
                {
                  fp = StrMake (fMain->lPath->TextGet (), GridItem->Name);
                  Log ("Open", fp);
                  free (fp);
                }
            }
        }
  }

/*
void ActionOpenInTerminal (_Container *Container)
  {
    bool exe;
    //
    exe = false;
    if (fMain->Focused == fMain->gFiles)   // grid focussed
      if (ListGoto (Dir, fMain->gFiles->Selected1.y - 1, &GridIndex, &GridItem))
        if (FileIsExecutable (GridItem->Name))
          exe = true;
    if (exe)
      Open (GridItem->Name, true);
    else
      system (Settings.Terminal);
  }*/

void ActionEdit (_Button *Button)
  {
    char *FullFilename, *f;
    //
    if (ListGoto (Dir, fMain->gFiles->Selected1.y - 1, &GridIndex, &GridItem))
      if (!GridItem->Directory && !GridItem->SymLink)
        {
          FullFilename = (char *) malloc (MaxPath);
          f = FullFilename;
          StrCat (&f, fMain->lPath->TextGet ());
          StrCat (&f, GridItem->Name);
          *f = 0;
          FileEdit (FullFilename, Button == NULL);
          free (FullFilename);
        }
  }

#ifdef Preview
void ActionPreview (_Container *Container)
  {
    if (fMain->bPreview->Down)
      {
        if (ListGoto (Dir, fMain->gFiles->Selected1.y - 1, &GridIndex, &GridItem))
          if (!GridItem->Directory)
            PreviewFile (GridItem->Name);
      }
    else
      if (fPreview)
        fPreview->Die ();
  }
#endif // Preview

char *ActionGridCellRead (_GridView *Grid, int x, int y)
  {
    char *f, *p;
    //
    if (ListGoto (Dir, y - 1, &GridIndex, &GridItem))
      {
        if (fMain->RenameDefault)
          {
            fMain->gFiles->ActionCellWrite = ActionGridCellWrite;
            return (fMain->RenameDefault);
          }
        if (GridItem->Directory)   // go to directory
          {
            DebugAddS ("Go Dir", GridItem->Name);
            f = StrMake (fMain->lPath->TextGet (), GridItem->Name);
            fMain->lPath->TextSet (f);
            free (f);
            PathToTree ();
          }
        else   // Open File / Go to File
          {
            f = StrPosLast (GridItem->Name, PathDelimiter);   // Split Directory and filename
            if (fMain->gFiles->Columns <= 4) // No Path Column  ///(f == 0)   // no directory component
              if (Settings.DoubleClickOpens)
                ActionOpen (NULL);   // Open file
              else
                ActionEdit (fMain->bEdit);
            else   // contains a directory (from a deep search) so go there
              {
                if (f)
                  {
                    *f++ = 0;   // Separate Path and Filename
                    p = StrMake (fMain->lPath->TextGet (), GridItem->Name);
                    fMain->lPath->TextSet (p);
                    free (p);
                  }
                else
                  f = GridItem->Name;
                StrAssignCopy (&fMain->FileSelect, f);   // Set target filename
                PathToTree ();
              }
          }
      }
    return NULL;
  }

void SortChange (void)
  {
    // Note Currently selected file
    if (ListGoto (Dir, fMain->gFiles->Selected1.y - 1, &GridIndex, &GridItem))
      StrAssignCopy (&fMain->FileSelect, GridItem->Name);
    Dir = ListSort (Dir, DirItemsCompare2);
    GridRefresh ();
  }

void ActionGridColumnHighlight (_GridView *Grid, int x)
  {
    static int ColumnOld = 1;
    //
    //DebugAddInt ("Sort Column", x);
    if (x == 0)   // Toggle Directories-first
      {
        if (DirSort != 2)   // except for Size sorting
          DirSortDirFirst = !DirSortDirFirst;
        Grid->ColumnHighlight = ColumnOld;
      }
    else
      {
        if (x == ColumnOld && x == 1)   // 2nd click on "Name"
          DirSort = 1 - DirSort;   // Flip between Name and Ext
        else
          DirSort = ColumnToSort [x];
        DirSortDirFirst = true;
      }
    ColumnOld = Grid->ColumnHighlight;
    SortChange ();
    /*// Note Currently selected file
    if (ListGoto (Dir, Grid->Selected1.y - 1, &GridIndex, &GridItem))
      StrAssignCopy (&fMain->FileSelect, GridItem->Name);
    Dir = ListSort (Dir, DirItemsCompare2);
    GridIndex = -1;
    fMain->FindFileSelected ();
    //fMain->gFiles->CellOffset = {0, 0};
    free (fMain->gFiles->CellHeights);
    fMain->gFiles->CellHeights = NULL;
    fMain->gFiles->ScrollToSelected ();
    fMain->gFiles->Invalidate ();*/
  }

void ActionGridCellSelect (_GridView *Grid, int x, int y)
  {
    static int yOld = -1;
    //
    //####DebugAddP ("Grid Select", {x, y});
    if (y != yOld)
      {
        yOld = y;
        #ifdef Preview
        ActionPreview (NULL);
        #endif // Preview
      }
  }

void ActionCutCopy (_Button *Button)
  {
    char *ItemsNew, *p;
    int Len;
    int a, b;
    int nd, nf, nl;
    char St [100];
    //
    nd = nf = nl = 0;
    ItemsNew = NULL;
    if (Button->Down)
      if (fMain->Focused == fMain->gFiles)   // File Grid has focus
        {
          StrAssignCopy (&ItemsRootPath, fMain->lPath->TextGet ());
          b = fMain->gFiles->Selected1.y;
          Len = 0;
          p = ItemsNew;
          while (true)
            {
              a = fMain->gFiles->Selected0.y;
              while (true)
                {
                  if (ListGoto (Dir, a - 1, &GridIndex, &GridItem))
                    {
                      if (ItemsNew)
                        if (GridItem->SymLink)
                          nl++;
                        else if (GridItem->Directory)
                          nd++;
                        else
                          nf++;
                      if (Len)
                        Len += StrCat (&p, '\n');
                      Len += StrCat (&p, GridItem->Name);
                    }
                  if (a < b) a++; else if (a > b) a--; else break;
                }
              if (ItemsNew)
                break;
              ItemsNew = (char *) malloc (Len + 1);
              Len = 0;
              p = ItemsNew;
            }
          *p = 0;
        }
      else   // tree has focus. Select from Tree
        {
          StrAssignCopy (&ItemsRootPath, fMain->lPath->TextGet ());
          *StrPosLast (ItemsRootPath, PathDelimiter) = 0;   // remove trailing slash
          p = StrFindFilename (ItemsRootPath);   // Find selected directory portion of path
          StrAssignCopy (&ItemsNew, p);   // Copy above to ItemsNew
          *p = 0;   // and clear the rest
          nd++;
        }
    StrAssign (&Items, ItemsNew);
    ClipboardSet (Button->Form->Window, ItemsNew);   // and put in Clipboard
    // Status
    p = St;
    StrCat (&p, "Clipboard: ");
    if (nd + nf == 0)
      StrCat (&p, "empty");
    else
      StrDirsFileLinks (&p, nd, nf, nl);
    *p = 0;
    StrAssignCopy (&fMain->Status, St);
    StrAssignCopy (&fMain->Error, NULL);
    // Debug
    #ifdef DEBUG
    if (Items)
      {
        DebugAddS ("Cut/Copy ItemsRootPath:", ItemsRootPath);
        puts (Items);
      }
    else
      puts ("** NO ITEMS");
    #endif // DEBUG
  }

typedef struct
  {
    char* Items;
    char* ItemsRootPath;
    char* DestRootPath;
    bool  Cut_, Copy_, Link_;
  } _PasteParams;

int ThreadPaste (void *arg)
  {
    _PasteParams *PasteParams;
    int CommonDestPath;
    char *SourcePath, *DestPath, *p;
    char *p1;   // Start of Item
    char *p2;   // End of Item
    char c;
    //
    ThreadsAbort = false;
    Threads++;
    Thread = true;
    //DebugAdd ("ThreadPaste");//####
    PasteParams = (_PasteParams *) arg;
    CommonDestPath = StrLen (PasteParams->DestRootPath);
    //DebugAdd (PasteParams->Items);
    SourcePath = (char *) malloc (MaxPath);
    DestPath = (char *) malloc (MaxPath);
    p1 = PasteParams->Items;
    while (p1 && *p1)
      {
        if (ThreadsAbort)
          break;
        // Break off a single line
        p2 = StrPos (p1, '\n');
        if (p2 == NULL)   // Last Item
          p2 = StrPos (p1, '\0');
        c = *p2;
        *p2 = 0;
        // Build Source and Destination full paths
        p = SourcePath;
        StrCat (&p, PasteParams->ItemsRootPath);
        StrCat (&p, p1);
        *p = 0;
        p = DestPath;
        StrCat (&p, PasteParams->DestRootPath);
        if (!Settings.PastePath)
          p1 = StrFindFilename (p1);
        StrCat (&p, p1);
        *p = 0;
        // Do the Cut / Copy
        if (PasteParams->Copy_)
          FileCopy (SourcePath, DestPath, CommonDestPath);
        else if (PasteParams->Cut_)
          FileMove (SourcePath, DestPath, CommonDestPath);
        else if (PasteParams->Link_)
          MakeFileLink (SourcePath, DestPath);
        *p2 = c;
        if (c)
          p2++;   // step past \n (if its there)
        p1 = p2;
      }
    free (SourcePath);
    free (DestPath);
    free (PasteParams->Items);
    free (PasteParams->ItemsRootPath);
    free (PasteParams->DestRootPath);
    free (PasteParams);
    Reread = true;
    Threads--;
    return 0;
  }

void ActionPaste (_Container *Container)
  {
    _PasteParams *PasteParams;
    //
    if (fConflict)
      delete fConflict;
    fConflict = NULL;
    if (!Items)
      MessageBoxError (/*{300, 80},*/ "Hint\vCopy items before Pasting");
    else if (StrSame (ItemsRootPath, fMain->lPath->TextGet ()) && fMain->gFiles->Columns == 4)
      MessageBoxError (/*{300, 80},*/ "Hint\vGo to a Destination before Pasting");
    else if (!Settings.PastePath && !ItemNamesUnique ())
      MessageBoxError (/*{300, 80},*/ "Hint\vCan NOT Paste similar names without their Paths");
    else
      {
        PasteParams = (_PasteParams *) malloc (sizeof (_PasteParams));
        memset (PasteParams, 0, sizeof (_PasteParams));   // sets everything to 0 / NULL
        StrAssignCopy (&PasteParams->Items, Items);
        StrAssignCopy (&PasteParams->ItemsRootPath, ItemsRootPath);
        StrAssignCopy (&PasteParams->DestRootPath, fMain->lPath->TextGet ());
        if (Container == NULL)
          PasteParams->Link_ = true;
        else
          {
            PasteParams->Cut_ = fMain->bCut->Down;
            PasteParams->Copy_ = fMain->bCopy->Down;
          }
        StartThread (ThreadPaste, PasteParams);
        if (fMain->bCut->Down)
          {
            fMain->bCut->Down = false;
            fMain->bCut->Invalidate ();
            StrAssign (&Items, NULL);
            StrAssignCopy (&fMain->Status, "");
          }
      }
  }

void ActionRename (_Container *Container)
  {
    int a, b;
    char *Stem, *Ext;
    int StemLen;
    //
    if (fMain->Focused == fMain->gFiles)
      {
        a = fMain->gFiles->Selected0.y;
        b = fMain->gFiles->Selected1.y;
        // Find Common Stem or Ext
        Stem = Ext = NULL;
        fMain->RenameStemDiff = fMain->RenameExtDiff = false;
        StemLen = 0;
        while (true)   // go fom b to a
          {
            if (ListGoto (Dir, b - 1, &GridIndex, &GridItem))
              if (Stem == NULL)
                {
                  Stem = GridItem->Name;
                  Ext = StrFindFileExtension (GridItem->Name);
                  if (Ext)   // Extension exists
                    StemLen = Ext - Stem;
                }
              else
                {
                  if (StrCmp (Stem, GridItem->Name, StemLen) != 0)
                    fMain->RenameStemDiff = true;
                  if (StrCmp (Ext, StrFindFileExtension (GridItem->Name)) != 0)
                    fMain->RenameExtDiff = true;
                }
            if (b == a)
              break;
            if (b < a) b++; else b--;
          }
        if (fMain->RenameStemDiff && fMain->RenameExtDiff)   // impossible to group rename
          MessageBoxError (/*{400, 50},*/ "Hint\vGroup renames: All selected files must have either the \auSAME Stem or SAME Extension\au");
        else
          {
            StrAssignCopy (&fMain->RenameDefault, Stem);
            fMain->gFiles->Selected1.x = 1;   // Edit the Filename column
            fMain->gFiles->EditCellInitiate ();
          }
      }
    else if (fMain->Focused == fMain->tDir)
      fMain->tDir->NodeEdit ();
  }

void ActionDuplicate (void)
  {
    int a, b;
    _DirItem di;
    char *p;
    //
    if (fMain->Focused == fMain->gFiles)
      {
        a = fMain->gFiles->Selected0.y;
        b = fMain->gFiles->Selected1.y;
        while (true)   // go from b to a
          {
            if (ListGoto (Dir, b - 1, &GridIndex, &GridItem))
              {
                StrAssignCopy (&fMain->FileSelect, GridItem->Name);
                //Item = StrMake (fMain->lPath->TextGet (), GridItem->Name);
                if (!Duplicate (GridItem))
                  {
                    fMain->ErrorAdd ("Duplicate", GridItem->Name);
                    break;
                  }
                //free (Item);
              }
            if (b == a)
              break;
            if (b < a) b++; else b--;
          }
      }
    else if (fMain->Focused == fMain->tDir)
      {
        StrAssignCopy (&fMain->FileSelect, fMain->tDir->Selected->Text);
        chdir ("..");
        p = GetCurrentPath ();
        fMain->lPath->TextSet (p);
        free (p);
        if (DirItemFromFilename (fMain->FileSelect, &di))
          if (!Duplicate (&di))
            fMain->ErrorAdd ("Duplicate", GridItem->Name);
        DirItemFree (&di);
        PathToTree ();
        ActionNodeSelected (fMain->tDir);
      }
    Reread = true;
  }

void ActionProperties_ (_DirItem *DirItem, byte PropertyFlags)
  {
    int a, b;
    time_t dt;
    //
    a = fMain->gFiles->Selected0.y;
    b = fMain->gFiles->Selected1.y;
    while (true)   // go fom b to a
      {
        if (ListGoto (Dir, a - 1, &GridIndex, &GridItem))
          if (!GridItem->Directory)
            {
              // Attributes
              if (PropertyFlags & PropertyFlagsAttr)   // Attrib has changed
                if (chmod (GridItem->Name, DirItem->Attrib) == 0)   // Success
                  GridItem->Attrib ^= (GridItem->Attrib ^ DirItem->Attrib) & 0x1FF;   // Apply changed bits
                else   // Failed
                  {
                    fMain->ErrorAdd ("File Permission", GridItem->Name);
                    break;
                  }
              // DateTime
              if (PropertyFlags & PropertyFlagsDateTime)   // DateTime has changed
                {
                  if (PropertyFlags & PropertyFlagsOffset)
                    dt = GridItem->DateTime + DirItem->DateTime;
                  else
                    dt = DirItem->DateTime;
                  if (FileDateTimeWrite (GridItem->Name, dt))
                    GridItem->DateTime = dt;
                  else
                    {
                      fMain->ErrorAdd ("Date Time", fProperties->DirItem.Name);
                      break;
                    }
                }
              #ifndef _Windows
              if (PropertyFlags & PropertyFlagsUserGroup)   // User / Group has changed
                if (chown (fProperties->DirItem.Name, fProperties->DirItem.User, fProperties->DirItem.Group) != 0)
                  fMain->ErrorAdd ("User / Group", GridItem->Name);
              #endif // _Windows
            }
        if (a < b) a++; else if (a > b) a--; else break;
      }
    fMain->gFiles->Invalidate ();
  }

void ActionProperties (_Container *Container)
  {
    if (fMain->Focused == fMain->gFiles)
      if (ListGoto (Dir, fMain->gFiles->Selected1.y - 1, &GridIndex, &GridItem))
        EditFileProperties (ActionProperties_, GridItem, Abs (fMain->gFiles->Selected0.y - fMain->gFiles->Selected1.y) + 1);
  }

int ThreadDelete (void *arg)
  {
    char *Items;
    char *p1, *p2;
    char c;
    int Root;
    //
    Threads++;
    ThreadsAbort = false;
    Thread = true;
    Root = -1;
    Items = (char *) arg;
    p1 = Items;
    while (p1 && *p1)
      {
        if (ThreadsAbort)
          break;
        // Split out a single line
        p2 = StrPos (p1, '\n');
        if (p2 == NULL)   // Last Item
          p2 = StrPos (p1, '\0');
        c = *p2;
        *p2 = 0;
        // Get Root path length if we don't have it
        if (Root < 0)
          Root = StrLen (p1);
        else
          DeleteItem (p1, Root);
        // Step to next line
        *p2 = c;
        if (c)
          p2++;   // step past \n (if its there)
        p1 = p2;
      }
    free (Items);
    Reread = true;
    Threads--;
    return 0;
  }

void ActionDelete_ (int Result, void *Parameter)
  {
    if (Result == 0)
      StartThread (ThreadDelete, Parameter);
    else
      free (Parameter);
  }

const char *lf_ = "\n";

void ActionDelete (_Container *Container)
  {
    char *Items, *p;
    int a, b;
    int Len;
    char *St, *s;
    int nFiles, nDirs, nLinks;
    char *n;
    //
    if (fConflict)
      delete fConflict;
    fConflict = NULL;
    nFiles = nDirs = nLinks = 0;
    n = NULL;
    Items = NULL;
    b = fMain->gFiles->Selected1.y;
    Len = 0;
    while (true)   // then add the items
      {
        p = Items;
        Len += StrCat (&p, fMain->lPath->TextGet ());   // Start with the Root path
        Len += StrCat (&p, '\n');
        if (fMain->Focused == fMain->tDir)   // Tree has focus
          {
            chdir ("..");   // get out of here first
            a = StrLen (fMain->lPath->TextGet ());
            Len += StrCat (&p, fMain->lPath->TextGet (), a - 1);   // so add the current directory (- slash)
            nDirs = 1;
            n = fMain->tDir->Selected->Text;
          }
        else if (fMain->Focused == fMain->gFiles)   // Grid has focus
          {
            a = fMain->gFiles->Selected0.y;
            while (true)
              {
                if (ListGoto (Dir, a - 1, &GridIndex, &GridItem))
                  {
                    Len += StrCat (&p, fMain->lPath->TextGet ());
                    Len += StrCat (&p, GridItem->Name);
                    Len += StrCat (&p, '\n');
                    if (Items)
                      if (GridItem->SymLink)
                        nLinks++;
                      else if (GridItem->Directory)
                        nDirs++;
                      else
                        nFiles++;
                     n = GridItem->Name;
                  }
                if (a < b) a++; else if (a > b) a--; else break;
              }
          }
        if (Items)
          break;
        Items = (char *) malloc (Len + 1);
        p = Items;
      }
    *p = 0;
    //DebugAddS ("Delete", Items);//####
    // Confirm with User: Build Question. show Message Box
    s = St = (char *) malloc (StrLen (n) + 100);
    StrCat (&s, "Warning\vDelete: \ab");
    if ((nDirs + nFiles + nLinks) == 1 && n)
      StrCat (&s, n);
    else
      StrDirsFileLinks (&s, nDirs, nFiles, nLinks);
    StrCat (&s, "\ab");
    *s = 0;
    if (MessageResponseBox ({300, 80}, St, "Yes\tNo", ActionDelete_, Items))
      {
        fMessageResponseBox->Container->Colour = cRedLight;
        fMessageResponseBox->Container->ColourGrad = -1;
      }
    else
      free (Items);
    free (St);
  }

void ActionNewFile (_Container *Container)
  {
    int File;
    //
    File = -1;
    StrAssignCopy (&fMain->FileSelect, "NewFile");
    if (!FileExists (fMain->FileSelect))
      File = FileOpen (fMain->FileSelect, foWrite);
    if (File >= 0)
      {
        FileClose (File);
        Reread = true;
        DoRename = true;
      }
    else
      fMain->ErrorAdd ("Create File", NULL);
    fMain->gFiles->FocusSet ();
  }

void ActionNewDir (_Container *Container)
  {
    StrAssignCopy (&fMain->FileSelect, "NewDirectory");
    if (!DirMake (fMain->FileSelect))
      fMain->ErrorAdd ("Create Directory", NULL);
    else
      {
        Reread = true;
        DoRename = true;
      }
    fMain->gFiles->FocusSet ();
  }

void ActionDirUp (_Container *Container)
  {
    if (fMain->tDir->Selected->Parent->Parent)
      {
        StrAssignCopy (&fMain->FileSelect, fMain->tDir->Selected->Text);
        fMain->tDir->Selected = fMain->tDir->Selected->Parent;
        fMain->tDir->ScrollToSelected ();
        ActionNodeSelected (fMain->tDir);
        fMain->tDir->Invalidate ();
      }
  }

void ActionSearch (_Container *Container)
  {
    if (fSearch == NULL)
      fSearch = new _FormSearch (MousePos ());
  }

int FontSize;

void ActionScale (_Container *Btn)
  {
    if (Btn == fMain->bUp)
      FontSize++;
    else if (Btn == fMain->bDown)
      FontSize--;
    fMain->cBody->FontSet (NULL, FontSize);
    fMain->cBody->InvalidateAll (true);
  }

void ActionRefresh (_Container *Bnt)
  {
    _TreeViewNode *n;
    //
    n = fMain->tDir->Nodes->Children;
    while (n)
      {
        n->Expanded = false;
        n = n->NodeNext (NULL);
      }
    PathToTree ();
    ActionNodeSelected (fMain->tDir);
  }

void ActionSummary (_Container *Container)
  {
    _DirItem *di;
    _SummaryItem *SummaryList, *si;
    char *ext;
    //
    SummaryList = NULL;
    di = Dir;
    while (di)
      {
        if (!di->Directory)
          {
            ext = StrFindFileExtension (di->Name);
        //if (ext)
            si = SummaryFind (&SummaryList, ext);
            si->Count++;
            si->TotalSize += di->Size;
          }
        di = di->Next;
      }
    DebugAddInt ("Summary Size", ListLength (SummaryList)); //####
    new _FormSummary (MousePos (), SummaryList);
  }

void ActionStatusClear (_Container *Button)
  {
    if (Threads)
      ThreadsAbort = true;
    else
      StatCopy = StatMove = StatSkip = StatDelete = 0;
    StatChange = true;
  }

void ViewToggleFilter (_Menu *Menu)
  {
    Menu->Ticked ^= Bit [Menu->Selected % 100];
    DirFilter = Menu->Ticked;
    Reread = true;
  }

void ActionPopupSort (_SelectionList *List)
  {
    if (List->Selected >= 0)
      {
        // Note Currently selected file
        if (ListGoto (Dir, fMain->gFiles->Selected1.y - 1, &GridIndex, &GridItem))
          StrAssignCopy (&fMain->FileSelect, GridItem->Name);
        if (List->Selected == 0)   // Dirs First
          DirSortDirFirst = !DirSortDirFirst;
        else
          DirSort = List->Selected - 1;
        Dir = ListSort (Dir, DirItemsCompare2);
        GridIndex = -1;
        fMain->FindFileSelected ();
        fMain->gFiles->ScrollToSelected ();
        fMain->gFiles->ColumnHighlight = SortToColumn [DirSort];
        fMain->gFiles->Invalidate ();
      }
    List->Die ();
  }

const char MessageHelp [] =
  "Help\v"
  "\ab\au\a+\a+Stewy's File Browser\a-\a-\au\ab \eS\n" //"\a2"
  "\n"
  "Explore your file system with the Directory tree.\n"
  "\n"
  "\auSort\au: Change by clicking on the Column Title. "
  "Click Name twice for Name-Extension. "
  "Click Icon Title to mix Directories with Files.\n"
  "\n"
  "Jump to a file in the list by typing (part) of its name. "
  "Type \ab=\ab to find the next match.\n"
  "\n"
  "\auOpen\au files with a double click.\n"
  "\n"
  "\auCut / Copy\au Files and Directories and \auPaste\au them in other Folders:\n"
  "We only copy files that need copying (else Skipped). "
  "Older files are overwritten (see Settings). "
  "Newer files will \abnot\ab be overwritten withour your permission: "
  "These will be listed in a new window: "
  "Select which ones to replace then click \auProceed\au (otherwise \auAbort\au). "
  "Click \auCompare\au to see the differences between the selected Source and Destination files.\n"
  "\n"
  "\auRename\au: When renaming a group of files, they must have either the same Extension OR Stem.\n"
  "\n"
  "\auProperties\au: Change the Properties of all Selected File(s)\n"
  "Date Time: Year/Month/Day HHMM:SS. "
  "Absolute: Sets all files to the new Date Time. "
  "Relative: Changes all files by the same amount. "
  "Permission bits as per Posix standard. Directories eXecute bits are protected (from you). "
  "User / Group: (Linux only).\n"
  "\n"
  "\auSearch\au: Deep file search, including sub-Folders, matching Filenames and / or Contents:\n"
  "Results include a relative \aiPath\ai column. "
  "This \aiPath\ai will be included when Pasting files. (see Settings). "
  "The \aiContaining Target\ai will be the default Search Target on future Edits.\n"
  "\n"
  "The \a1red Tabs\a0 are bookmarks. Adjacent are the \auGo-Up\au and \auRefresh\au buttons.\n"
  "\n"
  "\auRight Click\au: for quick access to some commands.\n"
  "\n"
  "The Top Menu (\auFile\au \auEdit\au \auView\au \auHelp\au) allows access to all commands:\n"
  "\n"
  "\auFile\au -> Rename, Edit, Edit Hex, Delete, Properties, New File, New Directory, Open*, Terminal, Unmount.\n"
  "\n"
  "\auEdit\au -> Cut, Copy, Paste, Paste Link, Duplicate.\n"
  "\n"
  "\auView\au -> Hidden Files, System Files, Search, Summary, Settings, View Log:\n"
  "  Hidden / System: Toggles viewable files.\n"
  "  Search: Look deeply for files baased on part name and (optionally) contents.\n"
  "  Summary: Stats by extension.\n"
  "  Settings: Change how File Browser looks / works.\n"
  "  View Log: Shows a log of all operations.\n"
  "\n"
  "\auManage -> Organize Files\au: "
  "Moves selected files into sub-directoried based on the file properties.\n"
  "\n"
  "\auManage -> Delete Empty Directories\au: n"
  "Recursively searches for and deletes empty directories.\n"
  "\n"
  "\auManage -> List Unmatched Files\au: "
  "Recursively compares the contents of the Clipboard to the current Directory. "
  "Lists all files within the current directory that DO NOT have a corresponding file in the Clipboard. "
  "Typically, after backing up to another location (using Copy/Paste), use this to see what else is in the target. "
  "These may be old, or redundant files that you can move elsewhere or delete.\n"
  "\n"
  "\auHelp\au -> Help, About.\n"
  "\n"
  "\auStatus\au:\n"
  "  Shows Errors, A Summary of the Current Directory OR the File-Clipboard.\n"
  "  Shows file operation Stats.\n"
  "  Click \eG to Clear Messages, Stats or ABORT current file operation.\n"
  "\n"
  "\auKeyboard\au:\n"
  "\abArrows\ab, \abHome\ab, \abEnd\ab, \abTab\ab, \abEnter\ab, \abShift\ab, \abIns\ab, \abDel\ab, \ab^X\ab, \ab^C\ab, \ab^V\ab, \abEsc\ab all do what you'd expect. "
  "Type \abEsc\ab to Go-Up a Directory or close a sub window. "
  "\ab^S\abort change. "
  "\ab^F\abile Search. "
  "\ab^E\abdit file. "
  "\ab^R\abename file(s). "
  "\ab^Q\abuit.\n"
  "\n"
  "\auEdit File\au: The powerful built in Editor:\n"
  "  -Text and Binary files\n"
  "  -Syntax Highlighting\n"
  "  -Macros\n"
  "  -See Edit->Help for details\n"
  //"  -Infinite UNDO: \ab^Z\ab\n"
  //"  -Auto Complete word: \ab^<space>\ab (repeatable)\n"
  //"  -cOmpare Selection with Clipboard: \ab^O\ab\n"
  //"  -\ab^Del\ab = Delete word\n"
  //"  -\abSh-Del\ab = Delete Line\n"
  //"  -\abAlt-Del\ab = Delete to End of Line\n"
  //"  -\abEnter\ab = Insert Line Break. \ab^Enter\ab = same but at End of Line\n"
  //"  -\ab^B\ab = Swap places with Bookmark\n"
  //"  -Move cursor by words: \ab^\eI\ab / \ab^\eJ\ab\n"
  //"  -Change case of word(s): aaa Aaa AAA: \ab^K\ab\n"
  //"  -Duplicate line: \ab^D\ab\n"
  //"  -Move a line up / down: \abAlt\eK\ab / \abAlt\eL\ab\n"
  //"  -Outdents / Indents line(s): \abAlt\eI\ab / \abAlt\eJ\ab\n"
  //"  -Scroll Text up / down: \ab^\eK\ab / \ab^\eL\ab\n"
  //"  -Jump to matching bracket: \ab^]\ab\n"
  //"  -\ab^F\abind. \ab^N\abext. \ab^P\abrevious.\n"
  //"  -\ab^G\aboto Line number.\n"
  //"  -\ab^J\abump to <Path>/<File>: Brings the File Browser to the file in the text.\n"
  //"  -\ab^S\abave.\n"
  //"  -Recordable Macros:\n"
  //"     Record:\n"
  //"       Click on Red Record button.\n"
  //"       Select a Function key (from the list).\n"
  //"       it records the Macro as you type/edit.\n"
  //"       Click Red button again.\n"
  //"     Playback: Press the Function key you chose when recording.\n"
  "\nFile Browser uses my \abWidgets\ab library: a multi targeted GUI development package.";

char MessageAbout [] =
  "About\v"
  "\n\ab\a+Stewy's File Browser - Revision ######\a-\ab\n\n"
  "Copyright 2025, 2026 Stewart Tunbridge & Pi Micros. All rights reserved.\n"
  "Distributed under the AGPL-3.0 license license for free use and reuse with acknowledgement and the License.\n"
  "\nStewart Tunbridge - stewarttunbridge@gmail.com\n"
  "Pi Micros - pimicros@gmail.com\n"
  "\aupimicros.com.au\n"
  "github.com/StewartTunbridge\au\n";

void ActionMenu (_Menu *Menu)
  {
    char *St, *Cmnd, *c;
    int Res;
    //
    switch (Menu->DataInt * 100 + Menu->Selected)
      {
        // File
        case   0: ActionRename (fMain->bRename); break; // File>Rename
        case   1: ActionEdit (fMain->bEdit); break;   // File>Edit
        case   2: ActionEdit (NULL); break;   // File>Edit HEX
        case   3: ActionDelete (fMain->bDelete); break;   // File>Delete
        case   4: ActionProperties (NULL); break;   // File>Properties
        case   5: ActionNewFile (NULL); break;   // File>New File
        case   6: ActionNewDir (NULL); break;   // File>New Directory
        case   7: ActionOpen (fMain->bOpen); break;   // File>Open File / Directory
        case   8: // Execute
                  if (fExecute)
                    delete (fExecute);
                  St = NULL;
                  if (fMain->Focused == fMain->tDir)   // allow focus on TreeDir
                    //St = fMain->tDir->Selected->Text; this needs a ../
                    St = fMain->lPath->TextGet ();
                  else if (ListGoto (Dir, fMain->gFiles->Selected1.y - 1, &GridIndex, &GridItem))
                    St = GridItem->Name;
                  if (St)
                    fExecute = new _FormExecute (MousePos (), St);
                  else
                    Beep ();
                  break;
        //case   8: ActionOpenInTerminal (NULL); break;   // File>Open File / Directory
        case   9: Res = system (Settings.Terminal);   // File>Terminal
                  DebugAddInt ("system", Res);
                  if (Res)
                    fMain->ErrorAdd ("Terminal Error", Settings.Terminal);
                  break;
        case  10: // Unmount
                  #ifdef _Windows
                  system ("C:"); //%%%%
                  #else
                  SetCurrentPath ("..");   // get off this drive
                  #endif
                  St = fMain->lPath->TextGet ();
                  if (St)
                    {
                      Cmnd = (char *) malloc (StrLen (St) + 32);
                      c = Cmnd;
                      #ifdef _Windows
                      StrCat (&c, "mountvol ");
                      StrCat (&c, St [0]);
                      StrCat (&c, St [1]);
                      StrCat (&c, " /p");
                      #else
                      //Res = umount (Path);
                      StrCat (&c, "umount \"");
                      StrCat (&c, St);
                      StrCat (&c, '\"');
                      #endif // _Windows
                      *c = 0;
                      Res = system (Cmnd);
                      free (Cmnd);
                      #ifdef _Windows
                      if (Res < 0)
                      #else
                      if (Res)
                      #endif
                        MessageBoxError (/*{400, 100},*/ "Error\vCan NOT unmount this location");
                      Reread = true;
                    }
                  break;
        //case  11: ActionSearch (Menu); break;
        //
        // Edit
        case 100: // Cut
        case 101: // Copy
                  if (Menu->Selected == 0)   // Cut
                    {
                      fMain->bCut->Down = true;
                      fMain->bCopy->Down = false;
                      ActionCutCopy (fMain->bCut);
                    }
                  else   // Copy
                    {
                      fMain->bCut->Down = false;
                      fMain->bCopy->Down = true;
                      ActionCutCopy (fMain->bCopy);
                    }
                  fMain->bCut->DataInt = 0;
                  fMain->bCopy->DataInt = 0;
                  fMain->bCut->Invalidate ();
                  fMain->bCopy->Invalidate ();
                  break;
        case 102: ActionPaste (fMain->bPaste); break;
        case 103: ActionPaste (NULL); break;   // Paste Link
        case 104: ActionDuplicate (); break;
        //
        // View
        case 200:
        case 201: ViewToggleFilter (Menu);
                  break;
        case 202: ActionSearch (Menu);
                  break;
        case 203: ActionSummary (Menu);
                  break;
        case 204: // Settings
                  if (fSettings == NULL)
                    fSettings = new _FormSettings (MousePos ());
                  break;
        case 205: // View Log
                  FileEdit ("Log", 0, &LogData);
                  break;
        //
        // Manage
        case 300: //Organize files
                  OrganizeFiles ();
                  break;
        case 301: DeleteEmptyDirectories ();
                  break;
        case 302: //List Unmatched Files
                  ListUnmatchedFiles ();
                  break;
        //
        // Help
        case 400: MessageBox ({600, 600}, (char *) MessageHelp, 14);   // Help
                  fMessageBox->Container->Bitmap = BitmapLoad (fMessageBox->Window, "FileBrowser.bmp");
                  fMessageBox->lMessage->AlignVert = aTop;
                  fMessageBox->lMessage->Align = aLeft;
                  fMessageBox->lMessage->Margin = 4;
                  fMessageBox->Container->Colour = cBlueLight;
                  fMessageBox->Container->ColourGrad = cWhite;
                  break;
        case 401: St = StrPos ((char *) MessageAbout, "######");   // About
                  if (St)
                    {
                      StrDelete (St, 6);
                      StrInsert (St, Revision_);
                    }
                  MessageBox ({500, 260}, (char *) MessageAbout);
                  fMessageBox->lMessage->AlignVert = aTop;
                  fMessageBox->lMessage->Margin = 4;
                  fMessageBox->Container->Colour = cRedLight;
                  fMessageBox->Container->ColourGrad = cYellow; //cOrange;
                  break;

      }
  }

void ActionPopup (_MenuPopup *Menu)
  {
    _Rect Pos;
    _SelectionList *sl;
    //
    //DebugAddInt ("Popup", Menu->Selected);
    switch (Menu->Selected)
      {
        case  0: // Sort
                 Pos = {Menu->SelectionList->Rect.x, Menu->SelectionList->Rect.y, 0, 0};
                 sl = new _SelectionList (Menu->SelectionList->Parent, Pos, "_    Dirs First\t    Sort by Name\t    Sort by Ext\t    Sort by Size\t    Sort by Date Time", (_Action) ActionPopupSort);
                 sl->Boxed = 0x0001;
                 sl->Ticked = DirSortDirFirst;
                 sl->Dotted = Bit [DirSort] << 1;
                 break;
        case  1: // Select All
                 fMain->gFiles->Selected0.y = fMain->gFiles->Title.y;
                 fMain->gFiles->Selected1.y = fMain->gFiles->Rows - 1;
                 fMain->gFiles->Invalidate ();
                 break;
        case  2: // Cut
                 fMain->bCut->Down = true;//!fMain->bCut->Down;
                 fMain->bCopy->Down = false;
                 ActionCutCopy (fMain->bCut);
                 fMain->bCut->Invalidate ();
                 fMain->bCopy->Invalidate ();
                 break;
        case  3: // Copy
                 fMain->bCopy->Down = true;//!fMain->bCopy->Down;
                 fMain->bCut->Down = false;
                 ActionCutCopy (fMain->bCopy);
                 fMain->bCut->Invalidate ();
                 fMain->bCopy->Invalidate ();
                 break;
        case  4: ActionPaste (fMain->bPaste); break;   // Paste
        case  5: ActionPaste (NULL); break;   // Paste Link
        case  6: ActionRename (fMain->bRename); break;   // Rename
        case  7: ActionDuplicate (); break;   // Duplicate
        case  8: ActionOpen (fMain->bOpen); break;   // Open
        case  9: ActionEdit (fMain->bEdit); break;   // Edit
        case 10: ActionDelete (fMain->bDelete); break;   // Delete
        case 11: ActionNewFile (NULL); break;   // New File
        case 12: ActionNewDir (NULL); break;   // New Directory
                 break;
      }
  }

void ButtonOperate (_Button *Button)
  {
    Button->DownSet (true);   //Button->Down;
    Button->Invalidate ();
    if (Button->Action)
      Button->Action (Button);
  }

char JumpToTarget [64];
int JumpToY;
char JumpToPrev;
bool Quit = false;

// Add to JumpToTarget and jump to the next matching Filename

void AddToJumpTo (char c)
  {
    int yStart;
    //
    if (c && !JumpToPrev)   // new search target
      JumpToTarget [0] = 0;   // then clear old target
    JumpToPrev = c;
    if (c)   // adding to target
      if (StrLen (JumpToTarget) + 1 < sizeof (JumpToTarget))
        StrAppend (JumpToTarget, c);
    // loop thru Dir looking for JumpToTarget
    yStart = JumpToY = fMain->gFiles->Selected1.y;   // start
    while (true)
      {
        if (c == 0)   // next
          {
            JumpToY++;
            if (JumpToY >= fMain->gFiles->Rows)
              JumpToY = 1;
            if (JumpToY == yStart)   // not found
              {
                Beep ();
                JumpToTarget [0] = 0;   // clear target
                break;
              }
          }
        if (ListGoto (Dir, JumpToY - 1, &GridIndex, &GridItem))
          if (StrPos (StrFindFilename (GridItem->Name), JumpToTarget))   // found
            {
              fMain->gFiles->Selected0.y = fMain->gFiles->Selected1.y = JumpToY;
              fMain->gFiles->ScrollToSelected ();
              fMain->gFiles->FocusSet ();
              fMain->gFiles->Invalidate ();
              break;
            }
        c = 0;
      }
  }

bool FormMainEventPreview (_Form *Form, _Event *Event)
  {
    bool Res;
    //
    if (fMessageResponseBox)   // make message response boxes modal
      return true;
    if (fOrganizeFiles)   // make file manage modal
      return true;
    if (fProperties)   // make proerties modal
      return true;
    if (ThreadSearchRunning)
      return true;
    if (JumpToY != fMain->gFiles->Selected0.y)   // we have moved so start again
      JumpToTarget [0] = 0;
    Res = false;
    if (Event->Type == etKeyDown && (fMain->Focused == fMain->tDir || fMain->Focused == fMain->gFiles))
      {
        Res = true;
        if (Event->Key == esc)
          ActionDirUp (fMain->tDir);
        else if (Event->Key == Cntrl ('S'))
          {
            DirSort = (DirSort + 1) % ((fMain->gFiles->Columns == 4) ? 4 : 5);
            SortChange ();
          }
        else if (Event->Key == KeyDel)
          ActionDelete (NULL);
        else if (Event->Key == Cntrl ('X'))
          ButtonOperate (fMain->bCut);
        else if (Event->Key == Cntrl ('C'))
          ButtonOperate (fMain->bCopy);
        else if (Event->Key == Cntrl ('V'))
          ActionPaste (fMain->bPaste);
        else if (Event->Key == Cntrl ('F'))
          ActionSearch (NULL);
        else if (Event->Key == Cntrl ('E'))
          ActionEdit (fMain->bEdit);
        else if (Event->Key == Cntrl ('R'))
          ActionRename (fMain->bRename);
        else if (Event->Key == Cntrl ('Q'))
          Quit = true;
        else if (Event->Key == '=' || Event->Key == KeyF1 + 2) //&& fMain->Focused == fMain->gFiles)
          AddToJumpTo (0);
        else if (Event->Key >= ' ' && Event->Key < 0x7F) //&& fMain->Focused == fMain->gFiles)
          AddToJumpTo (Event->Key);
        else
          Res = false;
      }
    return Res;
  }

void _FormMain::ErrorAdd (char *Err, char *File)
  {
    char *St, *s;
    //
    St = NULL;
    if (Err)
      {
        St = (char *) malloc (StrLen (Err) + StrLen (File) + 32);
        s = St;
        StrCat (&s, "** ERROR: ");
        StrCat (&s, Err);
        if (File)
          {
            StrCat (&s, " \ab");
            StrCat (&s, File);
            StrCat (&s, "\ab");
          }
        *s = 0;
      }
    StrAssign (&Error, St);
  }

bool _FormMain::FindFileSelected (void)
  {
    _DirItem *d;
    int n;
    //
    d = Dir;
    n = 0;
    if (FileSelect)
      while (d)
        {
          if (StrSame (d->Name, FileSelect))
            {
              gFiles->Selected0.y = gFiles->Selected1.y = n + 1;   // And select the FileSelect
              gFiles->ScrollToSelected ();
              return true;
            }
          d = d->Next;
          n++;
        }
    return false;
  }

/*void ActionTest (_Container *c)
  {
    fMain->gFiles->Invalidate ();
  } #### */

const int Widths [] = {20, 200, 85, 150, 85};

_FormMain::_FormMain (char *Title): _Form (Title, {0, 0, 650, 500}, waResizable)
  {
    const int mh = 22;   // menu height
    const int bw = 52;   // button width
    const int bh = 36;   // button height
    const int lh = 26;   // label height
    const int tw = 160;   // Tree Width
    const int th = 28;   // Tabs Height
    const int sh = 20;   // Status Height
    int x, y;
    char bm [64], *b;
    int i;
    //
    //Container->ColourGrad = ColourAdjust (cGreen, 160);
    Container->FontSet (NULL);   // Default Font and Size
    Container->Bitmap = BitmapLoad (Window, "FileBrowser.bmp");
    y = 0;
    MenuBar = new _Container (Container, {0, 0, 0, mh});
    MenuBar->Colour = cTextBgnd;
    x = 0;
    mFile = new _Menu (MenuBar, {x, 0, 0, 0}, "File\t\e3 Rename\t\et Edit\t\et Edit Hex\t\e5 Delete\t_\eD Properties\t\e6 New File\t_\e7 New Directory\t\e4 Open\t\eC Execute\t\eC Terminal\t\eF Unmount", (_Action) ActionMenu);
    mFile->DataInt = 0;
    x += mFile->Rect.Width;
    mEdit = new _Menu (MenuBar, {x, 0, 0, 0}, "Edit\t\e0 Cut\t\e1 Copy\t\e2 Paste\t\e2 Paste Link\t\e1 Duplicate", (_Action) ActionMenu);
    mEdit->DataInt = 1;
    x += mEdit->Rect.Width;
    mView = new _Menu (MenuBar, {x, 0, 0, 0}, "View\t    Hidden Files\t_    System Files\t\eB Search\t_\eV Summary\tSettings ...\tLog ...", (_Action) ActionMenu);
    mView->DataInt = 2;
    mView->Boxed = 0x0001 | 0x0002;
    x += mView->Rect.Width;
    mManage = new _Menu (MenuBar, {x, 0, 0, 0}, "Manage\tOrganize Files\tDelete Empty Directories\tList Unmatched Files", (_Action) ActionMenu);
    mManage->DataInt = 3;
    x += mManage->Rect.Width;
    mHelp = new _Menu (MenuBar, {x, 0, 0, 0}, "Help\tHelp\tAbout", (_Action) ActionMenu);
    mHelp->DataInt = 4;
    y += mh;
    //
    Toolbar = new _Container (Container, {0, y, 0, bh}, NULL); y += bh;
    x = 0;
    //
    #ifdef Preview
    bPreview = new _Button (Toolbar, {x, 0, bw, 0}, "\eE\nPreview", (_Action) ActionPreview); x += bw;
    bPreview->Toggle = true;
    #endif // Preview
    bCut = new _Button (Toolbar, {x, 0, bw, 0}, "\e0\nCut", (_Action) ActionCutCopy); x += bw;
    bCut->DataInt = 0;
    bCut->Toggle = true;
    bCut->Group = 1;
    bCopy = new _Button (Toolbar, {x, 0, bw, 0}, "\e1\nCopy", (_Action) ActionCutCopy); x += bw;
    bCopy->DataInt = 1;
    bCopy->Toggle = true;
    bCopy->Group = 1;
    //
    bPaste = new _Button (Toolbar, {x, 0, bw, 0}, "\e2\nPaste", ActionPaste); x += bw;
    bRename = new _Button (Toolbar, {x, 0, bw, 0}, "\e3\nRename", ActionRename); x += bw;
    //bProperties = new _Button (Toolbar, {x, 0, bw, 0}, "\e3\nProperties", ActionProperties); x += bw;
    bDelete = new _Button (Toolbar, {x, 0, bw, 0}, "\e5\nDelete", ActionDelete); x += bw;
    bOpen = new _Button (Toolbar, {x, 0, bw, 0}, "\e4\nOpen", ActionOpen); x += bw;
    bEdit = new _Button (Toolbar, {x, 0, bw, 0}, "\et\nEdit", (_Action) ActionEdit); x += bw;
    //bNewFile = new _Button (Toolbar, {x, 0, bw, 0}, "\e6\nNew File", ActionNewFile); x += bw;
    //bNewDir = new _Button (Toolbar, {x, 0, bw, 0}, "\e7\nNew Dir", ActionNewDir); x += bw;
    bSearch = new _Button (Toolbar, {x, 0, bw, 0}, "\eB\nSearch", ActionSearch); x += bw;
    //bSummary = new _Button (Toolbar, {x, 0, bw, 0}, "\eV\nSummary", ActionSummary); x += bw;
    x = bh / 2;
    bUp = new _ButtonArrow (Toolbar, {-x, 0, x, x}, NULL, (_Action) ActionScale, dUp);
    bDown = new _ButtonArrow (Toolbar, {-x, x, 0, 0}, NULL, (_Action) ActionScale, dDown);
    //
    x = 0;
    b = bm;
    i = 0;
    while (true)
      {
        StrCat (&b, "B");
        NumToStr (&b, i + 1);
        if (++i >= SIZEARRAY (Bookmarks))
          break;
        StrCat (&b, '\t');
      }
    *b = 0;
    tBookmarks = new _Tabs (Container, {x, y, -2 * th, th}, bm, (_Action) ActionTabSelected, ActionDrag);
    tBookmarks->ColourText = cRed;
    tBookmarks->Colour = cForm1;
    bDirUp = new _Button (Container, {-2 * th, y, th, th}, "\eW", ActionDirUp);
    bRefresh = new _Button (Container, {-th, y, 0, th}, "\eH", ActionRefresh);
    //bRefresh->Colour = -1;
    y += th;
    //
    lPath = new _Label (Container, {4, y, -4, lh}, NULL, aLeft);
    lPath->Colour = Container->Colour;
    lPath->ColourText = cBlue;
    lSize = new _Label (Container, {4, y, -4, lh}, NULL, aRight); lSize->ColourText = cGreenDark;
    //lSize = new _Label (lPath, {0}, NULL, aRight); lSize->ColourText = cGreenDark;
    pmPathCopy = new _MenuPopup (lPath, {0}, "Copy Path", ActionCopyPath);
    y += lh;
    //
    cBody = new _Container (Container, {0, y, 0, -sh});
    cBody->FontSet (NULL);   // Default Font and Size
    mFiles = new _MenuPopup (cBody, {0},
      "Sort ...\t_Select All\t\e0 Cut\t\e1 Copy\t\e2 Paste\t_\e2 Paste Link\t\e3 Rename\t\e1 Duplicate\t\e4 Open\t\et Edit\t_\e5 Delete\t\e6 New File\t\e7 New Directory",
      (_Action) ActionPopup);
    x = 0;
    tDir = new _TreeView (cBody, {x, 0, tw, 0}, (_Action) ActionNodeSelected);
    tDir->ColourText = cBlue;
    tDir->ActionNodeEditWrite = ActionNodeEditWrite;
    // #### new _Button (tDir, {-20, -20, 0, 0}, "X", ActionTest);
    x += tw;
    Splitter = new _Splitter (cBody, {x, 0, 8, 0});
    x += 8;
    gFiles = new _GridView (cBody, {x, 0, 0, 0}, NULL, 5, 12);
    gFiles->Title = gFiles->Locked = {0, 1};
    gFiles->Options = goGridLines | goSelectMulti | goSelectLine | goAdjustColumn | goColumnHighlight;
    gFiles->ColumnHighlight = SortToColumn [DirSort];
    for (x = 0; x < SIZEARRAY (Widths); x++)
      gFiles->CellWidthSet (x, Widths [x]);
    gFiles->ActionCellDraw = ActionGridCellDraw;
    gFiles->ActionCellRead = ActionGridCellRead;
    gFiles->ActionColumnHighlight = ActionGridColumnHighlight;
    gFiles->ActionCellSelect = ActionGridCellSelect;
    //gFiles->Colour = cAquaLight; //to adjust the LockedCells colour, put in a container and set its colour
    //
    lStatus = new _Label (Container, {0, -sh, 0, 0}, NULL, aLeft, bLowered);
    bStatus = new _Button (lStatus, {-sh, 0, 0, 0}, "\eG", ActionStatusClear);   // x - clear
    Spinner = new _Wait (lStatus, {-2*sh, 0, sh, 0});
    Spinner->ColourText = cBlue;
    //mStatus = new _MenuPopup (lStatus, {0}, "\eg Clear Stats\t\eh Abort", (_Action) ActionPopupStats);
    Error = NULL;
    Status = NULL;
    FileSelect = NULL;
    RenameDefault = NULL;
    RenameStemDiff = RenameExtDiff = false;
    for (x = 0; x < SIZEARRAY (Bookmarks); x++)
      Bookmarks [x] = NULL;
    EventPreview = FormMainEventPreview;
  }

_FormMain::~_FormMain ()
  {
    int x;
    //
    free (Error);
    free (Status);
    free (FileSelect);
    for (x = 0; x < SIZEARRAY (Bookmarks); x++)
      free (Bookmarks [x]);
  }

const char *SettingsFile = ".FileBrowser";

void SettingsSave (void)
  {
    int f;
    char *St, *s;
    int i, *ip;;
    //
    s = StrMake (PathStart, SettingsFile);
    f = FileOpen (s, foWrite);
    free (s);
    St = (char *) malloc (MaxPath);
    if (f > 0)
      {
        s = St;
        StrCat (&s, "OverwriteOlder\t");
        NumToStr (&s, Settings.OverwriteOlder);
        *s = 0;
        FileWriteLine (f, St);
        //
        s = St;
        StrCat (&s, "PastePath\t");
        NumToStr (&s, Settings.PastePath);
        *s = 0;
        FileWriteLine (f, St);
        //
        s = St;
        StrCat (&s, "DoubleClick\t");
        NumToStr (&s, Settings.DoubleClickOpens);
        *s = 0;
        FileWriteLine (f, St);
        //
        s = St;
        StrCat (&s, "Colours\t");
        NumToHex (&s, Settings.ColourDir);
        StrCat (&s, '\t');
        NumToHex (&s, Settings.ColourFile);
        *s = 0;
        FileWriteLine (f, St);
        //
        s = St;
        StrCat (&s, "Open\t");
        StrCat (&s, Settings.Open);
        StrCat (&s, '\t');
        StrCat (&s, Settings.OpenExe);
        StrCat (&s, '\t');
        StrCat (&s, Settings.RunCommand);
        *s = 0;
        FileWriteLine (f, St);
        //
        s = St;
        StrCat (&s, "Terminal\t");
        StrCat (&s, Settings.Terminal);
        *s = 0;
        FileWriteLine (f, St);
        //
        s = St;
        StrCat (&s, "EditFont\t");
        StrCat (&s, Settings.EditFont);
        *s = 0;
        FileWriteLine (f, St);
        //
        s = St;
        StrCat (&s, "EditFontSize\t");
        IntToStr (&s, Settings.EditFontSize);
        *s = 0;
        FileWriteLine (f, St);
        //
        s = St;
        StrCat (&s, "EditTarget\t");
        StrCat (&s, Settings.EditTarget);
        *s = 0;
        FileWriteLine (f, St);
        // Macros
        for (i = 0; i < SIZEARRAY (Settings.Macros); i++)
          if (Settings.Macros [i] && Settings.Macros [i][0])
            {
              s = St;
              StrCat (&s, "Macro#");
              *StrPos (St, '#') = IntToHexDigit (i);
              ip = Settings.Macros [i];
              while (*ip)
                {
                  StrCat (&s, '\t');
                  NumToHex (&s, *ip++);
                }
              *s = 0;
              FileWriteLine (f, St);
            }
        // Execute
        s = St;
        StrCat (&s, "Execute\t");
        StrCat (&s, CommandHistory);
        *s = 0;
        FileWriteLine (f, St);
        // Search
        s = St;
        StrCat (&s, "SearchTargetHistory\t");
        StrCat (&s, SearchTargetHistory);
        *s = 0;
        FileWriteLine (f, St);
        //
        FileClose (f);
      }
    free (St);
  }

bool SettingsLoad (void)
  {
    int f;
    int Size;
    char *Data, *p1, *p2, *p3;
    int i, j;
    //
    Settings = {0};
    Settings.OverwriteOlder = true;
    Settings.PastePath = true;
    Settings.DoubleClickOpens = true;
    Settings.ColourDir = ColourAdjust (cYellow, 185);
    Settings.ColourFile = ColourAdjust (cAqua, 195);
    #ifdef _Windows
    StrAssignCopy (&Settings.Open, "start \"\" \"@\"");
    StrAssignCopy (&Settings.OpenExe, "start \"\" \"@\"");
    StrAssignCopy (&Settings.RunCommand, "start cmd /k @");  // %%%%
    StrAssignCopy (&Settings.Terminal, "start cmd");
    #else
    StrAssignCopy (&Settings.Open, "xdg-open \'@\'");
    StrAssignCopy (&Settings.OpenExe, "\'./@\' &");
    StrAssignCopy (&Settings.RunCommand, "xterm -hold -e \'@\' &");
    StrAssignCopy (&Settings.Terminal, "xterm &");
    #endif
    p1 = StrMake (PathStart, SettingsFile);
    f = FileOpen (p1, foRead);
    free (p1);
    if (f > 0)
      {
        Size = FileSize (f);
        Data = (char *) malloc (Size + 1);
        FileRead (f, (byte *) Data, Size);
        Data [Size] = 0;   // Terminate String
        p1 = Data;
        while (*p1)
          {
            p2 = StrPos (p1, '\n');
            *p2++ = 0;
            if (StrMatch (&p1, "OverwriteOlder\t"))
              Settings.OverwriteOlder = StrGetNum (&p1);
            else if (StrMatch (&p1, "PastePath\t"))
              Settings.PastePath = StrGetNum (&p1);
            else if (StrMatch (&p1, "DoubleClick\t"))
              Settings.DoubleClickOpens = StrGetNum (&p1);
            else if (StrMatch (&p1, "Colours\t"))
              {
                Settings.ColourDir = StrGetHex (&p1);
                Settings.ColourFile = StrGetHex (&p1);
              }
            else if (StrMatch (&p1, "Open\t"))
              {
                i = 0;
                while (true)
                  {
                    p3 = StrPos (p1, '\t');
                    if (p3)
                      *p3++ = 0;
                    if (i == 0) StrAssignCopy (&Settings.Open, p1);
                    if (i == 1) StrAssignCopy (&Settings.OpenExe, p1);
                    if (i == 2) StrAssignCopy (&Settings.RunCommand, p1);
                    i++;
                    if (!p3)
                      break;
                    p1 = p3;
                  }
              }
            else if (StrMatch (&p1, "Terminal\t"))
              StrAssignCopy (&Settings.Terminal, p1);
            else if (StrMatch (&p1, "EditFont\t"))
              StrAssignCopy (&Settings.EditFont, p1);
            else if (StrMatch (&p1, "EditFontSize\t"))
              Settings.EditFontSize = StrGetNum (&p1);
            else if (StrMatch (&p1, "EditTarget\t"))
              StrAssignCopy (&Settings.EditTarget, p1);
            else if (StrMatch (&p1, "Macro"))
              {
                i = StrGetHex (&p1);
                if (i < SIZEARRAY (Settings.Macros))
                  {
                    Settings.Macros [i] = (int *) malloc (MacroMax * sizeof (int));
                    j = 0;
                    while (*p1 && j < MacroMax)
                      Settings.Macros [i][j++] = StrGetHex (&p1);
                    Settings.Macros [i][j] = 0;
                  }
              }
            else if (StrMatch (&p1, "Execute\t"))
              StrAssignCopy (&CommandHistory, p1);
            else if (StrMatch (&p1, "SearchTargetHistory\t"))
              StrAssignCopy (&SearchTargetHistory, p1);
            else
              DebugAddS ("Unrecognized Setting", p1);
            p1 = p2;
          }
        free (Data);
        FileClose (f);
        return true;
      }
    return false;
  }

void SettingsFree (void)
  {
    int i;
    //
    free (Settings.Terminal);
    free (Settings.Open);
    free (Settings.OpenExe);
    free (Settings.RunCommand);
    free (Settings.EditFont);
    free (Settings.EditTarget);
    for (i = 0; i < SIZEARRAY (Settings.Macros); i++)
      if (Settings.Macros [i])
        free (Settings.Macros [i]);
     Settings = {0};
  }

char *LogFilename = "Log";

void LogSave (void)
  {
    char *FilePath;
    int File;
    //
    FilePath = StrMake (PathStart, LogFilename);
    File = FileOpen (FilePath, foWrite);
    free (FilePath);
    if (File >= 0)
      {
        FileWrite (File, (byte *) LogData, StrLen (LogData));
        FileClose (File);
        DebugAdd ("Log Saved"); //####
      }
  }

void LogLoad (void)
  {
    char *FilePath;
    int File;
    int Len;
    //
    FilePath = StrMake (PathStart, LogFilename);
    File = FileOpen (FilePath, foRead);
    free (FilePath);
    if (File >= 0)
      {
        Len = FileSize (File);
        DebugAddInt ("LogLoad: Len ", Len);
        if (Len)
          {
            LogData = (char *) malloc (Len + 1);
            FileRead (File, (byte *) LogData, Len);
            LogData [Len] = 0;
          }
        FileClose (File);
      }
    #ifdef DEBUG
    else  // ####
      DebugAdd ("LogLoad fail *****");
    #endif
  }

int main_ (int argc, char *argv [])
  {
    _Bitmap *Icon;
    char St [256], *s;
    char *LogDataOld;
    int LogTick;
    //
    FontSize = FontSizeDefault;
    ButtonStyleDefault = bsFlat;
    fMain = new _FormMain ("Stewy's File Browser");
    Icon = BitmapLoad (fMain->Window, "Folders.bmp");
    WindowSetIcon (fMain->Window, Icon);
    BitmapDestroy (Icon);
    SettingsLoad ();
    SettingsApply ();
    LogLoad ();
    LogDataOld = LogData;
    LogTick = 0;
    #ifdef _Windows
    int Drives;
    char d;
    char DrivePath [] = "?:";
    Drives = GetLogicalDrives ();
    d = 'A';
    while (Drives)
      {
        if (Drives & 1)
          {
            DrivePath [0] = d;
            fMain->tDir->Nodes->NodeAdd (true, DrivePath);
          }
        d++;
         Drives >>= 1;
      }
    #endif
    DoRename = false;
    s = GetCurrentPath ();
    if (s)
      fMain->lPath->TextSet (s);
    free (s);
    PathToTree ();
    while (!FormsUpdate () && !Quit)   // Do everything, until Quit requested
      {
        usleep (10000);
        // Maintain Log file
        if (LogData != LogDataOld)
          {
            LogTick = ClockMS ();
            LogDataOld = LogData;
          }
        if (LogTick && ClockMS () - LogTick > 5000)   // log changed 5 seconds ago
          {
            LogSave ();
            LogTick = 0;
          }
        // Reread Directory if requested
        if (Reread)
          {
            Reread = false;
            ActionNodeSelected (fMain->tDir);
            JumpToTarget [0] = 0;
          }
        if (DoRename)
          ActionRename (NULL);
        DoRename = false;
        // Maintain Status Bar
        if (fMain->Error)
          {
            fMain->lStatus->Colour = cRedLight;
            fMain->lStatus->TextSet (fMain->Error);
            StrAssign (&fMain->Error, NULL);
            StrAssign (&fMain->Status, NULL);
            StatChange = false;
          }
        else
          if (fMain->Status)
            {
              fMain->lStatus->Colour = -1;
              fMain->lStatus->TextSet (fMain->Status);
              StrAssign (&fMain->Status, NULL);
            }
          else
            {
              if (StatChange || Thread != (Threads > 0))
                {
                  StatChange = false;
                  // Conflict Form?
                  if (Threads == 0 && Thread)   // finished stuff
                    {
                      if (ConflictList)
                        fConflict = new _FormConflict (MousePos ());
                      Thread = false;
                    }
                  fMain->lStatus->Colour = cGreenLight;
                  s = St;
                  StrCat (&s, "Stats: Copied \ab");
                  IntToStr (&s, StatCopy, DigitsCommas);
                  StrCat (&s, "\ab,  Moved \ab");
                  IntToStr (&s, StatMove, DigitsCommas);
                  StrCat (&s, "\ab,  Skipped \ab");
                  IntToStr (&s, StatSkip, DigitsCommas);
                  StrCat (&s, "\ab,  Deleted \ab");
                  IntToStr (&s, StatDelete, DigitsCommas);
                  StrCat (&s, "\ab");
                  if (ThreadsAbort)
                    StrCat (&s, "  \a1\abABORTED\ab");
                  *s = 0;
                  fMain->lStatus->TextSet (St);
                }
              fMain->Spinner->VisibleSet (Threads);
            }
      }
    while (FormList)
      delete FormList;
    SettingsSave ();
    //LogSave ();
    DirFree (Dir);
    free (Items);
    free (ItemsRootPath);
    ConflictListFree (ConflictList);
    SettingsFree ();
    SearchFree ();
    ExecuteFree ();
    return 0;
  }
