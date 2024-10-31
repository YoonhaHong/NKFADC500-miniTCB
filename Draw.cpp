#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH2.h"
#include "TString.h"
#include "TStyle.h"
#include "TTree.h"

#include <iostream>
#include <fstream>
#include <numeric>
#include <vector>
using namespace std;

const int CoinCut1[2] = { -5, 5 };// -10<dt<10ns
const int CoinCut2[2] = { -1, 1 };// -2<dt<2ns
const int ADCAxisMax = 100;
const int DTAxisMax = 30;

void Draw(const int RunNo=20001, const char* inPath = "./data", const char* Suffix = "", bool Print = false)
{
    gStyle->SetOptStat(0);
    gStyle->SetTitleSize(0.05, "xyz"); 
    gStyle->SetLabelSize(0.04, "xyz");      
    gStyle->SetStatFontSize(0.04);           
    gStyle->SetTitleOffset(0.8);           
	const int nCh  = 4;
	vector<int> TrigTh = {25, 25, 50, 50};
	const int DLen = 512;

	//for parsing
	ifstream logFile( Form( "%s/log_%d.txt",inPath, RunNo ) );
	if (!logFile.is_open() ) cerr << "No log file!, can not know ADC threshold" <<endl;

	string line;
	while ( getline( logFile, line) ){
		size_t pos = line.find("Trigger LUT value        :");
		if( pos != string::npos ) break;
	}
    cout<<line<<endl;
	istringstream iss2(line);
    string temp;
    string TRGlogic;
	while( iss2 >> temp ){ if( temp == ":" ) break;}
    iss2 >> TRGlogic;
    //cout << TRGlogic << endl;

	while ( getline( logFile, line) ){
		size_t pos = line.find("Discrimination threshold :");
		if( pos != string::npos ) break;
	}
	cout<<line<<endl;
	int value;
    istringstream iss(line);
	while( iss >> temp ){ if( temp == ":" ) break;}
	for( int Ch=0; Ch<nCh; Ch++) iss>>TrigTh[Ch];
	//for( int thr:TrigTh ) cout<<thr<<endl;

	TFile* F  = new TFile(Form("%s/FADCData_%i.root", inPath, RunNo));
	TTree* T = (TTree*)F->Get("T");

 	const int nEvents = T->GetEntries();
	cout << "Number of events: " << nEvents << endl;
	

	const int nADC = (DLen-32)/2;
	const int nTDC = nADC/4;


	//-------------------------------------------
	ULong_t fDatTime[nCh];  //Data starting time (ns)
	UInt_t  fADC[nCh][nADC]; //ADC

	T->SetBranchAddress("dtime",    &fDatTime);
	T->SetBranchAddress("adc",      &fADC);
	

	TH2F* H2_Waveform[nCh]; //Sampling index vs. trigger #, bin content is ADC
    int ADCMAX = 500, ADCMIN = 300;                        
    TH1F* H1_Pedestal[nCh]; 
    TH1F* H1_PeakAmp[nCh]; 
	TH1F* H1_PeakCut[nCh];
    TH1F* H1_dt = new TH1F( "dt", "dt(SC2-SC1)", 2*nADC, -nADC, nADC );
 	TH1F* H1_dtCoin1 = new TH1F( "H1_dtCoin1", "", 2*nADC,-nADC,nADC);
    TH1F* H1_dtCoin2 = new TH1F( "H1_dtCoin2", "", 2*nADC,-nADC,nADC);

    TH2F* H2_PeakAmp = new TH2F( "H2_PeakAmp_ch1ch2", "ADC Correlation", ADCAxisMax, 0, ADCAxisMax
                                                                  , ADCAxisMax, 0, ADCAxisMax);
	TH1F* H1_PeakAmpCoin1[nCh];
	TH1F* H1_PeakAmpCoin2[nCh];

	TGraph* Rate[nCh];

	for(int Ch=0; Ch<nCh; Ch++){
		H2_Waveform[Ch] = new TH2F( Form("H2_Waveform_ch%i",Ch+1), Form("Waveform CH%i",Ch+1), nADC, 0, nADC, 4095, 0, 4095);

        H1_Pedestal[Ch] = new TH1F( Form("H1_Pedestal%i", Ch+1), Form("Pedestal CH%i", Ch+1), 200, 300, 500 );
        H1_PeakAmp[Ch] = new TH1F( Form("PeakAmp_ch%i", Ch+1), "", 4000+10, -10, 4000);
        H1_PeakCut[Ch] = new TH1F( Form("PeakCut_ch%i", Ch+1), "", 4000+10, -10, 4000);


        H1_PeakAmpCoin1[Ch] = new TH1F( Form("PeakAmpCoin1_ch%i", Ch+1), "", 4000+10, -10, 4000);
        H1_PeakAmpCoin2[Ch] = new TH1F( Form("PeakAmpCoin2_ch%i", Ch+1), "", 4000+10, -10, 4000);

		Rate[Ch] = new TGraph();
	}
    
	//------------------------------------------
	//Read the FADC data and fill H2
	
	T -> GetEntry(0);
	ULong_t StartTime = fDatTime[0];

	for (int iEv=0; iEv<nEvents; iEv++)
	{
		T->GetEntry(iEv);
        float PeakAmp[nCh] = {0};
        float PeakTime[nCh] = {0};
		
		
		for (int iCh=0; iCh<nCh; iCh++)
		{
            std::vector<short> vecADC;
            vecADC.resize(0);
			for (int c=0; c<nADC; c++)
			{
                UInt_t& tADC = fADC[iCh][c];
                H2_Waveform[iCh] -> Fill( c, tADC );
                vecADC.push_back( tADC );

                if(tADC > ADCMAX) ADCMAX = tADC;
                else if(tADC < ADCMIN) ADCMIN = tADC;
			}//c, # of sampling per event

            float sum = accumulate(vecADC.begin()+1, vecADC.begin()+20, 0);
            float Pedestal = sum / 19;

            auto max_iter = std::max_element(vecADC.begin(), vecADC.end());
            PeakAmp[iCh] = *max_iter - Pedestal;
            PeakTime[iCh] = std::distance(vecADC.begin(), max_iter);

            H1_Pedestal[iCh] -> Fill( Pedestal );
            H1_PeakAmp[iCh] -> Fill( PeakAmp[iCh] );
			if( PeakAmp[iCh] < TrigTh[iCh] ) H1_PeakCut[iCh] -> Fill( PeakAmp[iCh] );
			Rate[iCh] -> SetPoint( Rate[iCh]->GetN(), (fDatTime[0]-StartTime), PeakAmp[iCh] );
		}//Ch

        float dt = PeakTime[1] - PeakTime[0];
        H1_dt -> Fill( dt );
        H2_PeakAmp -> Fill( PeakAmp[0], PeakAmp[1] );

		if( CoinCut1[0] <= dt && dt <= CoinCut1[1] ) {
			H1_PeakAmpCoin1[0] -> Fill( PeakAmp[0] );
			H1_PeakAmpCoin1[1] -> Fill( PeakAmp[1] );
			H1_dtCoin1 -> Fill( dt );
		}

		if( CoinCut2[0] <= dt && dt <= CoinCut2[1]  ) {
			H1_PeakAmpCoin2[0] -> Fill( PeakAmp[0] );
			H1_PeakAmpCoin2[1] -> Fill( PeakAmp[1] );
			H1_dtCoin2 -> Fill( dt );
		}


	}//iEv

	TCanvas* c1 = new TCanvas("c1","c1",1000, 1000);
	c1 -> Divide(3, 3, 0.001, 0.001);
	TPad* PeakPad[4];
	TH1* PeakFrame[4];
	const int nRebin = 2;
	int YAxisMax = H1_PeakAmp[0] -> GetMaximum();
	YAxisMax *= 10;


	for(int b=0; b<2; b++)
	{
		for(int c=0; c<5; c++)
		{
            c1 -> cd(b + 3 * c + 1);
            if(c==0)
			{
                gPad -> SetLogz();
				H2_Waveform[b] -> GetYaxis() -> SetRangeUser(ADCMIN, ADCMAX);
				H2_Waveform[b] -> Draw("COLZ");
		        H2_Waveform[b] -> GetXaxis()->SetTitle("iSample [2ns]");
		        H2_Waveform[b] -> GetYaxis()->SetTitle( "ADC" );
		        H2_Waveform[b] -> GetYaxis()->SetTitleOffset( 1.  );

			}else if(c==1){
                gPad -> SetLogy();
                H1_Pedestal[b]->SetLineColor(1);
				H1_Pedestal[b] -> GetXaxis() -> SetTitle("Pedestal [ADC]");
				H1_Pedestal[b]->Draw();

            }else if(c==2){
                gPad -> SetLogy();
				PeakFrame[b] = gPad -> DrawFrame( -5, 0.5, ADCAxisMax, YAxisMax );
				PeakFrame[b] -> SetTitle( Form("ADC_{peak} CH%d", b+1) );
				PeakFrame[b] -> GetXaxis() -> SetTitle( "ADC_{peak}" );
				PeakFrame[b] -> Draw();

				H1_PeakAmp[b] -> Rebin( nRebin );
				H1_PeakAmp[b] -> Draw("same");

				H1_PeakCut[b]->SetFillColor(2);
				H1_PeakCut[b]->SetFillStyle(3003);
				H1_PeakCut[b] -> Rebin( nRebin );
				H1_PeakCut[b]->Draw("same"); //Draw Quality Cut


				H1_PeakAmpCoin1[b]->SetLineColor(2);
				H1_PeakAmpCoin1[b]->SetLineStyle(2);
				H1_PeakAmpCoin1[b]->SetLineWidth(3);
				H1_PeakAmpCoin1[b] -> Rebin( nRebin );
				H1_PeakAmpCoin1[b]->Draw("same");

				H1_PeakAmpCoin2[b]->SetLineColor(3);
				H1_PeakAmpCoin2[b]->SetLineStyle(2);
				H1_PeakAmpCoin2[b]->SetLineWidth(3);
				H1_PeakAmpCoin2[b] -> Rebin( nRebin );
				H1_PeakAmpCoin2[b]->Draw("same");
            }
		}//c
	}//b
	//Subscription-----------------------------
    c1->cd(3);
    TLatex l;
    string TRGsym;
    if(TRGlogic == "CCCC") TRGsym = "CH2"; 
    else if(TRGlogic == "AAAA") TRGsym = "CH1"; 
    else if(TRGlogic == "8888") TRGsym = "AND"; 
    else if(TRGlogic == "EEEE") TRGsym = "OR"; 
    else                        TRGsym = "ERR";

    l.SetTextAlign(12);
    l.SetTextSize(0.09);
    l.DrawLatex(0.15,0.8,Form("RunNo. %d", RunNo) );
    l.DrawLatex(0.15,0.6,Form("# of Events = %d", nEvents) );
    l.DrawLatex(0.15,0.4,Form("Trigger Logic = %s", TRGsym.c_str() ) );
    l.DrawLatex(0.15,0.2,Form("Threshold = %d ADC", TrigTh[0]) );

	//dt-----------------------------
	c1->cd(6);
	int dtMax = H1_dt -> GetMaximum(); 
    //gPad -> SetLogy();
	TH1* dtFrame = gPad -> DrawFrame( -DTAxisMax, 0.5, DTAxisMax, dtMax * 1.5 );
	dtFrame -> SetTitle( "#Delta t(SC2-SC1)" );
	dtFrame -> GetXaxis() -> SetTitle( "#Delta t(SC2-SC1) [2ns]" );
	dtFrame -> Draw();

	H1_dt -> Draw("SAME");

	H1_dtCoin1 -> SetFillColor( 2 );
	H1_dtCoin1 -> SetLineColor( 2 );
	H1_dtCoin1 -> Draw("SAME");

	H1_dtCoin2 -> SetFillColor( 3 );
	H1_dtCoin2 -> SetLineColor( 3 );
	H1_dtCoin2 -> Draw("SAME");
	
	int Int_All = H1_dt->Integral();
	int Int_Coin1 = H1_dtCoin1 -> Integral();
	int Int_Coin2 = H1_dtCoin2 -> Integral();


	TLegend *leg_dt = new TLegend(0.16, 0.68, 0.36, 0.88);
    leg_dt->SetFillStyle(0);
    leg_dt->SetBorderSize(0);
    leg_dt->SetTextSize(0.05);
    leg_dt -> AddEntry( H1_dt, Form("# of Events: %d", Int_All), "l");
    leg_dt -> AddEntry( H1_dtCoin1, Form("# of %d ns #leq #Delta t #leq %d ns : %d ", CoinCut1[0]*-2, CoinCut1[1]*2, Int_Coin1) );
    leg_dt -> AddEntry( H1_dtCoin2, Form("# of %d ns #leq #Delta t #leq %d ns : %d ", CoinCut2[0]*2, CoinCut2[1]*2, Int_Coin2) );
    leg_dt -> Draw();


	//Correlation-----------------------------
	c1->cd(9);
	gPad -> SetLogy(0);
	H2_PeakAmp -> Draw("colz");
	H2_PeakAmp -> GetXaxis() -> SetTitle( "ADC_{peak} CH1" );
	H2_PeakAmp -> GetYaxis() -> SetTitle( "ADC_{peak} CH2" );
    H2_PeakAmp -> GetYaxis()->SetTitleOffset( 0.8 );

	c1 -> SaveAs( Form("fig/%i.pdf", RunNo ) );
		
	TCanvas *c3 = new TCanvas("c3", "c3", 3000, 600 ); 
	c3 -> cd();
	Rate[0] -> Draw();

	return;


	if(0){
		TFile *out = new TFile( Form("KEK_Hist/%i_Rate.root", RunNo), "recreate" );
	
		Rate[0] -> Write();
		Rate[1] -> Write();
	
		out -> Write();
		out -> Close();
	}

}

