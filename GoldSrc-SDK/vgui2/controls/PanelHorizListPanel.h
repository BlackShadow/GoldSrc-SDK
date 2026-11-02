#pragma once

#include "Panel.h"
#include "vgui/Dar.h"

namespace vgui2
{
class ScrollBar;

class PanelHorizListPanel : public Panel
{
public:
	PanelHorizListPanel( Panel* parent, const char* panelName );
	~PanelHorizListPanel() override;

	virtual int computeHPixelsNeeded();
	virtual int AddItem( Panel* panel );
	virtual int GetItemCount();
	virtual Panel* GetItem( int itemIndex );
	virtual void RemoveItem( int itemIndex );
	virtual void DeleteAllItems();
	virtual Panel* GetCellRenderer( int row );
	virtual void OnSliderMoved( int position );

	void GetSliderRange( int& low, int& high );
	int GetSliderPosition();
	void SetSliderPosition( int position );
	PanelMap_t* GetPanelMap() override;
	void ApplySchemeSettings( IScheme* scheme ) override;
	Panel* GetEmbedded() { return _embedded; }
	ScrollBar* GetScrollBar() { return _hbar; }
	static PanelMap_t m_PanelMap;

protected:
	typedef struct dataitem_s
	{
		Panel* panel;
	} DATAITEM;

	DATAITEM* GetDataItem( int itemIndex );
	void PerformLayout() override;
	void PaintBackground() override;
	void OnMouseWheeled( int delta ) override;

private:
	static MessageMapItem_t m_MessageMap[1];
	Dar<DATAITEM*>	_dataItems;
	ScrollBar*		_hbar;
	Panel*			_embedded;
	int				m_itemPad;
	int				_tableStartX;
	int				_tableStartY;
};
}
