////////////////////////////////////////////////////////////////////////////
//
// COMPARE FORM
//
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
//
// SUPPORT

typedef struct
  {
    int Start, End;   // inclusive
    int Span (void);   // noninclusive
  } _Range;

int _Range::Span (void)
  {
    return End - Start;
  }

_Range FindLargestMatchingBlock (char *S1, char *S2, _Range R1, _Range R2)
  {
    _Range Block, BlockMax;
    //
    Block = {-1, -1};
    BlockMax = {-1, -1};
    while (true)
      {
        if (R1.Start >= R1.End || R2.Start >= R2.End)
          break;
        if (!S1 [R1.Start] || !S2 [R2.Start])
          break;
        if (S1 [R1.Start] == S2 [R2.Start])
          {
            if (Block.Start < 0)
              Block.Start = R1.Start;
            Block.End = R1.Start + 1;
            if (Block.Span () > BlockMax.Span ())
              BlockMax = Block;
          }
        else
          Block.Start = -1;
        R1.Start++;
        R2.Start++;
      }
    return BlockMax;
  }

bool LineEnd (char Ch)
  {
    return Ch == '\r' || Ch == '\n' || Ch == '\0';
  }

bool LineMatch (char *p1, char *p2)
  {
    while (!LineEnd (*p1) && !LineEnd (*p2))
      {
        if (*p1 != *p2)
          return false;
        p1++;
        p2++;
      }
    return LineEnd (*p1) == LineEnd (*p2);
  }

/*
        Corr = FindLargestMatchingBlock (S1, S2, {R1.Start + Offset, R1.End}, R2);
        if (Corr.Span () > CorrMax.Span ())
          {
            CorrMax = Corr;
            *R1_ = Corr;
            *R2_ = {Corr.Start - Offset, Corr.End - Offset};
            //*Index1 = Corr.Start;
            //*Index2 = Corr.Start;
          }
      }
    Offset = 0;
    while (true)
      {
        if (R2.Start + Offset >= R2.End)
          break;
        if (S2 [R2.Start + Offset] == '\0')
          break;
        Corr = FindLargestMatchingBlock (S1, S2, R1, {R2.Start + Offset, R2.End});
        if (Corr.Span () > CorrMax.Span ())
          {
            CorrMax = Corr;
            *R1_ = {Corr.Start - Offset, Corr.End - Offset};
            *R2_ = Corr;
            //*Index1 = Corr.Start;
            //*Index2 = Corr.Start;
          }
        Offset++;
      }
    return CorrMax.Span ();
  }*/


////////////////////////////////////////////////////////////////////////////
//
// FORM

class _FormCompare: public _Form
  {
    public:
      _Label *lTitle;
      _ButtonArrow *bPrev, *bNext;
      _EditMultiline *eResult;
      //
      char *S1, *S2;
      int *S1Lines, *S2Lines;
      //
      void TitleSet (char *Name1, char *Name2);
      void ResultAdd (char *s, int *Lines, _Range R, byte Flag);
      int SetLines (char *St, int **Lines);
      int StrMaxCorrelation (_Range R1, _Range R2, _Range *R1_, _Range *R2_);
      void CompareText (_Range R1, _Range R2, int Threshold);
      _FormCompare (_Point Pos, char *p1, char *p2, char *Name1, char *Name2, int Threshold);
     ~_FormCompare ();
      //
      int ResultLen;
      char *Result, *Result_;
      byte *ResultFlags;
      int Differences;
  };

_FormCompare *fCompare;


////////////////////////////////////////////////////////////////////////////////////////////////////
//
// COLOUR HIGHLIGHTING CALLBACK

bool TextCallBackCompare (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size)
  {
    int i;
    //
    if (fCompare)
      {
        i = *Text - fCompare->eResult->TextGet ();
        if (fCompare->ResultFlags)
          if (i < fCompare->ResultLen)   // within flags length
            if (fCompare->ResultFlags [i] == 0x00)
              Font->Colour = cGray;
            else if (fCompare->ResultFlags [i] == 0x01)
              Font->Colour = cBlue;
            else
              Font->Colour = cRed;
      }
    return TextCallBackEdit (Window, Text, Font, Posn, Size);
  }


