//-----------------------------------------------------------------------
// <copyright file="Renderer.cpp" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//
// Module:
//      Renderer.cpp
//
// Description:
//
//-----------------------------------------------------------------------

#include "Common.h"

int InitColorTable(RGBQUAD rgb[], int bitCount);

// GDIPlus related variables and helper functions
const int RETURN_ERROR = -1;
#define ENCODER_BITMAP L"image/bmp"
const int MAX_GLANCE_DATA_LINES = 20;

// Define defaults for Renderer Capabilities static members
bool      CRenderer::m_fRendererCapabilitiesAreDefined = false;
LPCWSTR   CRenderer::m_pwszFontName = NULL;
DWORD     CRenderer::m_dwFontSizeInPixels = 0;          // char height. (not cell height)
DWORD     CRenderer::m_dwTitlebarHeight = 0;
DWORD     CRenderer::m_dwMenuItemHeight = 0;
DWORD     CRenderer::m_dwScrollAmountInPixels = 0;
// theme index param
DWORD     CRenderer::m_dwDefaultThemeIndex = 0;
// additional params
DWORD     CRenderer::m_dwFontSizeInPoint = 0;
COLORREF  CRenderer::m_rgbDefaultTextColor = 0;
COLORREF  CRenderer::m_rgbMenuActiveItemTextColor = 0;
//

// Define defaults for GDIPlus static members
void*     CRenderer::m_pGdiplusStartupInput = NULL;
ULONG_PTR CRenderer::m_gdiplusToken = NULL;
CLSID     CRenderer::m_encoderClsid = {0};

HRESULT SetDeviceParamDWORD(LPCWSTR pwszParamName, DWORD dwValue);

// Application Icons
//size_t CRenderer::m_nViewTopIcon = 0;

HRESULT CRenderer::SetRendererCaps(LPCWSTR pwszFontName,
                                    DWORD dwFontSizeInPixels,
                                    DWORD dwTitlebarHeight,
                                    DWORD dwMenuItemHeight,
                                    DWORD dwScrollAmountInPixels,
                                    DWORD dwDefaultThemeIndex)
{
    HRESULT hr = S_OK;

    if (m_fRendererCapabilitiesAreDefined)
    {
        ReleaseRendererCaps();
    }

    m_pwszFontName = _wcsdup(pwszFontName);

    m_dwFontSizeInPixels = dwFontSizeInPixels;
    m_dwTitlebarHeight = dwTitlebarHeight;
    m_dwMenuItemHeight = dwMenuItemHeight;
    m_dwScrollAmountInPixels = dwScrollAmountInPixels;
    m_dwDefaultThemeIndex = dwDefaultThemeIndex;
    
    m_fRendererCapabilitiesAreDefined = true;

    return hr;
}


HRESULT CRenderer::ReleaseRendererCaps(void)
{
    m_fRendererCapabilitiesAreDefined = false;

    SAFE_FREE(m_pwszFontName);
    
    return S_OK;
}

HRESULT CRenderer::GDIPlusInitialization(void)
{
    m_pGdiplusStartupInput = new(std::nothrow) GdiplusStartupInput;
    if (NULL == m_pGdiplusStartupInput)
    {
        return E_OUTOFMEMORY;
    }

    if (Ok == GdiplusStartup(&m_gdiplusToken, (GdiplusStartupInput*)m_pGdiplusStartupInput, NULL))
    {
        return SetGlobalEncoderClsid();
    }
    else
    {
        return E_FAIL;
    }
}


HRESULT CRenderer::GDIPlusShutdown(void)
{
    GdiplusShutdown(m_gdiplusToken);

    if (NULL != m_pGdiplusStartupInput)
    {
        delete m_pGdiplusStartupInput;
        m_pGdiplusStartupInput = NULL;
    }

    return S_OK;
}


HRESULT CRenderer::SetGlobalEncoderClsid(void)
{
    int nResult = GetEncoderClsid(ENCODER_BITMAP, &m_encoderClsid);
    if (RETURN_ERROR != nResult)
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}


int CRenderer::GetEncoderClsid(const wchar_t* format, CLSID* pClsid)
{
    UINT uiNumberOfEncoders = 0; // Number of image encoders
    UINT uiSizeOfEncodersArray = 0; // Size of the image encoder array in bytes
    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&uiNumberOfEncoders, &uiSizeOfEncodersArray);
    if (0 == uiSizeOfEncodersArray)
    {
        return RETURN_ERROR; // Failure
    }

    pImageCodecInfo = (ImageCodecInfo*)(malloc(uiSizeOfEncodersArray));
    if (NULL == pImageCodecInfo)
    {
        return RETURN_ERROR; // Failure
    }

    GetImageEncoders(uiNumberOfEncoders, uiSizeOfEncodersArray, pImageCodecInfo);

    for(UINT uiCounter = 0; uiCounter < uiNumberOfEncoders; uiCounter++)
    {
        if (0 == wcscmp(pImageCodecInfo[uiCounter].MimeType, format))
        {
            *pClsid = pImageCodecInfo[uiCounter].Clsid;
            free(pImageCodecInfo);
            return uiCounter; // Success
        }
    }

    free(pImageCodecInfo);
    return RETURN_ERROR; // Failure
}

