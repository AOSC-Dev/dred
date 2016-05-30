
//////////////////////////////////////////////////////////////////
//
// Private Cross Platform
//
//////////////////////////////////////////////////////////////////

// Helper for creating the root GUI element of a window.
drgui_element* dred_platform__create_root_gui_element(drgui_context* pGUI, dred_window* pWindow)
{
    drgui_element* pRootGUIElement = drgui_create_element(pGUI, NULL, sizeof(&pWindow), &pWindow);
    if (pRootGUIElement == NULL) {
        return NULL;
    }

    drgui_set_type(pRootGUIElement, "RootGUIElement");

    return pRootGUIElement;
}


//////////////////////////////////////////////////////////////////
//
// Win32
//
//////////////////////////////////////////////////////////////////

#ifdef DRED_WIN32
static const char* g_WindowClass = "dred_WindowClass";

#define GET_X_LPARAM(lp)    ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)    ((int)(short)HIWORD(lp))

static void dred_win32_track_mouse_leave_event(HWND hWnd)
{
    TRACKMOUSEEVENT tme;
    ZeroMemory(&tme, sizeof(tme));
    tme.cbSize    = sizeof(tme);
    tme.dwFlags   = TME_LEAVE;
    tme.hwndTrack = hWnd;
    TrackMouseEvent(&tme);
}

bool dred_is_win32_mouse_button_key_code(WPARAM wParam)
{
    return wParam == VK_LBUTTON || wParam == VK_RBUTTON || wParam == VK_MBUTTON || wParam == VK_XBUTTON1 || wParam == VK_XBUTTON2;
}

drgui_key dred_win32_to_drgui_key(WPARAM wParam)
{
    switch (wParam)
    {
    case VK_BACK:   return DRGUI_BACKSPACE;
    case VK_SHIFT:  return DRGUI_SHIFT;
    case VK_ESCAPE: return DRGUI_ESCAPE;
    case VK_PRIOR:  return DRGUI_PAGE_UP;
    case VK_NEXT:   return DRGUI_PAGE_DOWN;
    case VK_END:    return DRGUI_END;
    case VK_HOME:   return DRGUI_HOME;
    case VK_LEFT:   return DRGUI_ARROW_LEFT;
    case VK_UP:     return DRGUI_ARROW_UP;
    case VK_RIGHT:  return DRGUI_ARROW_RIGHT;
    case VK_DOWN:   return DRGUI_ARROW_DOWN;
    case VK_DELETE: return DRGUI_DELETE;

    default: break;
    }

    return (drgui_key)wParam;
}

int dred_win32_get_modifier_key_state_flags()
{
    int stateFlags = 0;

    SHORT keyState = GetAsyncKeyState(VK_SHIFT);
    if (keyState & 0x8000) {
        stateFlags |= DRED_KEY_STATE_SHIFT_DOWN;
    }

    keyState = GetAsyncKeyState(VK_CONTROL);
    if (keyState & 0x8000) {
        stateFlags |= DRED_KEY_STATE_CTRL_DOWN;
    }

    keyState = GetAsyncKeyState(VK_MENU);
    if (keyState & 0x8000) {
        stateFlags |= DRED_KEY_STATE_ALT_DOWN;
    }

    return stateFlags;
}

int dred_win32_get_mouse_event_state_flags(WPARAM wParam)
{
    int stateFlags = 0;

    if ((wParam & MK_LBUTTON) != 0) {
        stateFlags |= DRED_MOUSE_BUTTON_LEFT_DOWN;
    }

    if ((wParam & MK_RBUTTON) != 0) {
        stateFlags |= DRED_MOUSE_BUTTON_RIGHT_DOWN;
    }

    if ((wParam & MK_MBUTTON) != 0) {
        stateFlags |= DRED_MOUSE_BUTTON_MIDDLE_DOWN;
    }

    if ((wParam & MK_XBUTTON1) != 0) {
        stateFlags |= DRED_MOUSE_BUTTON_4_DOWN;
    }

    if ((wParam & MK_XBUTTON2) != 0) {
        stateFlags |= DRED_MOUSE_BUTTON_5_DOWN;
    }


    if ((wParam & MK_CONTROL) != 0) {
        stateFlags |= DRED_KEY_STATE_CTRL_DOWN;
    }

    if ((wParam & MK_SHIFT) != 0) {
        stateFlags |= DRED_KEY_STATE_SHIFT_DOWN;
    }


    SHORT keyState = GetAsyncKeyState(VK_MENU);
    if (keyState & 0x8000) {
        stateFlags |= DRED_KEY_STATE_ALT_DOWN;
    }


    return stateFlags;
}

