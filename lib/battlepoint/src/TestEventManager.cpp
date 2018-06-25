#include <EventManagers.h>
#include <Teams.h>
 
void TestEventManager::control_point_being_captured(Team team){};
void TestEventManager::control_point_captured(Team team){};
void TestEventManager::control_point_contested(){};
void TestEventManager::starting_game(){};
void TestEventManager::game_started(){};
void TestEventManager::victory(Team team){};
void TestEventManager::overtime(){};
void TestEventManager::ends_in_seconds(int secs){};
void TestEventManager::starts_in_seconds(int secs){};
void TestEventManager::cancelled(){};
