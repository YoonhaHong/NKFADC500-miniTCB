#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include "NoticeMINITCB_V2ROOT.h"
using namespace std;

int main(int argc, char** argv)
{
	std::string ipAddr = "192.168.0.2";
	if (argc == 2) ipAddr = argv[1];

	//Define NKMINITCB class
	NKMINITCB_V2 *tcb = new NKMINITCB_V2;

	//Open TCB
	int tcp_Handle = tcb->MINITCB_V2open((char*)ipAddr.c_str());
	tcb->MINITCB_V2stop(tcp_Handle); 
	//tcb->MINITCB_V2reset(tcp_Handle); //Reset TCB 
	tcb->MINITCB_V2close(tcp_Handle); //close MINITCB_V2 

	cout <<"TCB Stopped!\n";
	return 0;
}