LRESULT CALLBACK GenericWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    dred_window* pWindow = (dred_window*)GetWindowLongPtrA(hWnd, 0);
    if (pWindow == NULL) {
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }

    switch (msg)
    {
        case WM_CREATE:
        {
            // This allows us to track mouse enter and leave events for the window.
            dred_win32_track_mouse_leave_event(hWnd);
        } return 0;

        case WM_DESTROY:
        {
        } break;

        
        case WM_CLOSE:
        {
            dred_window_on_close(pWindow);
        } return 0;

        // show/hide
        case WM_WINDOWPOSCHANGING:
        {
            // Showing and hiding windows can be cancelled if false is returned by any of the event handlers.
            WINDOWPOS* pWindowPos = (WINDOWPOS*)lParam;
            assert(pWindowPos != NULL);

            if ((pWindowPos->flags & SWP_HIDEWINDOW) != 0 && pWindow->onHide) {
                if (!pWindow->onHide(pWindow, pWindow->onHideFlags)) {
                    pWindowPos->flags &= ~SWP_HIDEWINDOW;
                }

                pWindow->onHideFlags = 0;
            }

            if ((pWindowPos->flags & SWP_SHOWWINDOW) != 0 && pWindow->onShow) {
                if (!pWindow->onShow(pWindow)) {
                    pWindowPos->flags &= ~SWP_SHOWWINDOW;
                }
            }
        } break;


        case WM_MOUSELEAVE:
        {
            pWindow->isCursorOver = false;
            dred_window_on_mouse_leave(pWindow);
        } break;

        case WM_MOUSEMOVE:
        {
            // On Win32 we need to explicitly tell the operating system to post a WM_MOUSELEAVE event. The problem is that it needs to be re-issued when the
            // mouse re-enters the window. The easiest way to do this is to just call it in response to every WM_MOUSEMOVE event.
            if (!pWindow->isCursorOver) {
                dred_win32_track_mouse_leave_event(hWnd);

                pWindow->isCursorOver = true;
                dred_window_on_mouse_enter(pWindow);
            }

            dred_window_on_mouse_move(pWindow, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam));
        } break;



        case WM_NCLBUTTONDOWN:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_LEFT, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_LEFT_DOWN);
        } break;
        case WM_NCLBUTTONUP:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_up(pWindow, DRGUI_MOUSE_BUTTON_LEFT, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam));
            
        } break;
        case WM_NCLBUTTONDBLCLK:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_LEFT, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_LEFT_DOWN);
            dred_window_on_mouse_button_dblclick(pWindow, DRGUI_MOUSE_BUTTON_LEFT, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_LEFT_DOWN);
           
        } break;

        case WM_LBUTTONDOWN:
        {
            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_LEFT_DOWN);
        } break;
        case WM_LBUTTONUP:
        {
            dred_window_on_mouse_button_up(pWindow, DRGUI_MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam));
        } break;
        case WM_LBUTTONDBLCLK:
        {
            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_LEFT_DOWN);
            dred_window_on_mouse_button_dblclick(pWindow, DRGUI_MOUSE_BUTTON_LEFT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_LEFT_DOWN);
        } break;


        case WM_NCRBUTTONDOWN:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_RIGHT, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_RIGHT_DOWN);
        } break;
        case WM_NCRBUTTONUP:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_up(pWindow, DRGUI_MOUSE_BUTTON_RIGHT, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam));
        } break;
        case WM_NCRBUTTONDBLCLK:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_RIGHT, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_RIGHT_DOWN);
            dred_window_on_mouse_button_dblclick(pWindow, DRGUI_MOUSE_BUTTON_RIGHT, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_RIGHT_DOWN);
        } break;

        case WM_RBUTTONDOWN:
        {
            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_RIGHT_DOWN);
        } break;
        case WM_RBUTTONUP:
        {
            dred_window_on_mouse_button_up(pWindow, DRGUI_MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam));
        } break;
        case WM_RBUTTONDBLCLK:
        {
            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_RIGHT_DOWN);
            dred_window_on_mouse_button_dblclick(pWindow, DRGUI_MOUSE_BUTTON_RIGHT, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_RIGHT_DOWN);
        } break;


        case WM_NCMBUTTONDOWN:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_MIDDLE, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_MIDDLE_DOWN);
        } break;
        case WM_NCMBUTTONUP:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_up(pWindow, DRGUI_MOUSE_BUTTON_MIDDLE, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam));
        } break;
        case WM_NCMBUTTONDBLCLK:
        {
            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_MIDDLE, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_MIDDLE_DOWN);
            dred_window_on_mouse_button_dblclick(pWindow, DRGUI_MOUSE_BUTTON_MIDDLE, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_MIDDLE_DOWN);
        } break;

        case WM_MBUTTONDOWN:
        {
            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_MIDDLE_DOWN);
        } break;
        case WM_MBUTTONUP:
        {
            dred_window_on_mouse_button_up(pWindow, DRGUI_MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam));
        } break;
        case WM_MBUTTONDBLCLK:
        {
            dred_window_on_mouse_button_down(pWindow, DRGUI_MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_MIDDLE_DOWN);
            dred_window_on_mouse_button_dblclick(pWindow, DRGUI_MOUSE_BUTTON_MIDDLE, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), dred_win32_get_mouse_event_state_flags(wParam) | DRED_MOUSE_BUTTON_MIDDLE_DOWN);
        } break;

        case WM_MOUSEWHEEL:
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

            POINT p;
            p.x = GET_X_LPARAM(lParam);
            p.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &p);

            dred_window_on_mouse_wheel(pWindow, delta, p.x, p.y, dred_win32_get_mouse_event_state_flags(wParam));
        } break;


        case WM_KEYDOWN:
        {
            if (!dred_is_win32_mouse_button_key_code(wParam))
            {
                int stateFlags = dred_win32_get_modifier_key_state_flags();
                if ((lParam & (1 << 30)) != 0) {
                    stateFlags |= DRED_KEY_STATE_AUTO_REPEATED;
                }

                dred_window_on_key_down(pWindow, dred_win32_to_drgui_key(wParam), stateFlags);
            }
        } break;

        case WM_KEYUP:
        {
            if (!dred_is_win32_mouse_button_key_code(wParam))
            {
                int stateFlags = dred_win32_get_modifier_key_state_flags();
                dred_window_on_key_up(pWindow, dred_win32_to_drgui_key(wParam), stateFlags);
            }
        } break;

        // NOTE: WM_UNICHAR is not posted by Windows itself, but rather intended to be posted by applications. Thus, we need to use WM_CHAR. WM_CHAR
        //       posts events as UTF-16 code points. When the code point is a surrogate pair, we need to store it and wait for the next WM_CHAR event
        //       which will contain the other half of the pair.
        case WM_CHAR:
        {
            // Windows will post WM_CHAR events for keys we don't particularly want. We'll filter them out here (they will be processed by WM_KEYDOWN).
            if (wParam < 32 || wParam == 127)       // 127 = ASCII DEL (will be triggered by CTRL+Backspace)
            {
                if (wParam != VK_TAB  &&
                    wParam != VK_RETURN)    // VK_RETURN = Enter Key.
                {
                    break;
                }
            }


            if ((lParam & (1U << 31)) == 0)     // Bit 31 will be 1 if the key was pressed, 0 if it was released.
            {
                if (IS_HIGH_SURROGATE(wParam))
                {
                    assert(pWindow->utf16HighSurrogate == 0);
                    pWindow->utf16HighSurrogate = (unsigned short)wParam;
                }
                else
                {
                    unsigned int character = (unsigned int)wParam;
                    if (IS_LOW_SURROGATE(wParam))
                    {
                        assert(IS_HIGH_SURROGATE(pWindow->utf16HighSurrogate) != 0);
                        character = dr_utf16pair_to_utf32_ch(pWindow->utf16HighSurrogate, (unsigned short)wParam);
                    }

                    pWindow->utf16HighSurrogate = 0;


                    int repeatCount = lParam & 0x0000FFFF;
                    for (int i = 0; i < repeatCount; ++i)
                    {
                        int stateFlags = dred_win32_get_modifier_key_state_flags();
                        if ((lParam & (1 << 30)) != 0) {
                            stateFlags |= DRED_KEY_STATE_AUTO_REPEATED;
                        }

                        dred_window_on_printable_key_down(pWindow, character, stateFlags);
                    }
                }
            }
        } break;


        case WM_MOVE:
        {
            dred_window_on_move(pWindow, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
        } break;

        case WM_SIZE:
        {
            dred_window_on_size(pWindow, LOWORD(lParam), HIWORD(lParam));
        } break;


        case WM_SETFOCUS:
        {
            dred_window_on_focus(pWindow);
        } break;

        case WM_KILLFOCUS:
        {
            dred_window_on_unfocus(pWindow);
        } break;



        case WM_SETCURSOR:
        {
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(pWindow->hCursor);
                return TRUE;
            }
        } break;

        case WM_PAINT:
        {
            RECT rect;
            if (GetUpdateRect(hWnd, &rect, FALSE)) {
                drgui_draw(pWindow->pRootGUIElement, drgui_make_rect((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom), pWindow->pDrawingSurface);
            }
        } break;

        case WM_ERASEBKGND:  return 1;       // Never draw the background of the window - always leave that to dr_gui.
        default: break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool dred_platform_init__win32()
{
    // We'll be handling DPI ourselves. This should be done at the top.
    dr_win32_make_dpi_aware();

    // Window classes.
    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.cbWndExtra    = sizeof(dred_window*);
    wc.lpfnWndProc   = (WNDPROC)GenericWindowProc;
    wc.lpszClassName = g_WindowClass;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
    wc.style         = CS_DBLCLKS;
    if (!RegisterClassExA(&wc)) {
        return false;
    }

    return true;
}

void dred_platform_uninit__win32()
{
    UnregisterClassA(g_WindowClass, NULL);
}

int dred_platform_run__win32()
{
    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (bRet == -1) {
            return -42; // Unknown error.
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}

void dred_platform_post_quit_message__win32(int resultCode)
{
    PostQuitMessage(resultCode);
}





dred_window* dred_window_create__win32(dred_context* pDred)
{
    DWORD dwStyleEx = 0;
    DWORD dwStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW;
    HWND hWnd = CreateWindowExA(dwStyleEx, g_WindowClass, "dred", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, NULL, NULL, NULL, NULL);
    if (hWnd == NULL) {
        return NULL;
    }

    dred_window* pWindow = calloc(1, sizeof(*pWindow));
    if (pWindow == NULL) {
        goto on_error;
    }

    pWindow->pDred = pDred;
    pWindow->hWnd = hWnd;
    pWindow->hCursor = LoadCursor(NULL, IDC_ARROW);

    pWindow->pDrawingSurface = dr2d_create_surface_gdi_HWND(pDred->pDrawingContext, hWnd);
    if (pWindow->pDrawingSurface == NULL) {
        goto on_error;
    }

    pWindow->pRootGUIElement = dred_platform__create_root_gui_element(pDred->pGUI, pWindow);
    if (pWindow->pRootGUIElement == NULL) {
        goto on_error;
    }

    // The GUI panel needs to have it's initial size set.
    int windowWidth;
    int windowHeight;
    dred_window_get_size(pWindow, &windowWidth, &windowHeight);
    drgui_set_size(pWindow->pRootGUIElement, (float)windowWidth, (float)windowHeight);


    // The dred window needs to be linked to the Win32 window handle so it can be accessed from the event handler.
    SetWindowLongPtrA(hWnd, 0, (LONG_PTR)pWindow);

    return pWindow;


on_error:
    if (pWindow) {
        if (pWindow->pRootGUIElement) {
            drgui_delete_element(pWindow->pRootGUIElement);
        }

        if (pWindow->pDrawingSurface) {
            dr2d_delete_surface(pWindow->pDrawingSurface);
        }

        free(pWindow);
    }

    DestroyWindow(hWnd);
    return NULL;
}

void dred_window_delete__win32(dred_window* pWindow)
{
    if (pWindow == NULL) {
        return;
    }

    DestroyWindow(pWindow->hWnd);
    free(pWindow);
}


void dred_window_set_size__win32(dred_window* pWindow, unsigned int newWidth, unsigned int newHeight)
{
    RECT windowRect;
    RECT clientRect;
    GetWindowRect(pWindow->hWnd, &windowRect);
    GetClientRect(pWindow->hWnd, &clientRect);

    int windowFrameX = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
    int windowFrameY = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

    assert(windowFrameX >= 0);
    assert(windowFrameY >= 0);

    int scaledWidth  = newWidth  + windowFrameX;
    int scaledHeight = newHeight + windowFrameY;
    SetWindowPos(pWindow->hWnd, NULL, 0, 0, scaledWidth, scaledHeight, SWP_NOZORDER | SWP_NOMOVE);
}

void dred_window_get_size__win32(dred_window* pWindow, unsigned int* pWidthOut, unsigned int* pHeightOut)
{
    RECT rect;
    GetClientRect(pWindow->hWnd, &rect);

    if (pWidthOut != NULL) {
        *pWidthOut = rect.right - rect.left;
    }
    if (pHeightOut != NULL) {
        *pHeightOut = rect.bottom - rect.top;
    }
}


void dred_window_show__win32(dred_window* pWindow)
{
    ShowWindow(pWindow->hWnd, SW_SHOWNORMAL);
}

void dred_window_show_maximized__win32(dred_window* pWindow)
{
    ShowWindow(pWindow->hWnd, SW_SHOWMAXIMIZED);
}

void dred_window_hide__win32(dred_window* pWindow)
{
    ShowWindow(pWindow->hWnd, SW_HIDE);
}


void dred_window_set_cursor__win32(dred_window* pWindow, dred_cursor_type cursor)
{
    switch (cursor)
    {
        case dred_cursor_type_text:
        {
            pWindow->hCursor = LoadCursor(NULL, IDC_IBEAM);
        } break;

        case dred_cursor_type_cross:
        {
            pWindow->hCursor = LoadCursor(NULL, IDC_CROSS);
        } break;

        case dred_cursor_type_double_arrow_h:
        {
            pWindow->hCursor = LoadCursor(NULL, IDC_SIZEWE);
        } break;

        case dred_cursor_type_double_arrow_v:
        {
            pWindow->hCursor = LoadCursor(NULL, IDC_SIZENS);
        } break;


        case dred_cursor_type_none:
        {
            pWindow->hCursor = NULL;
        } break;

        //case cursor_type_arrow:
        case dred_cursor_type_default:
        default:
        {
            pWindow->hCursor = LoadCursor(NULL, IDC_ARROW);
        } break;
    }

    // If the cursor is currently inside the window it needs to be changed right now.
    if (dred_window_is_cursor_over(pWindow)) {
        SetCursor(pWindow->hCursor);
    }
}

bool dred_window_is_cursor_over__win32(dred_window* pWindow)
{
    return pWindow->isCursorOver;
}



static void dred_platform__on_global_capture_mouse__win32(drgui_element* pElement)
{
    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL) {
        SetCapture(pWindow->hWnd);
    }
}

static void dred_platform__on_global_release_mouse__win32(drgui_element* pElement)
{
    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL) {
        ReleaseCapture();
    }
}

