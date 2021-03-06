//============================================================================
// Name        : CallbackHandler.cpp
// Author      : AltF4
// Copyright   : 2011, GNU GPLv3
// Description : Handles incoming callback messages from game server
//============================================================================

#ifndef CALLBACKHANDLER_H_
#define CALLBACKHANDLER_H_

#include "ClientProtocolHandler.h"
#include "callback/MainLobbyCallbackChange.h"

#include "gtkmm.h"
#include <queue>

namespace RTT
{

class CallbackHandler
{
public:

	CallbackHandler();
	~CallbackHandler();

	bool Start();
	void Stop();

	CallbackChange *PopCallbackChange();

	Glib::Dispatcher m_sig_team_change;
	Glib::Dispatcher m_sig_color_change;
	Glib::Dispatcher m_sig_map_change;
	Glib::Dispatcher m_sig_speed_change;
	Glib::Dispatcher m_sig_victory_cond_change;
	Glib::Dispatcher m_sig_player_left;
	Glib::Dispatcher m_sig_kicked;
	Glib::Dispatcher m_sig_player_joined;
	Glib::Dispatcher m_sig_leader_change;
	Glib::Dispatcher m_sig_match_started;
	Glib::Dispatcher m_sig_callback_closed;
	Glib::Dispatcher m_sig_callback_error;

private:

	void CallbackThread();
	void PushCallbackChange(MainLobbyCallbackChange *change);

	Glib::Thread *m_thread;

	Glib::Mutex m_queueMutex;
	std::queue<MainLobbyCallbackChange*> m_changeQueue;
};

}
#endif /* CALLBACKHANDLER_H_ */