HRESULT CRenderer::CreateRenderingContext()
{
    HRESULT hr = S_OK;

    DeleteRenderingContext();

    //
    // Creating Device Context
    //
    HDC hDisplayDC = CreateDC(L"DISPLAY", NULL, NULL, NULL);

    if (hDisplayDC)
    {
        m_hDC = CreateCompatibleDC(hDisplayDC);

        if (m_hDC == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        DeleteDC(hDisplayDC);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    
    if (SUCCEEDED(hr))
    {
        int cbColorTable = 0;
        
        //
        // allocating BITMAPINFO
        //
        switch(CDevice::GetDeviceBitDepth())
        {
            case 1:
            case 4:
            case 8:
                cbColorTable += ((int)(1 << CDevice::GetDeviceBitDepth()) * (int)sizeof(RGBQUAD));
                break;
        }
        
        m_cbBmpInfo += sizeof(BITMAPINFOHEADER) + cbColorTable;
        
        m_pBmpInfo = (BITMAPINFO *)new(std::nothrow) BYTE[m_cbBmpInfo];
        
        if (m_pBmpInfo == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // initializing BITMAPINFO and create BITMAP
        //
        ZeroMemory(m_pBmpInfo, m_cbBmpInfo);

        m_pBmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        m_pBmpInfo->bmiHeader.biWidth = CDevice::GetDeviceHorizontalResolution();
        m_pBmpInfo->bmiHeader.biHeight = CDevice::GetDeviceVerticalResolution();
        m_pBmpInfo->bmiHeader.biPlanes = 1;
        m_pBmpInfo->bmiHeader.biBitCount = (WORD)CDevice::GetDeviceBitDepth();
        m_pBmpInfo->bmiHeader.biSizeImage = ((CDevice::GetDeviceHorizontalResolution()
                                            * CDevice::GetDeviceBitDepth() + 31) / 32) * 4
                                            * CDevice::GetDeviceVerticalResolution();

        if (CDevice::IsTopDownBitmapRequired())
        {
            m_pBmpInfo->bmiHeader.biHeight *= -1;
        }

        switch(CDevice::GetDeviceBitDepth())
        {
            case 1:
            case 4:
            case 8:
                InitColorTable(m_pBmpInfo->bmiColors, CDevice::GetDeviceBitDepth());
                break;
        }

        m_hBitmap = CreateDIBSection(
                            m_hDC,
                            m_pBmpInfo,
                            DIB_RGB_COLORS,
                            (VOID **)&m_pBmpBits,
                            NULL,
                            0);
        if (m_hBitmap)
        {
            m_hBitmapOld = (HBITMAP)SelectObject(m_hDC, m_hBitmap);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    
    if (SUCCEEDED(hr))
    {
        // calculate FontSizeInPoint from FontSizeInPixels
        m_dwFontSizeInPoint = MulDiv(CRenderer::GetFontSizeInPixels(), 72, GetDeviceCaps(m_hDC, LOGPIXELSY));

        // creating font
        LOGFONT logFont;
        ZeroMemory(&logFont, sizeof(logFont));
        logFont.lfHeight = m_dwFontSizeInPixels * -1;
        logFont.lfWeight = FW_NORMAL;
        wcsncpy_s(logFont.lfFaceName, LF_FACESIZE, GetFontName(), wcslen(GetFontName()) + 1);

        m_hFont = CreateFontIndirect(&logFont);
        if (m_hFont)
        {
            m_hFontOld = (HFONT)SelectObject(m_hDC, m_hFont);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = LoadThemeResources(GetDefaultThemeIndex());
    }

    if (SUCCEEDED(hr))
    {
        
        // load string resource
// disabling Return value ignored warning
#pragma warning(push)
#pragma warning(disable: 6031)
        m_strNoGadgetMessage.LoadString(IDS_NO_GADGET_MESSAGE);
        m_strMenuItemHome.LoadString(IDS_MENU_ITEM_HOME);
        m_strMenuItemBack.LoadString(IDS_MENU_ITEM_BACK);
        m_strMenuItemSelectUser.LoadString(IDS_MENU_ITEM_SELECT_USER);
        m_strSelectUserTitle.LoadString(IDS_SELECT_USER_TITLE);
        m_strMenuItemConfigure.LoadString(IDS_MENU_ITEM_CONFIGURE);
        m_strMenuItemSelectTheme.LoadString(IDS_MENU_ITEM_SELECT_THEME);
        m_strMenuItemQuit.LoadString(IDS_MENU_ITEM_QUIT);
#pragma warning(pop)
    }

    return hr;
}

HRESULT CRenderer::LoadThemeResources(UINT32 ThemeIndex)
{
    HRESULT hr = S_OK;

    if (ThemeIndex < THEME_COUNT)
    {
        // load bitmap resource
        m_imgBG.LoadFromResource(_Module.GetResourceInstance(), BG_BMP + (THEME_OFFSET * ThemeIndex));
        m_imgHomeIconBG.LoadFromResource(_Module.GetResourceInstance(), HOME_ICON_BG_BMP + (THEME_OFFSET * ThemeIndex));
        m_imgHilightCenter.LoadFromResource(_Module.GetResourceInstance(), HILIGHT_CENTER_BMP  + (THEME_OFFSET * ThemeIndex));
        m_imgTitlebar.LoadFromResource(_Module.GetResourceInstance(), TITLEBAR_BMP  + (THEME_OFFSET * ThemeIndex));
        m_imgSelectedMenuItem.LoadFromResource(_Module.GetResourceInstance(), SELECTED_MENU_ITEM_BMP  + (THEME_OFFSET * ThemeIndex));
        m_imgMenuItem.LoadFromResource(_Module.GetResourceInstance(), MENU_ITEM_BMP  + (THEME_OFFSET * ThemeIndex));
        m_imgThickLine.LoadFromResource(_Module.GetResourceInstance(), THICK_LINE_BMP  + (THEME_OFFSET * ThemeIndex));
        m_imgPopupFrame.LoadFromResource(_Module.GetResourceInstance(), POPUP_FRAME_BMP  + (THEME_OFFSET * ThemeIndex));
        m_imgPopupTitlebar.LoadFromResource(_Module.GetResourceInstance(), POPUP_TITLEBAR_BMP + (THEME_OFFSET * ThemeIndex));
        m_imgScrollbar.LoadFromResource(_Module.GetResourceInstance(), SCROLLBAR_BMP + (THEME_OFFSET * ThemeIndex));
        m_imgStatusbar.LoadFromResource(_Module.GetResourceInstance(), STATUSBAR_BMP + (THEME_OFFSET * ThemeIndex));
        m_imgButton.LoadFromResource(_Module.GetResourceInstance(), BUTTON_BMP + (THEME_OFFSET * ThemeIndex));

        CAtlStringW szTextColor;
        DWORD rgb;

// disabling Return value ignored warning
#pragma warning(push)
#pragma warning(disable: 6031)
        szTextColor.LoadString(IDS_THEME_TEXT_COLOR_DEFAULT + (ThemeIndex * THEME_OFFSET));
        swscanf_s(szTextColor, L"%6x", &rgb);
#pragma warning(pop)
        rgb = ( ((rgb & 0x0000FF) << 16) | ((rgb & 0xFF0000) >> 16) | (rgb & 0x00FF00) ); // Swap red and blue bytes
        SetDefaultTextColor(rgb); 
        SetMenuActiveItemTextColor(rgb);
    }
    else
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        hr = SetDeviceParamDWORD(L"DefaultThemeIndex", ThemeIndex);
    }

    return hr;
}

VOID CRenderer::DeleteRenderingContext()
{
    if (m_hFont != NULL)
    {
        if (m_hDC && m_hFontOld)
        {
            SelectObject(m_hDC, m_hFontOld);
            m_hFontOld = NULL;
        }
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }

    if (m_hBitmap)
    {
        if (m_hDC && m_hBitmapOld)
        {
            SelectObject(m_hDC, m_hBitmapOld);
            m_hBitmapOld = NULL;
        }
        DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }
    
    if (m_hDC != NULL)
    {
        DeleteDC(m_hDC);
        m_hDC = NULL;
    }

    if (m_pBmpInfo)
    {
        delete [] ((BYTE *)m_pBmpInfo);
        m_pBmpInfo = NULL;
        m_cbBmpInfo = 0;
    }
    
    // new UI staff
    m_imgBG.Destroy();
    m_imgHomeIconBG.Destroy();
    m_imgHilightCenter.Destroy();
    m_imgTitlebar.Destroy();
    m_imgSelectedMenuItem.Destroy();
    m_imgMenuItem.Destroy();
    m_imgThickLine.Destroy();
    m_imgPopupFrame.Destroy();
    m_imgPopupTitlebar.Destroy();
    m_imgScrollbar.Destroy();
    m_imgStatusbar.Destroy();
    m_imgButton.Destroy();
}

VOID CRenderer::StartRendering()
{
    // resetting rendering context
    SetBkMode(m_hDC, TRANSPARENT);

    SelectDefaultFont();
}

VOID CRenderer::SelectDefaultFont()
{
    SelectObject(m_hDC, m_hFont);
    SetTextColor(m_hDC, CRenderer::GetDefaultTextColor());
}

VOID CRenderer::DrawBackground()
{
    if (!m_imgBG.IsNull())
    {
        //
        // background bitmap
        //
        m_imgBG.Draw(m_hDC,
            0,
            0,
            CDevice::GetDeviceHorizontalResolution(),
            CDevice::GetDeviceVerticalResolution());
    }
}

VOID CRenderer::DrawTitlebar()
{
    RECT rect;
    
    rect.left = 0;
    rect.top = 0;
    rect.right = CDevice::GetDeviceHorizontalResolution();
    rect.bottom = CRenderer::GetTitlebarHeight();
    
    if (!m_imgTitlebar.IsNull())
    {
        m_imgTitlebar.Draw(m_hDC, rect);
    }
#if 0
    else
    {
        COLORREF rgbTitlebarColor = CRenderer::GetTitlebarColor();

        if (rgbTitlebarColor != 0xffffffff)
        {
            HBRUSH hbr = CreateSolidBrush(rgbTitlebarColor);
            FillRect(m_hDC, &rect, hbr);
            DeleteObject(hbr);
        }
    }
#endif
}

VOID CRenderer::DrawMenuItemBG(const RECT& rect, BOOL bSelected)
{
    if (!m_imgMenuItem.IsNull())
    {
        m_imgMenuItem.Draw(m_hDC, rect);
    }

    if (bSelected)
    {
        if (!m_imgSelectedMenuItem.IsNull())
        {
            StretchFrameBlt(
                m_hDC,
                rect.left,
                rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                m_imgSelectedMenuItem,
                SELECTED_MENU_ITEM_FRAME_WIDTH,     // left frame width
                SELECTED_MENU_ITEM_FRAME_HEIGHT,    // top frame height
                SELECTED_MENU_ITEM_FRAME_WIDTH,     // right frame width
                SELECTED_MENU_ITEM_FRAME_HEIGHT     // bottom frame height
                );
        }
    }
}

VOID CRenderer::DrawContextMenuItemBG(const RECT& rect, BOOL bSelected)
{
#ifdef _THICK_FRAME_CONTEXT_MENU
    if (bSelected)
    {
        if (!m_imgSelectedMenuItem.IsNull())
        {
            StretchFrameBlt(
                m_hDC,
                rect.left,
                rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                m_imgSelectedMenuItem,
                SELECTED_MENU_ITEM_FRAME_WIDTH,     // left frame width
                SELECTED_MENU_ITEM_FRAME_HEIGHT,    // top frame height
                SELECTED_MENU_ITEM_FRAME_WIDTH,     // right frame width
                SELECTED_MENU_ITEM_FRAME_HEIGHT     // bottom frame height
                );
        }
    }
#else
    if (bSelected)
    {
        if (CDevice::GetDeviceBitDepth() != 1)
        {
            FillRect(m_hDC, &rect, (HBRUSH)COLOR_HIGHLIGHT+1);
        }
        else
        {
            FillRect(m_hDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
        }
    }
#endif
}

VOID CRenderer::DrawMenuDivLine(HDC hDC, const RECT& rect, BOOL bContextMenu)
{
#ifdef _THICK_FRAME_CONTEXT_MENU
    UNREFERENCED_PARAMETER(bContextMenu);
    
    if (!m_imgThickLine.IsNull())
#else
    if (!bContextMenu && !m_imgThickLine.IsNull())
#endif
    {
        m_imgThickLine.Draw(hDC, rect);
    }
    else
    {
        HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
        HGDIOBJ hPenOld = SelectObject(hDC, hPen);

        MoveToEx(hDC, rect.left, rect.top, NULL);
        LineTo(hDC, rect.right, rect.top);

        SelectObject(hDC, GetStockObject(WHITE_PEN));

        MoveToEx(hDC, rect.left, rect.top + 1, NULL);
        LineTo(hDC, rect.right, rect.top + 1);

        SelectObject(hDC, hPenOld);
        DeleteObject(hPen);
    }
}

VOID CRenderer::DrawScrollbar(HDC hDC, const RECT& rect, LONG nScrollPos, LONG nContentHeight)
{
    int nClientHeight = rect.bottom - rect.top;
    int nScrollbarHeight;
    int yScrollbar;

    nScrollbarHeight = (int)(nClientHeight * ((double)nClientHeight / nContentHeight));

    if (nScrollbarHeight < SCROLLBAR_MIN_HEIGHT)
    {
        nScrollbarHeight = SCROLLBAR_MIN_HEIGHT;
    }
    
    yScrollbar = (int)((nClientHeight - nScrollbarHeight) * ((double)nScrollPos / (nContentHeight - nClientHeight)));

#ifdef _THIN_SCROLLBAR
    if (!m_imgScrollbar.IsNull())
    {
        //
        // draw scrollbar using bitmap resource
        // this is a bitmap dependent code.
        //
        StretchFrameBlt(
            hDC,
            rect.left,
            rect.top + yScrollbar,
            SCROLLBAR_WIDTH,
            nScrollbarHeight,
            m_imgScrollbar,
            0,     // left frame width
            2,     // top frame height
            0,     // right frame width
            2,     // bottom frame height
            0xFF00FF);    // transparent color
    }
    else
#endif
    {
        //
        // draw default scrollbar
        //
        FillRect(hDC, &rect, (HBRUSH)(COLOR_SCROLLBAR + 1));
        
        RECT rectSB = rect;
        
        rectSB.top = rect.top + yScrollbar;
        rectSB.bottom = rectSB.top + nScrollbarHeight;
        
        Draw3dRect(hDC, rectSB);
    }
}


VOID CRenderer::DrawPopupFrame(HDC hDC, const RECT &rect)
{
    if (!m_imgPopupFrame.IsNull())
    {
        StretchFrameBlt(
            hDC,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            m_imgPopupFrame,
            POPUP_FRAME_WIDTH,
            POPUP_FRAME_HEIGHT,
            POPUP_FRAME_WIDTH,
            POPUP_FRAME_HEIGHT,
            RGB(0xFF, 0x00, 0xFF));
    }
}


VOID CRenderer::DrawStatusbar(HDC hDC, const RECT& rect)
{
    if (!m_imgStatusbar.IsNull())
    {
        StretchFrameBlt(
            hDC,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            m_imgStatusbar,
            0,
            1,      // don't stretch the top line.
            0,
            0);
    }
}


VOID CRenderer::DrawButton(HDC hDC, const RECT& rect, const LPCWSTR pwszText, BOOL bSelected)
{
    if (!m_imgButton.IsNull())
    {
        // this is a bitmap resource dependent code.
        StretchFrameBlt(
            hDC,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            m_imgButton,
            bSelected ? 28 : 0,
            0,
            28,
            28,
            3,
            3,
            3,
            3,
            RGB(0xFF,0,0xFF));
    }

    RECT rectText = rect;
    
    InflateRect(&rectText, -3, 0);

    DrawText(hDC, pwszText, -1, &rectText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}


//
// this is the rendering routine in the original WDK Windows SideShow sample with few changes.
//
VOID CRenderer::_DrawTitleAndGlanceText(LPCWSTR wszTitle, LPCWSTR wszBody, int top, int bottom)
{
    if (NULL == wszTitle ||
        NULL == wszBody)
    {
        return;
    }

    size_t SizeOfBody = wcslen(wszBody) + 1;

    //
    HFONT hFontBak = NULL;
    HFONT hFontTitle = NULL;
    HFONT hFontBody = NULL;

    LOGFONT logFont;
    TEXTMETRIC tm;
    ZeroMemory(&logFont, sizeof(logFont));

    logFont.lfHeight = GLANCE_TITLE_FONT_SIZE * -1;
    logFont.lfWeight = FW_NORMAL;
    wcsncpy_s(logFont.lfFaceName, LF_FACESIZE, GetFontName(), wcslen(GetFontName()) + 1);
    hFontTitle = CreateFontIndirect(&logFont);
    
    logFont.lfHeight = GLANCE_TEXT_FONT_SIZE * -1;
    hFontBody = CreateFontIndirect(&logFont);
    
    hFontBak = (HFONT)SelectObject(m_hDC, hFontTitle);

    GetTextMetrics(m_hDC, &tm);
    //
    
    RECT rectTitle = {GLANCE_TEXT_LEFT_MARGIN,
                      top + GLANCE_TEXT_TOP_MARGIN - tm.tmInternalLeading,
                      CDevice::GetDeviceHorizontalResolution() - GLANCE_TEXT_RIGHT_MARGIN,
                      top + GLANCE_TEXT_TOP_MARGIN + tm.tmHeight};

    DrawText(m_hDC, // If return is 0, this function failed
             wszTitle,
             -1,
             &rectTitle,
             DT_NOPREFIX | DT_NOCLIP | DT_SINGLELINE | DT_TOP | DT_WORD_ELLIPSIS);

    if (0 < SizeOfBody) // Only try to display the body if it is greater than 0 characters
    {
        // Split wszBody into more lines if there is an embedded newline (specifically, a 0x000A) in the string
        const wchar_t wchNewline = 0x000A;
        int nBeginningOfLineIndex = 0;
        size_t SizeOfBody_Line[MAX_GLANCE_DATA_LINES] = {SizeOfBody, 0};
        int nNumberOfRenderedBodyLines = 1;
        
        for (size_t nIndex = 0; nIndex < SizeOfBody; nIndex++) // nIndex is the current cursor position in the string
        {
            // This if statement checks if we've hit the end of a line
            if ((wchNewline == wszBody[nIndex]) || // If the current character is a newline...
                (L'\0' == wszBody[nIndex]) || // or if the current character is a terminating NULL...
                (SizeOfBody <= nIndex + 1)) // or if we've hit the end of the buffer...
            {
                SizeOfBody_Line[nNumberOfRenderedBodyLines - 1] = nIndex + 1 - nBeginningOfLineIndex; // ...then set the size of the current line
                if ((SizeOfBody <= nIndex + 1) || // If this is the end of the buffer
                    (L'\0' == wszBody[nIndex + 1]) || // or if the next character is a terminating NULL
                    (MAX_GLANCE_DATA_LINES <= nNumberOfRenderedBodyLines)) // or if we've hit the maximum number of renderable lines
                {
                    break; // Exit the loop. nNumberOfRenderedLines is set correctly
                }
                // Else there is at least one more line

                nNumberOfRenderedBodyLines++;
                nBeginningOfLineIndex = (DWORD)nIndex + 1; // set the 'beginning of the line' index
            }
        }

        nBeginningOfLineIndex = 0;
//        int nY_Offset = top + 3 + m_dwLineHeightInPixels;
        int nY_Offset = rectTitle.bottom + GLANCE_TEXT_LINE_HEIGHT - GLANCE_TEXT_FONT_SIZE;
        SelectObject(m_hDC, hFontBody);
        for (int nLine = 0; nLine < nNumberOfRenderedBodyLines; nLine++)
        {
            // Display wszBody Line x if it exists

            RECT rectBody = {GLANCE_TEXT_LEFT_MARGIN,
                             nY_Offset,
                             CDevice::GetDeviceHorizontalResolution() - GLANCE_TEXT_RIGHT_MARGIN,
                             nY_Offset + GLANCE_TEXT_LINE_HEIGHT};
            //
            if (rectBody.bottom > bottom)
            {
                break;
            }
            //
            int SizeOfCurrentBodyLine = (((int)SizeOfBody_Line[nLine] - 1) < (int)SizeOfBody_Line[nLine]) ? ((int)SizeOfBody_Line[nLine] - 1) : (0);
            DrawText(m_hDC, // If return is 0, this function failed
                     wszBody + nBeginningOfLineIndex,
                     SizeOfCurrentBodyLine,
                     &rectBody,
                     DT_NOPREFIX | DT_NOCLIP | DT_SINGLELINE | DT_TOP | DT_WORD_ELLIPSIS);
            nBeginningOfLineIndex += (int)SizeOfBody_Line[nLine];
            nY_Offset += GLANCE_TEXT_LINE_HEIGHT;
        }
    }

    //
    SelectObject(m_hDC, hFontBak);
    DeleteObject(hFontTitle);
    DeleteObject(hFontBody);
    //
}

HRESULT CRenderer::DrawNoGadgetMessage()
{
    _DrawTitleAndGlanceText(
        CDevice::GetDeviceFriendlyName(),
        m_strNoGadgetMessage,
        0,
        CDevice::GetDeviceVerticalResolution());    

    return S_OK;
}

HRESULT CRenderer::DrawHomeScreen()
{
    size_t i;
    int xIconPos = (CDevice::GetDeviceHorizontalResolution() - HOME_ICON_SIZE) / 2;
    int yIconPos = (HOME_ICON_AREA_HEIGHT - HOME_ICON_SIZE) / 2;
    int xSpace = HOME_ICON_SPACE;

    if (!m_imgHomeIconBG.IsNull())
    {
        m_imgHomeIconBG.Draw(m_hDC,
            0,
            0,
            CDevice::GetDeviceHorizontalResolution(),
            m_imgHomeIconBG.GetHeight());
    }
    
    for(i = g_pDataManager->GetCurrentApplicationIndex(); i > 0 && xIconPos > 0; i--)
    {
        xIconPos -= (HOME_ICON_SIZE + xSpace);
    }

    for(; i < g_pDataManager->GetApplicationCount(); i++)
    {
        CNodeBase *pNode = g_pDataManager->GetApplicationByIndex(i);

        if (i == g_pDataManager->GetCurrentApplicationIndex())
        {
            //
            // Draw Selected Square
            //
            RECT rect;
            
            rect.left = xIconPos - (HOME_ICON_SELECTED_SIZE - HOME_ICON_SIZE) / 2;
            rect.top = yIconPos - (HOME_ICON_SELECTED_SIZE - HOME_ICON_SIZE) / 2;
            rect.right = rect.left + HOME_ICON_SELECTED_SIZE;
            rect.bottom = rect.top + HOME_ICON_SELECTED_SIZE;
            
            if (!m_imgHilightCenter.IsNull())
            {
                m_imgHilightCenter.Draw(m_hDC, rect);
            }
#if 0
            else
            {
                HBRUSH hBrush = CreateSolidBrush(CRenderer::GetMenuActiveItemColor());
                FillRect(m_hDC, &rect, hBrush);
                DeleteObject(hBrush);
            }
#endif

            //
            // Draw Glance Text
            //
            _DrawTitleAndGlanceText(
                pNode->GetTitle(),
                pNode->GetBody(),
                HOME_ICON_AREA_HEIGHT,
                CDevice::GetDeviceVerticalResolution());
        }
        
        pNode->GetIcon()->Draw(m_hDC, xIconPos, yIconPos);

        xIconPos += HOME_ICON_SIZE + xSpace;
    }

    return S_OK;
}

//
// Draw Popup Notification
//
HRESULT CRenderer::DrawNotification(CNodeNotification* pNotificationNode)
{
    RECT rect;
    RECT rectFrame;

    SelectObject(m_hDC, m_hFont);   // making sure that the default font is selected.
    
    //
    // notification frame rect
    //
    rectFrame.left = CONTENT_LEFT_MARGIN;
    rectFrame.top = CRenderer::GetTitlebarHeight() / 2;
    rectFrame.right = CDevice::GetDeviceHorizontalResolution() - CONTENT_RIGHT_MARGIN;
    rectFrame.bottom = CDevice::GetDeviceVerticalResolution() - CRenderer::GetTitlebarHeight() / 2;
    
    //
    // notification content rect
    //
    rect.left = rectFrame.left + POPUP_FRAME_WIDTH;
    rect.top = rectFrame.top + POPUP_TITLEBAR_HEIGHT + NOTIFICATION_Y_MARGIN;
    rect.right = rectFrame.right - POPUP_FRAME_WIDTH;
    rect.bottom = rectFrame.bottom - POPUP_FRAME_HEIGHT - NOTIFICATION_Y_MARGIN;

    //
    // Initialize Notification View if it's not already.
    //
    if (!pNotificationNode->m_bViewInitialized)
    {
        pNotificationNode->InitializeView(m_hDC, rect);
    }
    
    // adjusting popup window height
    if (pNotificationNode->m_view.GetContentHeight() < (rect.bottom - rect.top))
    {
        rect.top += ((rect.bottom - rect.top) - pNotificationNode->m_view.GetContentHeight()) / 2;
        rect.bottom = rect.top + pNotificationNode->m_view.GetContentHeight();

        rectFrame.top = rect.top - POPUP_TITLEBAR_HEIGHT - NOTIFICATION_Y_MARGIN;
        rectFrame.bottom = rect.bottom + POPUP_FRAME_HEIGHT + NOTIFICATION_Y_MARGIN;
    }
    
    // draw popup frame
    DrawPopupFrame(m_hDC, rectFrame);

    // draw popup titlebar
    StretchFrameBlt(
        m_hDC,
        rectFrame.left,
        rectFrame.top,
        rectFrame.right - rectFrame.left,
        POPUP_TITLEBAR_HEIGHT,
        m_imgPopupTitlebar,
        POPUP_FRAME_WIDTH,
        0,
        POPUP_FRAME_WIDTH,
        0,
        RGB(0xFF, 0x00, 0xFF));
    
    //
    // Draw Application Icon and Title
    //
    CNodeApplication* pAppNode = g_pDataManager->GetApplicationById(pNotificationNode->m_guidApplicationId);

    if (pAppNode)
    {
        RECT rectTitle;

        rectTitle.top = rectFrame.top;
        rectTitle.bottom = rectFrame.top + POPUP_TITLEBAR_HEIGHT;
        rectTitle.left = rect.left;
        rectTitle.right = rect.right;
        
        SIZE size = pAppNode->GetMiniIcon()->Draw(
                            m_hDC,
                            rectTitle.left,
                            rectTitle.top + (POPUP_TITLEBAR_HEIGHT - MINI_ICON_SIZE) / 2);

        if (size.cx)
        {
            rectTitle.left += (size.cx + 6);
        }
        
        DrawText(m_hDC, pAppNode->GetTitle(), -1, &rectTitle, DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_WORD_ELLIPSIS | DT_SINGLELINE);
    }

    //
    // Draw Notification Image
    //
    pNotificationNode->GetIcon()->AlphaBlend(
        m_hDC,
        rect.right - pNotificationNode->GetIcon()->GetWidth() - SCROLLBAR_WIDTH,
        rect.top,
        0xAF);

    //
    // Draw Notification Content
    //
    pNotificationNode->m_view.OnPaint(m_hDC, rect.left, rect.top, rect.right, rect.bottom);

    return S_OK;
}

HRESULT CRenderer::CreatePackedDIB(IStream *pStream)
{
    HRESULT hr = S_OK;

    if (!m_hBitmap)
    {
        return E_FAIL;
    }
    
    //
    // Write BITMAPFILEHEADER
    //
    BITMAPFILEHEADER bmfh;

    ZeroMemory(&bmfh, sizeof(bmfh));
    bmfh.bfType    = 'B' + 'M' * 0x100;
    bmfh.bfSize    = sizeof(bmfh) + m_cbBmpInfo + m_pBmpInfo->bmiHeader.biSizeImage;
    bmfh.bfOffBits = sizeof(bmfh) + m_cbBmpInfo;

    hr = pStream->Write(&bmfh, sizeof(bmfh), NULL);

    if (SUCCEEDED(hr))
    {
        //
        // Write BITMAPINFO (BITMAPINFOHEADER and ColorTable)
        //
        hr = pStream->Write(m_pBmpInfo, m_cbBmpInfo, NULL);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Write Bitmap Bits
        //
        hr = pStream->Write(m_pBmpBits, m_pBmpInfo->bmiHeader.biSizeImage, NULL);
    }
    
    return hr;
}

VOID Draw3dRect(HDC hdc, const RECT& rect)
{
    HPEN hpen;
    HGDIOBJ hpenOld;
    RECT rc = rect;

    FillRect(hdc, &rect, (HBRUSH)(COLOR_3DFACE + 1));

    hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT));
    hpenOld = SelectObject(hdc, hpen);
    ::MoveToEx(hdc, rc.right-1, rc.top, NULL);
    ::LineTo(hdc, rc.left, rc.top);
    ::LineTo(hdc, rc.left, rc.bottom-1);
    SelectObject(hdc, hpenOld);
    DeleteObject(hpen);

    hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
    hpenOld = SelectObject(hdc, hpen);
    ::MoveToEx(hdc, rc.left, rc.bottom-1, NULL);
    ::LineTo(hdc, rc.right-1, rc.bottom-1);
    ::LineTo(hdc, rc.right-1, rc.top);
    SelectObject(hdc, hpenOld);
    DeleteObject(hpen);

    InflateRect(&rc, -1, -1);

    hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHIGHLIGHT));
    hpenOld = SelectObject(hdc, hpen);
    ::MoveToEx(hdc, rc.right-1, rc.top, NULL);
    ::LineTo(hdc, rc.left, rc.top);
    ::LineTo(hdc, rc.left, rc.bottom-1);
    SelectObject(hdc, hpenOld);
    DeleteObject(hpen);

    hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
    hpenOld = SelectObject(hdc, hpen);
    ::MoveToEx(hdc, rc.left, rc.bottom-1, NULL);
    ::LineTo(hdc, rc.right-1, rc.bottom-1);
    ::LineTo(hdc, rc.right-1, rc.top);
    SelectObject(hdc, hpenOld);
    DeleteObject(hpen);
}

#define SET_RGBQUAD(a, b) do { *((DWORD *)&a) = b; } while(0)

int InitColorTable(RGBQUAD rgb[], int bitCount)
{
    int nClrUsed = 0;
    
    if (bitCount == 1)
    {
        SET_RGBQUAD(rgb[0], 0x000000);
        SET_RGBQUAD(rgb[1], 0xFFFFFF);
        nClrUsed = 2;
    }
    else if (bitCount == 4)
    {
        SET_RGBQUAD(rgb[0], 0x000000);
        SET_RGBQUAD(rgb[1], 0x800000);
        SET_RGBQUAD(rgb[2], 0x008000);
        SET_RGBQUAD(rgb[3], 0x808000);
        SET_RGBQUAD(rgb[4], 0x000080);
        SET_RGBQUAD(rgb[5], 0x800080);
        SET_RGBQUAD(rgb[6], 0x008080);
        SET_RGBQUAD(rgb[7], 0x808080);
        SET_RGBQUAD(rgb[8], 0xC0C0C0);
        SET_RGBQUAD(rgb[9], 0xFF0000);
        SET_RGBQUAD(rgb[10], 0x00FF00);
        SET_RGBQUAD(rgb[11], 0xFFFF00);
        SET_RGBQUAD(rgb[12], 0x0000FF);
        SET_RGBQUAD(rgb[13], 0xFF00FF);
        SET_RGBQUAD(rgb[14], 0x00FFFF);
        SET_RGBQUAD(rgb[15], 0xFFFFFF);
        nClrUsed = 16;
    }
    else if (bitCount == 8)
    {
        // standard web palette
        for(int r = 0; r <= 0xFF; r += 0x33)
        {
            for(int g = 0; g <= 0xFF; g += 0x33)
            {
                for(int b = 0; b <= 0xFF; b += 0x33)
                {
                    rgb[nClrUsed].rgbBlue  = (BYTE)b;
                    rgb[nClrUsed].rgbGreen = (BYTE)g;
                    rgb[nClrUsed].rgbRed   = (BYTE)r;
                    nClrUsed++;
                }
            }
        }
    }
    
    return nClrUsed;
}

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
    COLORREF rgbTransparent)
{
    StretchFrameBlt(
        hdcDest,
        xDest,
        yDest,
        nDestWidth,
        nDestHeight,
        imgSrc,
        0,
        0,
        imgSrc.GetWidth(),
        imgSrc.GetHeight(),
        nLeftFrameWidth,
        nTopFrameHeight,
        nRightFrameWidth,
        nBottomFrameHeight,
        rgbTransparent);
}

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
    COLORREF rgbTransparent)
{
    // left/top corner
    if (nLeftFrameWidth && nTopFrameHeight)
    {
        imgSrc.TransparentBlt(
            hdcDest,
            xDest,                                  // xDest
            yDest,                                  // yDest
            nLeftFrameWidth,                        // nDestWidth
            nTopFrameHeight,                        // nDestHeight
            xSrc,                                   // xSrc
            ySrc,                                   // ySrc
            nLeftFrameWidth,                        // nSrcWidth
            nTopFrameHeight,                        // nSrcHeigh
            rgbTransparent);
    }

    // right/top corner
    if (nRightFrameWidth && nTopFrameHeight)
    {
        imgSrc.TransparentBlt(
            hdcDest,
            xDest + nDestWidth - nRightFrameWidth,  // xDest
            yDest,                                  // yDest
            nRightFrameWidth,                       // nDestWidth
            nTopFrameHeight,                        // nDestHeight
            xSrc + nSrcWidth - nRightFrameWidth,    // xSrc
            ySrc,                                   // ySrc
            nLeftFrameWidth,                        // nSrcWidth
            nTopFrameHeight,                        // nSrcHeigh
            rgbTransparent);
    }
    
    // left/bottom corner
    if (nRightFrameWidth && nBottomFrameHeight)
    {
        imgSrc.TransparentBlt(
            hdcDest,
            xDest,                                      // xDest
            yDest + nDestHeight - nBottomFrameHeight,   // yDest
            nRightFrameWidth,                           // nDestWidth
            nBottomFrameHeight,                         // nDestHeight
            0,                                          // xSrc
            ySrc + nSrcHeight - nBottomFrameHeight,     // ySrc
            nRightFrameWidth,                           // nSrcWidth
            nBottomFrameHeight,                         // nSrcHeigh
            rgbTransparent);
    }
    
    // right/bottom corner
    if (nRightFrameWidth && nBottomFrameHeight)
    {
        imgSrc.TransparentBlt(
            hdcDest,
            xDest + nDestWidth - nRightFrameWidth,      // xDest
            yDest + nDestHeight - nBottomFrameHeight,   // yDest
            nRightFrameWidth,                           // nDestWidth
            nBottomFrameHeight,                         // nDestHeight
            xSrc + nSrcWidth - nRightFrameWidth,        // xSrc
            ySrc + nSrcHeight - nBottomFrameHeight,     // ySrc
            nRightFrameWidth,                           // nSrcWidth
            nBottomFrameHeight,                         // nSrcHeigh
            rgbTransparent);
    }
    
    // left frame
    if (nLeftFrameWidth)
    {
        imgSrc.TransparentBlt(
            hdcDest,
            xDest,                                                      // xDest
            yDest + nTopFrameHeight,                                    // yDest
            nLeftFrameWidth,                                            // nDestWidth
            nDestHeight - nTopFrameHeight - nBottomFrameHeight,         // nDestHeight
            xSrc,                                                       // xSrc
            ySrc + nTopFrameHeight,                                     // ySrc
            nLeftFrameWidth,                                            // nSrcWidth
            nSrcHeight - nTopFrameHeight - nBottomFrameHeight,          // nSrcHeigh
            rgbTransparent);
    }
    
    // top frame
    if (nTopFrameHeight)
    {
        imgSrc.TransparentBlt(
            hdcDest,
            xDest + nLeftFrameWidth,                                    // xDest
            yDest,                                                      // yDest
            nDestWidth - nLeftFrameWidth - nRightFrameWidth,            // nDestWidth
            nTopFrameHeight,                                            // nDestHeight
            xSrc + nLeftFrameWidth,                                     // xSrc
            ySrc,                                                       // ySrc
            nSrcWidth - nLeftFrameWidth - nRightFrameWidth,             // nSrcWidth
            nTopFrameHeight,                                            // nSrcHeigh
            rgbTransparent);
    }
    
    // right frame
    if (nRightFrameWidth)
    {
        imgSrc.TransparentBlt(
            hdcDest,
            xDest + nDestWidth - nRightFrameWidth,                      // xDest
            yDest + nTopFrameHeight,                                    // yDest
            nRightFrameWidth,                                           // nDestWidth
            nDestHeight - nTopFrameHeight - nBottomFrameHeight,         // nDestHeight
            xSrc + nSrcWidth - nRightFrameWidth,                        // xSrc
            ySrc + nTopFrameHeight,                                     // ySrc
            nRightFrameWidth,                                           // nSrcWidth
            nSrcHeight - nTopFrameHeight - nBottomFrameHeight,          // nSrcHeigh
            rgbTransparent);
    }

    // bottom frame
    if (nBottomFrameHeight)
    {
        imgSrc.TransparentBlt(
            hdcDest,
            xDest + nLeftFrameWidth,                                // xDest
            yDest + nDestHeight - nBottomFrameHeight,               // yDest
            nDestWidth - nLeftFrameWidth - nRightFrameWidth,        // nDestWidth
            nBottomFrameHeight,                                     // nDestHeight
            xSrc + nLeftFrameWidth,                                 // xSrc
            ySrc + nSrcHeight - nBottomFrameHeight,                 // ySrc
            nSrcWidth - nLeftFrameWidth - nRightFrameWidth,         // nSrcWidth
            nBottomFrameHeight,                                     // nSrcHeigh
            rgbTransparent);
    }

    // body
    imgSrc.TransparentBlt(
        hdcDest,
        xDest + nLeftFrameWidth,                                     // xDest
        yDest + nTopFrameHeight,                                     // yDest
        nDestWidth - nLeftFrameWidth - nRightFrameWidth,             // nDestWidth
        nDestHeight - nTopFrameHeight - nBottomFrameHeight,          // nDestHeight
        xSrc + nLeftFrameWidth,                                      // xSrc
        ySrc + nTopFrameHeight,                                      // ySrc
        nSrcWidth - nLeftFrameWidth - nRightFrameWidth,              // nSrcWidth
        nSrcHeight - nTopFrameHeight - nBottomFrameHeight,           // nSrcHeigh
        rgbTransparent);
}

