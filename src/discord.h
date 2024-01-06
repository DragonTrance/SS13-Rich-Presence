#ifndef DISCORD_H_INCLUDED
#define DISCORD_H_INCLUDED

#include "../lib/discord_game_sdk.h"

#define UNKNOWNSERVER "Unknown Server"
#define NOTCONNECTED "Not Connected"

struct Application {
  struct IDiscordCore* DiscordCore;
  struct IDiscordActivityManager* ActivityManager;
  struct DiscordActivity RichPresence;
  struct DiscordActivity LastPresence;
};

extern const int64_t DiscordID;

enum EDiscordResult CreateDiscordInstance(struct Application* restrict app);
enum EDiscordResult UpdateDiscordActivity(struct Application* restrict app);
void UpdateDiscordPresence(struct Application* restrict app);
void DestroyDiscordInstance(struct Application* restrict app);
void* Thread_DiscordUpdateCallbacks();
void DISCORD_CALLBACK DiscordCallback(void* app, enum EDiscordResult result);


#endif // DISCORD_H_INCLUDED
