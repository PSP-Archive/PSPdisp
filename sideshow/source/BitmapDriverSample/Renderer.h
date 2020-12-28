//-----------------------------------------------------------------------
// <copyright file="Renderer.h" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//
// Module:
//      Renderer.h
//
// Description:
//      This class implements the bitmap rendering portion of the driver.
//
//-----------------------------------------------------------------------


#pragma once

// Icon Size
#define MINI_ICON_SIZE 0x10
#define SMALL_ICON_SIZE 0x20
#define LARGE_ICON_SIZE 0x30

//
// Home Screen Parameters
//
#define HOME_ICON_SIZE              SMALL_ICON_SIZE
#define HOME_ICON_AREA_HEIGHT       52
#define HOME_ICON_SPACE             32
#define HOME_ICON_SELECTED_SIZE     46

#define GLANCE_TEXT_TOP_MARGIN      12
#define GLANCE_TEXT_LEFT_MARGIN     16
#define GLANCE_TEXT_RIGHT_MARGIN    16
#define GLANCE_TEXT_BOTTOM_MARGIN   12
#define GLANCE_TITLE_FONT_SIZE      11
#define GLANCE_TEXT_FONT_SIZE       18
#define GLANCE_TEXT_LINE_HEIGHT     25

//
// Content Parameter
//
#define _THIN_SCROLLBAR

#ifdef _THIN_SCROLLBAR
#define SCROLLBAR_WIDTH 5
#define SCROLLBAR_MIN_HEIGHT 6
#else
#define SCROLLBAR_WIDTH 8
#define SCROLLBAR_MIN_HEIGHT 6
#endif

#define CONTENT_LEFT_MARGIN  12
#define CONTENT_RIGHT_MARGIN 12

#define SELECTED_MENU_ITEM_FRAME_WIDTH 4
#define SELECTED_MENU_ITEM_FRAME_HEIGHT 4

#define DIALOG_BUTTON_AREA_HEIGHT 45
#define DIALOG_BUTTON_HEIGHT 24
#define DIALOG_BUTTON_MIN_WIDTH 80

//
// Context Menu Parameter
//
#define _THICK_FRAME_CONTEXT_MENU

#ifndef _THINK_FRAME_CONTEXT_MENU
#define CONTEXT_MENU_ARROW_HEIGHT 5
#endif

//
// Notification Parameter
//
#define NOTIFICATION_MIN_SEC    5

#define POPUP_FRAME_WIDTH       7
#define POPUP_FRAME_HEIGHT      7
#define POPUP_TITLEBAR_HEIGHT   28
#define NOTIFICATION_Y_MARGIN   4

//
//
//

#define RENDER_FLAG_INITMENU      1
#define RENDER_FLAG_CONTENT       2
#define RENDER_FLAG_CONTEXTMENU   4
#define RENDER_FLAG_NOTIFICATION  8

class CNodeBase;
class CNodeApplication;
class CNodeNotification;
class CNodeDefaultBackground;
class CContent;

class CRenderer
{
public:
    CRenderer(void)
    {
        m_hDC = NULL;
        m_hBitmap = NULL;
        m_hBitmapOld = NULL;
        m_hFont = NULL;
        m_hFontOld = NULL;
        m_pBmpInfo = NULL;
        m_cbBmpInfo = 0;
        m_pBmpBits = NULL;
    }

    ~CRenderer(void)
    {
        DeleteRenderingContext();
    }

    // Sets the renderer capabilities and defines any renderer rules
    static HRESULT SetRendererCaps(LPCWSTR pwszFontName, // The name of the font used for rendering
                                   DWORD   dwFontSizeInPixels, // The size of the font used for rendering
                                   DWORD   dwTitlebarHeight, // The height of titlebar.
                                   DWORD   dwMenuItemHeight, // The height of menu item (1 line).
                                   DWORD   dwScrollAmountInPixels, // Amount of pixels to scroll each time the user presses the "more/less glance data" button
                                   DWORD   dwDefaultThemeIndex = 0); // Default theme index

    // Releases memory used by RendererCaps and resets all values to 0 / NULL
    static HRESULT ReleaseRendererCaps(void);

    // RendererCaps Accessors
    static bool    AreRendererCapabilitiesDefined(void) {return m_fRendererCapabilitiesAreDefined;}
    static LPCWSTR GetFontName(void) {return m_pwszFontName;}
    static DWORD   GetFontSizeInPixels(void) {return m_dwFontSizeInPixels;}
    static DWORD   GetTitlebarHeight(void) {return m_dwTitlebarHeight;}
    static DWORD   GetMenuItemHeight(void) {return m_dwMenuItemHeight;}
    static DWORD   GetScrollAmountInPixels(void) {return m_dwScrollAmountInPixels;}

    static DWORD   GetDefaultThemeIndex(void) {return m_dwDefaultThemeIndex;}

    // additional
    static DWORD   GetFontSizeInPoint(void) {return m_dwFontSizeInPoint;}
    
    static COLORREF GetDefaultTextColor() { return m_rgbDefaultTextColor; }
    static void     SetDefaultTextColor(COLORREF rgb) { m_rgbDefaultTextColor = rgb; }

    static COLORREF GetMenuActiveItemTextColor() { return m_rgbMenuActiveItemTextColor; }
    static void     SetMenuActiveItemTextColor(COLORREF rgbItemTextColor) { m_rgbMenuActiveItemTextColor = rgbItemTextColor; }
    //

