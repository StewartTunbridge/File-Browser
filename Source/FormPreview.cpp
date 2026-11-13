////////////////////////////////////////////////////////////////////////////////////////////////////
//
// PREVIEW FORM
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class _FormPreview: public _Form
  {
    public:
      _EditMultiline *eText;
      _Image *Image;
      //
      _FormPreview (_Point Pos);
     ~_FormPreview (void);
      //
      void FileLoad (char *Filename);
  };

_FormPreview *fPreview;

void _FormPreview::FileLoad (char *Filename)
  {
    Image->VisibleSet (false);
    eText->VisibleSet (false);
    //
    BitmapDestroy (Image->Bitmap);
    Image->Bitmap = NULL;
    eText->TextSet (NULL);
    //
    Image->Bitmap = BitmapLoad (Window, Filename);
    if (Image->Bitmap)   // Image Loaded
      Image->VisibleSet (true);
    else if (FileSize (Filename) > (1 << 20))
      fMain->ErrorAdd ("Preview Large File", Filename);
    else if (eText->FileLoad (Filename))
      eText->VisibleSet (true);
    else
      fMain->ErrorAdd ("Can not preview", Filename);
    Container->InvalidateAll (true);
  }

_FormPreview::_FormPreview (_Point Pos) : _Form (NULL, {Pos.x, Pos.y, 500, 500}, waResizable | waAlwaysOnTop)
  {
    Container->FontSet (NULL);
    Image = new _Image (Container, {0}, NULL);
    eText = new _EditMultiline (Container, {0}, NULL);
  }

_FormPreview::~_FormPreview (void)
  {
    fPreview = NULL;
  }

void PreviewFile (char *Filename)
  {
    _Point Pos;
    //
    if (fPreview == NULL)
      {
        Pos = MousePos ();
        fPreview = new _FormPreview (Pos);
      }
    fPreview->FileLoad (Filename);
  }