static void dred_platform__on_global_capture_keyboard__win32(drgui_element* pElement, drgui_element* pPrevCapturedElement)
{
    (void)pPrevCapturedElement;

    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL) {
        pWindow->pElementWithKeyboardCapture = pElement;
        SetFocus(pWindow->hWnd);
    }
}

static void dred_platform__on_global_release_keyboard__win32(drgui_element* pElement, drgui_element* pNewCapturedElement)
{
    (void)pNewCapturedElement;

    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL) {
        SetFocus(NULL);
    }
}

static void dred_platform__on_global_dirty__win32(drgui_element* pElement, drgui_rect relativeRect)
{
    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL)
    {
        drgui_rect absoluteRect = relativeRect;
        drgui_make_rect_absolute(pElement, &absoluteRect);


        RECT rect;
        rect.left   = (LONG)absoluteRect.left;
        rect.top    = (LONG)absoluteRect.top;
        rect.right  = (LONG)absoluteRect.right;
        rect.bottom = (LONG)absoluteRect.bottom;

#if 0
        // Scheduled redraw.
        InvalidateRect(pWindow->hWnd, &rect, FALSE);
#else
        // Immediate redraw.
        RedrawWindow(pWindow->hWnd, &rect, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
#endif
    }
}
#endif




//////////////////////////////////////////////////////////////////
//
// GTK+ 3
//
//////////////////////////////////////////////////////////////////

