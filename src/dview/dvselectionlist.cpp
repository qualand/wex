#include <wx/dcbuffer.h>

#include "wex/dview/dvselectionlist.h"


DEFINE_EVENT_TYPE(wxEVT_DVSELECTIONLIST)
	
BEGIN_EVENT_TABLE( wxDVSelectionListCtrl, wxScrolledWindow )
	EVT_SIZE(              wxDVSelectionListCtrl::OnResize    )
	EVT_ERASE_BACKGROUND(  wxDVSelectionListCtrl::OnErase     )
	EVT_PAINT(             wxDVSelectionListCtrl::OnPaint     )
	EVT_LEFT_DOWN(         wxDVSelectionListCtrl::OnLeftDown  )
	EVT_MOTION(            wxDVSelectionListCtrl::OnMouseMove )
	EVT_LEAVE_WINDOW(      wxDVSelectionListCtrl::OnLeave     )
END_EVENT_TABLE()

wxDVSelectionListCtrl::wxDVSelectionListCtrl( wxWindow* parent, wxWindowID id, 
	int num_cols, bool radio_first_col,
	const wxPoint& pos, const wxSize& size )
	: wxScrolledWindow( parent, id, pos, size, wxCLIP_CHILDREN )
{
	SetBackgroundStyle(::wxBG_STYLE_CUSTOM );
	
	if (num_cols < 1) num_cols = 1;
	if (num_cols >= NMAXCOLS) num_cols = NMAXCOLS-1;

	m_numCols = num_cols;
	m_radioFirstCol = radio_first_col;

	m_lastEventRow = 0;
	m_lastEventCol = 0;
	m_lastEventValue = false;

	m_bestSize = wxSize(150, 500); // updated by Invalidate()
}

wxDVSelectionListCtrl::~wxDVSelectionListCtrl()
{
	FreeRowItems();
}

void wxDVSelectionListCtrl::FreeRowItems()
{
	for ( std::vector<row_item*>::iterator it = m_itemList.begin();
		it != m_itemList.end();
		++it )
		delete *it;

	m_itemList.clear();
	m_orderedItems.clear();
}

int wxDVSelectionListCtrl::Append(const wxString& name, const wxString& group)
{
	row_item *x = new row_item;
	x->label = name;
	x->group = group;
	for (int i=0;i<NMAXCOLS;i++)
	{
		x->value[i] = false;
		x->enable[i] = true;
	}

	m_itemList.push_back( x );
	x->row_index = m_itemList.size() - 1;

	Organize();
	Invalidate();
	return m_itemList.size()-1;
}

void wxDVSelectionListCtrl::RemoveAt(int row)
{
	if (row < 0 || row >= (int) m_itemList.size()) return;
	delete m_itemList[ row ];
	m_itemList.erase( m_itemList.begin() + (size_t)row );
	
	DeAssignLineColour( row );
	Organize();
	Invalidate();
}

void wxDVSelectionListCtrl::RemoveAll()
{
	DeAssignAll();
	FreeRowItems();
	m_orderedItems.clear();
	Invalidate();
}

int wxDVSelectionListCtrl::Length()
{
	return (int)m_itemList.size();
}

void wxDVSelectionListCtrl::ClearColumn(int col)
{
	if ( col < 0 || col >= NMAXCOLS || col >= m_numCols  ) return;

	for (size_t i=0;i<m_itemList.size();i++)
	{
		m_itemList[i]->value[ col ] = false;
		HandleLineColour(i);
	}

	Refresh();
}

int wxDVSelectionListCtrl::SelectRowWithNameInCol(const wxString& name, int col)
{
	if ( col < 0 || col >= NMAXCOLS || col >= m_numCols ) return -1;

	for (size_t i=0;i<m_itemList.size();i++)
	{
		if ( m_itemList[i]->label == name )
		{
			m_itemList[i]->value[ col ] = true;
			
			HandleRadio( i, col );
			HandleLineColour(i);
			Refresh();
			return i;
		}
	}

	return -1;
}

void wxDVSelectionListCtrl::SelectRowInCol(int row, int col)
{
	if ( col < 0 || col >= NMAXCOLS || col >= m_numCols ) return;
	if ( row < 0 || row >= m_itemList.size() ) return;

	m_itemList[row]->value[ col ] = true;
	
	HandleRadio(row,col);	
	HandleLineColour(row);
	Refresh();
}

void wxDVSelectionListCtrl::Enable(int row, int col, bool enable)
{
	if ( col < 0 || col >= NMAXCOLS || col >= m_numCols ) return;
	if ( row < 0 || row >= m_itemList.size() ) return;

	if (!enable) m_itemList[row]->value[ col ] = false;
	m_itemList[row]->enable[col] = enable;
	Refresh();
}

bool wxDVSelectionListCtrl::IsRowSelected(int row, int start_col)
{
	if ( row < 0 || row >= m_itemList.size() ) return false;

	for (int i=start_col;i<NMAXCOLS;i++)
		if ( m_itemList[row]->value[i] )
			return true;

	return false;
}

