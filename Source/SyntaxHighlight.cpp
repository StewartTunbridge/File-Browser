////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SYNTAX HIGHLIGHT
//
////////////////////////////////////////////////////////////////////////////////////////////////////


#define shEnc 0x01
#define shEOL 0x02
#define shSOL 0x04
//#define shRep 0x04

typedef struct
  {
    char *Token, *TokenClose;   // Token to highlight and its opposite (if shEnc)
    byte Tags;                  // shEnc: Enclose all between Token & TokenClose. shEnc: To end of line
    int Colour;                 // Colour of the highlight | Style << 24
  } _SyntaxItem;

const _SyntaxItem SyntaxItemsHex [] =   // Syntax table for Hex/ASCII dump
  {
    {"\"", "\"", shEnc, cAquaDark},
    {NULL, NULL, 0, 0}
  };

const _SyntaxItem SyntaxItemsC [] =   // Syntax table for c/c++/h/h++
  {
    {"#include", NULL, shEOL, cGreenDark},
    {"#define", NULL, shEOL, cGreenDark},
    {"#ifdef", NULL, shEOL, cGreenDark},
    {"#if", NULL, shEOL, cGreenDark},
    {"#else", NULL, shEOL, cGreenDark},
    {"#endif", NULL, shEOL, cGreenDark},

    {"//", NULL, shEOL, cGrayDark | (fsItalic << 24)},
    {"/*", "*/", shEnc, cGrayDark | (fsItalic << 24)},

    {"typedef", NULL, 0, cBlue | (fsBold << 24)},
    {"extern", NULL, 0, cBlue | (fsBold << 24)},
    {"const", NULL, 0, cBlue | (fsBold << 24)},
    {"volatile", NULL, 0, cBlue | (fsBold << 24)},
    {"static", NULL, 0, cBlue | (fsBold << 24)},
    {"struct", NULL, 0, cBlue | (fsBold << 24)},
    {"union", NULL, 0, cBlue | (fsBold << 24)},
    {"void", NULL, 0, cBlue | (fsBold << 24)},
    {"long", NULL, 0, cBlue | (fsBold << 24)},
    {"unsigned", NULL, 0, cBlue | (fsBold << 24)},
    {"int", NULL, 0, cBlue | (fsBold << 24)},
    {"char", NULL, 0, cBlue | (fsBold << 24)},
    {"bool", NULL, 0, cBlue | (fsBold << 24)},
    {"enum", NULL, 0, cBlue | (fsBold << 24)},
    {"real", NULL, 0, cBlue | (fsBold << 24)},
    {"new", NULL, 0, cBlue | (fsBold << 24)},

    {"class", NULL, 0, cBlue | (fsBold << 24)},
    {"public", NULL, 0, cBlue | (fsBold << 24)},
    {"private", NULL, 0, cBlue | (fsBold << 24)},
    {"protected", NULL, 0, cBlue | (fsBold << 24)},
    {"virtual", NULL, 0, cBlue | (fsBold << 24)},

    {"if", NULL, 0, cAquaDark | (fsBold << 24)},
    {"else", NULL, 0, cAquaDark | (fsBold << 24)},
    {"while", NULL, 0, cAquaDark | (fsBold << 24)},
    {"do", NULL, 0, cAquaDark | (fsBold << 24)},
    {"switch", NULL, 0, cAquaDark | (fsBold << 24)},
    {"case", NULL, 0, cAquaDark | (fsBold << 24)},
    {"default", NULL, 0, cAquaDark | (fsBold << 24)},
    {"break", NULL, 0, cAquaDark | (fsBold << 24)},
    {"return", NULL, 0, cAquaDark | (fsBold << 24)},

    {"*", NULL, 0, cRed},
    {"{", NULL, 0, cRed},
    {"}", NULL, 0, cRed},
    {"(", NULL, 0, cRed},
    {")", NULL, 0, cRed},
    {"[", NULL, 0, cRed},
    {"]", NULL, 0, cRed},
    {"=", NULL, 0, cRed},
    {"<", NULL, 0, cRed},
    {">", NULL, 0, cRed},
    {".", NULL, 0, cRed},
    {",", NULL, 0, cRed},
    {":", NULL, 0, cRed},
    {";", NULL, 0, cRed},
    {"+", NULL, 0, cRed},
    {"-", NULL, 0, cRed},
    {"*", NULL, 0, cRed},
    {"/", NULL, 0, cRed},
    {"&", NULL, 0, cRed},
    {"|", NULL, 0, cRed},
    {"!", NULL, 0, cRed},
    {"^", NULL, 0, cRed},
    {"%", NULL, 0, cRed},
    {"~", NULL, 0, cRed},

    {"\"", "\"", shEnc, cAquaDark | (fsItalic << 24)},
    {"'", "'", shEnc, cAquaDark | (fsItalic << 24)},

    {NULL, NULL, 0, 0}
  };

