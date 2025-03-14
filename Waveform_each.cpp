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

int iEv = 0;
const int DLen = 32768;
const int nADC = (DLen-32)/2;
const int nTDC = nADC/4;
const int nCh = 4;
int fADC[nCh][nADC]; //ADCy(iEv);

TTree* T;
TCanvas* c1;

int GetDataLength(const char* inFile)
{
	//Reading 1st event, 1st header (32 bytes fixed), 1st channel's very first four bits will be enough
	ifstream in;
	in.open(inFile, std::ios::binary);
	if (!in.is_open()) { cout <<"GetDataLength - cannot open the file! Stop.\n"; return 1; }

	char data[32];
	in.read(data, 32);
	unsigned long dataLength = 0;
	for (int i=0; i<4; i++) dataLength += ((ULong_t)(data[4*i] & 0xFF) << 8*i);

	in.close();
	return (int)dataLength;
}//GetDataLength


void DrawNextEv();

void Waveform_each(const int RunNo=20001, const char* inPath = "./data")
{
	gStyle->SetOptStat(0);
	TFile* F  = new TFile(Form("%s/FADCData_%i.root", inPath, RunNo));
	T = (TTree*)F->Get("T");

 	const int nEvents = T->GetEntries();
	cout << "Number of events: " << nEvents << endl;

	T->SetBranchAddress("adc",      &fADC);
	c1 = new TCanvas("c1","c1",400*4,400);
	c1 -> Divide(4, 1, 0.01, 0.01);
	DrawNextEv();


}

void DrawNextEv(){
    T->GetEntry(iEv);
    TH1F* H1_Waveform[nCh]; //Sampling index vs. trigger #, bin content is ADC
    int ADCMAX = 500;
    int ADCMIN = 300;

    for (int iCh=0; iCh<nCh; iCh++)
	{
		H1_Waveform[iCh] = new TH1F( Form("ev%i_ch%i",iEv+1, iCh+1), Form("Waveform_ch%i",iCh+1), nADC, 0, nADC);
		H1_Waveform[iCh] -> Sumw2();
		for (int c=0; c<nADC; c++)
		{
			int& tADC = fADC[iCh][c];
			H1_Waveform[iCh]->Fill(c, tADC );
			if(tADC > ADCMAX) ADCMAX = tADC;
			else if(tADC < ADCMIN) ADCMIN = tADC;
		}//c, # of sampling per event
        c1 -> cd(iCh+1);
        gPad -> SetRightMargin(0.13);
        H1_Waveform[iCh] -> Draw("HIST CX");
        H1_Waveform[iCh] -> GetYaxis() -> SetRange(ADCMIN, ADCMAX);
        H1_Waveform[iCh] -> GetYaxis() -> SetTitle( "ADC" );
        H1_Waveform[iCh] -> GetXaxis() -> SetTitle( "iSample [2 ns]" );
	}//Ch
    c1->Draw();
    c1->Update();

	iEv++;
}