#ifdef DRED_GTK
int g_GTKMainLoopResultCode = 0;
GdkCursor* g_GTKCursor_Default = NULL;
GdkCursor* g_GTKCursor_IBeam = NULL;
GdkCursor* g_GTKCursor_Cross = NULL;
GdkCursor* g_GTKCursor_DoubleArrowH = NULL;
GdkCursor* g_GTKCursor_DoubleArrowV = NULL;

static drgui_key dred_gtk_to_drgui_key(guint keyval)
{
    switch (keyval)
    {
    case GDK_KEY_BackSpace: return DRGUI_BACKSPACE;
    case GDK_KEY_Shift_L:   return DRGUI_SHIFT;
    case GDK_KEY_Shift_R:   return DRGUI_SHIFT;
    case GDK_KEY_Escape:    return DRGUI_ESCAPE;
    case GDK_KEY_Page_Up:   return DRGUI_PAGE_UP;
    case GDK_KEY_Page_Down: return DRGUI_PAGE_DOWN;
    case GDK_KEY_End:       return DRGUI_END;
    case GDK_KEY_Begin:     return DRGUI_HOME;
    case GDK_KEY_Left:      return DRGUI_ARROW_LEFT;
    case GDK_KEY_Up:        return DRGUI_ARROW_UP;
    case GDK_KEY_Right:     return DRGUI_ARROW_RIGHT;
    case GDK_KEY_Down:      return DRGUI_ARROW_DOWN;
    case GDK_KEY_Delete:    return DRGUI_DELETE;

    default: break;
    }

    return (drgui_key)keyval;
}

static int dred_gtk_get_modifier_state_flags(guint stateFromGTK)
{
    int result = 0;

    if ((stateFromGTK & GDK_SHIFT_MASK) != 0) {
        result |= DRED_KEY_STATE_SHIFT_DOWN;
    }
    if ((stateFromGTK & GDK_CONTROL_MASK) != 0) {
        result |= DRED_KEY_STATE_CTRL_DOWN;
    }
    if ((stateFromGTK & GDK_MOD1_MASK) != 0) {
        result |= DRED_KEY_STATE_ALT_DOWN;
    }

    if ((stateFromGTK & GDK_BUTTON1_MASK) != 0) {
        result |= DRED_MOUSE_BUTTON_LEFT_DOWN;
    }
    if ((stateFromGTK & GDK_BUTTON2_MASK) != 0) {
        result |= DRED_MOUSE_BUTTON_MIDDLE_DOWN;
    }
    if ((stateFromGTK & GDK_BUTTON3_MASK) != 0) {
        result |= DRED_MOUSE_BUTTON_RIGHT_DOWN;
    }
    if ((stateFromGTK & GDK_BUTTON4_MASK) != 0) {
        result |= DRED_MOUSE_BUTTON_4_DOWN;
    }
    if ((stateFromGTK & GDK_BUTTON5_MASK) != 0) {
        result |= DRED_MOUSE_BUTTON_5_DOWN;
    }

    return result;
}

static int dred_from_gtk_mouse_button(guint buttonGTK)
{
    switch (buttonGTK) {
        case 1: return DRGUI_MOUSE_BUTTON_LEFT;
        case 2: return DRGUI_MOUSE_BUTTON_MIDDLE;
        case 3: return DRGUI_MOUSE_BUTTON_RIGHT;
        default: return (int)buttonGTK;
    }
}


bool dred_platform_init__gtk()
{
    gtk_init(NULL, NULL);

    g_GTKCursor_Default      = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_LEFT_PTR);
    g_GTKCursor_IBeam        = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_XTERM);
    g_GTKCursor_Cross        = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_CROSSHAIR);
    g_GTKCursor_DoubleArrowH = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_SB_H_DOUBLE_ARROW);
    g_GTKCursor_DoubleArrowV = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_SB_V_DOUBLE_ARROW);

    return true;
}

void dred_platform_uninit__gtk()
{
}

int dred_platform_run__gtk()
{
    gtk_main();
    return g_GTKMainLoopResultCode;
}

void dred_platform_post_quit_message__gtk(int resultCode)
{
    g_GTKMainLoopResultCode = resultCode;
    gtk_main_quit();
}




static gboolean dred_gtk_cb__on_close(GtkWidget* pGTKWindow, GdkEvent* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;
    (void)pEvent;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    dred_window_on_close(pWindow);
    return true;
}

static void dred_gtk_cb__on_hide(GtkWidget* pGTKWindow, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return;
    }

    if ((pWindow->onHideFlags & DRED_HIDE_BLOCKED) != 0) {
        pWindow->onHideFlags &= ~DRED_HIDE_BLOCKED;
        return;
    }

    if (!dred_window_on_hide(pWindow, pWindow->onHideFlags)) {
        dred_window_show(pWindow);    // The event handler returned false, so prevent the window from being hidden.
    }
}

static void dred_gtk_cb__on_show(GtkWidget* pGTKWindow, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return;
    }

    if (!dred_window_on_show(pWindow)) {
        dred_window_hide(pWindow, DRED_HIDE_BLOCKED);    // The event handler returned false, so prevent the window from being shown.
    } else {
        gtk_widget_grab_focus(GTK_WIDGET(pWindow->pGTKWindow));
    }
}

static void dred_gtk_cb__on_paint(GtkWidget* pGTKWindow, cairo_t* pCairoContext, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return;
    }

    // NOTE: Because we are using dr_2d to draw the GUI, the last argument to drgui_draw() must be a pointer
    //       to the relevant dr2d_surface object.

    double clipLeft;
    double clipTop;
    double clipRight;
    double clipBottom;
    cairo_clip_extents(pCairoContext, &clipLeft, &clipTop, &clipRight, &clipBottom);

    drgui_rect drawRect;
    drawRect.left   = (float)clipLeft;
    drawRect.top    = (float)clipTop;
    drawRect.right  = (float)clipRight;
    drawRect.bottom = (float)clipBottom;
    drgui_draw(pWindow->pRootGUIElement, drawRect, pWindow->pDrawingSurface);

    // At this point the GUI has been drawn, however nothing has been drawn to the window yet. To do this we will
    // use cairo directly with a cairo_set_source_surface() / cairo_paint() pair. We can get a pointer to dr_2d's
    // internal cairo_surface_t object as shown below.
    cairo_surface_t* pCairoSurface = dr2d_get_cairo_surface_t(pWindow->pDrawingSurface);
    if (pCairoSurface != NULL)
    {
        cairo_set_source_surface(pCairoContext, pCairoSurface, 0, 0);
        cairo_paint(pCairoContext);
    }
}