const _SyntaxItem SyntaxItemsPascal [] =   // Syntax tabler for Pascal
  {
    {"//", NULL, shEOL, cGrayDark | (fsItalic << 24)},
    {"(*", "*)", shEnc, cGrayDark | (fsItalic << 24)},
    {"{", "}", shEnc, cGrayDark | (fsItalic << 24)},

    {"unit", NULL, 0, cGreenDark | (fsBold << 24)},
    {"interface", NULL, 0, cGreenDark | (fsBold << 24)},
    {"uses", NULL, 0, cGreenDark | (fsBold << 24)},
    {"implementation", NULL, 0, cGreenDark | (fsBold << 24)},

    {"class", NULL, 0, cBlue | (fsBold << 24)},
    {"public", NULL, 0, cBlue | (fsBold << 24)},
    {"private", NULL, 0, cBlue | (fsBold << 24)},
    {"protected", NULL, 0, cBlue | (fsBold << 24)},
    {"virtual", NULL, 0, cBlue | (fsBold << 24)},

    {"property", NULL, 0, cBlue | (fsBold << 24)},
    {"read", NULL, 0, cBlue | (fsBold << 24)},
    {"write", NULL, 0, cBlue | (fsBold << 24)},

    {"type", NULL, 0, cBlue | (fsBold << 24)},
    {"record", NULL, 0, cBlue | (fsBold << 24)},
    {"class", NULL, 0, cBlue | (fsBold << 24)},
    {"const", NULL, 0, cBlue | (fsBold << 24)},
    {"volatile", NULL, 0, cBlue | (fsBold << 24)},
    {"procedure", NULL, 0, cBlue | (fsBold << 24)},
    {"function", NULL, 0, cBlue | (fsBold << 24)},
    {"constructor", NULL, 0, cBlue | (fsBold << 24)},
    {"destructor", NULL, 0, cBlue | (fsBold << 24)},
    {"var", NULL, 0, cBlue | (fsBold << 24)},
    {"longint", NULL, 0, cBlue | (fsBold << 24)},
    {"integer", NULL, 0, cBlue | (fsBold << 24)},
    {"char", NULL, 0, cBlue | (fsBold << 24)},
    {"boolean", NULL, 0, cBlue | (fsBold << 24)},
    {"string", NULL, 0, cBlue | (fsBold << 24)},
    {"pointer", NULL, 0, cBlue | (fsBold << 24)},
    {"float", NULL, 0, cBlue | (fsBold << 24)},

    {"begin", NULL, 0, cBlue | (fsBold << 24)},
    {"end", NULL, 0, cBlue | (fsBold << 24)},

    {"if", NULL, 0, cAquaDark | (fsBold << 24)},
    {"else", NULL, 0, cAquaDark | (fsBold << 24)},
    {"while", NULL, 0, cAquaDark | (fsBold << 24)},
    {"do", NULL, 0, cAquaDark | (fsBold << 24)},
    {"switch", NULL, 0, cAquaDark | (fsBold << 24)},
    {"case", NULL, 0, cAquaDark | (fsBold << 24)},
    {"of", NULL, 0, cAquaDark | (fsBold << 24)},
    {"default", NULL, 0, cAquaDark | (fsBold << 24)},
    {"break", NULL, 0, cAquaDark | (fsBold << 24)},
    {"return", NULL, 0, cAquaDark | (fsBold << 24)},

    {"*", NULL, 0, cRed},
    {"(", NULL, 0, cRed},
    {")", NULL, 0, cRed},
    {"[", NULL, 0, cRed},
    {"]", NULL, 0, cRed},
    {"=", NULL, 0, cRed},
    {"<", NULL, 0, cRed},
    {">", NULL, 0, cRed},
    {".", NULL, 0, cRed},
    {",", NULL, 0, cRed},
    {":", NULL, 0, cRed},
    {";", NULL, 0, cRed},
    {"+", NULL, 0, cRed},
    {"-", NULL, 0, cRed},
    {"*", NULL, 0, cRed},
    {"/", NULL, 0, cRed},
    {"&", NULL, 0, cRed},
    {"|", NULL, 0, cRed},
    {"^", NULL, 0, cRed},
    {"%", NULL, 0, cRed},

    {"'", "'", shEnc, cAquaDark | (fsItalic << 24)},

    {NULL, NULL, 0, 0}
  };

