

//========================================
const char * zh_g_cpt[10] = { "npuz", "guess", "close_d ", "upd1 ", "upd2 ",
"fupd ", "hpair ", "htripl ", " ", " " };
#include "go_0xx_cpp.h"
#include "go_1xx_cpp.h"
#include "go_2xx_cpp.h"
#include "go_3xx_cpp.h"
#include "go_4xx_cpp.h"
#include "go_5xx_cpp.h"
void Go_0xx(){
	 switch (sgo.command) {
	// case 0: Go_c0(); break;// test on brute force
	 case 10: Go_c10(); break;// extract valid puzzles from entry
	 case 11: Go_c11(); break;// count valid puzzles in entry 
	 case 21: Go_c21(); break;// count valid puzzles in entry 
	 }
}
void Go_sol_1xx(){
	cout << "command 1xx command=" << sgo.command << endl;
	pm_go.opprint =sgo.bfx[9];
	pm_go.opprint2 = sgo.bfx[8];
	if (sgo.command >= 120 && sgo.command < 140) {
		Go_c1xx(sgo.command); return;
	}
	switch (sgo.command){
	case 110: Go_c110(); break;// template solve serate mode
	case 111: Go_c111(); break;// solve/split quick serate mode
	case 112: Go_c112(); break;// solve full explain mode
	case 118: Go_c118(); break;// study a sub grid
	case 199: Go_c199(); break;// current test 
	}
	cerr << "back go-sol_1xx" << endl;

}
void Go_gen_2xx(){
	cout << "command 2xx command=" << sgo.command << endl;
	switch (sgo.command){
	case 200: Go_c200(); break;// split the entry file in files 1;2;3
	case 201: Go_c201(); break;// change n clues or 1 to n clues
	case 202: Go_c202(); break;// gen symmetry of given

	case 210:  Go_c210(); break;// create a seed on a pattern
	case 211:  Go_c211(); break;// create a seed gen interim file
	case 212:  Go_c212(); break;// create a seed gen interim file

	case 217:  Go_c217(); break;// restore a data base (v1=n v2=guesses)
	case 218:  Go_c218(); break;// extract played seeds
	case 219:  Go_c219(); break;// restore a data base

	case 221:  Go_c221(); break;// gen +-1 out of the pattern std is high
	//case 222:  Go_c222(); break;// gen +-2 out of the pattern  
	//case 231:  Go_c231(); break;// change 1 add 1 out of the pattern 
	}
}
void Go_can_3xx(){
	cout << "command 3xx command=" << sgo.command << endl;
	switch (sgo.command) {
	case 310: Go_c310(); break;// canonical add if -v0- add -s0-
	case 320: Go_c320(); break;// canonical pattern add if -v0- add -s0-
	}

}
/* subtask v0 for task 400
0 add sequence
1 add string0
2 add nclues
9 add stcd puz to entry
10 '.' for empty cell
11 erase '"' in entry
12 cut entry to v1
15 mantext in output
16 mintext in output
21 extract 81 character starting in v1
22 first v1 puzzles in output 1 others output2
23 sampling start v1 one every v2
40 count digits
41 count given per band
42 count digits per band
43 count given per unit
44count digits per unit
*/

void Go_misc_4xx(){
	cout << "command 4xx command=" << sgo.command << endl;
	switch (sgo.command){
	case 400: Go_c400(); break;// small tasks  see subtask v0
	case 401: Go_c401(); break;// .dat to .txt  
	case 402: Go_c402(); break;// morph rows cols diag  s1 s2 v1
	case 440: Go_c440(); break;// parse game submissions  
	case 445: Go_c445(); break;// split the entry file on int param 
	case 446: Go_c446(); break;// split on pot hardest flexible criteria 
	case 470: Go_c470(); break;// extract not equal in regressive test
	case 480: Go_c480(); break;// add compressed clues to entry (game data base)
	case 481: Go_c480(); break;// check/update game data base
	case 484: Go_c484(); break;// restore game data base
	case 485: Go_c485(); break;// find close to entry in base
	case 490: Go_c490(); break;// extract puzzles with central or diagonal symmetry
	case 491: Go_c491(); break;// morph to pattern -s0-
	case 492: Go_c492(); break;// find minimal pattern for central diagonal symmetry

	case 495: Go_c495(); break;// check/update game data base

	}

}
void Go_misc_5xx() {
	cout << "command 5xx command=" << sgo.command << endl;
	switch (sgo.command) {
	case 500: Go_c500(); break;// check no 17 2 cells in one band 
	case 510: Go_c510(); break;// check no 17 2 cells in one band 
	case 511: Go_c511(); break;// check no 17 2 cells in one band 

	}

}

void Go_0( ){
	memset(p_cptg, 0, sizeof p_cptg);// used in debugging sequences only
	memset(p_cpt1g, 0, sizeof p_cpt1g);// used in debugging sequences only
	memset(p_cpt2g, 0, sizeof p_cpt2g);// used in debugging sequences only
	// open  outputs files 1;2;3 output +_filex.txt
	if (sgo.foutput_name){
		char zn[200];
		strcpy(zn, sgo.foutput_name);
		int ll = (int)strlen(zn);
		strcpy(&zn[ll], "_file1.txt");
		fout1.open(zn);  
		zn[ll + 5] = '2'; fout2.open(zn);
		zn[ll + 5] = '3'; fout3.open(zn);
	}
	switch (sgo.command / 100){
	case 0: Go_0xx(); break;
	case 1: Go_sol_1xx(); break;
	case 2: Go_gen_2xx(); break;
	case 3: Go_can_3xx(); break;
	case 4: Go_misc_4xx(); break;
	case 5: Go_misc_5xx(); break;
	}
	cerr << "go_0 return" << endl;
}
