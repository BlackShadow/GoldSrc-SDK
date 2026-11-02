#pragma once

#include "Panel.h"
#include <vector>

namespace vgui2 { class ScrollBar; }

class BuyPresetListBox : public vgui2::Panel
{
	DECLARE_CLASS_SIMPLE_NAMED( BuyPresetListBox, vgui2::Panel, "vgui::Panel" );

public:
	struct dataitem_s
	{
		vgui2::Panel* panel;
		void* userData;
	};

	BuyPresetListBox( vgui2::Panel* parent, const char* panelName );
	~BuyPresetListBox() override;
	virtual int AddItem( vgui2::Panel* panel, void* userData );
	virtual int GetItemCount() const;
	void SwapItems( int first, int second );
	void MakeItemVisible( int index );
	vgui2::Panel* GetItemPanel( int index ) const;
	void* GetItemUserData( int index );
	void SetItemUserData( int index, void* data );
	virtual void RemoveItem( int index );
	virtual void DeleteAllItems();
	void OnSizeChanged( int wide, int tall ) override;
	MESSAGE_FUNC_INT( OnSliderMoved, "ScrollBarSliderMoved", position );
	void OnMouseWheeled( int delta ) override;
	virtual void MoveScrollBarToTop();

protected:
	virtual int computeVPixelsNeeded();
	void PerformLayout() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void OnCommand( const char* command ) override;

private:
	std::vector<dataitem_s> m_items;
	vgui2::ScrollBar* m_vbar;
	vgui2::Panel* m_pPanelEmbedded;
	int m_iScrollbarSize;
	int m_iDefaultHeight;
	int m_iPanelBuffer;
	int m_visibleIndex;
	int m_lastSize;
};