const _SyntaxItem SyntaxItemsPython [] =   // Syntax table for Python
  {
    {"#", NULL, shEOL, cGrayDark | (fsItalic << 24)},
    {"/*", "*/", shEnc, cGrayDark | (fsItalic << 24)},

    {"False", NULL, 0, cBlue | (fsBold << 24)},
    {"True", NULL, 0, cBlue | (fsBold << 24)},
    {"and", NULL, 0, cBlue | (fsBold << 24)},
    {"as", NULL, 0, cBlue | (fsBold << 24)},
    {"assert", NULL, 0, cBlue | (fsBold << 24)},
    {"async", NULL, 0, cBlue | (fsBold << 24)},
    {"await", NULL, 0, cBlue | (fsBold << 24)},
    {"break", NULL, 0, cBlue | (fsBold << 24)},
    {"class", NULL, 0, cBlue | (fsBold << 24)},
    {"continue", NULL, 0, cBlue | (fsBold << 24)},
    {"def", NULL, 0, cBlue | (fsBold << 24)},
    {"del", NULL, 0, cBlue | (fsBold << 24)},
    {"elif", NULL, 0, cBlue | (fsBold << 24)},
    {"else", NULL, 0, cBlue | (fsBold << 24)},
    {"except", NULL, 0, cBlue | (fsBold << 24)},
    {"finally", NULL, 0, cBlue | (fsBold << 24)},
    {"for", NULL, 0, cBlue | (fsBold << 24)},
    {"from", NULL, 0, cBlue | (fsBold << 24)},
    {"global", NULL, 0, cBlue | (fsBold << 24)},
    {"if", NULL, 0, cBlue | (fsBold << 24)},
    {"import", NULL, 0, cBlue | (fsBold << 24)},
    {"in", NULL, 0, cBlue | (fsBold << 24)},
    {"is", NULL, 0, cBlue | (fsBold << 24)},
    {"lambda", NULL, 0, cBlue | (fsBold << 24)},
    {"nonlocal", NULL, 0, cBlue | (fsBold << 24)},
    {"not", NULL, 0, cBlue | (fsBold << 24)},
    {"or", NULL, 0, cBlue | (fsBold << 24)},
    {"pass", NULL, 0, cBlue | (fsBold << 24)},
    {"raise", NULL, 0, cBlue | (fsBold << 24)},
    {"return", NULL, 0, cBlue | (fsBold << 24)},
    {"try", NULL, 0, cBlue | (fsBold << 24)},
    {"while", NULL, 0, cBlue | (fsBold << 24)},
    {"with", NULL, 0, cBlue | (fsBold << 24)},
    {"yield   ", NULL, 0, cBlue | (fsBold << 24)},

    {"*", NULL, 0, cRed},
    {"{", NULL, 0, cRed},
    {"}", NULL, 0, cRed},
    {"(", NULL, 0, cRed},
    {")", NULL, 0, cRed},
    {"[", NULL, 0, cRed},
    {"]", NULL, 0, cRed},
    {"=", NULL, 0, cRed},
    {"<", NULL, 0, cRed},
    {">", NULL, 0, cRed},
    {".", NULL, 0, cRed},
    {",", NULL, 0, cRed},
    {":", NULL, 0, cRed},
    {";", NULL, 0, cRed},
    {"+", NULL, 0, cRed},
    {"-", NULL, 0, cRed},
    {"*", NULL, 0, cRed},
    {"/", NULL, 0, cRed},
    {"&", NULL, 0, cRed},
    {"|", NULL, 0, cRed},
    {"!", NULL, 0, cRed},
    {"^", NULL, 0, cRed},
    {"%", NULL, 0, cRed},
    {"~", NULL, 0, cRed},

    {"\"", "\"", shEnc, cAquaDark | (fsItalic << 24)},
    {"'", "'", shEnc, cAquaDark | (fsItalic << 24)},

    {NULL, NULL, 0, 0}
  };

