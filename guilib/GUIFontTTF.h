/*!
\file GUIFont.h
\brief 
*/

#ifndef CGUILIB_GUIFONTTTF_H
#define CGUILIB_GUIFONTTTF_H
#pragma once

// forward definition
struct FT_FaceRec_;
struct FT_LibraryRec_;

typedef struct FT_FaceRec_ *FT_Face;
typedef struct FT_LibraryRec_ *FT_Library;

// flags for alignment
#define XBFONT_LEFT       0x00000000
#define XBFONT_RIGHT      0x00000001
#define XBFONT_CENTER_X   0x00000002
#define XBFONT_CENTER_Y   0x00000004
#define XBFONT_TRUNCATED  0x00000008
#define XBFONT_JUSTIFIED  0x00000010

#define FONT_STYLE_NORMAL       0
#define FONT_STYLE_BOLD         1
#define FONT_STYLE_ITALICS      2
#define FONT_STYLE_BOLD_ITALICS 3

/*!
 \ingroup textures
 \brief 
 */
class CGUIFontTTF
{
  friend class CGUIFont;
  struct Character
  {
    short offsetX, offsetY;
    float left, top, right, bottom;
    float advance;
    WCHAR letter;
  };
public:

  CGUIFontTTF(const CStdString& strFileName);
  virtual ~CGUIFontTTF(void);

  void Clear();

  bool Load(const CStdString& strFilename, int height = 20, int style = FONT_STYLE_NORMAL, float aspect = 1.0f);

  void Begin();
  void End();

  const CStdString& GetFileName() const { return m_strFileName; };

protected:
  void AddReference();
  void RemoveReference();

  void GetTextExtentInternal(const WCHAR* strText, FLOAT* pWidth,
                             FLOAT* pHeight = NULL, BOOL bFirstLineOnly = FALSE);

  void DrawTextInternal(FLOAT fOriginX, FLOAT fOriginY, DWORD *pdw256ColorPalette, BYTE *pbColours,
                            const WCHAR* strText, DWORD cchText, DWORD dwFlags = 0,
                            FLOAT fMaxPixelWidth = 0.0f);

  int m_iHeight;
  int m_iStyle;
  CStdString m_strFilename;

  // Stuff for pre-rendering for speed
  inline Character *GetCharacter(WCHAR letter);
  bool CacheCharacter(WCHAR letter, Character *ch);
  inline void RenderCharacter(float posX, float posY, const Character *ch, D3DCOLOR dwColor);
  void ClearCharacterCache();

  LPDIRECT3DDEVICE8 m_pD3DDevice;

  LPDIRECT3DTEXTURE8 m_texture;      // texture that holds our rendered characters (8bit alpha only)
  unsigned int m_textureWidth;       // width of our texture
  unsigned int m_textureHeight;      // heigth of our texture
  int m_posX;                        // current position in the texture
  int m_posY;

  Character *m_char;                 // our characters
  Character *m_charquick[255];       // ascii chars here
  int m_maxChars;                    // size of character array (can be incremented)
  int m_numChars;                    // the current number of cached characters

  float m_ellipsesWidth;               // this is used every character (width of '.')

  unsigned int m_cellBaseLine;
  unsigned int m_cellHeight;
  unsigned int m_cellAscent;          // typical distance from baseline to top of glyph - based on the 'W'
  unsigned int m_lineHeight;

  DWORD m_dwNestedBeginCount;             // speedups

  // freetype stuff
  FT_Face    m_face;

  float m_originX;
  float m_originY;

  static int justification_word_weight;

  CStdString m_strFileName;
private:
  int m_referenceCount;
};

#endif
