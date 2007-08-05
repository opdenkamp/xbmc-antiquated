#include "include.h"
#include "GUILargeImage.h"
#include "TextureManager.h"
#include "../xbmc/GUILargeTextureManager.h"

CGUILargeImage::CGUILargeImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture)
    : CGUIImage(dwParentID, dwControlId, posX, posY, width, height, texture, 0)
{
  ControlType = GUICONTROL_LARGE_IMAGE;
}

CGUILargeImage::~CGUILargeImage(void)
{

}

void CGUILargeImage::AllocateOnDemand()
{
  // if we're hidden, we can free our resources and return
  if (!IsVisible() && m_visible != DELAYED)
  {
    if (m_bDynamicResourceAlloc && IsAllocated())
      FreeResources();
    m_bWasVisible = false;
    return;
  }

  // either visible or delayed - we need the resources allocated in either case
  if (!m_texturesAllocated || !m_vecTextures.size())
    AllocResources();
}

void CGUILargeImage::PreAllocResources()
{
  FreeResources();
  if (!m_image.diffuse.IsEmpty())
    g_TextureManager.PreLoad(m_image.diffuse);
}

void CGUILargeImage::AllocResources()
{
  if (m_strFileName.IsEmpty())
    return;
  if (m_vecTextures.size())
    FreeTextures();
  // don't call CGUIControl::AllocTextures(), as this resets m_hasRendered, which we don't want
  m_bInvalidated = true;
  m_bAllocated = true;

  // Calculate the final screen size for optimal quality
  LPDIRECT3DTEXTURE8 texture = g_largeTextureManager.GetImage(m_strFileName, m_iTextureWidth, m_iTextureHeight, !m_texturesAllocated);
  m_texturesAllocated = true;

  if (!texture)
    return;

  m_vecTextures.push_back(texture);

#ifdef HAS_XBOX_D3D
  m_linearTexture = true;
#else
  m_linearTexture = false;
#endif

  CalculateSize();

  LoadDiffuseImage();
}

void CGUILargeImage::FreeTextures()
{
  if (m_texturesAllocated)
    g_largeTextureManager.ReleaseImage(m_strFileName);

  if (m_diffuseTexture)
    g_TextureManager.ReleaseTexture(m_image.diffuse);
  m_diffuseTexture = NULL;
  m_diffusePalette = NULL;

  m_vecTextures.erase(m_vecTextures.begin(), m_vecTextures.end());
  m_iCurrentImage = 0;
  m_iCurrentLoop = 0;
  m_iImageWidth = 0;
  m_iImageHeight = 0;
  m_texturesAllocated = false;
}

