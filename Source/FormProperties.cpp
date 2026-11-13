////////////////////////////////////////////////////////////////////////////////////////////////////
//
// PROPERTIES FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#define PropertyFlagsAttr 0x01
#define PropertyFlagsDateTime 0x02
#define PropertyFlagsOffset 0x04
#define PropertyFlagsUserGroup 0x08

typedef void (*_ActionResultDirItem) (_DirItem *Result, byte PropertyFlags);

class _FormProperties: public _Form
  {
    public:
      _Label *lDate;
      _Edit *eDate;
      _Label *lPermissionBits;
      _CheckBox *cbPermissionBits [9];
      _ButtonRadio *brAbsolute, *brRelative;
      _Edit *eUser, *eGroup;
      _Container *Toolbar;
      _Button *bOK, *bCancel;
      _FormProperties (_Point Pos, _ActionResultDirItem ActionResultDirItem_, _DirItem *DirItem_, char *Title);
     ~_FormProperties ();
      //
      _DirItem DirItem;
      _ActionResultDirItem ActionResultDirItem;
  };

_FormProperties *fProperties;

void ActionPropertiesOK (_Container *Button)
  {
    bool OK;
    byte Flags;
    int Attrib, i;
    int ab;
    tm tmp;
    char *Dat;
    struct passwd *Onr;
    struct group *Grp;
    //
    OK = true;
    Flags = 0x00;
    // Read Attribute bits
    Attrib = fProperties->DirItem.Attrib;
    for (i = 0; i < SIZEARRAY (fProperties->cbPermissionBits); i++)
      {
        ab = (1 << (8 - i));
        fProperties->DirItem.Attrib &= ~ab;
        if (fProperties->cbPermissionBits [i]->Down)
          fProperties->DirItem.Attrib |= ab;
      }
    if (Attrib != fProperties->DirItem.Attrib)
      Flags |= PropertyFlagsAttr;
    // Read DateTime
    if (fProperties->eDate->Changed)
      {
        Flags |= PropertyFlagsDateTime;
        Dat = fProperties->eDate->TextGet ();
        OK = false;
        if (StrGetDateTime (&Dat, &tmp))
          {
            OK = true;
            if (fProperties->brRelative->Down)
              {
                Flags |= PropertyFlagsOffset;
                fProperties->DirItem.DateTime = mktime (&tmp) - fProperties->DirItem.DateTime;
              }
            else
              fProperties->DirItem.DateTime = mktime (&tmp);
          }
        if (!OK)
          MessageBoxError ("Hint\vDate Time SYNTAX ERROR");
      }
    //
    #ifndef _Windows
    if (fProperties->eUser->Changed || fProperties->eGroup->Changed)
      {
        Flags |= PropertyFlagsUserGroup;
        Onr = getpwnam (fProperties->eUser->TextGet ());
        Grp = getgrnam (fProperties->eGroup->TextGet ());
        if (Onr && Grp)
          {
            fProperties->DirItem.User = Onr->pw_uid;
            fProperties->DirItem.Group = Grp->gr_gid;
          }
        else
          {
            MessageBoxError ("Hint\vUser or Group NAME ERROR");
            OK = false;
          }
      }
    #endif // _Windows
    if (OK)
      {
        if (fProperties->ActionResultDirItem)
          fProperties->ActionResultDirItem (&fProperties->DirItem, Flags);
        Button->Form->Die ();
      }
  }

void ActionPropertiesCancel (_Container *Button)
  {
    Button->Form->Die ();
  }

const char *RWX [] = {"r", "w", "x"};

bool EventPreviewProperties (_Form *Form, _Event *Event)
  {
    bool Res;
    //
    //if (fMessageBox)
    //  return true;
    Res = false;
    if (Event->Type == etKeyDown)
      {
        Res = true;
        if (Event->Key == KeyEnter)
          ActionPropertiesOK (fProperties->bOK);
        else if (Event->Key == esc)
          ActionPropertiesCancel (fProperties->bCancel);
        else
          Res = false;
      }
    return Res;
  }