bool wxDVSelectionListCtrl::IsSelected(int row, int col)
{
	if ( col < 0 || col >= NMAXCOLS || col >= m_numCols ) return false;
	if ( row < 0 || row >= m_itemList.size() ) return false;

	return m_itemList[row]->value[col];
}

void wxDVSelectionListCtrl::GetLastEventInfo(int* row, int* col, bool* isNowChecked)
{
	if (row) *row = m_lastEventRow;
	if (col) *col = m_lastEventCol;
	if (isNowChecked) *isNowChecked = m_lastEventValue;
}

wxString wxDVSelectionListCtrl::GetRowLabel(int row)
{
	if ( row < 0 || row >= m_itemList.size() ) return wxEmptyString;
	return m_itemList[row]->label;
}

wxString wxDVSelectionListCtrl::GetSelectedNamesInCol(int col)
{
	//Return list of names from col that are selected.
	wxString names;
	for ( int i=0; i<Length(); i++ )
	{
		if (IsSelected(i, col)) //Always col 0.  Top graph.
		{
			names += GetRowLabel(i);
			names += ';'; //Can't be same as used to separate keys/values in settings.
		}
	}
	return names;
}

std::vector<int> wxDVSelectionListCtrl::GetSelectionsInCol( int col )
{
	std::vector<int> list;
	size_t len = m_itemList.size();
	if ( len > 0 )
	{
		list.reserve( len/2 );
		for( int i=0; i<len; i++ )
			if ( IsSelected( i, col ) )
				list.push_back( i );
	}
	return list;
}

int wxDVSelectionListCtrl::GetNumSelected( int col )
{
	int count = 0;
	size_t len = m_itemList.size();
	for( int i=0; i<len; i++ )
		if ( IsSelected( i, col ) )
			count++;
	return count;
}

void wxDVSelectionListCtrl::Organize()
{
	m_orderedItems.clear();
	if (m_itemList.size() < 1) return;

	m_orderedItems.reserve( m_itemList.size() );

	wxArrayString groups;
	for (size_t i=0;i<m_itemList.size();i++)
		if ( !m_itemList[i]->group.IsEmpty() 
			&& groups.Index( m_itemList[i]->group ) == wxNOT_FOUND)
			groups.Add( m_itemList[i]->group );

	for (size_t i=0;i<groups.Count();i++)
	{
		for (size_t k=0;k<m_itemList.size();k++)
			if ( m_itemList[k]->group == groups[i] )
				m_orderedItems.push_back( m_itemList[k] );
	}

	for (size_t i=0;i<m_itemList.size();i++)
		if ( m_itemList[i]->group.IsEmpty() )
			m_orderedItems.push_back( m_itemList[i] );
}

void wxDVSelectionListCtrl::RecalculateBestSize()
{
	wxClientDC dc(this);
	dc.SetFont( *wxNORMAL_FONT );

	// calculate desired geometry here
	int width = 0;
	int height = m_itemHeight;
	wxString last_group;
	for (size_t i=0;i<m_orderedItems.size();i++)
	{
		height += m_itemHeight; // each one is 15 units high
		if (last_group != m_orderedItems[i]->group)
		{
			wxSize sz = dc.GetTextExtent( m_orderedItems[i]->group );
			if (sz.GetWidth() > width)
				width = sz.GetWidth();

			height += m_itemHeight; // additional height for group label
			last_group = m_orderedItems[i]->group;
		}

		wxSize sz = dc.GetTextExtent( m_orderedItems[i]->label );
		if (sz.GetWidth() > width)
			width = sz.GetWidth();
	}
	
	width += 4*m_xOffset + m_numCols*(m_boxSize+3);
	m_bestSize.Set( width, height );
}

void wxDVSelectionListCtrl::Invalidate()
{
	RecalculateBestSize();
	ResetScrollbars();
	InvalidateBestSize();
}

void wxDVSelectionListCtrl::ResetScrollbars()
{
	int hpos, vpos;
	GetViewStart( &hpos, &vpos );
	SetScrollbars(1,1,m_bestSize.GetWidth(),m_bestSize.GetHeight(),hpos,vpos);	
	InvalidateBestSize();
}

wxSize wxDVSelectionListCtrl::DoGetBestSize() const
{
	return m_bestSize;
}


void wxDVSelectionListCtrl::OnResize(wxSizeEvent &evt)
{
	ResetScrollbars();
}

void wxDVSelectionListCtrl::OnErase(wxEraseEvent &evt)
{
	/* nothing to do */
}

