////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SETTINGS FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class _FormSettings: public _Form
  {
    public:
      _Section *sOperations;
      //_Label *lOperations;
      _CheckBox *cbOverwriteOlder, *cbPastePath;
      _Label *lDoubleClick;
      _DropList *dlDoubleClick;
      _Section *sAppearance;
      //_Label *lAppearance;
      _Label *lColours;
      _Button *bColourDir, *bColourFile;
      _Section *sSystem;
      //_Label *lSystem;
      _Label *lOpen, *lTerminal;
      _Edit *eOpen1, *eOpen2, *eOpen3, *eTerminal;
      _FormSettings (_Point Pos);
      ~_FormSettings (void);
  };

_FormSettings *fSettings = NULL;

void ActionSettingsChange (_Container *Container)
  {
    Settings.OverwriteOlder = fSettings->cbOverwriteOlder->Down;
    Settings.PastePath = fSettings->cbPastePath->Down;
    Settings.DoubleClickOpens = fSettings->dlDoubleClick->Selected;
    StrAssignCopy (&Settings.Open, fSettings->eOpen1->TextGet ());
    StrAssignCopy (&Settings.OpenExe, fSettings->eOpen2->TextGet ());
    StrAssignCopy (&Settings.RunCommand, fSettings->eOpen3->TextGet ());
    StrAssignCopy (&Settings.Terminal, fSettings->eTerminal->TextGet ());
    Settings.ColourDir = fSettings->bColourDir->Colour;
    Settings.ColourFile = fSettings->bColourFile->Colour;
    SettingsApply ();
  }

void ActionColourSelected (int Result, void *Parameter)
  {
    _Button *bColour;
    //
    if (Parameter)
      {
        bColour = (_Button *) Parameter;
        bColour->Colour = Result;
        bColour->Invalidate ();
        ActionSettingsChange (NULL);
      }
  }

void ActionColour (_Container *Button)
  {
    ColourSelect ("Adjust Colour", Button->Colour, ActionColourSelected, Button);
  }

bool FormSettingsEventPreview (_Form *Form, _Event *Event)
  {
    if (Event->Type == etWindowClose)
      delete fSettings;
    else if (Event->Type == etKeyDown && Event->Key == esc)
      delete fSettings;
    //else if (Event->Type == etKeyDown && Event->Key == KeyEnter)
    //  ActionSettingsOK (NULL);
    else
      return false;
    return true;
  }

int RectY2 (_Container *Container)
  {
    return Container->Rect.y + Container->Rect.Height;
  }

_FormSettings::_FormSettings (_Point Pos): _Form ("Settings", {Pos.x, Pos.y, 380, 346}, waAlwaysOnTop)
  {
    int y;
    const int gap = 4;
    const int col = 100;
    const int bh = 24;
    const int bw = 64;
    //int lh;
    //
    Container->Colour = cWhite;
    Container->ColourGrad = -1;
    Container->FontSet (NULL);   // default
    //lh = Container->FontFind ()->YAdvance;
    //
    sOperations = new _Section (Container, {gap, gap, -gap, 120}, "\ab\asOperations\ab\as", ColourAdjust (cGreen, 80));
    y = 30;
    //y += 100 + gap;
    cbOverwriteOlder = new _CheckBox (sOperations, {col, y, -gap, bh}, "Automatically Overwrite Older Files in \auPaste\au", ActionSettingsChange);
    cbOverwriteOlder->Down = Settings.OverwriteOlder;
    y += bh + gap;
    cbPastePath = new _CheckBox (sOperations, {col, y, -gap, bh}, "Include \auPath\au column in \auPaste\au (Search results)", ActionSettingsChange);
    cbPastePath->Down = Settings.PastePath;
    y += bh + gap;
    lDoubleClick = new _Label (sOperations, {0, y, col - gap, bh}, "DoubleClick", aRight);
    dlDoubleClick = new _DropList (sOperations, {col, y, 80, bh}, "Edit\tOpen", ActionSettingsChange);
    dlDoubleClick->Selected = Settings.DoubleClickOpens;
    //y += bh + lh;
    //
    sAppearance = new _Section (Container, {gap, RectY2 (sOperations) + gap, -gap, 65}, "\ab\asAppearance\ab\as", ColourAdjust (cBlue, 160));
    y = 30;
    lColours = new _Label (sAppearance, {0, y, col - gap, bh}, "Colours", aRight);
    bColourDir = new _Button (sAppearance, {col, y, bw, bh}, "Dirs", ActionColour);
    bColourDir->Colour = Settings.ColourDir;
    bColourDir->ColourGrad = -1;
    bColourDir->DataInt = 0;
    bColourFile = new _Button (sAppearance, {col + gap + bw, y, bw, bh}, "Files", ActionColour);
    bColourFile->Colour = Settings.ColourFile;
    bColourFile->ColourGrad = -1;
    bColourFile->DataInt = 1;
    //y += bh + lh;
    //
    sSystem = new _Section (Container, {gap, RectY2 (sAppearance) + gap, -gap, -gap}, "\ab\asSystem\ab\as", ColourAdjust (cRed, 160));
    y = 30;
    lOpen = new _Label (sSystem, {0, y, col - gap, bh + bh + gap}, "Open @", aRight);
    eOpen1 = new _Edit (sSystem, {col, y, -gap, bh}, Settings.Open, ActionSettingsChange);
    y += bh + gap;
    eOpen2 = new _Edit (sSystem, {col, y, -gap, bh}, Settings.OpenExe, ActionSettingsChange);
    y += bh + gap;
    new _Label (sSystem, {0, y, col - gap, bh}, "Run Command", aRight);
    eOpen3 = new _Edit (sSystem, {col, y, -gap, bh}, Settings.RunCommand, ActionSettingsChange);
    y += bh + gap;
    lTerminal = new _Label (sSystem, {0, y, col - gap, bh}, "Open a Terminal", aRight);
    eTerminal = new _Edit (sSystem, {col, y, -gap, bh}, Settings.Terminal, ActionSettingsChange);
    //
    //bOK = new _Button (Container, {0, -bh, bw, bh}, "OK", ActionSettingsOK);
    //bCancel = new _Button (Container, {-bw, -bh, bw, bh}, "Cancel", ActionSettingsCancel);
    EventPreview = FormSettingsEventPreview;
  }

_FormSettings::~_FormSettings (void)
  {
    fSettings = NULL;
  }