_FormProperties::_FormProperties (_Point Pos, _ActionResultDirItem ActionResultDirItem_, _DirItem *DirItem_, char *Title) : _Form (Title, {Pos.x, Pos.y, 250, 210}, waAlwaysOnTop)
  { //"File Properties"
    const int border = 4;
    const int lh = 32;
    const int col = 84;
    const int cbw = 14;
    const int bh = 32;
    const int bw = 64;
    int x, y;
    int i;
    char Dat [100], *d;
    tm *tmp;
    struct passwd *User;
    struct group *Group;
    //
    DirItem = *DirItem_;
    ActionResultDirItem = ActionResultDirItem_;
    Container->FontSet (NULL);
    Container->Colour = cPurpleLight;
    x = border;
    y = border;
    //
    lDate = new _Label (Container, {x, y, col, lh}, "Date Time", aLeft); x += col;
    tmp = localtime (&DirItem.DateTime);
    d = Dat;
    DateTimeToStr (&d, tmp);
    *d = 0;
    eDate = new _Edit (Container, {x, y+2, -border, lh-border}, Dat);
    x = border;
    y += lh;
    brAbsolute = new _ButtonRadio (Container, {x, y, 80, lh/2}, "Absolute");
    brAbsolute->Down = true;
    x += 80;
    brRelative = new _ButtonRadio (Container, {x, y, 100, lh/2}, "Relative");
    //cbOffset = new _CheckBox (Container, {x, y, 0, lh}, "Offset Only");
    x = border;
    y += lh + border;
    //
    lPermissionBits = new _Label (Container, {x, y, col, lh}, "Permissions"); x += col;
    for (i = 0; i < SIZEARRAY (cbPermissionBits); i++)
      {
        cbPermissionBits [i] = new _CheckBox (Container, {x, y+4, cbw, lh-8}, NULL);
        cbPermissionBits [i]->StyleCross = true;
        cbPermissionBits [i]->Down = DirItem.Attrib & (1 << (8 - i));
        new _Label (Container, {x, y - 8, cbw, 16}, (char *) RWX [i % 3], aCenter);   // add the rwx tag
        x += cbw;
        if (i % 3 == 2)
          x += 8;
      }
    y += lh + border;
    //
    x = border;
    new _Label (Container, {x, y, col, lh}, "User");
    #ifndef _Windows
    User = getpwuid (DirItem_->User);
    if (User)
      eUser = new _Edit (Container, {x + col, y+2, -border, lh-border}, User->pw_name);
    #endif // _Windows
    y += lh;
    new _Label (Container, {x, y, col, lh}, "Group");
    #ifndef _Windows
    Group = getgrgid (DirItem_->Group);
    if (Group)
      eGroup = new _Edit (Container, {x + col, y+2, -border, lh-border}, Group->gr_name);
    #endif // _Windows
    //
    Toolbar = new _Container (Container, {0, -bh, 0, 0});
    x = 0; //border;
    bOK = new _Button (Toolbar, {x, 0, bw, 0}, "\a2OK", ActionPropertiesOK); x += bw;
    bCancel = new _Button (Toolbar, {x, 0, bw, 0}, "\a1Cancel", ActionPropertiesCancel);
    EventPreview = EventPreviewProperties;
  }

_FormProperties::~_FormProperties (void)
  {
    fProperties = NULL;
    //fMain->Container->EnabledSet (true);
  }

void EditFileProperties (_ActionResultDirItem ActionResultDirItem, _DirItem *DirItem, int Count)
  {
    char Title [120], *t;
    //
    if (fProperties == NULL)
      {
        //fMain->Container->EnabledSet (false);
        t = Title;
        if (Count == 1)
          StrCat (&t, DirItem->Name, 100);
        else
          {
            IntToStr (&t, Count);
            StrCat (&t, " items");
          }
        *t = 0;
        fProperties = new _FormProperties (MousePos (), ActionResultDirItem, DirItem, Title);
      }
  }