////////////////////////////////////////////////////////////////////////////
//
// COMPARE / BUILD RESULT

void StrAdd (char **St, int *StLen, char *Add, int AddLen)
  {
    char *StNew;
    //
    StNew = (char *) malloc (*StLen + AddLen + 1);
    memcpy (StNew, *St, *StLen);
    memcpy (StNew + *StLen, Add, AddLen);
    free (*St);
    *St = StNew;
    *StLen += AddLen;
  }

void _FormCompare::ResultAdd (char *s, int *Lines, _Range R, byte Flag)
  {
    char *New;
    _Range Index;
    //
    if (R.Span () > 0)
      {
        Index.Start = ArrayGetInt (&Lines, R.Start, 0);
        Index.End = ArrayGetInt (&Lines, R.End, 0);
        if (Index.End == 0)
          Index.End = StrLen (s);
        New = (char *) malloc (ResultLen + Index.Span () + 1);
        memcpy (New, Result, ResultLen);
        memcpy (&New [ResultLen], &s [Index.Start], Index.Span ());
        free (Result);
        Result = New;
        //
        New = (char *) malloc (ResultLen + Index.Span () + 1);
        memcpy (New, ResultFlags, ResultLen);
        memset (New + ResultLen, Flag, Index.Span ());
        free (ResultFlags);
        ResultFlags = (byte *) New;
        //
        ResultLen += Index.Span ();
        if (Flag)
          Differences++;
      }
  }

int _FormCompare::StrMaxCorrelation (_Range R1, _Range R2, _Range *R1_, _Range *R2_)
  {
    int Offset, Match, MatchMax, i;
    //
    R1_->Start = R1_->End = R1.Start;
    R2_->Start = R2_->End = R2.Start;
    //CorrMax = {-1, -1};
    Match = 0;
    MatchMax = 0;
    for (Offset = 0; R1.Start + Offset < R1.End; Offset++)
      for (Match = i = 0; R1.Start + Offset + i < R1.End && R2.Start + i < R2.End; i++)
        if (!LineMatch (&S1 [ArrayGetInt (&S1Lines, R1.Start + Offset + i)], &S2 [ArrayGetInt (&S2Lines, R2.Start + i)]))
          Match = 0;
        else
          if (++Match > MatchMax)
            {
              R1_->Start = R1.Start + Offset + i - (Match - 1);
              R2_->Start = R2.Start + i - (Match - 1);
              MatchMax = Match;
            }
    for (Offset = 0; R2.Start + Offset < R2.End; Offset++)
      for (Match = i = 0; R1.Start + i < R1.End && R2.Start + Offset + i < R2.End; i++)
        if (!LineMatch (&S1 [ArrayGetInt (&S1Lines, R1.Start + i)], &S2 [ArrayGetInt (&S2Lines, R2.Start + Offset + i)]))
          Match = 0;
        else
          if (++Match > MatchMax)
            {
              R1_->Start = R1.Start + i - (Match - 1);
              R2_->Start = R2.Start + Offset + i - (Match - 1);
              MatchMax = Match;
            }
    R1_->End = R1_->Start + MatchMax;
    R2_->End = R2_->Start + MatchMax;
    return MatchMax;
  }

void _FormCompare::CompareText (_Range R1, _Range R2, int Threshold)
  {
    int Len;
    _Range R1_, R2_;
    //
    Threshold = 5; //%%%%
    Len = StrMaxCorrelation (R1, R2, &R1_, &R2_);
    DebugAddInt ("StrMaxCorrelation", Len);
    //DebugAddP ("StrMaxCorrelation 1", *(_Point *)&R1_); //####
    //DebugAddP ("StrMaxCorrelation 2", *(_Point *)&R2_);
    if (R1_.Start - R1.Start < Threshold || R2_.Start - R2.Start < Threshold)
      {
        ResultAdd (S1, S1Lines, {R1.Start, R1_.Start}, 0x01);
        ResultAdd (S2, S2Lines, {R2.Start, R2_.Start}, 0x02);
      }
    else
      CompareText ({R1.Start, R1_.Start}, {R2.Start, R2_.Start}, Threshold);
    ResultAdd (S1, S1Lines, {R1_.Start, R1_.End}, 0x00);   // Common
    if (R1.End - R1_.End < Threshold || R2.End - R2_.End < Threshold || Len == 0)
      {
        ResultAdd (S1, S1Lines, {R1_.End, R1.End}, 0x01);
        ResultAdd (S2, S2Lines, {R2_.End, R2.End}, 0x02);
      }
    else
      CompareText ({R1_.End, R1.End}, {R2_.End, R2.End}, Threshold);
  }

