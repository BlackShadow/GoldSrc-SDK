#include "CreateCommandMenu.h"
#include "iviewport.h"
#include "ComboBox.h"
#include "Menu.h"
#include "MenuItem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace vgui2;

void CCreateCommandComboMenu::CreateCommandComboMenu( char* menuFile, ComboBox* parent, int playerTeam )
{
	m_iIgnoreDepth = 0;
	m_iIgnoring = 0;
	m_iDepth = 0;
	Menu* menu = CreateCommandMenu( menuFile, playerTeam, parent );
	if ( menu )
	{
		parent->SetMenu( menu );
		menu->AddActionSignalTarget( parent );
	}
}

Menu* CCreateCommandComboMenu::CreateCustomButton( Panel* parent, Menu* menu, char*, char* text,
	char* command, bool )
{
	if ( !strcmp( command, "!CHANGETEAM" ) )
	{
		Menu* submenu = new Menu( parent, "SubMenu" );
		menu->AddCascadingMenuItem( text, parent, submenu, NULL );
		int teams = gViewPortInterface->GetNumberOfTeams();
		for ( int team = 0; team < 4; team++ )
		{
			if ( teams >= team && gViewPortInterface->GetClientDllInterface()->TeamNumber() != team )
			{
				char teamCommand[268];
				sprintf( teamCommand, "jointeam %d", team + 1 );
				submenu->AddMenuItem( gViewPortInterface->GetTeamName( team ), teamCommand, parent, NULL );
			}
		}
		submenu->AddMenuItem( "#Team_AutoAssign", "jointeam 5", parent, NULL );
		submenu->AddMenuItem( "#Menu_Spectate", "spectate", parent, NULL );
	}
	else if ( !strcmp( command, "!CHANGECLASS" ) )
	{
		if ( gViewPortInterface->GetValidClasses( gViewPortInterface->GetClientDllInterface()->TeamNumber() ) != -1 )
		{
			Menu* submenu = new Menu( parent, "SubMenu" );
			menu->AddCascadingMenuItem( text, parent, submenu, NULL );
		}
	}
	else if ( !strcmp( command, "!MAPBRIEFING" ) || !strcmp( command, "!CLASSDESC" ) ||
		!strcmp( command, "!SERVERINFO" ) )
	{
		menu->AddMenuItem( text, command + 1, parent, NULL );
	}
	return menu;
}

Menu* CCreateCommandComboMenu::CreateCommandMenu( char* menuFile, int playerTeam, Panel* parent )
{
	Menu* root = new Menu( parent, menuFile );
	root->SetVisible( false );
	char* file = (char*)gViewPortInterface->GetClientDllInterface()->COM_LoadFile( menuFile, 5, NULL );
	if ( !file )
		return NULL;

	char lastButtonText[32] = "file start";
	char token[1024];
	Menu* menu = root;
	char* data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( file, token );
	while ( token[0] )
	{
		if ( token[0] == '}' )
		{
			if ( menu != root )
			{
				menu = root;
				if ( m_iDepth > 0 )
				{
					m_pMenuStack.Pop( menu );
					m_iDepth--;
				}
			}
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
			continue;
		}

		char text[32] = {};
		char boundKey[32] = {};
		char custom[32] = {};
		char command[128] = {};
		char map[256] = {};
		bool isCustom = false;
		bool toggle = false;
		bool toggleState = false;
		int teamOnly = -1;
		strncpy( custom, token, sizeof( custom ) );
		custom[sizeof( custom ) - 1] = 0;
		if ( !strcmp( custom, "CUSTOM" ) )
		{
			isCustom = true;
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
		}
		else if ( !strcmp( custom, "MAP" ) )
		{
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
			strncpy( map, token, sizeof( map ) );
			map[sizeof( map ) - 1] = 0;
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
		}
		else if ( !strncmp( custom, "TEAM", 4 ) )
		{
			teamOnly = strtol( custom + 4, NULL, 10 );
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
		}
		else if ( !strncmp( custom, "TOGGLE", 6 ) )
		{
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
			toggleState = _strnicmp( token, "true", 4 ) == 0;
			toggle = true;
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
		}
		else if ( IsCustomToken( token ) )
		{
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
			if ( menu != root )
			{
				menu = root;
				if ( m_iDepth > 0 )
				{
					m_pMenuStack.Pop( menu );
					m_iDepth--;
				}
			}
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
			continue;
		}

		strncpy( boundKey, token, sizeof( boundKey ) );
		text[sizeof( text ) - 1] = 0;
		data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
		strncpy( text, token, sizeof( text ) );
		text[sizeof( text ) - 1] = 0;
		strcpy( lastButtonText, text );
		data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
		strncpy( command, token, sizeof( command ) );
		command[sizeof( command ) - 1] = 0;

		if ( isCustom )
		{
			data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
			Menu* customMenu = CreateCustomButton( parent, menu, boundKey, text, command, token[0] == '{' );
			if ( customMenu )
			{
				menu = customMenu;
			}
			else
			{
				m_iIgnoreDepth++;
				while ( m_iIgnoreDepth )
				{
					data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
					if ( token[0] == '{' )
						m_iIgnoreDepth++;
					else if ( token[0] == '}' )
						m_iIgnoreDepth--;
				}
			}
			if ( token[0] == '{' )
				data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
			continue;
		}

		if ( command[0] != '{' )
		{
			if ( teamOnly == -1 )
			{
				if ( toggle )
				{
					menu->AddCheckableMenuItem( text, command, parent, NULL );
					menu->GetMenuItem( menu->GetMenuID( menu->GetItemCount() - 1 ) )->SetChecked( toggleState );
				}
				else
				{
					menu->AddMenuItem( text, command, parent, NULL );
				}
			}
			else if ( teamOnly == playerTeam )
			{
				menu->AddMenuItem( text, command, parent, NULL );
			}
		}
		else
		{
			if ( map[0] )
			{
				const char* level = gViewPortInterface->GetClientDllInterface()->GetLevelName();
				char mapName[256];
				if ( level )
				{
					snprintf( mapName, sizeof( mapName ) - 1, "maps/%s.bsp", map );
					mapName[sizeof( mapName ) - 1] = 0;
					if ( strcmp( mapName, level ) )
					{
						m_iIgnoreDepth++;
						while ( m_iIgnoreDepth )
						{
							data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
							if ( token[0] == '{' )
								m_iIgnoreDepth++;
							else if ( token[0] == '}' )
								m_iIgnoreDepth--;
						}
						data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
						continue;
					}
				}
			}
			Menu* submenu = new Menu( parent, "SubMenu" );
			menu->AddCascadingMenuItem( text, parent, submenu, NULL );
			m_pMenuStack.Push( menu );
			menu = submenu;
			m_iDepth++;
		}
		data = gViewPortInterface->GetClientDllInterface()->COM_ParseFile( data, token );
	}
	gViewPortInterface->GetClientDllInterface()->COM_FreeFile( data );
	return root;
}

bool CCreateCommandComboMenu::IsCustomToken( const char* )
{
	return false;
}
