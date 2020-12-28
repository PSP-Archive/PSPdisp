//-----------------------------------------------------------------------
// <copyright file="DataManager.cpp" company="Microsoft">
//      Copyright (c) Microsoft Corporation. All rights reserved.
// </copyright>
//
// Module:
//      DataManager.cpp
//
// Description:
//
//-----------------------------------------------------------------------

/*
            +----------------------------------------------+ 
            |                                              |
            |          Select User Screen                  |
            |                                              |
            | m_pCurrentApplication = &m_BuiltinAppliation |
            | m_CurrentContentId == 10001                  |
            |                                              |
            +----------------------------------------------+ 
                      |                         ^
                      |                         |
                      |                         |
                      |                         |
                      |                         |
                      |                    +-------------+  m_pCurrentApplication = NULL
                      v                    | Select User |  m_CurrentContentId = 0
      +------------------------------+     +-------------+  ContextMenu = m_HomeScreenContextMenuContent
      |                              |          ^
      |         Home Screen          |          | context
      |                              | ---------+ menu
      | m_pCurrentApplication = NULL |
      | m_CurrentContentId = 0       |
      |                              |
      +------------------------------+ 
                  ^        |
                  |        |
       [Home/Back]|        | [OK]
                  |        |
                  |        V
      +-------------------------------------------------------------------------+
      |                                                                         |
      |                      Windows SideShow Gadget Content                    |
      |                                                                         |
      |  m_pCurrentApplication == m_ApplicationNodes[m_nApplicationMenuIndex]   |
      |  m_CurrentContentId != 0                                                |
      |                                                                         |
      +-------------------------------------------------------------------------+
*/

#include "Common.h"

#ifdef _ASSIGNED_USER_MODEL
#include <sddl.h>
#define CONTENT_ID_SELECT_USER 10001
#endif

#define CONTENT_ID_CONFIGURE 10002 
#define CONTENT_ID_SELECT_THEME 10003
#define CONTENT_ID_QUIT 10004

#define DEFAULT_CONTEXT_MENU_SCF "<body><menu id=\"0\" /></body>"

CDataManager::CDataManager(CWssBasicDDI *pDDI)
    : m_DefaultContextMenuContent(0)
    , m_HomeScreenContextMenuContent(0)
    , m_ThemeIndex(0)
    , m_BuiltinApplication(GUID_NULL)
{
    m_pDDI = pDDI;  // used for sending ss event and getting current assigned user.

    m_nApplicationMenuIndex = 0;

    m_pCurrentApplication = NULL;

    m_CurrentContentId = 0;
    m_bContextMenu = FALSE;
    m_ContextMenuId = 0;

    m_pImageSink = NULL;
}


CDataManager::~CDataManager(void)
{
    SAFE_RELEASE(m_pImageSink);

    m_timer.Uninitialize();

    _DeleteAllApplications();

    m_renderer.DeleteRenderingContext();

    DeleteCriticalSection(&m_CriticalSection);

#ifdef _ASSIGNED_USER_MODEL
    for (size_t i = 0; i < m_Users.size(); i++)
    {
        delete m_Users[i];
    }
    m_Users.clear();
#endif
}

HRESULT CDataManager::Initialize()
{
    HRESULT hr = S_OK;
    
    try
    {
        InitializeCriticalSection(&m_CriticalSection);
    }
    catch(...)
    {
        hr = E_FAIL;
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_renderer.CreateRenderingContext();
    }
    
    if (SUCCEEDED(hr))
    {
        hr= m_timer.Initialize();
    }
    
    if (SUCCEEDED(hr))
    {
        hr = CreateStreamOnHGlobal(NULL, TRUE, &m_pImageSink);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Initialize DefaultContextMenu which has only "Back" and "Home" Item.
        // DefaultContextMenu will be used when [Menu] button is pressed and context doesn't have context menu.
        //
        hr = m_DefaultContextMenuContent.SetData((BYTE *)DEFAULT_CONTEXT_MENU_SCF, sizeof(DEFAULT_CONTEXT_MENU_SCF));
    }

    if (SUCCEEDED(hr))
    {
        //
         // special context menu for home screen 
        //
        hr = _InitializeHomeScreenContextMenuContent();
    }

#ifdef _ASSIGNED_USER_MODEL
    if (SUCCEEDED(hr))
    {
        //
        // if there is no assigned user, go to select user screen.
        //
        if (m_pDDI->GetCurrentUserSID() == NULL && !IsValidSid(m_pDDI->GetCurrentUserSID()))
        {
            _CreateSelectUserContent(FALSE);
            
            m_pCurrentApplication = &m_BuiltinApplication;
            SetCurrentContentId(CONTENT_ID_SELECT_USER);
        }
    }
#endif

    //
    // if everything went well, send initial bitmap to the device.
    //
    if (SUCCEEDED(hr))
    {
        RenderAndSendDataToDevice();
    }
    
    return hr;
}