void _FormCompare::TitleSet (char *Name1, char *Name2)
  {
    char *Titl, *t;
    int l;
    //
    l = 0;
    Titl = t = NULL;
    while (true)
      {
        l += StrCat (&t, "\ab");
        l += StrAddThings (&t, Differences, "Difference");
        l += StrCat (&t, "  \a4");
        l += StrCat (&t, Name1); //"Source
        l += StrCat (&t, " \a1");
        l += StrCat (&t, Name2); //"Destination
        l += StrCat (&t, "\ab");   // Title, colours consistent with CompareTextBlocksColours
        if (Titl)
          break;
        Titl = t = (char *) malloc (l + 1);
      }
    *t = 0;
    lTitle->TextSet (Titl);
    free (Titl);
  }

// Record the start of every line

int _FormCompare::SetLines (char *St, int **Lines)
  {
    int Line;
    char *s;
    //
    *Lines = NULL;
    Line = 0;
    s = St;
    while (true)
      {
        ArraySetInt (Lines, Line++, s - St);
        while (!LineEnd (*s))
          s++;
        if (*s == '\0')
          break;
        if (s [0] == '\r' && s [1] == '\n')
          s++;   // step past \r
        s++;   // step past \n or \r
      }
    return Line;
  }

void ActionJump (_ButtonArrow *BA)
  {
    char *Text;
    int i;
    byte flags;
    //
    Text = fCompare->eResult->TextGet ();
    i = fCompare->eResult->Selected [0];
    flags = 0x00;
    if (fCompare->ResultFlags)
      flags = fCompare->ResultFlags [i];
    while (true)
      {
        if (BA->Direction == dLeft)
          i--;
        else
          i++;
        if (i <= 0 || Text [i] == '\0' || i >=  fCompare->ResultLen)
          {
            Beep ();
            break;
          }
        if (fCompare->ResultFlags [i] != flags)
          {
            fCompare->eResult->MoveToIndex (i);
            fCompare->eResult->ScrollToCursor ();
            fCompare->eResult->Invalidate ();
            break;
          }
      }
  }

_FormCompare::_FormCompare (_Point Pos, char *p1, char *p2, char *Name1, char *Name2, int Threshold): _Form ("Text Comparison", {Pos.x, Pos.y, 740, 300}, waResizable | waAlwaysOnTop)
  {
    int sz;
    int n1, n2;
    //
    Container->FontSet (NULL);
    sz = Container->Font->YAdvance + 8;
    lTitle = new _Label (Container, {0, 0, 0, sz}, NULL);
    bPrev = new _ButtonArrow (lTitle, {-2 * sz, 0, sz, 0}, NULL, (_Action) ActionJump, dLeft);
    bNext = new _ButtonArrow (lTitle, {-sz, 0, sz, 0}, NULL, (_Action) ActionJump, dRight);
    eResult = new _EditMultiline (Container, {0, sz, 0, 0}, NULL);
    //
    Differences = 0;
    Result = Result_ = NULL;
    ResultFlags = NULL;
    ResultLen = 0;
    S1 = p1;
    S2 = p2;
    n1 = SetLines (S1, &S1Lines);
    n2 = SetLines (S2, &S2Lines);
    CompareText ({0, n1}, {0, n2}, Threshold);  // %%%% globalize Threshold
    if (Result)
      Result [ResultLen] = '\0';
    eResult->TextSet (Result);
    free (Result);
    Result = NULL;
    TitleSet (Name1, Name2);
    eResult->TextCallBack = TextCallBackCompare;
  }

_FormCompare::~_FormCompare (void)
  {
    free (Result);
    free (ResultFlags);
    fCompare = NULL;
  }

