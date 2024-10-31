#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include "NoticeMINITCB_V2ROOT.h"
using namespace std;

int main(int argc, char** argv)
{
	string ipAddr = "192.168.0.2";
	if (argc == 2) ipAddr = argv[1];

	//Define USB3TCB class
	NKMINITCB_V2 *tcb = new NKMINITCB_V2;

	//Open TCB 
	int tcp_Handle = tcb->MINITCB_V2open((char*)ipAddr.c_str());
	cout << "tcp_Handle: " << tcp_Handle << endl;
	//tcb->MINITCB_V2reset(tcp_Handle);	//Reset TCB 
	tcb->MINITCB_V2start(tcp_Handle);
	tcb->MINITCB_V2close(tcp_Handle);

	cout <<"TCB Started!\n";
	return 0;
}
