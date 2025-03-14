#include "TCanvas.h"
#include "TFile.h"
#include "TGraph.h"
#include "TH1.h"
#include "TH1.h"
#include "TString.h"
#include "TStyle.h"
#include "TTree.h"
#include "TDirectory.h"

#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <iostream>
#include <fstream>
#include <numeric>
#include <vector>

using namespace std;

const int nCh  = 4;
int nEvents;
FILE* fp;   
ifstream in;

TCanvas* cPulse = new TCanvas("cPulse","cPulse",2000,500);
TLegend* LegInfo[nCh];
TPad* Pad_Pulse[nCh];
TH1* Frame_Pulse[nCh];
TH1F* H1_Pulse[nCh];

void ReadADC(int RunNo);
void DrawNextEv(int RunNo);

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

void Draw_EachEvent( int RunNo = 10017, char* inDir = "./data" )
{

    TString Name; Name.Form( "%s/FADCData_%d_1.dat", inDir, RunNo ); 
	in.open(Form("./data/FADCData_%d_1.dat", RunNo), std::ios::binary);
    gStyle->SetOptStat(0);
    //cPulse -> Divide(8,4,0,0);
    cPulse -> Divide(4,1);
    DrawNextEv(RunNo);
    return;
}



void DrawNextEv(int RunNo)
{
    const int ADCMax = 200;
    //ifstream in;
	const int DLen = GetDataLength(Form("./data/FADCData_%i_1.dat",RunNo)); //header (32) + body (vary), per ch
	const int TLen = DLen * nCh; //Total length
	const int nADC = (DLen - 32)/2; //# of ADC samples per event
	const int nTDC = nADC/4;
    char data[TLen];
	char dataChop[nCh][DLen];
    std::vector<int> tempADC;

	cout << "DLen: " << DLen << endl;
	cout << "nADC: " << nADC << endl;

    ifstream in;
	in.read(data, TLen);

	for (int j=0; j<nCh; j++)
	{
        delete H1_Pulse[j];
        H1_Pulse[j] = new TH1F( Form("Pulse_ch%i", j+1), "", nADC, 0, nADC);
        tempADC.resize(0);
	    for (int k=0; k<DLen; k++) dataChop[j][k] = data[j + 4*k];

			ULong_t tDataLen = 0;
			for (int a=0; a<4; a++) tDataLen += ((ULong_t)(dataChop[j][a] & 0xFF) << 8*a);

			UInt_t tTrigType = dataChop[j][6] & 0x0F;

            ULong_t tTrigNum = 0;
            for (int a=0; a<4; a++) tTrigNum += ((ULong_t)(dataChop[j][7+a] & 0xFF) << 8*a);

            ULong_t tTrigTime = ( (ULong_t)(dataChop[j][11] & 0xFF)        ) * 8    + //Fine time: 8 ns/unit
                ( (ULong_t)(dataChop[j][12] & 0xFF)        ) * 1000 + //Coarse time: 1 us/unit
                ( (ULong_t)(dataChop[j][13] & 0xFF) << 8   ) * 1000 +
                ( (ULong_t)(dataChop[j][14] & 0xFF) << 8*2 ) * 1000;

            UInt_t tModuleID = dataChop[j][15] & 0xFF;
            UInt_t tChannel  = dataChop[j][16] & 0xFF;
            if (tChannel<1 || tChannel>4) cout <<Form("WARNING: irregular ch ID found: %i\n", tChannel);

            ULong_t tLocTNum = 0;
            for (int a=0; a<4; a++) tLocTNum += ( (ULong_t)(dataChop[j][17+a] & 0xFF) << 8*a );

            UInt_t tLocTPat = 0;
            for (int a=0; a<2; a++) tLocTPat += ( (dataChop[j][21+a] & 0xFF) << 8*a );

            UInt_t tPed = 0;
            for (int a=0; a<2; a++) tPed += ( (dataChop[j][23+a] & 0xFF) << 8*a );

            //unsigned int: 16 bit, unsigned long: 32 bit, unsigned long long = 64 bit
            unsigned long long tDatTime = ( (unsigned long long)(dataChop[j][25] & 0xFF)        ) * 8    +
                ( (unsigned long long)(dataChop[j][26] & 0xFF)        ) * 1000 +
                ( (unsigned long long)(dataChop[j][27] & 0xFF) << 8   ) * 1000 +
                ( (unsigned long long)(dataChop[j][28] & 0xFF) << 8*2 ) * 1000 +
                ( (unsigned long long)(dataChop[j][29] & 0xFF) << 8*3 ) * 1000 +
                ( (unsigned long long)(dataChop[j][30] & 0xFF) << 8*4 ) * 1000 +
                ( (unsigned long long)(dataChop[j][31] & 0xFF) << 8*5 ) * 1000;

            /*
               ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
               Body
Byte: [Bits], Contents

0: [7~0], ADC data[0][ 7:0]
1: [3~0], ADC data[0][11:8]
1: [7~4], TDC data[0][ 3:0]
2: [7~0], ADC data[1][ 7:0]
3: [3~0], ADC data[1][11:8]
3: [7~4], TDC data[0][ 7:4]
4: [7~0], ADC data[2][ 7:0]
5: [3~0], ADC data[2][11:8]
5: [7~4], TDC data[0][11:8]
6: [7~0], ADC data[3][ 7:0]
7: [3~0], ADC data[3][11:8]
7: [7~4], 0 (empty)
---------------------------
8: [7~0], ADC data[0][ 7:0]
...

             * 4 ADC samples and 1 TDC sample per 8 bytes, then repeat for 'N = (data_length - 32)/2'
             * 8 ps per TDC channel
             ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
             */

            for (int a=0; a<nADC; a++)
            {
                const int iSmp = 32 + 2*a;
                UInt_t tADC = (dataChop[j][iSmp] & 0xFF) + ((dataChop[j][iSmp + 1] & 0xF) << 8);

                tempADC.push_back( tADC );
//H1_Pulse[j] -> Fill(a+1, tADC);
            }

            for (int a=0; a<nTDC; a++)
            {
                const int iSmp = 32 + 8*a;
                UInt_t tTDC = ((dataChop[j][iSmp + 1] & 0xF0) >> 4) + 
                    ((dataChop[j][iSmp + 3] & 0xF0)) +
                    ((dataChop[j][iSmp + 5] & 0xF0) << 4);
            }
          float sum = std::accumulate(tempADC.begin(), tempADC.begin()+100, 0);
        float Pedestal = sum / 100;

        auto min_element_iter = std::min_element(tempADC.begin(), tempADC.end());
        int min_value = *min_element_iter;
        int min_index = std::distance(tempADC.begin(), min_element_iter);

        float PeakAmp = abs( (float)min_value - Pedestal );
        //cout << PeakAmp << endl;


        for( int c=0; c<nADC; c++){
            H1_Pulse[j] -> Fill( c+1, (float)tempADC[c]-Pedestal );
        }  


        cPulse -> cd( j+1 );
        Pad_Pulse[j] = new TPad( Form("Pad_Pulse%d", j), "", 0, 0, 1, 1); 
        Pad_Pulse[j] -> SetMargin(.00, .00, .00, .00);
        Pad_Pulse[j] -> Draw();
        Pad_Pulse[j] -> cd();

        //Frame_Pulse[j] = Pad_Pulse[j] -> DrawFrame( 0, -1500, nADC, 1500);
        //Frame_Pulse[j] -> Draw();

        Pad_Pulse[j] -> cd();
        H1_Pulse[j] -> Draw( "SAME HIST X" );

    }//j, ch
    cPulse -> Draw();
}

