DAQ code for NOTICE KOREA FADC500 with miniTCB
## Single modules daq for Sci trigger

`./INIT.sh` : Initialize minitcb & NKFADC500 according to setup_singlemodule.txt
- **EXECUTE ONE MORE TIME** if Pedestal is not well applied;    ex) Pedestal: 2000  0  2000  0 

`./RUN.sh` : Specifies how many buffers to receive as an argument; if no argument is given, it will continue receiving indefinitely
- 1 buffer = 8 Events for Recording Length = 512 ns
- Display the number of buffers when they accumulate every 10 counts

`./STOP.sh` : Stop the ongoing run

## Draw figures
`root -l 'Draw_YH.cpp( RUNNUMBER_TO_DRAW )` : Visualize the data from the given RUNNUMBER as an argument
- Get ADC(waveform), triggered time, etc from `./data/FADCData_RUNNO.root`
- Get Threshold from `./data/log_RUNNO.txt`
- Figure is saved at `./fig/RUNNUMBER_TO_DRAW.pdf`


  -	Row 1: x = index of event, y = index of time, z(color) = ADC values
  -	Row 2: Average pulse shape without pedestal subtraction
  -	Row 3: Distribution of the pedestal
  -	Row 4: Average pulse shape with pedestal subtraction
  -	Row 5: Peak ADC histogram
	  - Column 3: Distribution of dt
    - Column 4: x = Peak ADC of CH1, y = Peak ADC of CH2.