const _SyntaxItem SyntaxItemsLog [] =   // Syntax table for the Log
  {
    {"Open\t", NULL, shSOL | shEOL, cGreenDark},
    {"Copy\t", NULL, shSOL | shEOL, cBlueDark},
    {"Move\t", NULL, shSOL | shEOL, cBlueDark | cRedDark},
    {"Delete\t", NULL, shSOL | shEOL, cRedDark},
    {"Written\t", NULL, shSOL | shEOL, cBrown},
    {"Execute\t", NULL, shSOL | shEOL, cRed},
    {NULL, NULL, 0, 0}
  };

const _SyntaxItem SyntaxItemsHTML [] =   // Syntax table for the HTML
  {
    {"<!doctype", NULL, 0, cBlue | (fsBold << 24)},
    {"<header", NULL, 0, cBlue | (fsBold << 24)},
    {"</header>", NULL, 0, cBlue | (fsBold << 24)},
    {"<html", NULL, 0, cBlue | (fsBold << 24)},
    {"</html>", NULL, 0, cBlue | (fsBold << 24)},
    {"<head", NULL, 0, cBlue | (fsBold << 24)},
    {"</head>", NULL, 0, cBlue | (fsBold << 24)},
    {"<title", NULL, 0, cBlue | (fsBold << 24)},
    {"</title>", NULL, 0, cBlue | (fsBold << 24)},
    {"<body", NULL, 0, cBlue | (fsBold << 24)},
    {"</body>", NULL, 0, cBlue | (fsBold << 24)},
    {"<style", NULL, 0, cBlue | (fsBold << 24)},
    {"</style", NULL, 0, cBlue | (fsBold << 24)},
    {"<script", NULL, 0, cBlue | (fsBold << 24)},
    {"</script", NULL, 0, cBlue | (fsBold << 24)},
    {"<main", NULL, 0, cBlue | (fsBold << 24)},
    {"</main", NULL, 0, cBlue | (fsBold << 24)},
    {"<table", NULL, 0, cBlue | (fsBold << 24)},
    {"</table", NULL, 0, cBlue | (fsBold << 24)},
    {"<code", NULL, 0, cBlue | (fsBold << 24)},
    {"</code", NULL, 0, cBlue | (fsBold << 24)},
    {"<pre", NULL, 0, cBlue | (fsBold << 24)},
    {"</pre", NULL, 0, cBlue | (fsBold << 24)},

    {"<meta", NULL, 0, cBlue | (fsBold << 24)},
    {"<link", NULL, 0, cBlue | (fsBold << 24)},
    {"<input", NULL, 0, cBlue | (fsBold << 24)},

    {"<p", NULL, 0, cBlue | (fsBold << 24)},
    {"</p>", NULL, 0, cBlue | (fsBold << 24)},
    {"<br", NULL, 0, cBlue | (fsBold << 24)},


    {"<h1", NULL, 0, cBlue | (fsBold << 24)},
    {"</h1>", NULL, 0, cBlue | (fsBold << 24)},
    {"<h2", NULL, 0, cBlue | (fsBold << 24)},
    {"</h2>", NULL, 0, cBlue | (fsBold << 24)},
    {"<h3", NULL, 0, cBlue | (fsBold << 24)},
    {"</h3>", NULL, 0, cBlue | (fsBold << 24)},
    {"<h4", NULL, 0, cBlue | (fsBold << 24)},
    {"</h4>", NULL, 0, cBlue | (fsBold << 24)},
    {"<h5", NULL, 0, cBlue | (fsBold << 24)},
    {"</h5>", NULL, 0, cBlue | (fsBold << 24)},
    {"<h6", NULL, 0, cBlue | (fsBold << 24)},
    {"</h6>", NULL, 0, cBlue | (fsBold << 24)},
    {"<ul", NULL, 0, cBlue | (fsBold << 24)},
    {"</ul>", NULL, 0, cBlue | (fsBold << 24)},
    {"<ol", NULL, 0, cBlue | (fsBold << 24)},
    {"</ol>", NULL, 0, cBlue | (fsBold << 24)},
    {"<li", NULL, 0, cBlue | (fsBold << 24)},
    {"</li>", NULL, 0, cBlue | (fsBold << 24)},
    {"<nav", NULL, 0, cBlue | (fsBold << 24)},
    {"</nav>", NULL, 0, cBlue | (fsBold << 24)},
    {"<label", NULL, 0, cBlue | (fsBold << 24)},
    {"</label>", NULL, 0, cBlue | (fsBold << 24)},
    {"<span", NULL, 0, cBlue | (fsBold << 24)},
    {"</span>", NULL, 0, cBlue | (fsBold << 24)},
    {"<svg", NULL, 0, cBlue | (fsBold << 24)},
    {"</svg>", NULL, 0, cBlue | (fsBold << 24)},
    {"<hr", NULL, 0, cBlue | (fsBold << 24)},
    {"</hr>", NULL, 0, cBlue | (fsBold << 24)},
    {"<tr", NULL, 0, cBlue | (fsBold << 24)},
    {"</tr>", NULL, 0, cBlue | (fsBold << 24)},
    {"<th", NULL, 0, cBlue | (fsBold << 24)},
    {"</th>", NULL, 0, cBlue | (fsBold << 24)},
    {"<td", NULL, 0, cBlue | (fsBold << 24)},
    {"</td>", NULL, 0, cBlue | (fsBold << 24)},

    {"<b>", NULL, 0, cBlue | (fsBold << 24)},
    {"</b>", NULL, 0, cBlue | (fsBold << 24)},
    {"<u>", NULL, 0, cBlue | (fsBold << 24)},
    {"</u>", NULL, 0, cBlue | (fsBold << 24)},
    {"<i>", NULL, 0, cBlue | (fsBold << 24)},
    {"</i>", NULL, 0, cBlue | (fsBold << 24)},
    {"<em>", NULL, 0, cBlue | (fsBold << 24)},
    {"</em>", NULL, 0, cBlue | (fsBold << 24)},
    {"<big>", NULL, 0, cBlue | (fsBold << 24)},
    {"</big>", NULL, 0, cBlue | (fsBold << 24)},
    {"<strong>", NULL, 0, cBlue | (fsBold << 24)},
    {"</strong>", NULL, 0, cBlue | (fsBold << 24)},

    {"<a", NULL, 0, cBlue | (fsBold << 24)},
    {"</a>", NULL, 0, cBlue | (fsBold << 24)},
    {"<div", NULL, 0, cBlue | (fsBold << 24)},
    {"</div>", NULL, 0, cBlue | (fsBold << 24)},
    {"<img", NULL, 0, cBlue | (fsBold << 24)},
    {"</img>", NULL, 0, cBlue | (fsBold << 24)},
    {"<form", NULL, 0, cBlue | (fsBold << 24)},
    {"</form>", NULL, 0, cBlue | (fsBold << 24)},
    {"<button", NULL, 0, cBlue | (fsBold << 24)},
    {"</button>", NULL, 0, cBlue | (fsBold << 24)},

    //{"href", NULL, 0, cBlue | (fsBold << 24)},
    //{"type", NULL, 0, cBlue | (fsBold << 24)},
    //{"rel", NULL, 0, cBlue | (fsBold << 24)},
    //{"class", NULL, 0, cBlue | (fsBold << 24)},
    //{"src", NULL, 0, cBlue | (fsBold << 24)},
    //{"alt", NULL, 0, cBlue | (fsBold << 24)},
    //{"title", NULL, 0, cBlue | (fsBold << 24)},
    //{"width", NULL, 0, cBlue | (fsBold << 24)},
    //{"height", NULL, 0, cBlue | (fsBold << 24)},
    //{"for", NULL, 0, cBlue | (fsBold << 24)},

    {"<!--", "-->", shEnc, cGrayDark | (fsItalic << 24)},

    //{"<", NULL, 0, cBlue | (fsBold << 24)},
    {">", NULL, 0, cBlue | (fsBold << 24)},

    {"=", NULL, 0, cRed | (fsBold << 24)},
    {":", NULL, 0, cRed | (fsBold << 24)},
    {"\"", "\"", shEnc, cGreenDark | (fsItalic << 24)},

    {NULL, NULL, 0, 0}
  };

