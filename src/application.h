#ifndef APPLICATION_H_INCLUDED
#define APPLICATION_H_INCLUDED

#include <pthread.h>
#include "../lib/discord_game_sdk.h"

typedef struct {
  DWORD ProcessID;
  HWND WindowHandle;
  WINBOOL WindowVisible;
  char* WindowTitle;
  char* FileName;
  char* FullPath;
} ProcessWindowInstance;

typedef struct {
  char ServerName[128];
  char Address[128];
  bool IsLewd;
} ServerInfo;

#define SERVERSIZE 14
extern ServerInfo ServerNames[SERVERSIZE];


/*** VARIABLES ***/
extern const char* DreamseekerEXE;

extern ProcessWindowInstance* I_Dreamseeker;
extern size_t S_Dreamseeker;

extern HWND ConsoleWindow;
extern HINSTANCE MainHInstance;

extern pthread_t ThreadIDs[4];

extern bool ApplicationWantsToExit;


/*** PROCS ***/

unsigned long strhash(unsigned char *str);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND RichPresenceGUI(int nCmdShow);
void CreateTrayIcon(HWND hwnd, DWORD action);
ProcessWindowInstance HwndToProcessDetails(HWND hwnd);
void ProcessWindowInstance_Free(ProcessWindowInstance* Instance);
WINBOOL EnumProcessWindows(HWND hwnd, LPARAM lparam);
WINBOOL UpdateWindows();
void CreateConsoleWindow();
void* Thread_WatchWindows();
void* Thread_GetMessages();


#endif // APPLICATION_H_INCLUDED
