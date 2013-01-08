#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/stc/stc.h>
#include <wx/webview.h>

#include "wex/plot/plplotctrl.h"
#include "wex/plot/pllineplot.h"
#include "wex/plot/plscatterplot.h"

#include "wex/dview/dvplotctrl.h"

#include "wex/codeedit.h"
#include "wex/lkscript.h"
#include "wex/sched.h"


void TestPLPlot( wxWindow *parent )
{
	wxFrame *frame = new wxFrame( parent, wxID_ANY, wxT("wxPLPlotCtrl in \x01dc\x03AE\x03AA\x00C7\x00D6\x018C\x01dd"), wxDefaultPosition, wxSize(850,500) );
	frame->SetIcon( wxICON( appicon ) );
		
	wxPLPlotCtrl *plot = new wxPLPlotCtrl( frame, wxID_ANY, wxDefaultPosition, wxDefaultSize );
	//plot->SetBackgroundColour( *wxWHITE );
	plot->SetTitle( wxT("Demo Plot: using \\theta(x)=sin(x)^2, x_0=1\n\\zeta(x)=3\\dot sin^2(x)") );

		
	wxPLLabelAxis *mx = new wxPLLabelAxis( -1, 12, "Months of the year (\\Chi\\Psi)" );
	mx->ShowLabel( false );

	mx->Add( 0,  "Jan" );
	mx->Add( 1,  "Feb" );
	mx->Add( 2,  "March\nMarzo" );
	mx->Add( 3,  "Apr" );
	mx->Add( 4,  "May" );
	mx->Add( 5,  "June\nJunio" );
	mx->Add( 6,  "July" );
	mx->Add( 7,  "August" );
	mx->Add( 8,  "September" );
	mx->Add( 9,  "October" );
	mx->Add( 10, "November" );
	mx->Add( 11, "December\nDeciembre" );

	plot->SetXAxis2( mx );
	/*
	plot->SetYAxis1( new wxPLLinearAxis(-140, 150, wxT("y1 label\ntakes up 2 lines")) );
	plot->SetYAxis1( new wxPLLinearAxis(-11, -3, wxT("\\theta(x)")), wxPLPlotCtrl::PLOT_BOTTOM );
		
	*/
	//plot->SetScaleTextSize( true );

	plot->ShowGrid( true, true );
		
	plot->SetXAxis1( new wxPLLogAxis( 0.01, 100, "\\nu  (m^3/kg)" ) );		
//	plot->SetXAxis1( new wxPLTimeAxis( 4, 200 ) );

	std::vector< wxRealPoint > sine_data;
	std::vector< wxRealPoint > cosine_data;
	std::vector< wxRealPoint > tangent_data;
	for (double x = -6; x < 12; x+= 0.01)
	{
		sine_data.push_back( wxRealPoint( x, 3*sin( x )*sin( x ) ) );
		cosine_data.push_back( wxRealPoint( x/2, 2*cos( x/2 )*x ) );
		tangent_data.push_back( wxRealPoint( x, x*tan( x ) ) );
	}


	plot->AddPlot( new wxPLLinePlot( sine_data, "3\\dot sin^2(x)", "forest green", wxPLLinePlot::DOTTED ), 
		wxPLPlotCtrl::X_BOTTOM, 
		wxPLPlotCtrl::Y_LEFT, 
		wxPLPlotCtrl::PLOT_TOP);


	plot->GetXAxis1()->SetLabel( "Bottom X Axis has a great sequence of \\nu  values!" );
		

	plot->AddPlot( new wxPLLinePlot( cosine_data, "cos(\\Omega_\\alpha  )", *wxRED, wxPLLinePlot::DASHED ), 
		wxPLPlotCtrl::X_BOTTOM, 
		wxPLPlotCtrl::Y_LEFT,
		wxPLPlotCtrl::PLOT_TOP);
		
	wxPLLinePlot *lltan = new wxPLLinePlot( tangent_data, "\\beta\\dot tan(\\beta)", *wxBLUE, wxPLLinePlot::SOLID, 1, false );
	lltan->SetAntiAliasing( true );
	plot->AddPlot( lltan, 
		wxPLPlotCtrl::X_BOTTOM, 
		wxPLPlotCtrl::Y_LEFT,
		wxPLPlotCtrl::PLOT_BOTTOM);


	std::vector< wxRealPoint > pow_data;
	for (double i=0.01;i<20;i+=0.1)
		pow_data.push_back( wxRealPoint(i, pow(i,3)-0.02*pow(i,6) ) );
		

	plot->AddPlot( new wxPLLinePlot( pow_data, "i^3 -0.02\\dot i^6", *wxBLACK, wxPLLinePlot::SOLID ),
		wxPLPlotCtrl::X_BOTTOM,
		wxPLPlotCtrl::Y_LEFT,
		wxPLPlotCtrl::PLOT_TOP );



	plot->GetYAxis1()->SetLabel( "Pressure (kPa)" );
	plot->GetYAxis1()->SetColour( *wxRED );
	plot->GetYAxis1()->SetWorld( -20, 20 );
	//plot->SetFont( wxFont( 10, wxFONTFAMILY_ROMAN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false ) );

	/*
	wxFrame *frame2 = new wxFrame( 0, wxID_ANY, wxT("GCDC vs DC"), wxDefaultPosition, wxSize( 850, 950 ) );
	TextLayoutDemo *tldemo = new TextLayoutDemo( frame2 );
	frame2->Show();
	*/

	wxLogWindow *log = new wxLogWindow( frame , "Log");
	wxLog::SetActiveTarget(log);
	log->Show();
		
	plot->SetAllowHighlighting( true );
				
	frame->Show();
}

#include "wex/dview/dvtimeseriesdataset.h"

void TestDView( wxWindow *parent )
{
	wxFrame *frame = new wxFrame( parent, wxID_ANY, "Test wxDView", wxDefaultPosition, wxSize(900,700) );
	wxDVPlotCtrl *dview = new wxDVPlotCtrl( frame, wxID_ANY );

	std::vector<double> data(8760);

	for (size_t i=0;i<8760;i++)
		data[i] = 123+i*100-i*i*0.01;
	dview->AddDataSet( new wxDVArrayDataSet( "curve 1", data ), "Group 1" );

	for (size_t i=0;i<8760;i++)
		data[i] = ::sin( i*0.001 ) * 10000;
	dview->AddDataSet( new wxDVArrayDataSet( "curve 2", data ), "Group 2");
	
	for (size_t i=0;i<8760;i++)
		data[i] = ::cos( i*0.01 ) * 15000;
	dview->AddDataSet( new wxDVArrayDataSet( "curve 3", data ), "Group 1");

	for (size_t i=0;i<8760;i++)
		data[i] = ::sin( i*0.1 ) * 5000;
	dview->AddDataSet( new wxDVArrayDataSet( "curve 4", data ), "Group 2");
	
	for (size_t i=0;i<8760;i++)
		data[i] = ::cos( i*0.0001 ) *2500;
	dview->AddDataSet( new wxDVArrayDataSet( "curve 5", data ), "Group 2");

	frame->Show();
}



class MyApp : public wxApp
{
public:
	bool OnInit()
	{
		wxInitAllImageHandlers();
		TestDView( 0 );

		wxFrame *frm = new wxFrame(NULL, wxID_ANY, "SchedCtrl", wxDefaultPosition, wxSize(400,300));
		wxSchedCtrl *sch = new wxSchedCtrl( frm, wxID_ANY );
		sch->SetupTOUGrid();
		frm->Show();

		return true;
	}
};

IMPLEMENT_APP( MyApp );
