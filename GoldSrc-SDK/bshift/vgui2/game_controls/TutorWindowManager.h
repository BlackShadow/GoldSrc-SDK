#pragma once

class CTutorTextWindow;
class CTutorStateWindow;
struct cl_entity_s;

enum CreateTutorWindowReturnType
{
	TUTOR_WINDOW_CREATED = 2,
	TUTOR_WINDOW_FAILED
};

class TutorWindowManager
{
public:
	TutorWindowManager();
	virtual ~TutorWindowManager();
	void Think( float time );
	void CloseAllWindows();
	void CloseTutorTextWindow();
	void NewTutorWindow( const wchar_t* message, int id, int msgClass, int isSpectator );
	void NewTutorLine( int entindex, int id );
	void NewTutorState( const wchar_t* string );
	CreateTutorWindowReturnType CreateNewWindow( int id );
	void PostWindowCreation( int id );
	void DrawLineFromWindowToEntity();
	bool IsTutorTextWindowOpen() const { return m_currentWindow != 0; }
	void InsertPlayerNames( wchar_t* string, int stringLength );

private:
	int					m_hardwareMode;
	CTutorTextWindow*	m_currentWindow;
	bool				m_animating;
	int					m_currentMessageID;
	cl_entity_s*		m_currentLineEntity;
	int					m_lineAttachedToThisMessageID;
	CTutorStateWindow*	m_currentStateWindow;
};