static void dred_gtk_cb__on_configure(GtkWidget* pGTKWindow, GdkEventConfigure* pEvent, gpointer pUserData)
{
    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return;
    }

    // If the window's size has changed, it's panel and surface need to be resized, and then redrawn.
    if (pEvent->width != dr2d_get_surface_width(pWindow->pDrawingSurface) || pEvent->height != dr2d_get_surface_height(pWindow->pDrawingSurface))
    {
        // Size has changed.

        // NOTE
        //
        // There's either an issue with GTK or I'm just not understanding something, but when the user resizes the window, for some
        // reason the internal clip used by GTK does not expand (it does shrink, though). The only way I've found to fix this is to
        // forcefully request an explicit resize which seems to cause GTK to correct it's clip. This is a bad hack and at some point
        // we should look at addressing this properly.
        gtk_window_resize(GTK_WINDOW(pWindow->pGTKWindow), pEvent->width, pEvent->height);


        // dr_2d does not support dynamic resizing of surfaces. Thus, we need to delete and recreate it.
        if (pWindow->pDrawingSurface != NULL) {
            dr2d_delete_surface(pWindow->pDrawingSurface);
        }
        pWindow->pDrawingSurface = dr2d_create_surface(pWindow->pDred->pDrawingContext, (float)pEvent->width, (float)pEvent->height);


        // Post the event.
        dred_window_on_size(pWindow, pEvent->width, pEvent->height);

        // Invalidate the window to force a redraw.
        gtk_widget_queue_draw(pGTKWindow);
    }

    pWindow->absoluteClientPosX = (int)pEvent->x;
    pWindow->absoluteClientPosY = (int)pEvent->y;
}

static gboolean dred_gtk_cb__on_mouse_enter(GtkWidget* pGTKWindow, GdkEventCrossing* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;
    (void)pEvent;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    pWindow->isCursorOver = true;
    gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(pWindow->pGTKWindow)), pWindow->pGTKCursor);

    dred_window_on_mouse_enter(pWindow);
    return true;
}

static gboolean dred_gtk_cb__on_mouse_leave(GtkWidget* pGTKWindow, GdkEventCrossing* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;
    (void)pEvent;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    pWindow->isCursorOver = false;

    dred_window_on_mouse_leave(pWindow);
    return true;
}

static gboolean dred_gtk_cb__on_mouse_move(GtkWidget* pGTKWindow, GdkEventMotion* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    dred_window_on_mouse_move(pWindow, pEvent->x, pEvent->y, dred_gtk_get_modifier_state_flags(pEvent->state));
    return false;
}

static gboolean dred_gtk_cb__on_mouse_button_down(GtkWidget* pGTKWindow, GdkEventButton* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    if (pEvent->type == GDK_BUTTON_PRESS) {
        dred_window_on_mouse_button_down(pWindow, dred_from_gtk_mouse_button(pEvent->button), pEvent->x, pEvent->y, dred_gtk_get_modifier_state_flags(pEvent->state));
    } else if (pEvent->type == GDK_2BUTTON_PRESS) {
        dred_window_on_mouse_button_dblclick(pWindow, dred_from_gtk_mouse_button(pEvent->button), pEvent->x, pEvent->y, dred_gtk_get_modifier_state_flags(pEvent->state));
    }

    return true;
}

static gboolean dred_gtk_cb__on_mouse_button_up(GtkWidget* pGTKWindow, GdkEventButton* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    dred_window_on_mouse_button_up(pWindow, dred_from_gtk_mouse_button(pEvent->button), pEvent->x, pEvent->y, dred_gtk_get_modifier_state_flags(pEvent->state));
    return true;
}

static gboolean dred_gtk_cb__on_mouse_wheel(GtkWidget* pGTKWindow, GdkEventScroll* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    gdouble delta_y = 0;
    if (pEvent->direction == GDK_SCROLL_UP) {
        delta_y = -1;
    } else if (pEvent->direction == GDK_SCROLL_DOWN) {
        delta_y = 1;
    }

    dred_window_on_mouse_wheel(pWindow, (int)-delta_y, pEvent->x, pEvent->y, dred_gtk_get_modifier_state_flags(pEvent->state));

    return true;
}

static gboolean dred_gtk_cb__on_key_down(GtkWidget* pGTKWindow, GdkEventKey* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    int stateFlags = dred_gtk_get_modifier_state_flags(pEvent->state);
    // TODO: Check here if key is auto-repeated.

    dred_window_on_key_down(pWindow, dred_gtk_to_drgui_key(pEvent->keyval), stateFlags);

    guint32 utf32 = gdk_keyval_to_unicode(pEvent->keyval);
    if (utf32 == 0) {
        if (pEvent->keyval == GDK_KEY_KP_Enter) {
            utf32 = '\r';
        }
    }

    if (utf32 != 0 && (stateFlags & DRED_KEY_STATE_CTRL_DOWN) == 0 && (stateFlags & DRED_KEY_STATE_ALT_DOWN) == 0) {
        if (!(utf32 < 32 || utf32 == 127) || utf32 == '\t' || utf32 == '\r') {
            dred_window_on_printable_key_down(pWindow, (unsigned int)utf32, stateFlags);
        }
    }

    return true;
}

static gboolean dred_gtk_cb__on_key_up(GtkWidget* pGTKWindow, GdkEventKey* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    dred_window_on_key_up(pWindow, dred_gtk_to_drgui_key(pEvent->keyval), dred_gtk_get_modifier_state_flags(pEvent->state));
    return true;
}

static gboolean dred_gtk_cb__on_receive_focus(GtkWidget* pGTKWindow, GdkEventFocus* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;
    (void)pEvent;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    dred_window_on_focus(pWindow);
    return true;
}

static gboolean dred_gtk_cb__on_lose_focus(GtkWidget* pGTKWindow, GdkEventFocus* pEvent, gpointer pUserData)
{
    (void)pGTKWindow;
    (void)pEvent;

    dred_window* pWindow = pUserData;
    if (pWindow == NULL) {
        return true;
    }

    dred_window_on_unfocus(pWindow);
    return true;
}