const _SyntaxItem SyntaxItemsXML [] =   // Syntax table for XML
  {
    {"<!--", "-->", shEnc, cGrayDark | (fsItalic << 24)},

    {"<", NULL, 0, cBlue | (fsBold << 24)},
    {">", NULL, 0, cBlue | (fsBold << 24)},

    {"\"", "\"", shEnc, cGreenDark | (fsItalic << 24)},

    {NULL, NULL, 0, 0}
  };

const _SyntaxItem SyntaxItemsJSON [] =   // Syntax table for JSON
  {
    {"false", NULL, 0, cBlue | (fsBold << 24)},
    {"true", NULL, 0, cBlue | (fsBold << 24)},
    {"null", NULL, 0, cBlue | (fsBold << 24)},

    {"[", NULL, 0, cRed},
    {"]", NULL, 0, cRed},
    {"{", NULL, 0, cRed},
    {"}", NULL, 0, cRed},
    {",", NULL, 0, cRed},
    {":", NULL, 0, cRed},

    /*{"*", NULL, 0, cRed},
    {"(", NULL, 0, cRed},
    {")", NULL, 0, cRed},
    {"=", NULL, 0, cRed},
    {"<", NULL, 0, cRed},
    {">", NULL, 0, cRed},
    {".", NULL, 0, cRed},
    {";", NULL, 0, cRed},
    {"+", NULL, 0, cRed},
    {"-", NULL, 0, cRed},
    {"*", NULL, 0, cRed},
    {"/", NULL, 0, cRed},
    ?{"&", NULL, 0, cRed},
    ?{"|", NULL, 0, cRed},
    ?{"!", NULL, 0, cRed},
    ?{"^", NULL, 0, cRed},
    ?{"%", NULL, 0, cRed},
    ?{"~", NULL, 0, cRed},*/

    //{"\\\"", NULL, 0, 0},
    {"\"", "\"", shEnc, cGreenDark | (fsItalic << 24)},

    {NULL, NULL, 0, 0}
  };


