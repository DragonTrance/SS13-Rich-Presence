#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <pthread.h>

#include "discord.h"
#include "application.h"

#ifdef _WIN32
#define sleep(X) Sleep(X);
#else
#define sleep(X) usleep(X * 1000);
#endif


//For those who don't know, if you #include <windows.h> (or user32), you can use WinMain as an entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
  // Creating the main application (the window, taskbar tray)
  MainHInstance = hInstance;
  HWND DummyWindow = RichPresenceGUI(SW_HIDE);
  if(!DummyWindow) return 0;
  CreateTrayIcon(DummyWindow, NIM_ADD);

  /// Main threads. Handles everything related to the app, and ensures seamless transitions with its activities
  pthread_create(&ThreadIDs[0], NULL, Thread_DiscordUpdateCallbacks, NULL);
  pthread_create(&ThreadIDs[1], NULL, Thread_WatchWindows, NULL);

  //MAIN WINDOW LOOP
  MSG msg = {0};
  while (!ApplicationWantsToExit && GetMessage(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  CreateTrayIcon(DummyWindow, NIM_DELETE);

  return 0;
}
