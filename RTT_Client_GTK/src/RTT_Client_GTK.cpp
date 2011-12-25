//============================================================================
// Name        : RTT_Client_GTK.cpp
// Author      : AltF4
// Copyright   : 2011, GNU GPLv3
// Description : GTK Interface to the Client_Core
//============================================================================

#include "ClientProtocolHandler.h"
#include "RTT_Client_GTK.h"
#include "MatchListColumns.h"
#include <iostream>
#include <gtkmm.h>
#include <arpa/inet.h>
#include <vector>

using namespace std;
using namespace Gtk;
using namespace RTT;

Glib::RefPtr<Builder> welcome_builder;

Window *welcome_window = NULL;

//The three Welcome Window panes
Box *welcome_box = NULL;
Box *lobby_box = NULL;
Box *match_lobby_box = NULL;

//First Pane Widgets
Button *button_custom = NULL;
Button *button_connect = NULL;
Statusbar *statusbar = NULL;
Box *box_custom = NULL;
Entry *entry_IP = NULL;
Entry *entry_port = NULL;
Entry *entry_username = NULL;
Entry *entry_password = NULL;

//Second Pane Widgets
Button *quit_server_button = NULL;
Button *create_match_button = NULL;
Button *create_match_submit = NULL;
Button *list_matches_button = NULL;
Entry *match_name_entry = NULL;
ComboBoxText *create_match_map_combo = NULL;
ComboBoxText *max_players_combo = NULL;
CheckButton *set_private_check = NULL;
Statusbar *status_lobby = NULL;
Box *create_match_box = NULL;
Notebook *match_lists = NULL;

//Third Pane Widgets
Button *leave_match_button = NULL;
Statusbar *match_lobby_status = NULL;

int SocketFD = 0;

vector <MatchListColumns*> MatchColumns;

void custom_server_click()
{
	statusbar->push("Set server settings, then hit Connect");
	box_custom->set_visible(true);
}

void connect_click()
{
	statusbar->push("Trying to connect...");

	//A little bit of input validation here
	string serverIP = entry_IP->get_text();

	struct sockaddr_in stSockAddr;
	int Res = inet_pton(AF_INET, serverIP.c_str(), &stSockAddr.sin_addr);
	if (Res == 0)
	{
		statusbar->push("Invalid IP address");
		return;
	}

	char *errString;
	uint serverPort = strtoul(entry_port->get_text().c_str(), &errString, 10);
	if( *errString != '\0' || entry_port->get_text().c_str() == '\0')
	{
		//Error occurred
		statusbar->push("Invalid port number");
		return;
	}

	string username = entry_username->get_text();
	string hashedPassword = entry_password->get_text();

	SocketFD = AuthToServer(serverIP, serverPort,
			username, (unsigned char*)hashedPassword.c_str());

	if( SocketFD > 0 )
	{
		statusbar->push("Connection Successful!");
		LaunchMainLobbyPane();
	}
	else
	{
		statusbar->push("Failed to connect to server");
	}
}

void create_match_submit_click()
{
	string matchName = match_name_entry->get_text();
	if(matchName.size() < 1 && matchName.size() > 20)
	{
		status_lobby->push("Invalid match name length");
		return;
	}

	if(create_match_map_combo->get_active_row_number() == -1)
	{
		status_lobby->push("Please select a map");
		return;
	}
	string mapName = create_match_map_combo->get_active_text();

	int maxPlayers = max_players_combo->get_active_row_number();
	if( maxPlayers == -1)
	{
		status_lobby->push("Please select a maximum number of players");
		return;
	}

	//TODO: Use this once private matches are implemented
	//bool privateMatch = set_private_check->get_active();

	struct MatchOptions options;
	options.maxPlayers = maxPlayers + 2; //+2 since the combo starts at 2

	if (CreateMatch(SocketFD, options) )
	{
		LaunchMatchLobbyPane();
	}
	else
	{
		status_lobby->push("Server returned failure to create match");
		return;
	}
}

void create_match_click()
{
	create_match_box->set_visible(true);
	match_lists->set_visible(false);
}