//////////////////////////////////////////////////////////////////////
//
// SyntaxHighlight - Create an array of colours for a line of text
// Caller must free

void StepSpace (char *Line, int *Index)
  {
    while (Line [*Index] == ' ' || Line [*Index] == '\t')
      (*Index)++;
  }

bool Match (char a, char b, bool UpCase_)
  {
    if (UpCase_)
      return (UpCase (a) == UpCase (b));
    // not Upcase
    return (a == b);
  }

int *SyntaxHighlight (const _SyntaxItem *SyntaxItems, char *Line, int Len, const bool UpCase)
  {
    int *Res;
    int a;   // index through Line
    int b;   // index through Token
    int i;   // index through Items
    char *p;
    //
    Res = NULL;
    if (SyntaxItems)
      {
        // Create colour array for result
        Res = (int *) malloc (Len * sizeof (int));
        for (i = 0; i < Len; i++)   // initialized to -1 (default colour)
          Res [i] = 0x0;
        i = a = b = 0;
        while (true)
          {
            if (b == 0)   // end of "prev" token so ...
              while (Line [a] == ' ' || Line [a] == '\t')   // ... step space
                a++;
            if (SyntaxItems [i].Token [b] == 0)   // end of token
              {
                if (!IsAlpha (Line [a]) || !IsAlpha (Line [a + b]) || SyntaxItems [i].Token [b - 1] < 'A')   // word complete or symmbol match or word+?
                  {
                    if (SyntaxItems [i].Tags & shSOL)   // if start of line
                      a = 0;
                    if (SyntaxItems [i].Tags & shEOL)   // if end of line
                      b = Len - a;   // go to the end of line
                    else if (SyntaxItems [i].Tags & shEnc)   // if enclosed by TokenClose
                      {
                        p = &Line [a + b];
                        while (true)
                          {
                            p = StrPos (p, SyntaxItems [i].TokenClose, 0, Len - (p - Line));   // look for closing token
                            if (!p)
                              {
                                b = Len - a;   // assume match to end of line
                                break;
                              }
                            if (p [-1] != '\\')   // ignore sloshed tokens
                              {
                                b = p - &Line [a] + StrLen (SyntaxItems [i].TokenClose);
                                break;
                              }
                            p++;
                          }
                      }
                    while (b--)
                      Res [a++] = SyntaxItems [i].Colour;
                    i = 0;
                    b = 0;
                  }
              }
            if (a >= Len)
              break;
            if (a + b < Len && Match (Line [a + b], SyntaxItems [i].Token [b], UpCase))   // matching
              b++;
            else   // not a match => next item
              {
                b = 0;
                i++;
                if (SyntaxItems [i].Token == NULL || SyntaxItems [i].Token [0] == '\0')   // no more
                  {
                    i = 0;
                    if (IsAlpha (Line [a]))
                      while (IsAlpha (Line [a]))
                        a++;
                    else
                      a++;
                  }
                if (Line [a] == '\\')   // on slosh, skip past
                  if (Line [a + 1])
                    a += 2;
              }
          }
      }
    return Res;
  }

