import os
import re
import time
import argparse
import ROOT
import numpy as np

def get_event_info(ADC):
    Pedestal = np.mean(ADC[:, 1:21], axis=1)  # 1부터 20까지의 평균
    max_values = np.max(ADC, axis=1)
    max_indices = np.argmax(ADC, axis=1) 
    Peak = max_values - Pedestal  

    return Peak, Pedestal, max_indices 
if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='NKFADC500 Drawing Code')
    parser.add_argument('-d', '--dirname', required=False, default = '../data',
                    help = 'directory of data')

    parser.add_argument('-r', '--runno', required=False, type=int, default = 20001,
                    help = 'RUNNUMBER of data to draw')

    parser.add_argument('-adc', '--adcmax', required=False, type=int, default = 100,
                    help = 'Maximum of ADC in figures')

    parser.add_argument('-dt', '--dtmax', required=False, type=int, default = 30,
                    help = 'Maximum of dt in figures')

    parser.add_argument('-dt1', '--dtcut1', required=False, nargs=2, default = (-5, 5),
                    help = 'dt cut of ADC,dt figures')

    parser.add_argument('-dt2', '--dtcut2', required=False, nargs=2, default = (-1, 1),
                    help = 'dt cut of ADC,dt figures')

    parser.add_argument
    args = parser.parse_args()

    ROOT.gStyle.SetOptStat(0)
    
    nCh = 4
    TrigTh = [25, 25, 50, 50]
    DLen = 512
    
    # Threshold parsing
    logFilePath = os.path.join(args.dirname, f"log_{args.runno}.txt")
    print( logFilePath)
    
    try:
        with open(logFilePath, 'r') as logFile:
            for line in logFile:
                if "Discrimination threshold :" in line:
                    print(line.strip())
                    break
    except FileNotFoundError:
        print("No log file! Cannot know ADC threshold")
    
    # Read the threshold values from the log file
    TrigTh = list( map(int, line.split()[-4:]) )
    
    # Open ROOT file and get the tree
    F = ROOT.TFile(os.path.join(args.dirname, f"FADCData_{args.runno}.root"))
    T = F.Get("T")
    
    nEvents = T.GetEntries()
    print(f"Number of events: {nEvents}")
    
    nADC = (DLen - 32) // 2
    nTDC = nADC // 4
    
    # Set branch addresses
    fDatTime = np.zeros(nCh, dtype=np.uint64)
    fADC = np.zeros((nCh, nADC), dtype=np.uint32)
    
    T.SetBranchAddress("dtime", fDatTime)
    T.SetBranchAddress("adc", fADC)

    # Histograms
    H2_Waveform = [ROOT.TH2F( f"H2_Waveform_ch{i+1}", f"Waveform_ch{i+1}", nADC, 0, nADC, 4095, 0, 4095) for i in range(nCh)];
    ADCMAX = 500
    ADCMIN = 200
    H1_Pedestal = [ROOT.TH1F(f"H1_Pedestal{i+1}", f"H1_Pedestal{i+1}", 200, 300, 500) for i in range(nCh)]
    H1_Pulse = [ROOT.TH1F(f"Pulse_ch{i+1}", "", nADC, 0, nADC) for i in range(nCh)]
    H1_Peak = [ROOT.TH1F(f"Peak_ch{i+1}", "", 2000 + 10, -10, 2000) for i in range(nCh)]
    H1_PeakCut = [ROOT.TH1F(f"PeakCut_ch{i+1}", "", 2000 + 10, -10, 2000) for i in range(nCh)]
    
    H1_dt = ROOT.TH1F("dt", "dt(SC2-SC1)", 2 * nADC, -nADC, nADC)
    H1_dtCoin1 = ROOT.TH1F("H1_dtCoin1", "", 2 * nADC, -nADC, nADC)
    H1_dtCoin2 = ROOT.TH1F("H1_dtCoin2", "", 2 * nADC, -nADC, nADC)
    
    H2_Peak = ROOT.TH2F("H2_Peak_ch1ch2", "CH1 vs CH2", 110, 0, 110, 110, 0, 110)
    H1_PeakCoin1 = [ROOT.TH1F(f"PeakCoin1_ch{i+1}", "", 2000 + 10, -10, 2000) for i in range(nCh)]
    H1_PeakCoin2 = [ROOT.TH1F(f"PeakCoin2_ch{i+1}", "", 2000 + 10, -10, 2000) for i in range(nCh)]
    
    Rate = [ROOT.TGraph() for _ in range(nCh)]
    
    # Read the FADC data and fill H2
    T.GetEntry(0)
    StartTime = fDatTime[0]
    
    for iEv in range(nEvents):
        T.GetEntry(iEv)

        Peak, Pedestal, PeakTime = get_event_info(fADC)
        for iCh in range(2):

            for c in range(nADC):
                H2_Waveform[iCh].Fill(c, fADC[iCh][c])
                if ADCMAX < fADC[iCh][c] : ADCMAX=fADC[iCh][c] 
                elif ADCMIN > fADC[iCh][c] : ADCMIN=fADC[iCh][c] 
            H1_Pedestal[iCh].Fill(Pedestal[iCh])
            H1_Peak[iCh].Fill(Peak[iCh])

            if Peak[iCh] < TrigTh[iCh]:
                H1_PeakCut[iCh].Fill(Peak[iCh])
                Rate[iCh].SetPoint(Rate[iCh].GetN(), fDatTime[0] - StartTime, Peak[iCh])

        dt = PeakTime[1] - PeakTime[0]
        H1_dt.Fill(dt)
        H2_Peak.Fill(Peak[0], Peak[1])

        if args.dtcut1[0] <= dt <= args.dtcut1[1]:
            H1_PeakCoin1[0].Fill(Peak[0])
            H1_PeakCoin1[1].Fill(Peak[1])
            H1_dtCoin1.Fill(dt)

        if args.dtcut2[0] <= dt <= args.dtcut2[1]:
            H1_PeakCoin2[0].Fill(Peak[0])
            H1_PeakCoin2[1].Fill(Peak[1])
            H1_dtCoin2.Fill(dt)

    # Create canvas and draw histograms
    c1 = ROOT.TCanvas("c1", "c1", 600*3, 600*2)
    c1.Divide(3, 2)
    
    PeakPad = [ROOT.TPad(f"Pad_CH{i+1}", f"CH{i+1}", 0, 0, 1, 1) for i in range(2)]
    nRebin = 2
    YAxisMax = max(H1_Peak[0].GetMaximum() * 10, 1)

    print(ADCMIN, ADCMAX)
    for b in range(2):
        PeakPad[b].SetMargin(0.13, 0.05, 0.12, 0.05)
        PeakPad[b].SetLogy()
        
        for c in range(2):
            c1.cd(b + 3 * c + 1)
            if c == 0:
                PeakPad[b].Draw()
                PeakPad[b].cd()

                PeakFrame = ROOT.gPad.DrawFrame(-5, 0.5, args.adcmax, YAxisMax)
                PeakFrame.GetXaxis().SetLabelSize(0.05)
                PeakFrame.GetYaxis().SetLabelSize(0.05)
                PeakFrame.GetXaxis().SetTitleSize(0.05)
                PeakFrame.GetYaxis().SetTitleSize(0.05)
                PeakFrame.GetXaxis().SetTitle(f"SC{b+1} ADC_{{peak}}")  # f-string을 사용하여 채널 이름 설정
                PeakFrame.Draw()

                H1_Peak[b].Rebin(nRebin)
                H1_Peak[b].Draw("same")

                H1_PeakCut[b].SetFillColor(2)
                H1_PeakCut[b].SetFillStyle(3003)
                H1_PeakCut[b].Rebin(nRebin)
                H1_PeakCut[b].Draw("same")

                H1_PeakCoin1[b].SetLineColor(2)
                H1_PeakCoin1[b].SetLineStyle(2)
                H1_PeakCoin1[b].SetLineWidth(3)
                H1_PeakCoin1[b].Rebin(nRebin)
                H1_PeakCoin1[b].Draw("same")

                H1_PeakCoin2[b].SetLineColor(3)
                H1_PeakCoin2[b].SetLineStyle(2)
                H1_PeakCoin2[b].SetLineWidth(3)
                H1_PeakCoin2[b].Rebin(nRebin)
                H1_PeakCoin2[b].Draw("same")
            elif c == 1:
                H2_Waveform[b].Draw("COLZ")
                H2_Waveform[b].GetYaxis().SetRange(int(ADCMIN), int(ADCMAX))

    c1.cd(3)
    dtMax = H1_dt.GetMaximum()
    dtFrame = ROOT.gPad.DrawFrame(-args.dtmax, 0.5, args.dtmax, dtMax * 1.3)
    dtFrame.GetXaxis().SetLabelSize(0.05)
    dtFrame.GetYaxis().SetLabelSize(0.05)
    dtFrame.GetXaxis().SetTitleSize(0.05)
    dtFrame.GetYaxis().SetTitleSize(0.05)
    dtFrame.GetXaxis().SetTitle("#Delta t(SC2-SC1) [2ns]")
    dtFrame.Draw()

    H1_dt.Draw("SAME")

    H1_dtCoin1.SetFillColor(2)
    H1_dtCoin1.SetLineColor(2)
    H1_dtCoin1.Draw("SAME")

    H1_dtCoin2.SetFillColor(3)
    H1_dtCoin2.SetLineColor(3)
    H1_dtCoin2.Draw("SAME")

    Int_All = H1_dt.Integral()
    Int_Coin1 = H1_dtCoin1.Integral()
    Int_Coin2 = H1_dtCoin2.Integral()

    leg_dt = ROOT.TLegend(0.16, 0.61, 0.36, 0.81)
    leg_dt.SetFillStyle(0)
    leg_dt.SetBorderSize(0)
    leg_dt.SetTextSize(0.05)
    leg_dt.AddEntry(H1_dt, f"# of Events: {Int_All:.0f}", "l")
    leg_dt.AddEntry(H1_dtCoin1, f"# of {args.dtcut1[0]*-2} ns #leq #Delta t #leq {args.dtcut1[1]*2} ns : {Int_Coin1:.0f}")
    leg_dt.AddEntry(H1_dtCoin2, f"# of {args.dtcut2[0]*2} ns #leq #Delta t #leq {args.dtcut2[1]*2} ns : {Int_Coin2:.0f}")
    leg_dt.Draw()

    c1.cd(6)
    ROOT.gPad.SetLogy(0)
    H2_Peak.Draw("colz")

    c1.Draw()
    c1.Update()
    c1.SaveAs("Test.pdf")





