////////////////////////////////////////////////////////////////////////////////////////////////////
//
// EXECUTE FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////


class _FormExecute: public _Form
  {
    public:
      _Label *lHelp;
      //_CheckBox *cbTerminal;
      _EditPopupDropList  *eCommand;
      _Button *bRun; //, *bClose;
      _FormExecute (_Point Pos, char *Name);
     ~_FormExecute ();
  };

_FormExecute *fExecute = NULL;

char *CommandHistory = NULL;

/*int system_ (char *Command, bool Terminal)
  {
    int Res;
    char *St, *s;
    //
    // Build History
    if (Command)
      {
        s = StrPos (CommandHistory, Command);
        if (s == 0)
          {
            s = St = (char *) malloc (StrLen (Command) + 1 + StrLen (CommandHistory) + 1);
            StrCat (&s, Command);
            StrCat (&s, '\t');
            StrCat (&s, CommandHistory);
            *s = '\0';
            StrAssign (&CommandHistory, St);
          }
      }
    // Build Command
    s = St = (char *) malloc (MaxPath);
    if (Terminal)
      {
        #ifdef _Windows
        StrCat (&s, "start cmd");
        if (Command)
          StrCat (&s, " /k ");
        #else
        //StrCat (&s, "xterm");//bash
        StrCat (&s, Settings.Terminal);
        //StrCat (&s, getenv ("SHELL")); does not open a new window
        if (Command)
          StrCat (&s, " -hold -e ");//bash
          //StrCat (&s, " -e ");//bash
        #endif // _Windows
      }
    StrCat (&s, Command);
    //if (Terminal)
      StrCat (&s, " &");
    *s = 0;
    //
    Res = system (St);
    DebugAddInt (St, Res);
    free (St);
    return Res;
  } %%%% */

void ActionExecuteRun (_Container *Container)
  {
    int Res;
    //
    fExecute->eCommand->DropListAdd (fExecute->eCommand->EditPopup->TextGet (), 8);
    StrAssignCopy (&CommandHistory, fExecute->eCommand->TextGet ());
    Res = RunCommand (fExecute->eCommand->EditPopup->TextGet ());
    if (Res)
      fMain->ErrorAdd ("Command Error", fExecute->eCommand->EditPopup->TextGet ());
    //fExecute->Die ();
    //fExecute = NULL;
  }

/*void ActionExecuteClose (_Container *Container)
  {
    fExecute->Die ();
    fExecute = NULL;
  }*/

bool FormExecuteEventPreview (_Form *Form, _Event *Event)
  {
    if (Event->Type == etWindowClose || (Event->Type == etKeyDown && Event->Key == esc))
      {
        delete fExecute;
        fExecute = NULL;
        return true;
      }
    else if (Event->Type == etKeyDown && Event->Key == KeyEnter)
      {
        ActionExecuteRun (NULL);
        return true;
      }
    return false;
  }

_FormExecute::_FormExecute (_Point Pos, char *Name): _Form ("Execute", {Pos.x, Pos.y, 400, 100}, waAlwaysOnTop | waResizable)
  {
    char *Name_;
    int x, y;
    char *St, *s;
    const int gap = 4;
    const int ht = 28;
    const int bh = 32;
    const int bw = 64;
    //
    y = gap;
    Container->Colour = cGreenLight;
    Container->FontSet (NULL);
    lHelp = new _Label (Container, {gap, y, 0, ht}, "Enter Command");
    //cbTerminal = new _CheckBox (Container, {0, y, -gap, ht}, "Terminal");
    //cbTerminal->Align = aRight;
    y += ht;
    //
    Name_ = StrReplace (Name, "\"", "\\\"");   // Replace " with \"
    s = St = (char *) malloc (StrLen (Name_) + 8);
    StrCat (&s, " \"");
    StrCat (&s, Name_);
    StrCat (&s, '\"');
    *s = 0;
    //
    eCommand = new _EditPopupDropList (Container, {gap, y, -gap, ht}, CommandHistory);
    eCommand->EditPopup->TextSet (St);
    //bPrev = new _ButtonArrow (Container, {- (ht + gap), y, ht, ht}, NULL, ActionPrevCommand, dDown);
    y += ht + gap;
    x = 0;
    bRun = new _Button (Container, {x, -bh, bw, 0}, "\ab\a2RUN\ab", ActionExecuteRun); x += bw;
    //bClose = new _Button (Container, {x, -bh, bw, 0}, "\ab\a1Close\ab", ActionExecuteClose);
    EventPreview = FormExecuteEventPreview;
    free (Name_);
    free (St);
  }

_FormExecute::~_FormExecute ()
  {
    fExecute = NULL;
  }

void ExecuteFree (void)
  {
    free (CommandHistory);
  }
