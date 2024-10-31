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
const int ADCMax = 2000;
const int ADCAxisMax = 200;

void Draw_YH(const int RunNo=20000, const char* inPath = "./data", const char* Suffix = "", bool Print = false)
{
	gStyle->SetOptStat(0);
	const int nCh  = 4;
	vector<int> TrigTh = {25, 25, 50, 50};
	const int DLen = 512;

	//for threshod parsing
	ifstream logFile( Form( "%s/log_%d.txt",inPath, RunNo ) );
	if (!logFile.is_open() ) cerr << "No log file!, can not know ADC threshold" <<endl;

	string line;
	while ( getline( logFile, line) ){
		size_t pos = line.find("Discrimination threshold :");
		if( pos != string::npos ) break;
	}
	cout<<line<<endl;
	istringstream iss(line);
	int value;
	string temp;
	while( iss >> temp ){ if( temp == ":" ) break;}
	for( int Ch=0; Ch<nCh; Ch++) iss>>TrigTh[Ch];
	//for( int thr:TrigTh ) cout<<thr<<endl;
	//-----------------------------

	TFile* F  = new TFile(Form("%s/FADCData_%i.root", inPath, RunNo));
	TTree* T = (TTree*)F->Get("T");

 	const int nEvents = T->GetEntries();
	cout << "Number of events: " << nEvents << endl;
	

	const int nADC = (DLen-32)/2;
	const int nTDC = nADC/4;


	//-------------------------------------------

	/*
	UInt_t  fModuleID[nCh]; //Module ID
	UInt_t  fChannel[nCh];  //Channel ID
	UInt_t  fTrigType[nCh]; //Trigger type
	UInt_t  fLocTPat[nCh];  //Local trigger pattern
	UInt_t  fPed[nCh];      //Pedestal
	ULong_t fDataLen[nCh];  //Data length
	ULong_t fTrigNum[nCh];  //Trigger #
	ULong_t fTrigTime[nCh]; //Trigger time (ns)
	ULong_t fLocTNum[nCh];  //Local trigger #
	*/
	ULong_t fDatTime[nCh];  //Data starting time (ns)
	UInt_t  fADC[nCh][nADC]; //ADC

	/*
	T->SetBranchAddress("mid",      &fModuleID);
	T->SetBranchAddress("ch",       &fChannel);
	T->SetBranchAddress("trigtype", &fTrigType);
	T->SetBranchAddress("ltrigpat", &fLocTPat);
	T->SetBranchAddress("ped",      &fPed);
	T->SetBranchAddress("dlength",  &fDataLen);
	T->SetBranchAddress("trignum",  &fTrigNum);
	T->SetBranchAddress("trigtime", &fTrigTime);
	T->SetBranchAddress("ltrignum", &fLocTNum);
	*/
	T->SetBranchAddress("dtime",    &fDatTime);
	T->SetBranchAddress("adc",      &fADC);
	

	TH2F* H2[nCh]; //Sampling index vs. trigger #, bin content is ADC
    TH1F* H1_Pedestal[nCh]; 
    TH1F* H1_Pulse[nCh]; //Mean Pulse
    TH1F* H1_PeakAmp[nCh]; 
	TH1F* H1_PeakCut[nCh];
    TH1F* H1_dt = new TH1F( "dt", "dt(SC2-SC1)", 2*nADC, -nADC, nADC );
 	TH1F* H1_dtCoin1 = new TH1F( "H1_dtCoin1", "", 2*nADC,-nADC,nADC);
    TH1F* H1_dtCoin2 = new TH1F( "H1_dtCoin2", "", 2*nADC,-nADC,nADC);

    TH2F* H2_PeakAmp = new TH2F( "H2_PeakAmp_ch1ch2", "CH1 vs CH2", 110, 0, 110, 110, 0, 110 );
	TH1F* H1_PeakAmpCoin1[nCh];
	TH1F* H1_PeakAmpCoin2[nCh];
	TH2F* H2_PeakTimeAmp[nCh];

	TGraph* Rate[nCh];

	for(int Ch=0; Ch<nCh; Ch++){
		H2[Ch] = new TH2F( Form("H2_ch%i",Ch+1), Form("H2_ch%i",Ch+1), nEvents, 0, nEvents, nADC, 0, nADC);
		H2[Ch] -> Sumw2();

        H1_Pedestal[Ch] = new TH1F( Form("H1_Pedestal%i", Ch+1), Form("H1_Pedestal%i", Ch+1), 200, 300, 500 );
        H1_Pulse[Ch] = new TH1F( Form("Pulse_ch%i", Ch+1), "", nADC, 0, nADC);
        H1_PeakAmp[Ch] = new TH1F( Form("PeakAmp_ch%i", Ch+1), "", ADCMax+10, -10, ADCMax);
        H1_PeakCut[Ch] = new TH1F( Form("PeakCut_ch%i", Ch+1), "", ADCMax+10, -10, ADCMax);


        H1_PeakAmpCoin1[Ch] = new TH1F( Form("PeakAmpCoin1_ch%i", Ch+1), "", ADCMax+10, -10, ADCMax);
        H1_PeakAmpCoin2[Ch] = new TH1F( Form("PeakAmpCoin2_ch%i", Ch+1), "", ADCMax+10, -10, ADCMax);
        H2_PeakTimeAmp[Ch] = new TH2F( Form("PeakTimeAmp_ch%i", Ch+1), Form("CH%i time vs ADC_{peak}", Ch+1), nADC, 0, nADC, ADCMax+10, -10, ADCMax);

		Rate[Ch] = new TGraph();
	}
    
	//-------------------------------------------
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
			for (int c=0; c<nADC; c++)
			{
				H2[iCh]->SetBinContent( iEv+1, c+1, fADC[iCh][c]);
			}//c, # of sampling per event


            TH1F* htemp = (TH1F*) H2[iCh] -> ProjectionY("", iEv+1, iEv+1);
			//if( iEv ==2 and iCh==0 ) htemp -> Draw();

            TF1* fit_ped1 = new TF1("fit_ped1","pol0",0,20);
            TF1* fit_ped2 = new TF1("fit_ped2","pol0",200,240);

            htemp -> Fit(fit_ped1,"R0LQ");
            htemp -> Fit(fit_ped2,"R0LQ");
            
            float Pedestal = fit_ped1 -> GetParameter(0);
            //float Pedestal2 = fit_ped2 -> GetParameter(0);
            //float Pedestal  = (Pedestal1 + Pedestal2) / 2.f ; 

            H1_Pedestal[iCh] -> Fill( Pedestal );

            for(int x=0; x<htemp -> GetNbinsX(); x++)
            {				
                if(htemp -> GetBinContent(x+1) == 0)
						continue;
                float prev = htemp -> GetBinContent(x+1);
                htemp -> SetBinContent(x+1,prev-Pedestal);
                H1_Pulse[iCh] -> Fill(x,prev-Pedestal);
            }

            PeakAmp[iCh] = htemp -> GetMaximum();
            PeakTime[iCh] = htemp -> GetBinCenter( htemp -> GetMaximumBin() ); 

            H1_PeakAmp[iCh] -> Fill( PeakAmp[iCh] );
            H2_PeakTimeAmp[iCh] -> Fill( PeakTime[iCh], PeakAmp[iCh] );

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

	TCanvas* c1 = new TCanvas("c1","c1",1.2*600*4,600*5);
	c1 -> Divide(4,5);
	TPad* PeakPad[4];
	TH1* PeakFrame[4];
	const int nRebin = 2;
	int YAxisMax = H1_PeakAmp[0] -> GetMaximum();
	YAxisMax *= 10;


	for(int b=0; b<nCh; b++)
	{
		PeakPad[b] = new TPad( Form("Pad_CH%d", b+1), Form("CH%d", b+1), 0, 0, 1, 1); 
		PeakPad[b] -> SetMargin(.13, .05, .12, .05);
		PeakPad[b] -> SetLogy();

		for(int c=0; c<5; c++)
		{
			c1 -> cd(b+4*c+1);
			if(c==0)
			{
				H2[b] -> GetXaxis() -> SetRangeUser(1, nEvents);
				H2[b] -> Draw("colz2");
				H2[b] -> GetXaxis() -> SetLabelSize(0.05);
				H2[b] -> GetYaxis() -> SetLabelSize(0.05);
				H2[b] -> GetXaxis() -> SetTitleSize(0.06);
				H2[b] -> GetYaxis() -> SetTitleSize(0.06);
			}else if (c==1)
			{
				TH1F* htemp = (TH1F*) H2[b] -> ProjectionY();
				htemp->Scale(1./ nEvents);
				htemp -> GetXaxis() -> SetLabelSize(0.05);
				htemp -> GetYaxis() -> SetLabelSize(0.05);
				htemp -> GetXaxis() -> SetTitleSize(0.06);
				htemp -> GetYaxis() -> SetTitleSize(0.06);
				htemp -> GetXaxis() -> SetTitle("iSample [2ns]");
				htemp->Draw();
			}else if (c==2)
			{
				H1_Pedestal[b]->SetLineColor(1);
				H1_Pedestal[b] -> GetXaxis() -> SetLabelSize(0.05);
				H1_Pedestal[b] -> GetYaxis() -> SetLabelSize(0.05);
				H1_Pedestal[b] -> GetXaxis() -> SetTitleSize(0.06);
				H1_Pedestal[b] -> GetYaxis() -> SetTitleSize(0.06);
				H1_Pedestal[b] -> GetXaxis() -> SetTitle("Pedestal [ADC]");
				H1_Pedestal[b]->Draw();
			}else if (c==3)
			{
				H1_Pulse[b]->Scale(1./ nEvents);
				H1_Pulse[b] -> GetXaxis() -> SetLabelSize(0.05);
				H1_Pulse[b] -> GetYaxis() -> SetLabelSize(0.05);
				H1_Pulse[b] -> GetXaxis() -> SetTitleSize(0.06);
				H1_Pulse[b] -> GetYaxis() -> SetTitleSize(0.06);
				H1_Pulse[b]->Draw();
			}else if (c==4)
			{
				
				PeakPad[b] -> Draw();
				PeakPad[b] -> cd();

				PeakFrame[b] = PeakPad[b] -> DrawFrame( -5, 0.5, ADCAxisMax, YAxisMax );
				PeakFrame[b] -> GetXaxis() -> SetLabelSize(0.05);
				PeakFrame[b] -> GetYaxis() -> SetLabelSize(0.05);
				PeakFrame[b] -> GetXaxis() -> SetTitleSize(0.05);
				PeakFrame[b] -> GetYaxis() -> SetTitleSize(0.05);
				PeakFrame[b] -> GetXaxis() -> SetTitle( Form("SC%d ADC_{peak}", b+1) );
				PeakFrame[b] -> Draw();

				H1_PeakAmp[b] -> GetXaxis() -> SetLabelSize(0.05);
				H1_PeakAmp[b] -> GetYaxis() -> SetLabelSize(0.05);
				H1_PeakAmp[b] -> GetXaxis() -> SetTitleSize(0.06);
				H1_PeakAmp[b] -> GetYaxis() -> SetTitleSize(0.06);
				H1_PeakAmp[b] -> Rebin( nRebin );
				H1_PeakAmp[b]->Draw("same");

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
		H2_PeakTimeAmp[b] -> GetXaxis()->SetTitle("iSample[2ns]");
		H2_PeakTimeAmp[b] -> GetYaxis()->SetTitle( Form("ADC_{SC%i peak}", b+1) );
	}//b

	c1->cd(19);
	TPad* dtPad = new TPad( "dtPad", "dtPad", 0, 0, 1, 1); 
	dtPad -> SetMargin(.13, .05, .12, .05);
	dtPad -> SetLogy();

	int dtMax = H1_dt -> GetMaximum(); 
	TH1* dtFrame = dtPad -> DrawFrame( -nADC, 0.5, nADC, dtMax * 1.3 );
	dtFrame -> GetXaxis() -> SetLabelSize(0.05);
	dtFrame -> GetYaxis() -> SetLabelSize(0.05);
	dtFrame -> GetXaxis() -> SetTitleSize(0.05);
	dtFrame -> GetYaxis() -> SetTitleSize(0.05);
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


	TLegend *leg_dt = new TLegend(0.16, 0.61, 0.36, 0.81);
    leg_dt->SetFillStyle(0);
    leg_dt->SetBorderSize(0);
    leg_dt->SetTextSize(0.05);
    leg_dt -> AddEntry( H1_dt, Form("# of Events: %d", Int_All), "l");
    leg_dt -> AddEntry( H1_dtCoin1, Form("# of %d ns #leq #Delta t #leq %d ns : %d ", CoinCut1[0]*-2, CoinCut1[1]*2, Int_Coin1) );
    leg_dt -> AddEntry( H1_dtCoin2, Form("# of %d ns #leq #Delta t #leq %d ns : %d ", CoinCut2[0]*2, CoinCut2[1]*2, Int_Coin2) );
    leg_dt -> Draw();


	c1->cd(20);
	gPad -> SetLogy(0);
	H2_PeakAmp -> Draw("colz");

	c1 -> SaveAs( Form("fig/%i.pdf", RunNo ) );
		
	TCanvas *c3 = new TCanvas("c3", "c3", 3000, 600 ); 
	c3 -> cd();
	Rate[0] -> Draw();

	return;


	if(0){
		TCanvas* c2 = new TCanvas("c2", "c2", 800*2, 600 );
		c2->Divide(2, 1);

		c2->cd(1);

		H2_PeakTimeAmp[0]->Draw("colz");

		c2->cd(2);

		TFile *out = new TFile( Form("KEK_Hist/%i_Rate.root", RunNo), "recreate" );
	
		Rate[0] -> Write();
		Rate[1] -> Write();
	
		out -> Write();
		out -> Close();
		H2_PeakTimeAmp[1]->Draw("colz");
	}

}

