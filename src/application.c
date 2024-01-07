#include "application.h"
#include <stdio.h>
#include <psapi.h>
#include <pthread.h>

const char* DreamseekerEXE = "dreamseeker.exe";

ProcessWindowInstance* I_Dreamseeker = NULL;
size_t S_Dreamseeker = 0;

HWND ConsoleWindow = 0;
HINSTANCE MainHInstance = 0;

pthread_t ThreadIDs[4];

bool ApplicationWantsToExit = false;


ServerInfo ServerNames[SERVERSIZE] = {
  {"Skyrat", "byond://skyrat13.space:22094", true},
  {"/tg/Station", "https://tgstation13.org/", false}, //TODO: make aliases, this server name has a space in the middle before it initializes
  {"Hyper Station", "byond://172.93.98.2:8058", true},
  {"Paradise Station", "byond://byond.paradisestation.org:6666", true},
  {"Bubberstation", "byond://46.4.35.160:3000", true},
  {"S.P.L.U.R.T.", "byond://64.20.39.142:42069", true},
  {"MonkeStation", "byond://209.222.101.195:1337", false},
  {"TerraGov Marine Corps", "byond://tgmc.tgstation13.org:5337", false},
  {"Goonstation", "https://goonhub.com", false},
  {"Aurora Station", "byond://136.243.0.189:1234", true},
  {"Citadel Station", "byond://ss13.citadel-station.net:44150", true},
  {"Burgerstation", "byond://64.20.37.50:1234", false},
  {"Outpost R505", "byond://92.223.27.168:4000", true},
  {"CM-SS13", "byond://51.222.10.115:1400", false}
};

unsigned long strhash(unsigned char *str)
{
  unsigned long hash = 5381;
  int c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash)  + c;
  }
  return hash;
}

/** WindowProc
  * Shrimply handles messages sent to this process, given to by the operating system.
  * @see https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues
  */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  //Recieved when something gets clicked on the popup menu
  case WM_COMMAND:
    int wmId = LOWORD(wParam);
    //int wmEvent = HIWORD(wParam);
    switch (wmId)
    {
    case 'X':
      DestroyWindow(hwnd);
      return 0;
    }
    break;
  case WM_CLOSE:
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    ApplicationWantsToExit = true;
    return 0;
  //Tray icon was interacted with
  case WM_USER + 1:
    if (lParam != WM_RBUTTONUP) break;

    HMENU pmenu = CreatePopupMenu();
    if(!pmenu) return 0;
    AppendMenu(pmenu, MF_STRING, 'X', "Exit");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(pmenu, 0, pt.x, pt.y, 0, hwnd, NULL);
    SendMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(pmenu);
    break;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/** RichPresenceGUI
  * The main application GUI handle, which is mainly used for application settings.
  * Closing this window does not exit the main application. The HWND for the window will still exist,
  * however there will always be a tray icon that the user can access,
  * which he can then use to exit and end the process.
  * @param nCmdShow The state the window will be in after it is created. SW_HIDE will disable it from being viewed.
  * @see https://learn.microsoft.com/en-us/windows/win32/winmsg/using-windows
  * @see for nCmdShow, https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-showwindow
  */
HWND RichPresenceGUI(int nCmdShow)
{
  const char RP_CLASS[] = "SS13 Rich Presence";

  WNDCLASS wc = {};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = MainHInstance;
  wc.lpszClassName = RP_CLASS;

  RegisterClass(&wc);

  HWND hwnd = CreateWindowEx(
    0,
    RP_CLASS,
    "SS13 Rich Presence",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    NULL,
    NULL,
    MainHInstance,
    NULL
  );

  if (!hwnd) return 0;
  ShowWindow(hwnd, nCmdShow);
  return hwnd;
}

/** CreateTrayIcon
  * Adds the application into the tray, allowing the main UI to be reloaded from here, or have the application be exited
  * @param hwnd The window handle the tray icon belongs to. Cannot be null, as the tray icon acts as if it's part of the window
  * @param action The action to tell the shell what to do with this. Can be NIM_ADD or NIM_DELETE
  */
void CreateTrayIcon(HWND hwnd, DWORD action)
{
  NOTIFYICONDATA nid = {};
  nid.cbSize = sizeof(NOTIFYICONDATA);
  nid.hWnd = hwnd;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_USER+1;
  strcpy(nid.szTip, "SS13 Rich Presence");
  Shell_NotifyIcon(action, &nid);
}

/** HWNDToProcessDetails
  * Given a window handle as an argument, fabricates a new ProcessWindowInstance.
  * From this instance, we compare various values in it.
  * Most values are not needed, but because of how windows works, it could be
  * more useful to include these unneeded variables in case we need them in the future.
  * @note You must free the instance with ProcessWindowInstance_Free() (the instance itself is not allocated, only the variables)
  * @param hwnd The window handle to access from
  * @returns ProcessWindowInstance
  */