void wxDVSelectionListCtrl::OnPaint(wxPaintEvent &evt)
{
	wxAutoBufferedPaintDC dc(this);
	DoPrepareDC(dc);

	int cwidth = 0, cheight = 0;
	GetClientSize( &cwidth, &cheight );
	
	// paint the background.
	wxColour bg = GetBackgroundColour();
	dc.SetBrush(wxBrush(bg));
	dc.SetPen(wxPen(bg,1));
	wxRect windowRect( wxPoint(0,0), GetClientSize() );
	CalcUnscrolledPosition(windowRect.x, windowRect.y,
		&windowRect.x, &windowRect.y);
	dc.DrawRectangle(windowRect);

	wxFont font_normal = *wxNORMAL_FONT;
	wxFont font_bold = *wxNORMAL_FONT;
	font_bold.SetWeight( wxFONTWEIGHT_BOLD );

	//font_normal.SetPointSize( font_normal.GetPointSize() - 1 );
	//font_bold.SetPointSize( font_bold.GetPointSize() - 1 );
	
	int y = m_yOffset;
	wxString last_group;
	for (size_t i=0;i<m_orderedItems.size();i++)
	{
		if ( last_group != m_orderedItems[i]->group )
		{
			dc.SetFont( font_bold );
			dc.DrawText( m_orderedItems[i]->group, m_xOffset/3, 
				y + m_itemHeight/2-dc.GetCharHeight()/2-1 );
			y += m_itemHeight;
			last_group = m_orderedItems[i]->group;
		}

		int x = m_xOffset;
		int yoff = (m_itemHeight-m_boxSize)/2;
		
		if ( IsRowSelected(m_orderedItems[i]->row_index, m_radioFirstCol ? 1 : 0) )
		{
			dc.SetBrush( wxBrush( m_orderedItems[i]->color, wxSOLID ) );
			dc.DrawRectangle( m_xOffset-4, 
				y+1, 
				m_numCols*m_boxSize + (m_numCols-1)*yoff+8, 
				m_itemHeight-2 );
		}

		for ( size_t c=0; c<(size_t)m_numCols; c++ )
		{
			wxColour color =  m_orderedItems[i]->enable[ c ] ? *wxBLACK : *wxLIGHT_GREY;
			m_orderedItems[i]->geom[c] = wxRect( x, y+yoff, m_boxSize, m_boxSize ); // save geometry to speed up mouse clicks
			
			dc.SetBrush( wxBrush( color, wxSOLID ) );
			dc.DrawRectangle( x, y+yoff, m_boxSize, m_boxSize );
			
			dc.SetBrush( wxBrush( *wxWHITE, m_orderedItems[i]->value[ c ] ? wxTRANSPARENT : wxSOLID ) );
			dc.DrawRectangle( x+2, y+yoff+2, m_boxSize-4, m_boxSize-4 );

			x += m_boxSize + yoff;
		}

		dc.SetFont( font_normal );
		dc.SetTextForeground( *wxBLACK );
		dc.DrawText( m_orderedItems[i]->label, x+2, y + m_itemHeight/2-dc.GetCharHeight()/2-1 );

		y += m_itemHeight;
	}
}

void wxDVSelectionListCtrl::OnLeftDown(wxMouseEvent &evt)
{
	int vsx, vsy;
	GetViewStart(&vsx,&vsy);
	int mx = vsx+evt.GetX();
	int my = vsy+evt.GetY();

	for (size_t i=0;i<m_orderedItems.size();i++)
	{
		for (size_t c=0;c<m_numCols;c++)
		{
			if ( m_orderedItems[i]->geom[c].Contains( mx, my ) )
			{
				if ( ! m_orderedItems[i]->enable[c] ) return;

				m_orderedItems[i]->value[c] = !m_orderedItems[i]->value[c];
								
				m_lastEventRow = m_orderedItems[i]->row_index;
				m_lastEventCol = c;
				m_lastEventValue = m_orderedItems[i]->value[c];

				HandleRadio( m_orderedItems[i]->row_index, c );
				HandleLineColour( m_orderedItems[i]->row_index );
				Refresh();
				
				wxCommandEvent evt(wxEVT_DVSELECTIONLIST, GetId());
				evt.SetEventObject(this);
				GetEventHandler()->ProcessEvent(evt);
				
				return;
			}
		}
	}
}

void wxDVSelectionListCtrl::OnMouseMove(wxMouseEvent &evt)
{
/*
	int vsx, vsy;
	GetViewStart(&vsx,&vsy);
	wxDVSelectionListCtrlItem *cur_item = LocateXY(vsx+evt.GetX(), vsy+evt.GetY());
	if (cur_item != mLastHoverItem)
	{
		if (mLastHoverItem) mLastHoverItem->Hover = false;
		mLastHoverItem = cur_item;
		if (cur_item) cur_item->Hover = true;

		Refresh();
	}
*/
}

void wxDVSelectionListCtrl::OnLeave(wxMouseEvent &evt)
{
/*	if (mLastHoverItem)
	{
		mLastHoverItem->Hover = false;
		mLastHoverItem = NULL;

		Refresh();
	}
*/
}


void wxDVSelectionListCtrl::HandleRadio( int r, int c )
{
	if ( c == 0 && m_radioFirstCol )
	{
		for (size_t k=0;k<m_itemList.size();k++)
		{
			m_itemList[k]->value[0] = ( k == r );			
			HandleLineColour( k );
		}
	}
}

void wxDVSelectionListCtrl::HandleLineColour(int row)
{
	bool row_selected = IsRowSelected( row );
	bool has_colour = IsColourAssigned( row );

	if ( row_selected && !has_colour )
		m_itemList[row]->color = AssignLineColour( row );
	else if ( !row_selected && has_colour )
		DeAssignLineColour( row );
}
