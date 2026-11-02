#include "common/BaseSystemModule.h"

#include "common/IBaseSystem.h"
#include "HLTV/common/common.h"

#include <string.h>

BaseSystemModule::BaseSystemModule()
{
	m_State = 0;
}

BaseSystemModule::~BaseSystemModule()
{
}

char* BaseSystemModule::GetName()
{
	return m_Name;
}

char* BaseSystemModule::GetType()
{
	return "GenericModule";
}

char* BaseSystemModule::GetStatusLine()
{
	return "No status available.\n";
}

void BaseSystemModule::ExecuteCommand( int, char* )
{
	m_System->DPrintf( "WARNING! Undeclared ExecuteCommand().\n" );
}

int BaseSystemModule::GetVersion()
{
	return COM_BuildNumber();
}

int BaseSystemModule::GetSerial()
{
	return m_Serial;
}

IBaseSystem* BaseSystemModule::GetSystem()
{
	return m_System;
}

bool BaseSystemModule::Init( IBaseSystem* system, int serial, char* name )
{
	if ( system == NULL )
	{
		return false;
	}

	m_System = system;
	m_State = 1;
	m_Serial = serial;
	m_SystemTime = 0.0;

	if ( name != NULL )
	{
		strncpy( m_Name, name, sizeof( m_Name ) - 1 );
		m_Name[sizeof( m_Name ) - 1] = '\0';
	}

	m_Listener.Init();
	return true;
}

void BaseSystemModule::RunFrame( double time )
{
	m_SystemTime = time;
}

void BaseSystemModule::ShutDown()
{
	if ( m_State == 4 )
	{
		return;
	}

	m_Listener.Clear( false );
	m_State = 4;

	if ( !m_System->RemoveModule( this ) )
	{
		m_System->DPrintf( "ERROR! BaseSystemModule::ShutDown: faild to remove module %s.\n", m_Name );
	}
}

void BaseSystemModule::RegisterListener( ISystemModule* module )
{
	ISystemModule* listener;

	listener = (ISystemModule*)m_Listener.GetFirst();
	while ( listener != NULL )
	{
		if ( listener->GetSerial() == module->GetSerial() )
		{
			m_System->DPrintf( "WARNING! BaseSystemModule::RegisterListener: module %s already added.\n",
					module->GetName() );
			return;
		}

		listener = (ISystemModule*)m_Listener.GetNext();
	}

	m_Listener.Add( module );
}

void BaseSystemModule::RemoveListener( ISystemModule* module )
{
	ISystemModule* listener;

	listener = (ISystemModule*)m_Listener.GetFirst();
	while ( listener != NULL )
	{
		if ( listener->GetSerial() == module->GetSerial() )
		{
			m_Listener.Remove( listener );
			return;
		}

		listener = (ISystemModule*)m_Listener.GetNext();
	}
}

void BaseSystemModule::FireSignal( unsigned int signal, void* data )
{
	ISystemModule* listener;

	listener = (ISystemModule*)m_Listener.GetFirst();
	while ( listener != NULL )
	{
		listener->ReceiveSignal( this, signal, data );
		listener = (ISystemModule*)m_Listener.GetNext();
	}
}

void BaseSystemModule::ReceiveSignal( ISystemModule* module, unsigned int signal, void* )
{
	m_System->DPrintf( "WARNING! Unhandled signal (%i) from module %s.\n", signal, module->GetName() );
}

int BaseSystemModule::GetState()
{
	return m_State;
}