HRESULT CDataManager::AddApplication(GUID guidApplicationId,
                                    LPCWSTR wszTitle,
                                    const BYTE* pIcon,
                                    size_t cbIcon,
                                    const BYTE* pMiniIcon,
                                    size_t cbMiniIcon)
{
    EnterCriticalSection(&m_CriticalSection);

    HRESULT hr = S_OK;

    // 1. See if an existing node has this guidApplicationId.
    // 2. If yes, replace the data of that one with this one. If no, put this one at the end.
    const size_t nApplicationCount = m_ApplicationNodes.size();
    CNodeApplication* pApplication = NULL;
    BOOL bNew = FALSE;
    
    for (size_t index = 0; index < nApplicationCount; index++)
    {
        if (TRUE == IsEqualGUID(guidApplicationId, m_ApplicationNodes[index]->m_guidApplicationId))
        {
            pApplication = m_ApplicationNodes[index];
            break;
        }
    }
    
    if (!pApplication)
    {
        pApplication = new(std::nothrow) CNodeApplication(guidApplicationId);
        
        if (pApplication)
        {
            bNew = TRUE;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    if (pApplication)
    {
        hr = pApplication->SetTitle(wszTitle);
        
        if (SUCCEEDED(hr))
        {
            if (pIcon && cbIcon)
            {
                hr = pApplication->SetIcon(pIcon, cbIcon);
            }
        }

        if (SUCCEEDED(hr))
        {
            if (pMiniIcon && cbMiniIcon)
            {
                hr = pApplication->SetMiniIcon(pMiniIcon, cbMiniIcon);
            }
        }
    }
    
    if (bNew)
    {
        if (SUCCEEDED(hr))
        {
            try
            {
                m_ApplicationNodes.push_back(pApplication);
            }
            catch(...)
            {
                hr = E_FAIL;
            }
        }
        
        if (FAILED(hr))
        {
            delete pApplication;
        }
    }

    if (SUCCEEDED(hr))
    {
        //
        // render only when application selection menu is active
        //
        if (m_CurrentContentId == 0)
        {
            RenderAndSendDataToDevice();
        }
    }
    
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}


//
// Update Glance Text
//
HRESULT CDataManager::UpdateApplicationNode(GUID guidApplicationId, LPCWSTR wszBody)
{
    EnterCriticalSection(&m_CriticalSection);

    HRESULT hr = S_OK;

    const size_t QuantityOfApplicationNodes = m_ApplicationNodes.size();
    size_t index = 0;
    for ((index); index < QuantityOfApplicationNodes; index++)
    {
        if (TRUE == IsEqualGUID(guidApplicationId, m_ApplicationNodes.at(index)->m_guidApplicationId))
        {
            CNodeApplication* ExistingApplicationNode = m_ApplicationNodes[index];
            hr = ExistingApplicationNode->SetBody(wszBody);
            break;
        }
    }
    if (QuantityOfApplicationNodes == index)
    {
        hr = REGDB_E_CLASSNOTREG;
    }

    if (SUCCEEDED(hr))
    {
        //
        // render only when application selection menu is active
        //
        if (m_CurrentContentId == 0 && index == m_nApplicationMenuIndex)
        {
            RenderAndSendDataToDevice();
        }
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

// Delete an application (node) from the ApplicationNode vector list
// Return: REGDB_E_CLASSNOTREG: The ApplicationID guid doesn't exist and there were no notifications using this ApplicationID guid
// Return: S_OK: The application was deleted
HRESULT CDataManager::DeleteApplication(GUID guidApplicationId)
{
    EnterCriticalSection(&m_CriticalSection);

    HRESULT hr = S_OK;
    BOOL bRenderForNotification = FALSE;
    
    HRESULT hrNotifications = DeleteAllNotificationsForAnApplication(guidApplicationId, &bRenderForNotification);

    const size_t QuantityOfApplicationNodes = m_ApplicationNodes.size();
    size_t index = 0;
    for ((index); index < QuantityOfApplicationNodes; index++)
    {
        if (TRUE == IsEqualGUID(guidApplicationId, m_ApplicationNodes.at(index)->m_guidApplicationId))
        {
            // current application is about to be removed.
            if (m_nApplicationMenuIndex == index &&
                m_CurrentContentId != 0)
            {
                // going to initial menu
                SetCurrentContentId(0);
                m_bContextMenu = FALSE;
                EmptyBackContentIdStack();
            }
            //
            
            delete m_ApplicationNodes[index];
            m_ApplicationNodes.erase(m_ApplicationNodes.begin() + index);

            // Ensure the index to the current application page is still correct
            if (m_nApplicationMenuIndex > index)
            {
                m_nApplicationMenuIndex--;
            }
            else if (m_nApplicationMenuIndex == index)
            {
                m_nApplicationMenuIndex = ForwardFindValidApplicationNodeIndex();
            }
            
            break;
        }
    }
    if ((QuantityOfApplicationNodes == index) && (S_FALSE == hrNotifications))
    {
        hr = REGDB_E_CLASSNOTREG;
    }

    if (SUCCEEDED(hr))
    {
        //
        // render only when current screen is initial menu.
        //
        if (m_CurrentContentId == 0 || bRenderForNotification)
        {
            RenderAndSendDataToDevice();
        }
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}


HRESULT CDataManager::DeleteAllApplications(void)
{
    EnterCriticalSection(&m_CriticalSection);

    _DeleteAllApplications();

    RenderAndSendDataToDevice();

    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

VOID CDataManager::_DeleteAllApplications()
{
    // Private Method. Does not need critical section

    DeleteAllNotifications();

    for (size_t index = 0; index < m_ApplicationNodes.size(); index++)
    {
        delete m_ApplicationNodes[index];
    }

    m_ApplicationNodes.clear();
    m_nApplicationMenuIndex = 0;

    SetCurrentContentId(CONTENT_ID_GLANCE);
    ClearContextMenu();
    EmptyBackContentIdStack();
}


HRESULT CDataManager::GetApplications(
    __deref_out_ecount(*pcAppIds) APPLICATION_ID** ppAppIds,
    __out DWORD* pcAppIds)
{
    if (NULL == ppAppIds || NULL == pcAppIds)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_CriticalSection);

    HRESULT hr = S_OK;
    const size_t  cApplications = m_ApplicationNodes.size();

    // Initialize out parameters
    *ppAppIds = NULL;
    *pcAppIds = 0;

    // Determine how much data we need to allocate
    if (DWORD_MAX > cApplications)
    {
        *pcAppIds = (DWORD)cApplications;
    }
    else
    {
        hr = E_UNEXPECTED;
    }     

    // Allocate memory to hold the APPLICATION_IDs
    if (SUCCEEDED(hr) && 0 != *pcAppIds)
    {
        *ppAppIds = (APPLICATION_ID*)::CoTaskMemAlloc(*pcAppIds * sizeof(APPLICATION_ID));
        if (NULL == *ppAppIds)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            ::ZeroMemory(*ppAppIds, *pcAppIds * sizeof(APPLICATION_ID));
        }
    }

    for (size_t index = 0; SUCCEEDED(hr) && index < *pcAppIds; index++)
    {
        memcpy(&(*ppAppIds)[index], &m_ApplicationNodes[index]->m_guidApplicationId, sizeof(APPLICATION_ID));
    }

    LeaveCriticalSection(&m_CriticalSection);

    if (FAILED(hr))
    {
        ::CoTaskMemFree(*ppAppIds);
        *ppAppIds = NULL;
        *pcAppIds = 0;
    }

    return hr;
}

HRESULT CDataManager::ChangeSortOrder(__in_ecount(cApps) const APPLICATION_ID* pApps, const DWORD cApps)
{
    // If the count of apps is 0 or the pointer to the list of apps is NULL, then there is no work to do
    if ((NULL == pApps) ||
        (0 == cApps))
    {
        return S_FALSE;
    }

    HRESULT hr = S_OK;

    EnterCriticalSection(&m_CriticalSection);

    // 1. Store current application index
    size_t index = 0;
    CNodeApplication* pCurrentNode = NULL;

    if (0 != m_ApplicationNodes.size())
    {
        pCurrentNode = m_ApplicationNodes[m_nApplicationMenuIndex];
    }

    // 2. Create a new application list, save the old one
    NODEAPPLICATIONVECTOR VectorOld;
    m_ApplicationNodes.swap(VectorOld);

    m_ApplicationNodes.clear();

    // 3. for each application in pApps
    //      a. if exists in old list, copy into new list, and erase from old list
    //      b. else if it does not exist, create new empty node in new list
    for (DWORD nApp = 0; nApp < cApps; ++nApp)
    {
        CNodeApplication* pNewNode = NULL;

        const size_t QuantityOfApplicationNodes = VectorOld.size();
        for (index = 0; index < QuantityOfApplicationNodes; index++)
        {
            if (TRUE == IsEqualGUID(pApps[nApp], VectorOld.at(index)->m_guidApplicationId))
            {
                pNewNode = VectorOld[index];
                VectorOld.erase(VectorOld.begin() + index);
                break;
            }
        }
        if (QuantityOfApplicationNodes == index)
        {
            pNewNode = new(std::nothrow) CNodeApplication(pApps[nApp]);
            if (NULL == pNewNode)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (NULL != pNewNode)
        {
            m_ApplicationNodes.push_back(pNewNode);
        }
    }

    // 4. copy any remaining nodes in old list to new list
    if (0 != VectorOld.size())
    {
        m_ApplicationNodes.insert(m_ApplicationNodes.end(), VectorOld.begin(), VectorOld.end());
    }

    // 5. free old list
    VectorOld.clear();

    // 6. reset current application index
    const size_t QuantityOfApplicationNodes = m_ApplicationNodes.size();
    for (index = 0; index < QuantityOfApplicationNodes; index++)
    {
        if (m_ApplicationNodes[index] == pCurrentNode)
        {
            break;
        }
    }

    if (index == QuantityOfApplicationNodes)
    {
        // We couldn't find the previously current node in the new list, so let's
        // find the first one that has valid data to display
        m_nApplicationMenuIndex = 0;
        m_nApplicationMenuIndex = ForwardFindValidApplicationNodeIndex();
    }
    else
    {
        // Reset the current node to what it was before we changed the order
        m_nApplicationMenuIndex = index;
    }

    // 7. re-render data.
    RenderAndSendDataToDevice();
    
    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}


HRESULT CDataManager::AddNotification(GUID guidApplicationId,
                                      unsigned long ulNotificationId,
                                      LPCWSTR wszTitle,
                                      LPCWSTR wszBody,
                                      const BYTE* pIcon,
                                      size_t cbIcon,
                                      FILETIME ftExpirationTime)
{
    EnterCriticalSection(&m_CriticalSection);

    HRESULT hr = S_OK;
    BOOL bRender = FALSE;

    CNodeNotification* pNotification = new(std::nothrow) CNodeNotification(guidApplicationId,
                                                                           ulNotificationId,
                                                                           ftExpirationTime);
    if (pNotification)
    {
        hr = pNotification->SetTitle(wszTitle);

        if (SUCCEEDED(hr))
        {
            hr = pNotification->SetBody(wszBody);
        }
        
        if (SUCCEEDED(hr))
        {
            if (pIcon && cbIcon)
            {
                hr = pNotification->SetIcon(pIcon, cbIcon);
            }
        }
        
        if (SUCCEEDED(hr))
        {
            try
            {
                m_NotificationNodes.push_back(pNotification);
            }
            catch(...)
            {
                hr = E_FAIL;
            }
        }
        
        if (SUCCEEDED(hr))
        {
            bRender = TRUE;
        }
        else
        {
            delete pNotification;
            pNotification = NULL;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        if (bRender)
        {
            RenderAndSendDataToDevice();
        }
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}


// This method deletes the first notification that matches the requirements. If an Independent Software Vendor (ISV)
// sends multiple notifications with the same NotificationID, they will need to call this
// method multiple times.
HRESULT CDataManager::DeleteNotification(GUID guidApplicationId, unsigned long ulNotificationId)
{
    EnterCriticalSection(&m_CriticalSection);

    HRESULT hr = S_OK;

    const size_t QuantityOfNotificationNodes = m_NotificationNodes.size();
    size_t index = 0;
    BOOL bRender = FALSE;
    
    for ((index); index < QuantityOfNotificationNodes; index++)
    {
        // NotificationID and ApplicationID must match
        if ((m_NotificationNodes.at(index)->m_ulNotificationId == ulNotificationId) &&
            (TRUE == IsEqualGUID(guidApplicationId, m_NotificationNodes.at(index)->m_guidApplicationId)))
        {
            //
            if (index == (m_NotificationNodes.size() - 1))
            {
                bRender = TRUE;
            }
            //
            delete m_NotificationNodes[index];
            m_NotificationNodes.erase(m_NotificationNodes.begin() + index);
            break;
        }
    }
    if (QuantityOfNotificationNodes == index)
    {
        hr = REGDB_E_CLASSNOTREG;
    }

    if (SUCCEEDED(hr))
    {
        if (bRender)
        {
            RenderAndSendDataToDevice();
        }
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}


// Return: S_OK: Deleted all notifications for the application
// Return: S_FALSE: There were no notifications to delete
HRESULT CDataManager::DeleteAllNotificationsForAnApplication(GUID guidApplicationId, BOOL *pbRender)
{
    // Private Method. Does not need critical section

    const size_t QuantityOfNotificationNodes = m_NotificationNodes.size();
    if (0 == QuantityOfNotificationNodes)
    {
        return S_FALSE;
    }

    // Enumerate through vector backwards so to delete multiple nodes in one pass
    for (size_t index = QuantityOfNotificationNodes - 1; index != (size_t)-1; index--) // Note: index is unsigned, which explains the termination check logic
    {
        if (TRUE == IsEqualGUID(guidApplicationId, m_NotificationNodes.at(index)->m_guidApplicationId))
        {
            //
            if (index == (QuantityOfNotificationNodes - 1))
            {
                *pbRender = TRUE;
            }
            //
            
            delete m_NotificationNodes[index];
            m_NotificationNodes.erase(m_NotificationNodes.begin() + index);
        }
    }

    return S_OK;
}


HRESULT CDataManager::DeleteAllNotifications(void)
{
    // Private Method. Does not need critical section

    for (size_t index = 0; index < m_NotificationNodes.size(); index++)
    {
        delete m_NotificationNodes[index];
    }

    m_NotificationNodes.clear();

    return S_OK;
}

//
// Device Event Handler
//
HRESULT CDataManager::HandleDeviceEvent(CDevice::DeviceEvent DeviceEvent)
{
    HRESULT hr = S_FALSE;

#if DBG
    WCHAR wszBuff[256];
    StringCchPrintf(wszBuff, COUNTOF(wszBuff), L">> HandleDeviceEvent %d\r\n", DeviceEvent);
    OutputDebugString(wszBuff);
#endif

    EnterCriticalSection(&m_CriticalSection);

    const size_t QuantityOfNotificationNodes = m_NotificationNodes.size();
    const size_t QuantityOfApplicationNodes = m_ApplicationNodes.size();

    if (DeviceEvent == CDevice::Initialize)
    {
        //
        // Initialize
        //
        SetCurrentContentId(CONTENT_ID_GLANCE);
        hr = S_OK;
    }
    else if (CDevice::RenderAgain == DeviceEvent)
    {
        hr = S_OK;
    }
    else if (0 != QuantityOfNotificationNodes)
    {
        //
        // If there is at least one notification, clear the last one (at end of vector) regardless of button press
        //
        switch (DeviceEvent)
        {
            case CDevice::ButtonSelect:
            {
                delete m_NotificationNodes[QuantityOfNotificationNodes - 1];
                m_NotificationNodes.erase(m_NotificationNodes.end() - 1);
                hr = S_OK;
                break;
            }
            case CDevice::ButtonUp:
            case CDevice::ButtonDown:
            {
                hr = m_NotificationNodes[QuantityOfNotificationNodes - 1]->m_view.OnDeviceEvent(DeviceEvent);
                break;
            }
        }
    }
    else if (g_pDataManager->IsContextMenuActive())
    {
        //
        // Context Menu
        //
        CContent *pTargetContent = NULL;

        if (!m_pCurrentApplication)
        {
            pTargetContent = &m_HomeScreenContextMenuContent;
        }
        else
        {
            if (m_ContextMenuId)
            {
                pTargetContent = m_pCurrentApplication->GetContent(m_ContextMenuId);
            }
            
            if (!pTargetContent)
            {
                pTargetContent = &m_DefaultContextMenuContent;
            }
        }

        hr = pTargetContent->OnDeviceEvent(DeviceEvent);
    }
    else if (m_pCurrentApplication == NULL)
    {
        //
        // home application menu / no gadget screen
        //
        switch (DeviceEvent)
        {
            case CDevice::ButtonMenu:
                //
                // built-in home screen context menu for select user...
                //
                SetContextMenu(0);
                hr = S_OK;
                break;
            case CDevice::ButtonNext:
                if (QuantityOfApplicationNodes > 1)
                {
                    m_nApplicationMenuIndex++;
                    m_nApplicationMenuIndex = ForwardFindValidApplicationNodeIndex();
                    hr = S_OK;
                }
                break;

            case CDevice::ButtonPrevious:
                if (QuantityOfApplicationNodes > 1)
                {
                    if ((0 == m_nApplicationMenuIndex) ||
                        (QuantityOfApplicationNodes <= m_nApplicationMenuIndex))
                    {
                        m_nApplicationMenuIndex = QuantityOfApplicationNodes - 1;
                    }
                    else
                    {
                        m_nApplicationMenuIndex--;
                    }
                    hr = S_OK;
                }
                break;
            case CDevice::ButtonSelect:
                if (QuantityOfApplicationNodes > 0)
                {
                    //
                    // transition to gadget homepage from home menu screen
                    //
                    m_pCurrentApplication = m_ApplicationNodes[m_nApplicationMenuIndex];

                    SetCurrentContentId(CONTENT_ID_HOME);   // 1

                    EmptyBackContentIdStack();

                    m_pDDI->SendApplicationEvent(
                        m_pCurrentApplication->m_guidApplicationId,
                        SIDESHOW_EVENTID_APPLICATION_ENTER,
                        NULL,
                        0);

                    m_pCurrentApplication->OnApplicationEnter(); // this calls OnContentEnter for CONTENT_ID_HOME

                    hr = S_OK;
                }
                break;
        }
    }
    else
    {
        //
        // Forwarding the device event to active content
        //
        CContent *pTargetContent = NULL;

        pTargetContent = m_pCurrentApplication->GetContent(m_CurrentContentId);

        if (pTargetContent)
        {
            hr = pTargetContent->OnDeviceEvent(DeviceEvent);
        }
        else
        {
            //
            // default context menu for content missing case.
            //
            if (DeviceEvent == CDevice::ButtonMenu)
            {
                SetContextMenu(0);
                hr = S_OK;
            }
        }
        
        if (hr == S_FALSE)
        {
            //
            // default back button behavior
            //
            if (DeviceEvent == CDevice::ButtonBack)
            {
                HandleBackButton();
                hr = S_OK;
            }
        }
    }

    if (hr == S_OK)
    {
        RenderAndSendDataToDevice();
    }

    LeaveCriticalSection(&m_CriticalSection);

    return S_OK;
}


void CDataManager::SetCurrentContentId(CONTENT_ID contentId)
{
    if (contentId == CONTENT_ID_GLANCE)     // 0
    {
        m_pCurrentApplication = NULL;
    }

    m_CurrentContentId = contentId;

    if (!m_BackContentIdStack.empty() && m_BackContentIdStack.top() == m_CurrentContentId)
    {
        m_BackContentIdStack.pop();
    }
}

//
// Back button
//
VOID CDataManager::HandleBackButton()
{
    if (m_CurrentContentId == CONTENT_ID_HOME)          // 1
    {
        GotoHome();
    }
    else
    {
        if (!m_BackContentIdStack.empty())
        {
            CONTENT_ID contentId = m_BackContentIdStack.top();
            m_BackContentIdStack.pop();

            CNodeApplication *pAppNode = g_pDataManager->GetCurrentApplication();
                        
            g_pDataManager->GetDDI()->SendNavigationEvent(
                pAppNode->m_guidApplicationId,
                g_pDataManager->GetCurrentContentId(),
                contentId,
                SCF_BUTTON_BACK);

            SetCurrentContentId(contentId);
        }
        else
        {
            GotoHome();
        }
    }
}

//
// Goto Home (Home means Initial Application Menu)
//
VOID CDataManager::GotoHome()
{
    EmptyBackContentIdStack();

    if (m_pCurrentApplication)
    {
        if (m_pCurrentApplication != &m_BuiltinApplication)
        {
            m_pDDI->SendApplicationEvent(
                m_pCurrentApplication->m_guidApplicationId,
                SIDESHOW_EVENTID_APPLICATION_EXIT,
                NULL,
                0);
        }
        
        m_pCurrentApplication->OnApplicationExit();
    }

    SetCurrentContentId(CONTENT_ID_GLANCE);     // 0
}

//
// Back ContentId Stack
//
HRESULT CDataManager::PushCurrentContentId()
{
    HRESULT hr = S_OK;
    
    
    try
    {
        if (m_BackContentIdStack.empty() || m_BackContentIdStack.top() != m_CurrentContentId)   // prevent to push same content id
        {
            m_BackContentIdStack.push(m_CurrentContentId);
        }
    }
    catch(...)
    {
        hr = E_FAIL;
    }
    
    return hr;
}

VOID CDataManager::EmptyBackContentIdStack()
{
    while(!m_BackContentIdStack.empty())
        m_BackContentIdStack.pop();
}

// Return: S_FALSE if display wasn't changed
// Return: S_OK if display is changed
HRESULT CDataManager::TimerMaintenance(void)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_CriticalSection);

    if (m_NotificationNodes.size() != 0)
    {
        BOOL bRender = FALSE;
        SYSTEMTIME stLocalTime;
        CFileTime ftSystemTime;
        
        GetLocalTime(&stLocalTime);
        SystemTimeToFileTime(&stLocalTime, &ftSystemTime);

        for (int index = (int)m_NotificationNodes.size() - 1; index >= 0; index--)
        {
            if (m_NotificationNodes[index]->m_ftExpirationTime <= ftSystemTime)
            {
#if DBG
                WCHAR buff[256];
                SYSTEMTIME stExpiration;
                FileTimeToSystemTime(&m_NotificationNodes[index]->m_ftExpirationTime, &stExpiration);

                StringCchPrintf(buff, COUNTOF(buff), L"Notification %s expiring...\r\n", m_NotificationNodes[index]->GetTitle());
                OutputDebugString(buff);
                StringCchPrintf(buff, COUNTOF(buff), L"  System Time %02d:%02d:%02d.%02d\r\n",
                                    stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond, stLocalTime.wMilliseconds);
                OutputDebugString(buff);
                
                StringCchPrintf(buff, COUNTOF(buff), L"  Expire Time %02d:%02d:%02d.%02d\r\n",
                                    stExpiration.wHour, stExpiration.wMinute, stExpiration.wSecond, stExpiration.wMilliseconds);
                OutputDebugString(buff);
#endif
                if ((DWORD)index == (m_NotificationNodes.size() - 1))
                {
                    bRender = TRUE;
                }
                
                delete m_NotificationNodes[index];
                m_NotificationNodes.erase(m_NotificationNodes.begin() + index);
            }
        }

        if (bRender)
        {
            RenderAndSendDataToDevice();
        }
    }

    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

//
// Render Content to Bitmap
//
HRESULT CDataManager::RenderProperPage(IStream *pStream, DWORD *pdwFlags)
{
    // Private Method. Does not need critical section

    HRESULT hr = S_FALSE;

    m_nApplicationMenuIndex = ForwardFindValidApplicationNodeIndex();

    m_renderer.StartRendering();

    m_renderer.DrawBackground();
    
    if (m_pCurrentApplication == NULL)
    {
        if (m_ApplicationNodes.size() == 0)
        {
            //
            // Draw No Gadget Message
            //
            hr = m_renderer.DrawNoGadgetMessage();
        }
        else
        {
            //
            // Draw Home Screen (Gadget Selection Screen)
            //
            hr = m_renderer.DrawHomeScreen();

            *pdwFlags |= RENDER_FLAG_INITMENU;
        }
    }
    else
    {
        m_renderer.DrawTitlebar();

        CContent *pContent = m_pCurrentApplication->GetContent(m_CurrentContentId);

        if (pContent)
        {
            //
            // Render Content
            //
            hr = pContent->RenderContent(m_renderer.GetDC());

            *pdwFlags |= RENDER_FLAG_CONTENT;
        }
        else
        {
            // 
            // missing content case
            //
#ifdef _ASSIGNED_USER_MODEL
            if (m_pCurrentApplication != &m_BuiltinApplication)
#endif
            {
                //
                // Send Content Missing Event
                //
                m_pDDI->SendContentMissingEvent(m_pCurrentApplication->m_guidApplicationId, m_CurrentContentId);
            }

            //
            // draw application name on titlebar
            //
            int left = CONTENT_LEFT_MARGIN;
            SIZE size = m_pCurrentApplication->GetMiniIcon()->Draw(m_renderer.GetDC(), left, (CRenderer::GetTitlebarHeight() - MINI_ICON_SIZE) / 2);

            if (size.cx)
            {
                left += size.cx + 6;
            }

            RECT rect = { left, 0, CDevice::GetDeviceHorizontalResolution(), CRenderer::GetTitlebarHeight() };

            DrawText(m_renderer.GetDC(),
                m_pCurrentApplication->GetTitle(),
                -1,
                &rect,
                DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | DT_SINGLELINE);
        }
    }
    
//  if (SUCCEEDED(hr))
    if (hr != E_OUTOFMEMORY)
    {
        //
        // Render Context Menu
        //
        if (IsContextMenuActive())
        {
            CContent *pContextMenuContent = NULL;

            if (m_pCurrentApplication == NULL)
            {
                pContextMenuContent = &m_HomeScreenContextMenuContent;
            }
            else
            {
                if (m_ContextMenuId)
                {
                    pContextMenuContent = m_pCurrentApplication->GetContent(m_ContextMenuId);
                    
                    if (!pContextMenuContent
#ifdef _ASSIGNED_USER_MODEL
                        && m_pCurrentApplication != &m_BuiltinApplication
#endif
                    )
                    {
                        m_pDDI->SendContentMissingEvent(m_pCurrentApplication->m_guidApplicationId, m_ContextMenuId);
                    }
                }

                if (!pContextMenuContent)
                {
                    pContextMenuContent = &m_DefaultContextMenuContent;
                }
            }

            hr = pContextMenuContent->RenderContextMenu(m_renderer.GetDC());

            *pdwFlags |= RENDER_FLAG_CONTEXTMENU;
        }
    }

    //
    // Draw Notification
    //
//  if (SUCCEEDED(hr))
    if (hr != E_OUTOFMEMORY)
    {
        if (0 != m_NotificationNodes.size())
        {
            hr = m_renderer.DrawNotification(m_NotificationNodes[m_NotificationNodes.size() - 1]);

            *pdwFlags |= RENDER_FLAG_NOTIFICATION;
        }
    }
    
    //
    //  Create Packed DIB
    //
//  if (SUCCEEDED(hr))
    if (hr != E_OUTOFMEMORY)
    {
        hr = m_renderer.CreatePackedDIB(pStream);
    }
    
    return hr;
}


HRESULT CDataManager::RenderAndSendDataToDevice()
{
    // Private Method. Does not need critical section

    HRESULT hr;
    DWORD dwRenderFlags = 0;
    
    if (!m_pImageSink)
    {
        return E_POINTER;
    }
    
    hr = RenderProperPage(m_pImageSink, &dwRenderFlags);
    
//  if (SUCCEEDED(hr))
    if (hr != E_OUTOFMEMORY)
    {
        HGLOBAL hGlobal = NULL;
        
        GetHGlobalFromStream(m_pImageSink, &hGlobal);
        
        if (hGlobal)
        {
            BYTE* pbBitmapData = (BYTE *)GlobalLock(hGlobal);
            size_t cbBitmapData = GlobalSize(hGlobal);
            
            CDevice::SendRenderedDataToDevice(pbBitmapData, (DWORD)cbBitmapData, dwRenderFlags);
            
            GlobalUnlock(hGlobal);
        }
        else
        {
            hr = E_FAIL;
        }
    }

    LARGE_INTEGER i = {0};
    m_pImageSink->Seek(i, STREAM_SEEK_SET, NULL);

    return hr;
}


size_t CDataManager::ForwardFindValidApplicationNodeIndex(void)
{
    // Private Method. Does not need critical section

    const size_t QuantityOfApplicationNodes = m_ApplicationNodes.size();

    if (0 == QuantityOfApplicationNodes)
    {
        return INVALID_APPLICATION_NODE_INDEX;
    }

    // Ensure the index is in range
    if (QuantityOfApplicationNodes <= m_nApplicationMenuIndex)
    {
        m_nApplicationMenuIndex = 0;
    }

    return m_nApplicationMenuIndex;
}

CNodeApplication* CDataManager::GetCurrentApplication()
{
    return m_pCurrentApplication;
}

CNodeApplication* CDataManager::GetApplicationById(REFAPPLICATION_ID rAppId)
{
    CNodeApplication* ret = NULL;
    const size_t QuantityOfApplicationNodes = m_ApplicationNodes.size();
    
    for (size_t index = 0; index < QuantityOfApplicationNodes; index++)
    {
        if (TRUE == IsEqualGUID(rAppId, m_ApplicationNodes.at(index)->m_guidApplicationId))
        {
            ret = m_ApplicationNodes.at(index);
            break;
        }
    }

    return ret;
}

//
// Contents
//

HRESULT CDataManager::AddContent(
                    REFAPPLICATION_ID rAppId,
                    CONTENT_ID contentId,
                    const unsigned char* pData,
                    const DWORD cbData)
{
    HRESULT hr = S_OK;
    CNodeApplication* pAppNode;
    
    EnterCriticalSection(&m_CriticalSection);
    
    pAppNode = GetApplicationById(rAppId);

    if (pAppNode)
    {
        hr = pAppNode->AddContent(contentId, pData, cbData);
    }
    else
    {
        hr = E_FAIL;
    }

    if (m_nApplicationMenuIndex != INVALID_APPLICATION_NODE_INDEX &&
        IsEqualGUID(rAppId, m_ApplicationNodes.at(m_nApplicationMenuIndex)->m_guidApplicationId) == TRUE)
    {
        BOOL bRender = FALSE;

        if (m_CurrentContentId)
        {
            //
            // if the added content is current content, then redraw.
            //
            if (m_CurrentContentId == contentId)
            {
                bRender = TRUE;
            }
            else
            {
                //
                // if the current content refers to the added content, then redraw.
                // this code is assuming that the added content is image.
                //
                CContent *pContent = pAppNode->GetContent(m_CurrentContentId);

                if (pContent)
                {
                    if (pContent->HasImageReferenceOf(contentId))
                    {
                        bRender = TRUE;
                    }
                }
            }
        }

        if (!bRender)
        {
            //
            // if the added content is currently activated context menu, then redraw.
            //
            if (m_bContextMenu && m_ContextMenuId == contentId)
            {
                bRender = TRUE;
            }
            else
            {
                CContent *pContent = pAppNode->GetContent(m_ContextMenuId);

                if (pContent)
                {
                    if (pContent->HasImageReferenceOf(contentId))
                    {
                        bRender = TRUE;
                    }
                }
            }
        }

        if (bRender)
        {
            RenderAndSendDataToDevice();
        }
    }
    
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CDataManager::RemoveContent(
                REFAPPLICATION_ID rAppId,
                CONTENT_ID contentId)
{
    HRESULT hr = S_OK;
    CNodeApplication* pAppNode;

    EnterCriticalSection(&m_CriticalSection);
    
    pAppNode = GetApplicationById(rAppId);

    if (pAppNode)
    {
        hr = pAppNode->RemoveContent(contentId);
    }
    else
    {
        hr = E_FAIL;
    }
    
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CDataManager::RemoveAllContent(REFAPPLICATION_ID rAppId)
{
    HRESULT hr = S_OK;
    CNodeApplication* pAppNode;

    EnterCriticalSection(&m_CriticalSection);
    
    pAppNode = GetApplicationById(rAppId);

    if (pAppNode)
    {
        hr = pAppNode->RemoveAllContent();
    }
    else
    {
        hr = E_FAIL;
    }
    
    LeaveCriticalSection(&m_CriticalSection);
    
    return hr;
}

//
// ContextMenu
//
VOID CDataManager::SetContextMenu(CONTENT_ID contextMenuId, UINT32 menuItemId)
{
    m_bContextMenu = TRUE;
    m_ContextMenuId = contextMenuId;
    m_ContextMenuPreviousItemId = menuItemId;

    CContent *pContextMenuContent = NULL;;

    if (!m_pCurrentApplication)
    {
        //
        // special handling for home screen context menu
        //
        pContextMenuContent = &m_HomeScreenContextMenuContent;
    }
    else
    {
        if (m_ContextMenuId)
        {
            pContextMenuContent = m_pCurrentApplication->GetContent(m_ContextMenuId);
        }

        if (!pContextMenuContent)
        {
            pContextMenuContent = &m_DefaultContextMenuContent;
        }
    }

    pContextMenuContent->OnContextMenuEnter();
}

VOID CDataManager::ClearContextMenu()
{
    m_bContextMenu = FALSE;
    m_ContextMenuId = 0;
    m_ContextMenuPreviousItemId = 0;
}

//
// Initialize Home Screen Context Menu
//
// <body><menu id="0"><item target="10001">Select User</item></menu></body>
//
HRESULT CDataManager::_InitializeHomeScreenContextMenuContent()
{
    HRESULT hr = S_OK;

    CScfMenuElement *pScfMenu = new(std::nothrow) CScfMenuElement();

    if (pScfMenu)
    {

#ifdef _ASSIGNED_USER_MODEL
        
        // add "Select user" to homescreen context menu
        CScfItemElement *pScfItemSelectUser = new(std::nothrow) CScfItemElement();
        
        if (pScfItemSelectUser)
        {
            
            pScfItemSelectUser->m_targetId = CONTENT_ID_SELECT_USER;
            pScfItemSelectUser->SetText(m_renderer.GetMenuItemSelectUserString());
            hr = pScfMenu->AddChild(pScfItemSelectUser);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

#endif
        // add "Exit SideShow mode" to homescreen context menu
        if (SUCCEEDED(hr))
        {
            CScfItemElement *pScfItemConfigure = new(std::nothrow) CScfItemElement();
            
            if (pScfItemConfigure)
            {
                pScfItemConfigure->m_targetId = CONTENT_ID_QUIT;
                pScfItemConfigure->SetText(m_renderer.GetMenuItemQuitString());
                hr = pScfMenu->AddChild(pScfItemConfigure);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }   
        }
/*
  // No need for this at the moment

        // add "Configure" to homescreen context menu
        if (SUCCEEDED(hr))
        {
            CScfItemElement *pScfItemConfigure = new(std::nothrow) CScfItemElement();
            
            if (pScfItemConfigure)
            {
                pScfItemConfigure->m_targetId = CONTENT_ID_CONFIGURE;
                pScfItemConfigure->SetText(m_renderer.GetMenuItemConfigureString());
                hr = pScfMenu->AddChild(pScfItemConfigure);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }   
        }
*/
#pragma warning(push)
#pragma warning(disable: 6326)
        if (THEME_COUNT > 1)
#pragma warning(pop)
        {
            // add "Select theme" to homescreen context menu
            if (SUCCEEDED(hr))
            {
                CScfItemElement *pScfItemSelectTheme = new(std::nothrow) CScfItemElement();
                
                if (pScfItemSelectTheme)
                {
                    pScfItemSelectTheme->m_targetId = CONTENT_ID_SELECT_THEME;
                    pScfItemSelectTheme->SetText(m_renderer.GetMenuItemSelectThemeString());
                    hr = pScfMenu->AddChild(pScfItemSelectTheme);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (FAILED(hr))
        {
            delete pScfMenu;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_HomeScreenContextMenuContent.SetScfTree(pScfMenu);
    }
    return hr;
}

//
// Initialize Select Theme Content
//
HRESULT CDataManager::_CreateSelectThemeMenuContent()
{
    HRESULT hr = S_OK;

    CContent *pSelectThemeContent = new(std::nothrow) CContent(CONTENT_ID_SELECT_THEME);

    if (pSelectThemeContent)
    {
        // add dummy content to create CContent object in built in app content cache.
        hr = m_BuiltinApplication.AddContent(CONTENT_ID_SELECT_THEME, (BYTE *)"x", 1);

        if (SUCCEEDED(hr))
        {
            pSelectThemeContent = m_BuiltinApplication.GetContent(CONTENT_ID_SELECT_THEME);
            
            if (!pSelectThemeContent)
            {
                hr = E_FAIL;
            }
        } 

        if (SUCCEEDED(hr))
        {

            CScfMenuElement *pScfMenu = new(std::nothrow) CScfMenuElement();

            if (pScfMenu)
            {  
                hr = pScfMenu->SetTitle(m_renderer.GetMenuItemSelectThemeString());
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            
            if (SUCCEEDED(hr))
            {
                CAtlStringW szThemeName;

                // add themes 
                for (int i = 0; i < THEME_COUNT; i++)
                {
                    CScfItemElement *pScfItemTheme = new(std::nothrow) CScfItemElement();
                    
                    if (pScfItemTheme)
                    {
                        // disabling Return value ignored warning
#pragma warning(push)
#pragma warning(disable: 6031)
                        szThemeName.LoadString(IDS_THEME_NAME_DEFAULT + (i * THEME_OFFSET));
#pragma warning(pop)
                        pScfItemTheme->SetText(szThemeName);
                        hr = pScfMenu->AddChild(pScfItemTheme);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (FAILED(hr))
                {
                    delete pScfMenu;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                hr = pSelectThemeContent->SetScfTree(pScfMenu);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }  

    }
    else
    {
        hr = E_OUTOFMEMORY;
    }  

    return hr;
}

//
// Home Screen Context Menu Event for Select User, etc.
// This is a callback from CContextMenuView.
//
HRESULT CDataManager::HandleInternalCommand(CONTENT_ID contentId, CONTENT_ID targetId, LONG nIndex)
{
    HRESULT hr = S_FALSE;

#ifdef _ASSIGNED_USER_MODEL
    if (contentId == CONTENT_ID_GLANCE)
    {
        //
        // home screen context menu event
        //
        if (targetId == CONTENT_ID_SELECT_USER)
        {
            hr = _CreateSelectUserContent(TRUE);

            if (SUCCEEDED(hr))
            {
                //
                // transition to Select User screen.
                //
                m_pCurrentApplication = &m_BuiltinApplication;
                
                SetCurrentContentId(CONTENT_ID_SELECT_USER);
            }
        }
    }
    else if (contentId == CONTENT_ID_SELECT_USER)
    {
        //
        // user was selected
        //
        m_pDDI->SendUserChangeRequestEvent(m_Users[nIndex]->GetSID());
        
        GotoHome();
        
        hr = S_OK;
    }
#endif
    if (targetId == CONTENT_ID_QUIT)
    {
        //
        // quit was selected
        //

        QuitSideShowMode();

        hr = S_OK;
    }

    if (targetId == CONTENT_ID_CONFIGURE)
    {
        //
        // configure was selected
        //

        // Configure the device. For example, launch the Windows Sideshow CPL
        ConfigureDevice();

        hr = S_OK;
    }

    else if (targetId == CONTENT_ID_SELECT_THEME)
    {
        //
        // transition to Select theme screen.
        //

        hr = _CreateSelectThemeMenuContent();

        if (SUCCEEDED(hr))
        {
            m_pCurrentApplication = &m_BuiltinApplication;
            
            SetCurrentContentId(CONTENT_ID_SELECT_THEME);
        }

        hr = S_OK;
    }
    else if (contentId == CONTENT_ID_SELECT_THEME)
    {
        //
        // theme was selected
        //

        hr = m_renderer.LoadThemeResources(nIndex);
        
        GotoHome();
        
    }


    return hr;
}

#ifdef _ASSIGNED_USER_MODEL

HRESULT CDataManager::AddUser(const SID* pUserSID)
{
    HRESULT hr = S_OK;
    BOOL bFound = FALSE;
    size_t i;

    EnterCriticalSection(&m_CriticalSection);

    for(i = 0; i < m_Users.size(); i++)
    {
        if (EqualSid(m_Users[i]->GetSID(), (SID *)pUserSID))
        {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        CUserInfo *pUserInfo = new(std::nothrow) CUserInfo();

        if (pUserInfo)
        {
            hr = pUserInfo->Initialize(pUserSID);

            if (SUCCEEDED(hr))
            {
                try
                {
                    m_Users.push_back(pUserInfo);
                }
                catch(...)
                {
                    hr = E_FAIL;
                }
            }

            if (FAILED(hr))
            {
                delete pUserInfo;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    //
    // If User Selection Screen, Need to Render
    //
    if (m_pCurrentApplication == &m_BuiltinApplication &&
        m_CurrentContentId == CONTENT_ID_SELECT_USER)
    {
        _CreateSelectUserContent(FALSE);

        RenderAndSendDataToDevice();
    }

    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

HRESULT CDataManager::RemoveUser(const SID* pUserSID)
{
    HRESULT hr = S_OK;
    BOOL bRender = FALSE;
    
    EnterCriticalSection(&m_CriticalSection);

    //
    // if the current user is removed, delete all gadgets
    //
    if (IsValidSid(m_pDDI->GetCurrentUserSID()) && EqualSid((PSID)pUserSID, m_pDDI->GetCurrentUserSID()))
    {
        _DeleteAllApplications();

        bRender = TRUE;
    }

    //
    // delete the user from user array.
    //
    for(size_t i = 0; i < m_Users.size(); i++)
    {
        if (EqualSid(m_Users[i]->GetSID(), (SID *)pUserSID))
        {
            delete m_Users[i];
            m_Users.erase(m_Users.begin() + i);
            break;
        }
    }

    //
    // if the current content is select user screen,
    // refresh select user content and render.
    //
    if (m_pCurrentApplication == &m_BuiltinApplication &&
        m_CurrentContentId == CONTENT_ID_SELECT_USER)
    {
        _CreateSelectUserContent(FALSE);

        bRender = TRUE;
    }

    if (bRender)
    {
        RenderAndSendDataToDevice();
    }

    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

// Create Select User content dynamically based on Users array.
// First create CONTENT_ID_SELECT_USER content if not exist.
// Next create ScfTree for the content.
// Finally put the ScfTree to the content.
//
HRESULT CDataManager::_CreateSelectUserContent(BOOL bDefAttr)
{
    HRESULT hr = S_OK;

    CContent *pSelectUserContent = m_BuiltinApplication.GetContent(CONTENT_ID_SELECT_USER);
    
    if (!pSelectUserContent)
    {
        // add dummy content to create CContent object in built in app content cache.
        hr = m_BuiltinApplication.AddContent(CONTENT_ID_SELECT_USER, (BYTE *)"x", 1);

        if (SUCCEEDED(hr))
        {
            pSelectUserContent = m_BuiltinApplication.GetContent(CONTENT_ID_SELECT_USER);
            
            if (!pSelectUserContent)
            {
                hr = E_FAIL;
            }
        } 
    }
    
    if (SUCCEEDED(hr))
    {
        // generate menu scf content tree
        CScfMenuElement *pScfMenu = new(std::nothrow) CScfMenuElement();

        if (pScfMenu)
        {
            hr = pScfMenu->SetTitle(m_renderer.GetSelectUserTitle());
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        
        if (SUCCEEDED(hr))
        {
            // loop users array.
            for(size_t i = 0; i < m_Users.size(); i++)
            {
                HRESULT hrTemp = S_OK;
                WCHAR *pwszUserName = NULL;
                WCHAR *pwszDomainName = NULL;
                
                // get user name and domain name
                hrTemp = GetAccountSidName((SID *)m_Users[i]->GetSID(), &pwszUserName, &pwszDomainName);
                
                if (SUCCEEDED(hrTemp))
                {
                    // create <item> element
                    CScfItemElement *pScfItem = new(std::nothrow) CScfItemElement();
                    
                    if (pScfItem)
                    {
                        // set domain\name to item text
                        hrTemp = pScfItem->AppendText(pwszDomainName, (UINT)wcslen(pwszDomainName));
                        
                        if (SUCCEEDED(hr))
                        {
                            hrTemp = pScfItem->AppendText(L"\\", 1);
                        }
                        
                        if (SUCCEEDED(hr))
                        {
                            hrTemp = pScfItem->AppendText(pwszUserName, (UINT)wcslen(pwszUserName));
                        }
                        
                        if (SUCCEEDED(hrTemp))
                        {
                            // set def=1 for current user item if necesarry.
                            if (bDefAttr)
                            {
                                if (IsValidSid(m_pDDI->GetCurrentUserSID()) &&
                                    EqualSid(m_pDDI->GetCurrentUserSID(), (SID *)m_Users[i]->GetSID()))
                                {
                                    pScfItem->m_def = 1;
                                }
                            }
                            
                            // add <item> to <menu>
                            hrTemp = pScfMenu->AddChild(pScfItem);
                        }
                        
                        if (FAILED(hrTemp))
                        {
                            delete pScfItem;
                        }
                    }
                    else
                    {
                        hrTemp = E_OUTOFMEMORY;
                    }

                    if (pwszUserName)   delete [] pwszUserName;
                    if (pwszDomainName) delete [] pwszDomainName;
                }
                
                if (hrTemp == E_OUTOFMEMORY)
                {
                    hr = hrTemp;
                    break;
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            // put generated menu scf tree to the select user content.
            hr = pSelectUserContent->SetScfTree(pScfMenu);
        }

        if (FAILED(hr))
        {
            delete pScfMenu;
        }
    }

    return hr;
}

#endif  // _ASSIGNED_USER_MODEL

