#ifndef DODCLASSMENU_H
#define DODCLASSMENU_H

#include "game_controls/classmenu.h"

namespace vgui2 { class RichText; }

const char* GetClassResourceFile( int menu );

class CDoDClassMenu : public CClassMenu
{
public:
	typedef CClassMenu BaseClass;
	CDoDClassMenu( vgui2::Panel* parent );
	void Paint() override;
	void OnTick() override;
	void Update( int* validClasses, int menu ) override;
	vgui2::Panel* CreateControlByName( const char* controlName ) override;
	vgui2::PanelMap_t* GetPanelMap() override;
	void SetVisibleButton( const char* name, bool visible );
	void SetButtonGreyout( const char* name, bool greyed );
	void UpdateNumClassLabel( int ignored );
	void OnClassPanelActivate( int activePanel );
	bool IsClassFull( int classNumber );
	void SetClassMask( int classMask )
	{
		m_iClassMask = classMask;
	}

	class ClassHelperPanel : public vgui2::EditablePanel
	{
	public:
		ClassHelperPanel( vgui2::Panel* parent, const char* panelName );
		virtual void SetAssociatedButton( vgui2::Panel* button );
		virtual vgui2::Panel* GetAssociatedButton();
		void SetVisible( bool visible ) override;
		vgui2::Panel* CreateControlByName( const char* controlName ) override;
		void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
		const char* GetClassPage( const char* className );
		int GetClassId()
		{
			return m_iClassId;
		}
		void SetClassId( int classId )
		{
			m_iClassId = classId;
		}
	private:
		int m_iClassId;
		vgui2::Panel* m_pAssociatedButton;
		vgui2::RichText* m_pClassInfoText;
	};
private:
	static vgui2::MessageMapItem_t m_MessageMap[2];
	static vgui2::PanelMap_t m_PanelMap;
	int m_iClassMask;
	int m_iFirst;
	int m_iActivePanel;
};

#endif