dred_window* dred_window_create__gtk(dred_context* pDred)
{
    GtkWidget* pGTKWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (pGTKWindow == NULL) {
        return NULL;
    }

    gtk_window_set_resizable(GTK_WINDOW(pGTKWindow),TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(pGTKWindow), 10);


    dred_window* pWindow = (dred_window*)calloc(1, sizeof(*pWindow));
    if (pWindow == NULL) {
        goto on_error;
    }

    pWindow->pDred = pDred;
    pWindow->pGTKWindow = pGTKWindow;

    pWindow->pRootGUIElement = dred_platform__create_root_gui_element(pDred->pGUI, pWindow);
    if (pWindow->pRootGUIElement == NULL) {
        goto on_error;
    }

    // Because we are drawing the GUI ourselves rather than through GTK, we want to disable GTK's default painting
    // procedure. To do this we use gtk_widget_set_app_paintable(pWindow, TRUE) which will disable GTK's default
    // painting on the main window. We then connect to the "draw" signal which is where we'll do all of our custom
    // painting.
    gtk_widget_set_app_paintable(pGTKWindow, TRUE);

    // These are the types of events we care about.
    gtk_widget_add_events(pGTKWindow,
        GDK_ENTER_NOTIFY_MASK   |
        GDK_LEAVE_NOTIFY_MASK   |
        GDK_POINTER_MOTION_MASK |
        GDK_BUTTON_PRESS_MASK   |
        GDK_BUTTON_RELEASE_MASK |
        GDK_SCROLL_MASK         |
        GDK_KEY_PRESS_MASK      |
        GDK_KEY_RELEASE_MASK    |
        GDK_FOCUS_CHANGE_MASK);

    g_signal_connect(pGTKWindow, "delete-event",         G_CALLBACK(dred_gtk_cb__on_close),             pWindow);     // Close.
    g_signal_connect(pGTKWindow, "hide",                 G_CALLBACK(dred_gtk_cb__on_hide),              pWindow);     // Hide.
    g_signal_connect(pGTKWindow, "show",                 G_CALLBACK(dred_gtk_cb__on_show),              pWindow);     // Show.
    g_signal_connect(pGTKWindow, "draw",                 G_CALLBACK(dred_gtk_cb__on_paint),             pWindow);     // Paint
    g_signal_connect(pGTKWindow, "configure-event",      G_CALLBACK(dred_gtk_cb__on_configure),         pWindow);     // Reposition and resize.
    g_signal_connect(pGTKWindow, "enter-notify-event",   G_CALLBACK(dred_gtk_cb__on_mouse_enter),       pWindow);     // Mouse enter.
    g_signal_connect(pGTKWindow, "leave-notify-event",   G_CALLBACK(dred_gtk_cb__on_mouse_leave),       pWindow);     // Mouse leave.
    g_signal_connect(pGTKWindow, "motion-notify-event",  G_CALLBACK(dred_gtk_cb__on_mouse_move),        pWindow);     // Mouse move.
    g_signal_connect(pGTKWindow, "button-press-event",   G_CALLBACK(dred_gtk_cb__on_mouse_button_down), pWindow);     // Mouse button down.
    g_signal_connect(pGTKWindow, "button-release-event", G_CALLBACK(dred_gtk_cb__on_mouse_button_up),   pWindow);     // Mouse button up.
    g_signal_connect(pGTKWindow, "scroll-event",         G_CALLBACK(dred_gtk_cb__on_mouse_wheel),       pWindow);     // Mouse wheel.
    g_signal_connect(pGTKWindow, "key-press-event",      G_CALLBACK(dred_gtk_cb__on_key_down),          pWindow);     // Key down.
    g_signal_connect(pGTKWindow, "key-release-event",    G_CALLBACK(dred_gtk_cb__on_key_up),            pWindow);     // Key up.
    g_signal_connect(pGTKWindow, "focus-in-event",       G_CALLBACK(dred_gtk_cb__on_receive_focus),     pWindow);     // Receive focus.
    g_signal_connect(pGTKWindow, "focus-out-event",      G_CALLBACK(dred_gtk_cb__on_lose_focus),        pWindow);     // Lose focus.

    return pWindow;

on_error:
    if (pWindow != NULL) {
        if (pWindow->pRootGUIElement) {
            drgui_delete_element(pWindow->pRootGUIElement);
        }

        if (pWindow->pDrawingSurface) {
            dr2d_delete_surface(pWindow->pDrawingSurface);
        }

        free(pWindow);
    }
    
    gtk_widget_destroy(pGTKWindow);
    return NULL;
}

void dred_window_delete__gtk(dred_window* pWindow)
{
    if (pWindow == NULL) {
        return;
    }

    drgui_delete_element(pWindow->pRootGUIElement);
    dr2d_delete_surface(pWindow->pDrawingSurface);
    gtk_widget_destroy(pWindow->pGTKWindow);
    free(pWindow);
}


void dred_window_set_size__gtk(dred_window* pWindow, unsigned int newWidth, unsigned int newHeight)
{
    gtk_window_resize(GTK_WINDOW(pWindow->pGTKWindow), (int)newWidth, (int)newHeight);
}

void dred_window_get_size__gtk(dred_window* pWindow, unsigned int* pWidthOut, unsigned int* pHeightOut)
{
    int width;
    int height;
    gtk_window_get_size(GTK_WINDOW(pWindow->pGTKWindow), &width, &height);

    if (*pWidthOut) *pWidthOut = width;
    if (*pHeightOut) *pHeightOut = height;
}


void dred_window_show__gtk(dred_window* pWindow)
{
    gtk_window_present(GTK_WINDOW(pWindow->pGTKWindow));
}

void dred_window_show_maximized__gtk(dred_window* pWindow)
{
    gtk_window_present(GTK_WINDOW(pWindow->pGTKWindow));    // <-- Is this needed?
    gtk_window_maximize(GTK_WINDOW(pWindow->pGTKWindow));
}

void dred_window_hide__gtk(dred_window* pWindow)
{
    gtk_widget_hide(GTK_WIDGET(pWindow->pGTKWindow));
}


void dred_window_set_cursor__gtk(dred_window* pWindow, dred_cursor_type cursor)
{
    switch (cursor)
    {
        case dred_cursor_type_text:
        {
            pWindow->pGTKCursor = g_GTKCursor_IBeam;
        } break;

        case dred_cursor_type_cross:
        {
            pWindow->pGTKCursor = g_GTKCursor_Cross;
        } break;

        case dred_cursor_type_double_arrow_h:
        {
            pWindow->pGTKCursor = g_GTKCursor_DoubleArrowH;
        } break;

        case dred_cursor_type_double_arrow_v:
        {
            pWindow->pGTKCursor = g_GTKCursor_DoubleArrowH;
        } break;


        case dred_cursor_type_none:
        {
            pWindow->pGTKCursor = NULL;
        } break;

        //case cursor_type_arrow:
        case dred_cursor_type_default:
        default:
        {
            pWindow->pGTKCursor = g_GTKCursor_Default;
        } break;
    }

    // If the cursor is currently inside the window it needs to be changed right now.
    if (dred_window_is_cursor_over(pWindow)) {
        gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(pWindow->pGTKWindow)), pWindow->pGTKCursor);
    }
}

bool dred_window_is_cursor_over__gtk(dred_window* pWindow)
{
    return pWindow->isCursorOver;
}



static void dred_platform__on_global_capture_mouse__gtk(drgui_element* pElement)
{
    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL) {
        //gdk_device_grab(gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_display_get_default())),
        //    gtk_widget_get_window(pWindow->pGTKWindow), GDK_OWNERSHIP_APPLICATION, false, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK, NULL, GDK_CURRENT_TIME);
        gdk_seat_grab(gdk_display_get_default_seat(gdk_display_get_default()),
            gtk_widget_get_window(pWindow->pGTKWindow), GDK_SEAT_CAPABILITY_POINTER, FALSE, NULL, NULL, NULL, NULL);
    }
}