typedef struct
  {
    const char *Ext;
    const char *Name;
    const _SyntaxItem *Syntax;
    const bool UpCase;
    const int Colour;
  } _Syntax;

const _Syntax Syntax [] =
  {
    {"", "no syntax", NULL, false, cWhite},
    {"", "Hex", SyntaxItemsHex, false, 0xE0E0E0},
    {"", "Log", SyntaxItemsLog, false, 0xC0FFFF},
    {".html", "HTML", SyntaxItemsHTML, true, cOrange},
    {".xml", "XML Data", SyntaxItemsXML, true, cRedLight | cBlueLight},
    {".c", "C code", SyntaxItemsC, false, 0x9AEFFE},   // 0xFFC0C0
    {".h", "C header", SyntaxItemsC, false, 0xB0FFF0},
    {".pas", "Pascal", SyntaxItemsPascal, true, 0xFFFFC0},
    {".py", "Python", SyntaxItemsPython, true, 0xBE8B4B},
    {".json", "JSON", SyntaxItemsC, true, cRedLight | cBlueLight},
    {NULL, NULL, NULL},
  };

int SyntaxHighlightSelect (char *Filename, bool Hex, bool Log)
  {
    int SyntaxID, s;
    //
    if (Filename == NULL)
      DebugAdd ("Filename NULL"); //####
    SyntaxID = 0;
    if (Hex)
      SyntaxID = 1;
    else if (Log)
      SyntaxID = 2;
    else
      {
        Filename = StrFindFileExtension (Filename);
        s = 1;
        while (true)
          {
            if (Syntax [++s].Ext == NULL)
              break;
            if (Syntax [s].Ext [0])   // real extension (not a special case)
              if (StrPos (Filename, Syntax [s].Ext))
                {
                  SyntaxID = s;
                  break;
                }
          }
      }
    DebugAddInt ("SyntaxID", SyntaxID); // ####
    return SyntaxID;
  }