//Refresh the match list
void list_matches_click()
{
	create_match_box->set_visible(false);
	match_lists->set_visible(true);

	int page = match_lists->get_current_page();

	//If no page is selected, then put us to page 1
	if( page == -1)
	{
		page = 1;
	}

	struct ServerStats stats = GetServerStats(SocketFD);

	//Determine how many pages we need to display these
	//	x / y (rounding up) = (x + y - 1) / y
	uint pagesNeeded = (stats.numMatches + MATCHES_PER_PAGE - 1) / MATCHES_PER_PAGE;
	if( pagesNeeded == 0 )
	{
		//We always want at least one page
		pagesNeeded = 1;
	}

	//If trying to select more than we have, select the last page
	if( page > (int)pagesNeeded-1 )
	{
		page = pagesNeeded-1;
	}

	//Remove all the existing pages
	for(int i = 0; i < match_lists->get_n_pages(); i++)
	{
		match_lists->remove_page();
	}
	//Put the new ones in
	TreeView *view;
	for(uint i = 0; i < pagesNeeded; i++)
	{
		view = manage(new TreeView());
		match_lists->append_page(*view, "worked");
	}

	//Populate the view we're currently selecting
	struct MatchDescription descriptions[MATCHES_PER_PAGE];
	uint numMatchesThisPage = ListMatches(SocketFD, page+1, descriptions);

	match_lists->set_current_page(page);

	MatchListColumns *columns = new MatchListColumns();

	Glib::RefPtr<ListStore> refListStore = ListStore::create(*columns);
	view->set_model(refListStore);

	for(uint i = 0; i < numMatchesThisPage; i++)
	{
		TreeModel::Row row = *(refListStore->append());
		row[columns->matchID] = (int)descriptions[page].ID;
		row[columns->maxPlayers] = (int)descriptions[page].maxPlayers;
	}

	view->append_column("ID", columns->matchID);
	view->append_column("Max Players", columns->maxPlayers);

	match_lists->show_all();
}

//Hides other windows (WelcomeWindow) and shows LobbyWindow
void LaunchMainLobbyPane()
{
	welcome_box->set_visible(false);
	lobby_box->set_visible(true);
	match_lobby_box->set_visible(false);
}

void LaunchServerConnectPane()
{
	welcome_box->set_visible(true);
	lobby_box->set_visible(false);
	match_lobby_box->set_visible(false);
}

void LaunchMatchLobbyPane()
{
	welcome_box->set_visible(false);
	lobby_box->set_visible(false);
	match_lobby_box->set_visible(true);
}

void leave_match_click()
{
	if( LeaveMatch(SocketFD) )
	{
		LaunchMainLobbyPane();
	}
	else
	{
		match_lobby_status->push("Error on server, couldn't leave match");
	}

}

void quit_server_click()
{
	ExitServer(SocketFD);
	LaunchServerConnectPane();
}

void InitGlobalWidgets()
{
	welcome_builder->get_widget("window_welcome", welcome_window);
	welcome_builder->get_widget("button_custom", button_custom);
	welcome_builder->get_widget("button_connect", button_connect);
	welcome_builder->get_widget("status_main", statusbar);
	welcome_builder->get_widget("box_custom", box_custom);
	welcome_builder->get_widget("entry_IP", entry_IP);
	welcome_builder->get_widget("entry_port", entry_port);
	welcome_builder->get_widget("entry_username", entry_username);
	welcome_builder->get_widget("entry_password", entry_password);

	welcome_builder->get_widget("welcome_box", welcome_box);
	welcome_builder->get_widget("lobby_box", lobby_box);
	welcome_builder->get_widget("match_lobby_box", match_lobby_box);

	welcome_builder->get_widget("quit_server_button", quit_server_button);
	welcome_builder->get_widget("create_match_button", create_match_button);
	welcome_builder->get_widget("create_match_submit", create_match_submit);
	welcome_builder->get_widget("list_matches_button", list_matches_button);
	welcome_builder->get_widget("match_name_entry", match_name_entry);
	welcome_builder->get_widget("create_match_map_combo", create_match_map_combo);
	welcome_builder->get_widget("max_players_combo", max_players_combo);
	welcome_builder->get_widget("set_private_check", set_private_check);
	welcome_builder->get_widget("status_lobby", status_lobby);
	welcome_builder->get_widget("create_match_box", create_match_box);
	welcome_builder->get_widget("match_lists", match_lists);

	welcome_builder->get_widget("leave_match_button", leave_match_button);
	welcome_builder->get_widget("match_lobby_status", match_lobby_status);

}

int main( int argc, char **argv)
{
	Main kit(argc, argv);

	//Initialize widgets used in Welcome Window
	welcome_builder = Builder::create_from_file("UI/WelcomeWindow.glade");

	InitGlobalWidgets();

	button_custom->signal_clicked().connect(sigc::ptr_fun(custom_server_click));
	button_connect->signal_clicked().connect(sigc::ptr_fun(connect_click));
	quit_server_button->signal_clicked().connect(sigc::ptr_fun(quit_server_click));
	create_match_button->signal_clicked().connect(sigc::ptr_fun(create_match_click));
	create_match_submit->signal_clicked().connect(sigc::ptr_fun(create_match_submit_click));
	list_matches_button->signal_clicked().connect(sigc::ptr_fun(list_matches_click));

	leave_match_button->signal_clicked().connect(sigc::ptr_fun(leave_match_click));

	Main::run(*welcome_window);
	return EXIT_SUCCESS;
}