ProcessWindowInstance HwndToProcessDetails(HWND hwnd)
{
  //Process ID to open
  DWORD ProcID;
  GetWindowThreadProcessId(hwnd, &ProcID);
  HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, ProcID);

  //Get the full file path of the process, then close (why do we need to close? it just works)
  char PathBuffer[MAX_PATH];
  GetModuleFileNameEx(hProc, NULL, PathBuffer, MAX_PATH);
  CloseHandle(hProc);

  //Trunctate the path, and use this to just retrieve the filename (allocates memory)
  char* exepath_buffer = strrchr(PathBuffer, '\\');
  char* ExecutableName = NULL;
  if(exepath_buffer != NULL)
    ExecutableName = strdup(exepath_buffer+sizeof(char)); //Extending one character because strrchr returns the \ part of the path

  //Gets the window title, allocates new memory
  char TitleBuffer[MAX_PATH*2];
  GetWindowText(hwnd, TitleBuffer, MAX_PATH*2);
  char* WindowTitle = strdup(TitleBuffer);

  //Returning the new instance :)
  ProcessWindowInstance CurrentInstance;
  CurrentInstance.ProcessID = ProcID;
  CurrentInstance.WindowHandle = hwnd;
  CurrentInstance.WindowVisible = IsWindowVisible(hwnd);
  CurrentInstance.WindowTitle = WindowTitle;
  CurrentInstance.FileName = ExecutableName;
  CurrentInstance.FullPath = strdup(PathBuffer);
  return CurrentInstance;
}

void ProcessWindowInstance_Free(ProcessWindowInstance* Instance)
{
  if(!Instance) return;
  free(Instance->WindowTitle);
  free(Instance->FileName);
  free(Instance->FullPath);
}

/** EnumProcessWindos
  * A callback given to the operating system, letting our application iterate through every window on the computer,
  * and then deciding whether or not the window belongs to Dreamseeker.
  * If a valid window is found, a new entry is put into I_Dreamseeker
  * @return Will always return TRUE, which tells windows to continue to the next window handle
  */
WINBOOL EnumProcessWindows(HWND hwnd, LPARAM lparam)
{
  ProcessWindowInstance CurrentInstance = HwndToProcessDetails(hwnd);

  //If this window is hidden, or if there is no title length
  if(!CurrentInstance.WindowVisible || !strlen(CurrentInstance.WindowTitle)) {
    ProcessWindowInstance_Free(&CurrentInstance);
    return true;
  }

  //First, check if there's already a process with the same ID in the list of instances we have
  //This is slow, I know...
  for(size_t i=0; i<S_Dreamseeker; i++) {
    if(I_Dreamseeker[i].ProcessID == CurrentInstance.ProcessID) {
      //Ignore this process, matches an existing process
      ProcessWindowInstance_Free(&CurrentInstance);
      return true;
    }
  }

  //This is not a valid title, since these are additional windows that DS may have opened (usually they're not visible)
  if(  !strcmp(CurrentInstance.WindowTitle, "Default IME") ||
    !strcmp(CurrentInstance.WindowTitle, "Options and Messages") ||
    !strcmp(CurrentInstance.WindowTitle, "BYOND: Your Game Is Starting") ||
    !strcmp(CurrentInstance.WindowTitle, "DIEmWin") ||
    !strcmp(CurrentInstance.WindowTitle, "MSCTFIME UI")) {
    ProcessWindowInstance_Free(&CurrentInstance);
    return true;
  }


  //Compare the file name with "dreamseeker.exe"
  if (!strcmp(CurrentInstance.FileName, DreamseekerEXE)) {
    I_Dreamseeker = realloc(I_Dreamseeker, sizeof(ProcessWindowInstance) * ++S_Dreamseeker);
    I_Dreamseeker[S_Dreamseeker-1] = CurrentInstance;
  } else {
    ProcessWindowInstance_Free(&CurrentInstance);
  }

  return true;
}

/// Primarily called by Thread_WatchWindows, and just handles the loading dreamseeker windows.
WINBOOL UpdateWindows()
{
  //First, iterate through the list of windows that we've already caught as valid servers
  //This is so that in the event the server's connection halts, your status wouldn't go away, since the title is now zero characters long
  if(S_Dreamseeker) {
    for(size_t i=0; i < S_Dreamseeker; i++) {
      ProcessWindowInstance Instance = I_Dreamseeker[i];
      if(!IsWindow(Instance.WindowHandle) || !IsWindowVisible(Instance.WindowHandle)) {
        ProcessWindowInstance_Free(&Instance);
        if(!--S_Dreamseeker)
          break;
        I_Dreamseeker = realloc(I_Dreamseeker, sizeof(ProcessWindowInstance) * S_Dreamseeker);
      }
    }
  }

  //Freeing up memory if no other dreamseeker instances exist
  if (!S_Dreamseeker) {
    free(I_Dreamseeker);
    I_Dreamseeker = NULL;
    S_Dreamseeker = 0;
  }

  WINBOOL result = EnumWindows(EnumProcessWindows, 0);
  return result;
}

void CreateConsoleWindow()
{
  AllocConsole();
  freopen("CONOUT$", "w", stdout);
  SetConsoleTitle("SS13 Rich Presence");
  ConsoleWindow = GetConsoleWindow();
}

void* Thread_WatchWindows()
{
  while(!ApplicationWantsToExit) {
    Sleep(400);
    if(ApplicationWantsToExit) return NULL;

    UpdateWindows();
  }

  pthread_exit(NULL);
  return NULL;
}