static void dred_platform__on_global_release_mouse__gtk(drgui_element* pElement)
{
    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL) {
        //gdk_device_ungrab(gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_display_get_default())), GDK_CURRENT_TIME);
        gdk_seat_ungrab(gdk_display_get_default_seat(gdk_display_get_default()));
    }
}

static void dred_platform__on_global_capture_keyboard__gtk(drgui_element* pElement, drgui_element* pPrevCapturedElement)
{
    (void)pPrevCapturedElement;

    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL) {
        pWindow->pElementWithKeyboardCapture = pElement;
        gtk_widget_grab_focus(GTK_WIDGET(pWindow->pGTKWindow));
    }
}

static void dred_platform__on_global_release_keyboard__gtk(drgui_element* pElement, drgui_element* pNewCapturedElement)
{
    (void)pNewCapturedElement;

    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL) {
        gtk_widget_grab_focus(NULL);
    }
}

static void dred_platform__on_global_dirty__gtk(drgui_element* pElement, drgui_rect relativeRect)
{
    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow != NULL && pWindow->pGTKWindow != NULL)
    {
        drgui_rect absoluteRect = relativeRect;
        drgui_make_rect_absolute(pElement, &absoluteRect);

        gtk_widget_queue_draw_area(pWindow->pGTKWindow,
            (gint)absoluteRect.left, (gint)absoluteRect.top, (gint)(absoluteRect.right - absoluteRect.left), (gint)(absoluteRect.bottom - absoluteRect.top));
    }
}
#endif




//////////////////////////////////////////////////////////////////
//
// Cross Platform
//
//////////////////////////////////////////////////////////////////

static void dred_platform__on_global_change_cursor(drgui_element* pElement, drgui_cursor_type cursor)
{
    dred_window* pWindow = dred_get_element_window(pElement);
    if (pWindow == NULL) {
        return;
    }

    switch (cursor)
    {
    case drgui_cursor_none:    dred_window_set_cursor(pWindow, dred_cursor_type_none);           break;
    case drgui_cursor_text:    dred_window_set_cursor(pWindow, dred_cursor_type_text);           break;
    case drgui_cursor_cross:   dred_window_set_cursor(pWindow, dred_cursor_type_cross);          break;
    case drgui_cursor_size_ns: dred_window_set_cursor(pWindow, dred_cursor_type_double_arrow_h); break;
    case drgui_cursor_size_we: dred_window_set_cursor(pWindow, dred_cursor_type_double_arrow_v); break;

    case drgui_cursor_default:
    default:
        {
            dred_window_set_cursor(pWindow, dred_cursor_type_default);
        } break;
    }
}


bool dred_platform_init()
{
#ifdef DRED_WIN32
    return dred_platform_init__win32();
#endif

#ifdef DRED_GTK
    return dred_platform_init__gtk();
#endif
}

void dred_platform_uninit()
{
#ifdef DRED_WIN32
    dred_platform_uninit__win32();
#endif

#ifdef DRED_GTK
    dred_platform_uninit__gtk();
#endif
}

int dred_platform_run()
{
#ifdef DRED_WIN32
    return dred_platform_run__win32();
#endif

#ifdef DRED_GTK
    return dred_platform_run__gtk();
#endif
}

void dred_platform_post_quit_message(int resultCode)
{
#ifdef DRED_WIN32
    dred_platform_post_quit_message__win32(resultCode);
#endif

#ifdef DRED_GTK
    dred_platform_post_quit_message__gtk(resultCode);
#endif
}

void dred_platform_bind_gui(drgui_context* pGUI)
{
#ifdef DRED_WIN32
    drgui_set_global_on_capture_mouse(pGUI, dred_platform__on_global_capture_mouse__win32);
    drgui_set_global_on_release_mouse(pGUI, dred_platform__on_global_release_mouse__win32);
    drgui_set_global_on_capture_keyboard(pGUI, dred_platform__on_global_capture_keyboard__win32);
    drgui_set_global_on_release_keyboard(pGUI, dred_platform__on_global_release_keyboard__win32);
    drgui_set_global_on_dirty(pGUI, dred_platform__on_global_dirty__win32);
#endif

#ifdef DRED_GTK
    drgui_set_global_on_capture_mouse(pGUI, dred_platform__on_global_capture_mouse__gtk);
    drgui_set_global_on_release_mouse(pGUI, dred_platform__on_global_release_mouse__gtk);
    drgui_set_global_on_capture_keyboard(pGUI, dred_platform__on_global_capture_keyboard__gtk);
    drgui_set_global_on_release_keyboard(pGUI, dred_platform__on_global_release_keyboard__gtk);
    drgui_set_global_on_dirty(pGUI, dred_platform__on_global_dirty__gtk);
#endif

    drgui_set_global_on_change_cursor(pGUI, dred_platform__on_global_change_cursor);
}




dred_window* dred_window_create(dred_context* pDred)
{
#ifdef DRED_WIN32
    return dred_window_create__win32(pDred);
#endif

#ifdef DRED_GTK
    return dred_window_create__gtk(pDred);
#endif
}

void dred_window_delete(dred_window* pWindow)
{
#ifdef DRED_WIN32
    dred_window_delete__win32(pWindow);
#endif

#ifdef DRED_GTK
    dred_window_delete__gtk(pWindow);
#endif
}


void dred_window_set_size(dred_window* pWindow, unsigned int newWidth, unsigned int newHeight)
{
#ifdef DRED_WIN32
    dred_window_set_size__win32(pWindow, newWidth, newHeight);
#endif

#ifdef DRED_GTK
    dred_window_set_size__gtk(pWindow, newWidth, newHeight);
#endif
}

void dred_window_get_size(dred_window* pWindow, unsigned int* pWidthOut, unsigned int* pHeightOut)
{
#ifdef DRED_WIN32
    dred_window_get_size__win32(pWindow, pWidthOut, pHeightOut);
#endif

#ifdef DRED_GTK
    dred_window_get_size__gtk(pWindow, pWidthOut, pHeightOut);
#endif
}


void dred_window_show(dred_window* pWindow)
{
#ifdef DRED_WIN32
    dred_window_show__win32(pWindow);
#endif

#ifdef DRED_GTK
    dred_window_show__gtk(pWindow);
#endif
}

void dred_window_show_maximized(dred_window* pWindow)
{
#ifdef DRED_WIN32
    dred_window_show_maximized__win32(pWindow);
#endif

#ifdef DRED_GTK
    dred_window_show_maximized__gtk(pWindow);
#endif
}

void dred_window_show_sized(dred_window* pWindow, unsigned int width, unsigned int height)
{
    dred_window_set_size(pWindow, width, height);
    dred_window_show(pWindow);
}