    // GDIPlus Methods
    static HRESULT GDIPlusInitialization(void);
    static HRESULT GDIPlusShutdown(void);
    static HRESULT SetGlobalEncoderClsid(void);
    static int GetEncoderClsid(const wchar_t* format, CLSID* pClsid);

    // Rendering Functions
    VOID StartRendering();
    VOID DrawBackground();
    VOID DrawTitlebar();
    VOID DrawMenuItemBG(const RECT& rect, BOOL bSelected);
    VOID DrawContextMenuItemBG(const RECT& rect, BOOL bSelected);
    VOID DrawMenuDivLine(HDC hDC, const RECT& rect, BOOL bContextMenu);
    VOID DrawScrollbar(HDC hDC, const RECT& rect, LONG nScrollPos, LONG nContentHeight);
    VOID DrawPopupFrame(HDC hDC, const RECT& rect);
    VOID DrawStatusbar(HDC hDC, const RECT& rect);
    VOID DrawButton(HDC hDC, const RECT& rect, const LPCWSTR wszText, BOOL bSelected);
    VOID SelectDefaultFont();
    
    HRESULT DrawNoGadgetMessage();
    HRESULT DrawHomeScreen();
    HRESULT DrawNotification(CNodeNotification* pNodeToRender);
    HRESULT CreateRenderingContext();
    HRESULT LoadThemeResources(const UINT32 ThemeIndex);
    VOID DeleteRenderingContext();
    HRESULT CreatePackedDIB(IStream *pStream);
    HDC GetDC() { return m_hDC; }

    // String Resource Accessor
    LPCWSTR GetMenuItemHomeString() { return m_strMenuItemHome; }
    LPCWSTR GetMenuItemBackString() { return m_strMenuItemBack; }
    LPCWSTR GetMenuItemSelectUserString() { return m_strMenuItemSelectUser; }
    LPCWSTR GetSelectUserTitle() { return m_strSelectUserTitle; }
    LPCWSTR GetMenuItemConfigureString() { return m_strMenuItemConfigure; }
    LPCWSTR GetMenuItemSelectThemeString() { return m_strMenuItemSelectTheme; }
    LPCWSTR GetMenuItemQuitString() { return m_strMenuItemQuit; }
    
private:
    VOID _DrawTitleAndGlanceText(LPCWSTR wszTitle, LPCWSTR wszBody, int top, int bottom);
    VOID _DrawBackground(const CImage& image);

    // Renderer Capabilities Members
    static bool     m_fRendererCapabilitiesAreDefined;
    static LPCWSTR  m_pwszFontName;
    static DWORD    m_dwFontSizeInPixels;
    static DWORD    m_dwTitlebarHeight;
    static DWORD    m_dwMenuItemHeight;
    static DWORD    m_dwScrollAmountInPixels;

    static DWORD    m_dwDefaultThemeIndex;
    
    // additional parameters
    static DWORD    m_dwFontSizeInPoint;
    static COLORREF m_rgbDefaultTextColor;
    static COLORREF m_rgbMenuActiveItemTextColor;

    //
    
    // GDIPlus Members
    static void* m_pGdiplusStartupInput;
    static ULONG_PTR m_gdiplusToken;
    static CLSID m_encoderClsid;

    HDC m_hDC;              // main DC to render content to bitmap
    HBITMAP m_hBitmap;      // rendering target bitmap
    HBITMAP m_hBitmapOld;
    HFONT m_hFont;
    HFONT m_hFontOld;
    BITMAPINFO *m_pBmpInfo;
    UINT m_cbBmpInfo;
    BYTE *m_pBmpBits;

    // Bitmap Resource Staff
    CImage m_imgBG;
    CImage m_imgHomeIconBG;
    CImage m_imgHilightCenter;
    CImage m_imgTitlebar;
    CImage m_imgSelectedMenuItem;
    CImage m_imgMenuItem;
    CImage m_imgThickLine;
    CImage m_imgPopupTitlebar;
    CImage m_imgPopupFrame;
    CImage m_imgScrollbar;
    CImage m_imgStatusbar;
    CImage m_imgButton;

    // String Resources
    CAtlStringW m_strNoGadgetMessage;
    CAtlStringW m_strMenuItemHome;
    CAtlStringW m_strMenuItemBack;
    CAtlStringW m_strMenuItemSelectUser;
    CAtlStringW m_strSelectUserTitle;
    CAtlStringW m_strMenuItemConfigure;
    CAtlStringW m_strMenuItemSelectTheme;
    CAtlStringW m_strMenuItemQuit;
};

VOID Draw3dRect(HDC hdc, const RECT& rect);

VOID StretchFrameBlt(
    HDC hdcDest,
    LONG xDest,
    LONG yDest,
    LONG nDestWidth,
    LONG nDestHeight,
    const CImage& imgSrc,
    LONG xSrc,
    LONG ySrc,
    LONG nSrcWidth,
    LONG nSrcHeight,
    LONG nLeftFrameWidth,
    LONG nTopFrameHeight,
    LONG nRightFrameWidth,
    LONG nBottomFrameHeight,
    COLORREF rgbTransparent = RGB(0, 0, 0));

VOID StretchFrameBlt(
    HDC hdcDest,
    LONG xDest,
    LONG yDest,
    LONG nDestWidth,
    LONG nDestHeight,
    const CImage& imgSrc,
    LONG nLeftFrameWidth,
    LONG nTopFrameHeight,
    LONG nRightFrameWidth,
    LONG nBottomFrameHeight,
    COLORREF rgbTransparent = RGB(0, 0, 0));

