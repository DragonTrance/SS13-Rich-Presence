#include <stdio.h>
#include <pthread.h>
#include "discord.h"
#include "application.h"

const int64_t DiscordID = 1186488798390009907;

/** Initializes discord, if it does not exist
  * @param app A pointer to the application variable. This is not a newly allocated space and will also not be free'd
  * @returns enum EDiscordResult
  */
enum EDiscordResult CreateDiscordInstance(struct Application* restrict app)
{
  if(!app) return DiscordResult_Ok;               //Pointer is null, cannot write to its memory
  if(app->DiscordCore) return DiscordResult_Ok;   //Discord already initialized
  memset(app, 0, sizeof(struct Application));     //Discord not initialized, set the pointer memory to zero to reset just in case

  // Initialize parameters for discord
  ///TODO: Events can be set here!
  struct DiscordCreateParams discordParams;
  DiscordCreateParamsSetDefault(&discordParams);
  discordParams.client_id = DiscordID;
  discordParams.flags = DiscordCreateFlags_Default;

  //Create the actual discord stuff, let the api do junk in the background
  enum EDiscordResult d_code = DiscordCreate(DISCORD_VERSION, &discordParams, &app->DiscordCore);
  if(d_code != DiscordResult_Ok) {
    printf("Failed to create a new RP instance (error %d).\n", d_code);
    app->DiscordCore->destroy(app->DiscordCore);
    return d_code;
  }

  app->ActivityManager = app->DiscordCore->get_activity_manager(app->DiscordCore);
  app->RichPresence.supported_platforms = PlatformRoleDesktop;

  memcpy(&app->LastPresence, &app->RichPresence, sizeof(struct DiscordActivity));
  return DiscordResult_Ok;
}

/** UpdateDiscordActivity()
  * Iterates through every dreamseeker instance and matches their window title against the ServerNames variable.
  * After finding a valid window, RichPresence is set, but not updated.
  * @param app A pointer to the application variable. This is not a newly allocated space and will also not be free'd
  * @returns enum EDiscordResult
  */
enum EDiscordResult UpdateDiscordActivity(struct Application* restrict app)
{
  //If no dreamseeker instances are cached in our program, make sure that we turn off our rich presence.
  //It is done this way, because discord does not actually clear the rich presence until it has been free'd from memory
  if(!S_Dreamseeker) {
    DestroyDiscordInstance(app);
    return DiscordResult_Ok;
  }

  //Ensure that the discord API is currently running. If not, this function creates it, initializing data into the variable
  enum EDiscordResult result = CreateDiscordInstance(app);
  if(result != DiscordResult_Ok)
    return result;

  // Too many servers to display in activity
  if(S_Dreamseeker > 1) {
    sprintf(app->RichPresence.details, "Connected to %zu Servers", S_Dreamseeker);
  }
  // Displays something like "Connected to /tg/station 13"
  else {
    //Check the server name and set it if found. Otherwise, defaults to Unknown Server
    char ConnectedServer[128] = UNKNOWNSERVER;
    ProcessWindowInstance Instance = I_Dreamseeker[0];
    for(size_t i = 0; i < SERVERSIZE; i++) {
      ServerInfo ServerNameCheck = ServerNames[i];
      if(strstr(Instance.WindowTitle, ServerNameCheck.ServerName)) {
        strcpy(ConnectedServer, ServerNameCheck.ServerName);
        ///TODO: Make hash secret here that enables a "Join" button on the user's profile that leads discord users to a byond link to connect them
        break;
      }
    }

    //setting the discord status
    sprintf(app->RichPresence.details, "Connected to %s", ConnectedServer);
  }

  ///TODO: move this into discord instance creation?
  if(!app->RichPresence.timestamps.start)
    app->RichPresence.timestamps.start = time(NULL);

  UpdateDiscordPresence(app);

  return DiscordResult_Ok;
}

///Sends update packets to discord, letting their servers know what we want our new status to be.
///Does not send new informtion if certain things are the same as last time
void UpdateDiscordPresence(struct Application* restrict app)
{
  if(!app) return;
  if(!app->DiscordCore) return;
  if(!strcmp(app->RichPresence.details, app->LastPresence.details) && !strcmp(app->RichPresence.state, app->LastPresence.state))
    return;

  app->ActivityManager->update_activity(app->ActivityManager, &app->RichPresence, NULL, DiscordCallback);
  memcpy(&app->LastPresence, &app->RichPresence, sizeof(struct DiscordActivity));
}

//Removes everything and cleans up, so the status shows up empty on discord
void DestroyDiscordInstance(struct Application* restrict app)
{
  if(!app) return;

  if(app->DiscordCore)
    app->DiscordCore->destroy(app->DiscordCore);
  app->DiscordCore = NULL;
  app->ActivityManager = NULL;
}

//Handle callbacks that need to happen
void* Thread_DiscordUpdateCallbacks()
{
  struct Application app = {0};
  while(!ApplicationWantsToExit) {
    Sleep(10);
    if(ApplicationWantsToExit) continue;

    UpdateDiscordActivity(&app);

    if(app.DiscordCore) {
      enum EDiscordResult DiscordResult = app.DiscordCore->run_callbacks(app.DiscordCore);
      if(DiscordResult != DiscordResult_Ok) {
        printf("Recieved error code %d (ignored).\n", DiscordResult);
      }
    }
  }
  pthread_exit(NULL);
  return NULL;
}

void DISCORD_CALLBACK DiscordCallback(void* app, enum EDiscordResult result)
{
  if(result == DiscordResult_Ok) return;
  printf("Callback Error: %X\n", result);
}

void DISCORD_CALLBACK OnActivityJoinRequest(struct Application* restrict app)
{
  if(!app) return;
}