void dred_window_hide(dred_window* pWindow, unsigned int flags)
{
    pWindow->onHideFlags = flags;

#ifdef DRED_WIN32
    dred_window_hide__win32(pWindow);
#endif

#ifdef DRED_GTK
    dred_window_hide__gtk(pWindow);
#endif
}


void dred_window_set_cursor(dred_window* pWindow, dred_cursor_type cursor)
{
#ifdef DRED_WIN32
    dred_window_set_cursor__win32(pWindow, cursor);
#endif

#ifdef DRED_GTK
    dred_window_set_cursor__gtk(pWindow, cursor);
#endif
}

bool dred_window_is_cursor_over(dred_window* pWindow)
{
#ifdef DRED_WIN32
    return dred_window_is_cursor_over__win32(pWindow);
#endif

#ifdef DRED_GTK
    return dred_window_is_cursor_over__gtk(pWindow);
#endif
}


void dred_window_on_close(dred_window* pWindow)
{
    if (pWindow->onClose) {
        pWindow->onClose(pWindow);
    }
}

bool dred_window_on_hide(dred_window* pWindow, unsigned int flags)
{
    if (pWindow->onHide) {
        return pWindow->onHide(pWindow, flags);
    }

    return true;    // Returning true means to process the message as per normal.
}

bool dred_window_on_show(dred_window* pWindow)
{
    if (pWindow->onShow) {
        return pWindow->onShow(pWindow);
    }

    return true;    // Returning true means to process the message as per normal.
}

void dred_window_on_activate(dred_window* pWindow)
{
    if (pWindow->onActivate) {
        pWindow->onActivate(pWindow);
    }
}

void dred_window_on_deactivate(dred_window* pWindow)
{
    if (pWindow->onDeactivate) {
        pWindow->onDeactivate(pWindow);
    }
}

void dred_window_on_size(dred_window* pWindow, unsigned int newWidth, unsigned int newHeight)
{
    if (pWindow->onSize) {
        pWindow->onSize(pWindow, newWidth, newHeight);
    }

    // Always resize the root GUI element so that it's the exact same size as the window.
    drgui_set_size(pWindow->pRootGUIElement, (float)newWidth, (float)newHeight);
}

void dred_window_on_move(dred_window* pWindow, int newPosX, int newPosY)
{
    if (pWindow->onMove) {
        pWindow->onMove(pWindow, newPosX, newPosY);
    }
}

void dred_window_on_mouse_enter(dred_window* pWindow)
{
    if (pWindow->onActivate) {
        pWindow->onActivate(pWindow);
    }
}

void dred_window_on_mouse_leave(dred_window* pWindow)
{
    if (pWindow->onMouseLeave) {
        pWindow->onMouseLeave(pWindow);
    }

    drgui_post_inbound_event_mouse_leave(pWindow->pRootGUIElement);
}

void dred_window_on_mouse_move(dred_window* pWindow, int mousePosX, int mousePosY, unsigned int stateFlags)
{
    if (pWindow->onMouseMove) {
        pWindow->onMouseMove(pWindow, mousePosX, mousePosY, stateFlags);
    }

    drgui_post_inbound_event_mouse_move(pWindow->pRootGUIElement, mousePosX, mousePosY, stateFlags);
}

void dred_window_on_mouse_button_down(dred_window* pWindow, int mouseButton, int mousePosX, int mousePosY, unsigned int stateFlags)
{
    if (pWindow->onMouseButtonDown) {
        pWindow->onMouseButtonDown(pWindow, mouseButton, mousePosX, mousePosY, stateFlags);
    }

    drgui_post_inbound_event_mouse_button_down(pWindow->pRootGUIElement, mouseButton, mousePosX, mousePosY, stateFlags);
}

void dred_window_on_mouse_button_up(dred_window* pWindow, int mouseButton, int mousePosX, int mousePosY, unsigned int stateFlags)
{
    if (pWindow->onMouseButtonUp) {
        pWindow->onMouseButtonUp(pWindow, mouseButton, mousePosX, mousePosY, stateFlags);
    }

    drgui_post_inbound_event_mouse_button_up(pWindow->pRootGUIElement, mouseButton, mousePosX, mousePosY, stateFlags);
}

void dred_window_on_mouse_button_dblclick(dred_window* pWindow, int mouseButton, int mousePosX, int mousePosY, unsigned int stateFlags)
{
    if (pWindow->onMouseButtonDblClick) {
        pWindow->onMouseButtonDblClick(pWindow, mouseButton, mousePosX, mousePosY, stateFlags);
    }

    drgui_post_inbound_event_mouse_button_dblclick(pWindow->pRootGUIElement, mouseButton, mousePosX, mousePosY, stateFlags);
}

void dred_window_on_mouse_wheel(dred_window* pWindow, int delta, int mousePosX, int mousePosY, unsigned int stateFlags)
{
    if (pWindow->onMouseWheel) {
        pWindow->onMouseWheel(pWindow, delta, mousePosX, mousePosY, stateFlags);
    }

    drgui_post_inbound_event_mouse_wheel(pWindow->pRootGUIElement, delta, mousePosX, mousePosY, stateFlags);
}

void dred_window_on_key_down(dred_window* pWindow, drgui_key key, unsigned int stateFlags)
{
    if (pWindow->onKeyDown) {
        pWindow->onKeyDown(pWindow, key, stateFlags);
    }

    if (pWindow->pRootGUIElement) {
        drgui_post_inbound_event_key_down(pWindow->pRootGUIElement->pContext, key, stateFlags);
    }
}

void dred_window_on_key_up(dred_window* pWindow, drgui_key key, unsigned int stateFlags)
{
    if (pWindow->onKeyUp) {
        pWindow->onKeyUp(pWindow, key, stateFlags);
    }

    if (pWindow->pRootGUIElement) {
        drgui_post_inbound_event_key_up(pWindow->pRootGUIElement->pContext, key, stateFlags);
    }
}

void dred_window_on_printable_key_down(dred_window* pWindow, unsigned int character, unsigned int stateFlags)
{
    if (pWindow->onPrintableKeyDown) {
        pWindow->onPrintableKeyDown(pWindow, character, stateFlags);
    }

    if (pWindow->pRootGUIElement) {
        drgui_post_inbound_event_printable_key_down(pWindow->pRootGUIElement->pContext, character, stateFlags);
    }
}

void dred_window_on_focus(dred_window* pWindow)
{
    if (pWindow->onFocus) {
        pWindow->onFocus(pWindow);
    }
}

void dred_window_on_unfocus(dred_window* pWindow)
{
    if (pWindow->onUnfocus) {
        pWindow->onUnfocus(pWindow);
    }
}


dred_window* dred_get_element_window(drgui_element* pElement)
{
    if (pElement == NULL) {
        return NULL;
    }

    dred_window** ppWindow = drgui_get_extra_data(drgui_find_top_level_element(pElement));
    if (ppWindow == NULL) {
        return NULL;
    }

    return *ppWindow;
}