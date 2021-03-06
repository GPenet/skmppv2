/* solving a batch of puzzles.
*/
#define _CRT_SECURE_NO_DEPRECATE
#define ISSOLVERSTEP

#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
using namespace std;
#include "main.h"
#include "solver_step.h"


//extern BF_FIX bf_fix;
//extern OPTIONS options;
extern ofstream  fout1, fout2, fout3, //fout4,
fout_diam,fout_pearl,fout_l65;
//extern FINPUT finput;

const char *orig[]={"row ","column ","box "," "};
const char *lc="ABCDEFGHI";
const char *orig1="RCB ";

PM_GO pm_go;
extern ZH_1D_GLOBAL zh1d_g;
extern ZHOU    zhou[50], zhou_i, zhou_solve;
extern ZH_GLOBAL zh_g;
extern ZH_GLOBAL2 zh_g2;
extern SGO sgo;

#include "solver_step_dyn_cpp.h"

// index rc/3 -> 3 boxes pas venu par tables, voir pourquoi
const unsigned int rowcoltobox[18]= 
{18,19,20,21,22,23,24,25,26,18,21,24,19,22,25,20,23,26};
//============================================= GINTBUF STORE_UL
int GINTBUF::Get(int n){
	if (ncur + n > n32) return -1; 
	int nret = ncur; 
	ncur += n;
	return nret;
}
int GINTBUF::Store(GINT * t){
	int n = t[0].u16[0];
	if (ncur + n > n32) return -1;
	memcpy(&buf[ncur], t, n * 4);
	int nret = ncur;
	ncur += n;
	return nret;
}
int GINTBUF::GetBack(GINT * d){
	if (nback >= ncur)return 0;
	GINT * t = &buf[nback];
	int n = t[0].u16[0];
	nback += n;
	memcpy(d, t, n * 4);
	return 1;
}
void GINTBUF::EraseOlds(int nkeep){
	if (nkeep < ncur || (!nkeep))return;
	int n = 0,i;
	for (i = nkeep; i < ncur; i++) buf[n++] = buf[i];
	ncur = i;
}
void STORE_UL::Print(const char * lib){
	cout << lib << " action on UL" << endl;
	char ws[82];
	cout << cells.String3X(ws) << "ul pattern type=" << type << endl;
	if (type == 1)
		cout << one_digit_elims.String3X(ws) << " one digit elimibations" << endl;
}
//============================================= BUG
int  BUG::DoChange(int iplus, int el){
	int  & wd = tplus_digits[iplus];
	int & elpar = el_par_ch[el], wx = wd & elpar;
	if (_popcnt32(wx) != 2) return 1; // not valid
	if (elpar != wx)return 1; // here one cell with extra digits, must be the same
	if (wchange) return(wchange != wx);// return 0 if ok
	wchange = wx;
	change_plus[iplus] = wd ^ wx;
	return 0;
}
int BUG::Init(){
	if (pm_go.opprint2 & 2)cout << "entry initbug " << endl;
	pm_go.hint.Set_Target(56);
	zz = zhou_solve.cells_unsolved;
	wplus = zz - zh_g.pairs;
	BF128 ww = wplus;
	ntplus = wplus.Count();
	if (0){
		char ws[82];
		cout << ww.String3X(ws) << " plus cells ntplus=" << ntplus << endl;
	}
	if (ntplus > 6) return 1;// maxi 6 cells
	aigun = 0;
	memset(el_par_ch, 0, sizeof el_par_ch);
	r_c.f = c_c.f = b_c.f = 0;   // used rows cols boxes by plus
	for (int i = 0; i < 9; i++)	r_cn[i] = c_cn[i] = b_cn[i] = 0;
	// set the parity of digits for bivalue cells in all elements
	or_plus_tot = 0;
	int nn = 0;
	while ((cell = ww.getFirstCell()) >= 0){
		ww.Clear_c(cell);
		zz &= cell_z3x[cell];
		if (zz.isEmpty())return 1; // must have cells seen by all wplus
		register int wd = zh_g2.dig_cells[cell];
		or_plus_tot |= wd;
		tplus[nn] = cell; tplus_digits[nn++] = wd;
		CELL_FIX &p = cellsFixedData[cell];
		r_c.Set(p.el);  r_cn[p.el]++;
		c_c.Set(p.pl);  c_cn[p.pl]++;
		b_c.Set(p.eb);  b_cn[p.eb]++;
	}
	if (pm_go.opprint2 & 2){
		char ws[82];
		cout << zz.String3X(ws) << " zz final ntplus=" << ntplus << endl;
		cout << oct << r_c.f << endl << c_c.f << endl << b_c.f << dec << endl;
	}
	ww = zh_g.pairs;
	while ((cell = ww.getFirstCell()) >= 0){
		ww.Clear_c(cell);
		register int wd = zh_g2.dig_cells[cell];
		CELL_FIX &p = cellsFixedData[cell];
		el_par_ch[p.el] ^= wd;	el_par_ch[p.plu] ^= wd;	el_par_ch[p.ebu] ^= wd;
	}
	// check all unused sets are already in parity
	for (int i = 0; i<9; i++){
		if (r_c.Off(i) && el_par_ch[i]) return 1;
		if (c_c.Off(i) && el_par_ch[i + 9]) return 1;
		if (b_c.Off(i) && el_par_ch[i + 18]) return 1;
	}
	if (pm_go.opprint2 & 2)cout << "check unused finished ok" << endl;
	// try now to solve each plus cell must be same in each unit
	int ntosolve = ntplus, solved = 0, aig;
	or_change = 0;
	while (ntosolve){
		aig = 1;
		for (int i = 0; i<ntplus; i++) if (!(solved & (1 << i))){
			int cell = tplus[i];
			CELL_FIX &p = cellsFixedData[cell];
			if (r_cn[p.el]>1 && c_cn[p.pl]>1 && b_cn[p.eb]>1)
				continue; // not possible to solve it yet
			wchange = 0;
			aig = 0;
			if (r_cn[p.el] == 1) if (DoChange(i, p.el)) return 1;
			if (c_cn[p.pl] == 1) if (DoChange(i, p.plu)) return 1;
			if (b_cn[p.eb] == 1) if (DoChange(i, p.ebu)) return 1;
			// adjust parity in unit with more than one
			tplus_digits[i] = wchange;
			el_par_ch[p.el] ^= wchange;
			el_par_ch[p.plu] ^= wchange;
			el_par_ch[p.ebu] ^= wchange;
			r_cn[p.el]--; c_cn[p.pl]--;  b_cn[p.eb]--;
			ntosolve--;
			or_change |= change_plus[i];
			solved |= 1 << i;
		}
		if (aig) return 1;
	}
	if (_popcnt32(or_change) > 4)	return 1;  // maxi  4 extra candidates
	return 0;
}
//============================================= WWUR2
void WWUR2::Init(int eunit){
	unit = eunit;
	wcells = zhou_solve.cells_unsolved;
	wcells &= units3xBM[unit];
	wcells_nserate = wcells.Count();
	if (locdiag)	cout << " wwur2 init cells=" << cellsFixedData[cell1].pt << " " << cellsFixedData[cell2].pt << " unit=" << unit
		<< " ul_plus=" << ul_plus << " degree=" << degree << " rbase=" << rbase << endl;
	cells_ur.clear(); cells_others_ur.clear(); cells_3.clear();
	wcells.Clear_c(cell1); wcells.Clear_c(cell2);// now cells to consider
	digits_ur = digs | digitsothers;
	for (int idig = 0; idig < 9; idig++){// find cells
		register int bit = 1 << idig;
		BF128 w = zh_g2.pm.pmdig[idig] & wcells;
		if (w.isEmpty())continue;
		if (digs & bit)cells_ur |= w;
		if (digitsothers & bit)cells_others_ur |= w;
		if (!(digits_ur & bit))cells_3 |= w;
	}
	w_b = cells_others_ur - cells_ur;
	w_c = wcells - cells_ur - cells_others_ur;
	wnaked = cells_others_ur - cells_3 - cells_ur;
	wclean = wcells - wnaked;
	if (0 && locdiag){
		char ws[82];
		cout << cells_ur.String3X(ws) << " cells_ur" << endl;
		cout << cells_others_ur.String3X(ws) << " cells_others_ur" << endl;
		cout << cells_3.String3X(ws) << " cells_3" << endl;
		cout << w_c.String3X(ws) << " w_c" << endl;
	}

}
int  WWUR2::Hidden_pair(){
	if (cells_ur.Count() == 1 && go_naked){// hidden pair see if active
		int cell = cells_ur.getFirstCell(), dcell = zh_g2.dig_cells[cell];
		if (dcell != digs){// some elimination
			zhou_solve.CleanCellForDigits(cell, dcell^digs);
			pm_go.hint.Done(rbase);	det_mess = " hidden pair ";
			return 1;
		}
	}
	return 0;
}
int WWUR2::Naked_pair(){
	if (0){
		cout << "search naked pair 	digits_ur = digs | digitsothers " << oct << digits_ur
			<< " " << digs << " " << digitsothers << dec << endl;
		char ws[82];
		cout << cells_ur.String3X(ws) << " cells_ur" << endl;
		cout << cells_others_ur.String3X(ws) << " cells_others_ur" << endl;
		cout << cells_3.String3X(ws) << " cells_3" << endl;
		cout << w_c.String3X(ws) << " w_c" << endl;
		cout << wclean.String3X(ws) << " wclean" << endl;
	}
	if (wclean.isNotEmpty()){// possible active naked set
		if (nothers == 2 && wnaked.Count() == 1 && cells_others_ur.Count()>1){// active naked pair ?
			for (int i = 0; i < 9; i++) if (digitsothers & (1 << i))
				zhou_solve.FD[i][0] -= wclean;
			pm_go.hint.Done(rbase + 1); det_mess = " naked pair ";
			return 1;
		}
	}
	return 0;
}
void WWUR2::InitFreeDigits(){
	dfree = 0x1ff ^ digits_ur;
	nfree = 0;
	for (int i = 0; i < 9; i++)
		if ((dfree & (1 << i)) && (wcells & zh_g2.pm.pmdig[i]).isNotEmpty())
			tfree[nfree++] = i;
}
int  WWUR2::Hidden_triplet(){
	if (!nfree) return 0;
	for (int i1 = 0; i1 < nfree; i1++){
		BF128 wd1 = cells_ur | (wcells & zh_g2.pm.pmdig[tfree[i1]]);
		if (wd1.Count() != 2) continue;
		int aig = 0, dw = digs | (1 << tfree[i1]);
		for (int i = 0; i < 9; i++) if (!(dw & (1 << i))){
			BF128 clean = zh_g2.pm.pmdig[i] & wd1;
			if (clean.isNotEmpty()){
				aig = 1;
				zhou_solve.FD[i][0] -= clean;
			}
		}
		if (aig){ pm_go.hint.Done(rbase + 1); det_mess = " hidden triplet "; return 1; }
		return 0;// nopn active triplet
	}
	return 0;
}
int WWUR2::Naked_triplet(){
	if (nothers == 3 && wnaked.Count() == 2 && cells_others_ur.Count()>2){// active naked triplet
		for (int i = 0; i < 9; i++) if (digitsothers & (1 << i))
			zhou_solve.FD[i][0] -= wclean;
		pm_go.hint.Done(rbase + 2); det_mess = " naked triplet "; return 1;
	}
	return 0;
}
int WWUR2::Naked_triplet21(){
	if (nothers - 2 || !nfree) return 0;
	if (locdiag)cout << "try  naked triplet 2+1" << endl;
	for (int i1 = 0; i1 < nfree; i1++){
		BF128 wd1 = w_b  & zh_g2.pm.pmdig[tfree[i1]];
		if (wd1.Count() >= 2) {		// check that no other digit is there
			for (int i2 = 0; i2 < nfree; i2++) if (i2 - i1){
				wd1 -= zh_g2.pm.pmdig[tfree[i2]];
			}
			if (wd1.Count() != 2) continue;
			int  dw = digitsothers | (1 << tfree[i1]);
			BF128 clean = cells_others_ur | (zh_g2.pm.pmdig[tfree[i1]] & wcells);
			clean -= wd1;
			if (zhou_solve.CleanCellsForDigits(clean, dw)){
				pm_go.hint.Done(rbase + 2); det_mess = " naked triplet 2+1 "; return 1;
			}
		}
	}
	return 0;
}
int WWUR2::Hidden_quad(){
	if (nfree < 2) return 0;
	if (locdiag)cout << "try hidden  quad 2+2" << endl;
	for (int i1 = 0; i1 < nfree - 1; i1++){
		BF128 wd1 = cells_ur | (wcells & zh_g2.pm.pmdig[tfree[i1]]);
		if (wd1.Count() > 3) continue;
		for (int i2 = i1 + 1; i2 < nfree; i2++){// test all pairs of digits
			BF128 wd2 = wd1 | (wcells & zh_g2.pm.pmdig[tfree[i2]]);
			if (wd2.Count() != 3) continue;
			int aig = 0, dw = digs | (1 << tfree[i1]) | (1 << tfree[i2]);
			for (int i = 0; i < 9; i++) if (!(dw & (1 << i))){
				BF128 clean = zh_g2.pm.pmdig[i] & wd2;
				if (clean.isNotEmpty()){
					aig = 1;
					zhou_solve.FD[i][0] -= clean;
				}
			}
			if (aig){ pm_go.hint.Done(rbase - 1); det_mess = " hidden quad 2+2 "; return 1; }
			return 0;// not active quad 
		}
	}
	return 0;
}
int WWUR2::Naked_quad(){
	if (nothers == 4 && wnaked.Count() == 3 && cells_others_ur.Count()>3){// active naked quad
		for (int i = 0; i < 9; i++) if (digitsothers & (1 << i))
			zhou_solve.FD[i][0] -= wclean;
		pm_go.hint.Done(rbase + 3); det_mess = " naked quad "; return 1;
	}
	return 0;
}
int WWUR2::Naked_quad31(){
	if (nothers - 3) return 0;
	if (locdiag)cout << "try  naked quad 3+1" << endl;
	for (int i1 = 0; i1 < 9; i1++){// can be any digit not other digits
		int bit1 = 1 << i1;
		if (digitsothers & bit1) continue;// not other digits
		BF128 w1 = cells_others_ur  & zh_g2.pm.pmdig[i1];
		if (w1.isEmpty())continue;
		BF128 wd1 = w1 | wnaked;
		if (wd1.Count() >= 3) {		// check that no other digit is there
			int dw = digitsothers | bit1;
			for (int i2 = 0; i2 <9; i2++){
				if (dw & (1 << i2)) continue;
				wd1 -= zh_g2.pm.pmdig[i2];
			}
			if (wd1.Count() != 3) continue;
			BF128 clean = cells_others_ur | (zh_g2.pm.pmdig[i1] & wcells);
			clean -= wd1;
			if (zhou_solve.CleanCellsForDigits(clean, dw)){
				pm_go.hint.Done(rbase + 3); det_mess = " naked quad 3+1 "; return 1;
			}
		}
	}
	return 0;
}
int WWUR2::Naked_quad22(){
	if (0){
		cout << "search quad 2+2 	digits_ur = digs | digitsothers " << oct << digits_ur
			<< " " << digs << " " << digitsothers << dec << endl;
		char ws[82];
		cout << cells_ur.String3X(ws) << " cells_ur" << endl;
		cout << cells_others_ur.String3X(ws) << " cells_others_ur" << endl;
		cout << cells_3.String3X(ws) << " cells_3" << endl;
		cout << w_b.String3X(ws) << " w_b" << endl;
		cout << wclean.String3X(ws) << " wclean" << endl;
	}
	if (nothers == 2){// try 2  extra digit +2 can be any other digit including common digits
		if (locdiag)cout << "try  naked quad 2+2 " << endl;
		for (int i1 = 0; i1 < 8; i1++){
			int bit1 = 1 << i1;
			if (digitsothers & bit1) continue;// not other digits
			BF128 w1 = cells_others_ur  & zh_g2.pm.pmdig[i1];
			if (w1.isEmpty())continue;
			BF128 wd1 = w1 | wnaked;
			for (int i2 = i1 + 1; i2 <9; i2++){
				int bit2 = 1 << i2;
				if (digitsothers & bit2) continue;// not other digits
				BF128 w2 = cells_others_ur  & zh_g2.pm.pmdig[i2];
				if (w2.isEmpty())continue;
				BF128 pci1i2 = wcells &zh_g2.pm.pmdig[i1] & zh_g2.pm.pmdig[i2] & zh_g.pairs;
				BF128 wd2 = wd1 | w2;
				wd2 |= pci1i2;
				if (wd2.Count() >= 3) {		// check that no other digit is there
					int dw = digitsothers | bit1 | bit2;
					for (int i3 = 0; i3 < 9; i3++) {
						if (dw & (1 << i3)) continue;
						wd2 -= zh_g2.pm.pmdig[i3];
					}
					if (wd2.Count() != 3) continue;
					BF128 clean = cells_others_ur | ((zh_g2.pm.pmdig[i1] | zh_g2.pm.pmdig[i2]) & wcells);
					clean -= wd2;
					if (zhou_solve.CleanCellsForDigits(clean, dw)){
						pm_go.hint.Done(rbase + 3); det_mess = " naked quad 2+2 "; return 1;
					}
				}
			}

		}
	}
	return 0;
}
int WWUR2::Go_serate_Fast(GINT64 & t){
	Set(t, " ");
	CELL_FIX cf1 = cellsFixedData[cell1], cf2 = cellsFixedData[cell2];
	if (cf1.eb == cf2.eb) if (Go_serate_unit_Fast(cf1.ebu)) return 1;
	if (cf1.el == cf2.el) if (Go_serate_unit_Fast(cf1.el)) return 1;
	if (cf1.pl == cf2.pl) if (Go_serate_unit_Fast(cf1.plu)) return 1;
	return 0;
}
int WWUR2::Go_serate(GINT64 & t, int etarget){

	Set(t, " ");

	locdiag = pm_go.opprint2 & 2;
	target = etarget;
	degree = target - rbase + 1;
	// serate degree 2 rating 45 hidden pair but rating 46 naked pair
	int hdegree = degree + 1;
	if (hdegree < nothers) hdegree = 0;// not ok for a hidden
	else if (hdegree >4)hdegree = 0;

	int ndegree = degree; // now the degree for naked 
	if (ndegree < nothers) ndegree = 0;// skipping 45 for naked
	else if (degree>4)ndegree = 0;// not in the target

	if (!(hdegree | ndegree)) return 0;// nothing to do


	CELL_FIX cf1 = cellsFixedData[cell1], cf2 = cellsFixedData[cell2];
	if (pm_go.opprint & 1)cout << "wwur2 go_serate cells " << cf1.pt << "  " << cf2.pt
		<< " target=" << target << endl;

	if (cf1.eb == cf2.eb) if (Go_serate_unit(cf1.ebu, hdegree, ndegree)) goto ok;
	if (cf1.el == cf2.el) if (Go_serate_unit(cf1.el, hdegree, ndegree)) goto ok;
	if (cf1.pl == cf2.pl) if (Go_serate_unit(cf1.plu, hdegree, ndegree)) goto ok;
	return 0;;
ok:
	if (pm_go.opprint & 2)cout <<"URs/ULs "<< det_mess << " cells " << cf1.pt << "  " << cf2.pt
		<< " rating=" << pm_go.hint.rating_done << endl;
	return 1;
}
int WWUR2::Go_serate_unit_Fast(int eunit){
	Init(eunit);
	InitFreeDigits();
	if (nothers > 2) goto triplet;
	if (wcells_nserate < 4) return 0;
	if (Naked_pair()) return 1;
	if (wcells_nserate > 4 && Hidden_pair()) return 1;
triplet:
	if (nothers > 3) goto quad;
	if (wcells_nserate < 6) return 0;
	switch (nothers){
	case 2:
		if (Naked_triplet21()) return 1;
		break;
	case 3:
		if (Naked_triplet()) return 1;
	}
	if (wcells_nserate>6 && Hidden_triplet()) return 1;
quad:
	if (wcells_nserate < 8) return 0;
	switch (nothers){
	case 2:
		if (Naked_quad22()) return 1;
		break;
	case 3:
		if (Naked_quad31()) return 1;
		break;
	case 4:
		if (Naked_quad()) return 1;
	}
	if (wcells_nserate>8 && Hidden_quad()) return 1;
	return 0;
}
int WWUR2::Go_serate_unit(int eunit, int hdegree, int ndegree){
	Init(eunit);
	if (pm_go.opprint & 1)cout << " go_serate unit " << eunit << "  hdegree=" << hdegree
		<< " ndegree=" << ndegree << " go_naked=" << go_naked << endl;

	InitFreeDigits();
	go_naked = wcells_nserate - 2 * hdegree;

	if (hdegree && go_naked>0) {// a hidden to search
		switch (hdegree){// try the appropriate hidden 
		case 2:
			if (Hidden_pair()) return 1;
			break;
		case 3:
			if (Hidden_triplet()) return 1;
			break;
		case 4:
			if (Hidden_quad()) return 1;
			break;
		}

	}
	go_naked = wcells_nserate - 2 * ndegree;
	//cout << "go_naked for naked=" << go_naked << endl;
	if (go_naked < 0) return 0;
	if (ndegree) {// naked  to search
		switch (ndegree){// try the appropriate hidden 
		case 2:
			if (Naked_pair()) return 1;
			break;
		case 3:
			switch (nothers){
			case 2:
				if (Naked_triplet21()) return 1;
				break;
			case 3:
				if (Naked_triplet()) return 1;
				break;
			}
			break;
		case 4:
			switch (nothers){
			case 2:
				if (Naked_quad22()) return 1;
				break;
			case 3:
				if (Naked_quad31()) return 1;
				break;
			case 4:
				if (Naked_quad()) return 1;
				break;
			}
			break;
		}

	}

	return 0;
}
//============================================= XSTATUS
void XSTATUS::Init(int dig){
	digit = dig;
	maxpas = 20;
	active = zh_g2.active_floor & (1 << digit);
	if (!active) return;
	if (pm_go.opprint2 & 2)cout << "init active digit " << dig + 1 << endl;
	memcpy(dig_sets, zh_g2.dig_rows[digit], 36);
	memcpy(&dig_sets[9], zh_g2.dig_cols[digit], 36);
	memcpy(&dig_sets[18], zh_g2.dig_boxes[digit], 36);
	pm = zh_g2.pm.pmdig[dig];
	elims = zh_g2.elim_floor[dig];
	pmbiv.SetAll_0();
	bivsets.f=0;
	for (int iu = 0; iu < 27; iu++){
		if (_popcnt32(dig_sets[iu]) == 2){
			BF128 w = units3xBM[iu]; w &= pm;
			bivsets.Set(iu);
			pmbiv |= w;
		}
	}
}

int XSTATUS::R65Xexpand(int xcell1, int xcell2, int loop, int * tback, BF128 & loopend){
	BF128 expon, expoff;
	expon.SetAll_0();	expoff = expon;
	if (loop){ expon.Set(xcell2);	expoff.Set(xcell1); }
	else { expon.Set(xcell1);	expoff.Set(xcell2); }	
	GINT64 t[160];// xcand, indsource
	int   nt = 2,  ntd = 1, ntp, npas = 1, xcell;
	t[0].u64 = xcell1;	t[1].u64 = xcell2;// source to 0
	if (0 &&pm_go.opprint2 & 4){
		cout << cellsFixedData[From_128_To_81[xcell1]].pt
			<< " " << cellsFixedData[From_128_To_81[xcell2]].pt
			<< " loop=" << loop << endl;
		if (loop){
			char ws[82];
			cout << loopend.String3X(ws) << " loopend condition"<<endl;
		}
	}
	while (npas++ < maxpas){ // safety, should never go so far
		int  offstep = (npas & 1) ^ loop;
		//if ((pm_go.opprint2 & 4) && !loop)cout << " pas="<<npas << " offstep="<<offstep << endl;
		ntp = ntd;
		ntd = nt;
		for (int i = ntp; i < ntd; i++){
			int wxc = t[i].u32[0],wc=From_128_To_81[wxc];
			BF128 wz = cell_z3x[wc];// cells seen
			//if ((pm_go.opprint2 & 4)&& !loop)cout << cellsFixedData[wc].pt <<" go" << endl;
			if (!offstep){// off to on mode
				wz &= pmbiv;
				BF128 w; w.SetAll_0();
				CELL_FIX & cf = cellsFixedData[wc];
				if (bivsets.On(cf.el)) w |= units3xBM[cf.el];
				if (bivsets.On(cf.plu)) w |= units3xBM[cf.plu];
				if (bivsets.On(cf.ebu)) w |= units3xBM[cf.ebu];
				wz &= (w- expon); //no way back
				if (loop && (loopend & wz).isNotEmpty()){// loopend
					tback[npas + 1] = xcell1;
					tback[npas] = (loopend & wz).getFirst128();
					register int Ri = i, Rn = npas;
					while (--Rn >= 0) {
						tback[Rn] = t[Ri].u32[0];
						Ri = t[Ri].u32[1];
					}
					return npas + 1;
				}
				if (wz.On(xcell1)) return 0; // contradiction in loop or loop in chan
				goto add;
			}
			else{// on to off mode
				if ((!loop) && npas > 2 && wz.On(xcell1)){// contradiction
					//if ((pm_go.opprint2 & 4) && !loop)cout << "end seen  pas=" << npas  << endl;
					tback[npas] = xcell1;
					register int Ri = i, Rn = npas;
					while (--Rn >= 0) {
						tback[Rn] = t[Ri].u32[0];
						Ri = t[Ri].u32[1];
					}
					return npas;
				}
				wz &= pmbiv;
				wz -= expoff;
			}
		add:
			if (wz.isEmpty()) 	continue;;
			while ((xcell = wz.getFirst128()) >= 0){
				wz.clearBit(xcell);
				t[nt].u32[1] = i;
				t[nt++].u32[0] = xcell;
				if (offstep) expoff.setBit(xcell); else expon.setBit(xcell);
			}
		}
		if (nt == ntd) return 0;
	}
	return 0;// safety should never be  more than 20 steps
}
void XSTATUS::AddStart(int xcell, int unit, GINT64 * t, int & nt, BF128 * tx, BF128 & x){
	if (0)cout << "addstart unit"<<unit<<" "
		<< cellsFixedData[From_128_To_81[xcell]].pt << endl;
	GINT64 w;
	w.u32[0] = xcell; w.u32[1] = unit;
	for (int i = 0; i < nt; i++) if (w.u64 == t[i].u64&& tx[nt]==x) return;
	tx[nt] = x;
	t[nt++] = w;
}
int XSTATUS::XCycle(int fast){
	if (!active) return 0;
	if (pm_go.opprint & 1){
		cout << "try XCycle for digit " << digit + 1 << endl;
		zhou_solve.DebugDigit(digit);
		char ws[82];
		cout << elims.String3X(ws) << " elims � voir" << endl;
	}
	// start from bi values seen by an elimination
	int xce, xce1, xce2, tback[30], nstarts = 0,iret=0;
	GINT64 tstarts[50];
	BF128 we = elims,txstarts[50]; // we still to study
	while ((xce = we.getFirst128()) >= 0){// next cell to elim
		we.clearBit(xce);
		int ce = From_128_To_81[xce];
		BF128 wwce = cell_z3x[ce]; wwce &= pmbiv; //possible starts
		if (wwce.Count() < 2) continue;
		CELL_FIX &cf = cellsFixedData[ce];
		//cout << "pour elim " << cf.pt << endl;

		for (int j = 0; j < 3; j++){// try sets not bivalues
			int unit = (j < 1) ? cf.el : ((j < 2) ? cf.plu : cf.ebu);
			BF128 zu = units3xBM[unit]; zu &= pmbiv;
			zu.clearBit(xce);// not the target if it is a bivalue
			if (zu.Count() < 2) continue;// usually 2 bivalues not more
			while (zu.Count() > 1){
				xce1 = zu.getFirst128();// take and clear the first cell
				zu.clearBit(xce1);
				CELL_FIX &cf1 = cellsFixedData[From_128_To_81[xce1]];
				for (int j1 = 0; j1 < 3; j1++){// look for the bi value unit
					int unit1 = (j1 < 1) ? cf1.el : ((j1 < 2) ? cf1.plu : cf1.ebu);
					if (unit1 == unit)continue;
					if (bivsets.Off(unit1))continue;
					AddStart(xce1, unit1, tstarts, nstarts,txstarts,zu);					
				}
			}

		}
	}
	//cout << "nstarts=" << nstarts << endl;
	for (int ist = 0; ist < nstarts; ist++){// now try each start
		int xce1 = tstarts[ist].u32[0], unit = tstarts[ist].u32[1];
		BF128 zu = units3xBM[unit]; zu &= pmbiv; zu.clearBit(xce1);
		xce2 = zu.getFirst128();
		int npas = R65Xexpand(xce1, xce2, 1, tback,txstarts[ist]);
		if (!npas)continue;
		if (npas <= 6 || fast){// do elims set iret to 1
			pm_go.nstore_xlc = 0; //clean longer if any
			if (CleanLoop(tback, npas)){
				if (npas < maxpas)maxpas = npas;
				iret = 1;
				if (pm_go.opprint & 2) {
					cout << "loop back npas=" << npas << " digit=" << digit + 1 << endl;
					for (int i = 0; i <= npas; i++) {
						int xcell = tback[i], cell = From_128_To_81[xcell];
						if (!(i & 1)) cout << "~";
						if (cell >= 0 && cell < 81)
							cout << cellsFixedData[cell].pt << " ";
						else cout << cell << " ";

					}
					cout << endl;

				}
			}
		}
		else if (!iret){// nothing to store if active and new
			BF128 w; w.SetAll_0();
			for (int i = 0; i < npas; i++) w.setBit(tback[i]);
			for (int i = 0; i < pm_go.nstore_xlc; i++){
				STORE_XLC & ss = pm_go.store_xlc[i];
				if (ss.dig == digit && ss.pattern == w) goto skip_it;

			}
			STORE_XLC & ss = pm_go.store_xlc[pm_go.nstore_xlc++];
			ss.pattern = w;
			ss.dig = digit;
			ss.loop = 1;
			memcpy(ss.t, tback, (npas + 1) * 4);
			ss.nt = npas;
			ss.rating = pm_go.hint.ChainLengthAdjusted(65, npas);
			if (pm_go.opprint & 2)cout << "store it rating = " << ss.rating << "pm_go.nstore_xlc=" << pm_go.nstore_xlc << endl;
		}
	skip_it:;
	}
	return iret;
}
int XSTATUS::CleanLoop(int * t, int nt){
	BF128  clean; clean.SetAll_0();
	for (int i = 1; i < nt; i += 2){// all pairs not granted bi values
		int cell1 = From_128_To_81[t[i]], cell2 = From_128_To_81[t[i + 1]];
		BF128 w = cell_z3x[cell1]; w &= cell_z3x[cell2];
		clean |= w & elims;
	}
	if (clean.isNotEmpty()){
		elims -= clean;
		zhou_solve.FD[digit][0] -= clean;
		return 1;
	}
	return 0;
}
int XSTATUS::CleanChain(int * t, int nt, BF128 & clean){
	clean.SetAll_0();
	int cell1 = From_128_To_81[t[1]], cell2 = From_128_To_81[t[nt-1]];
	BF128 w = cell_z3x[cell1]; w &= cell_z3x[cell2];
	clean |= w & elims;
	if (clean.isNotEmpty()){
		elims -= clean;
		zhou_solve.FD[digit][0] -= clean;
		return 1;
	}
	return 0;
}
int XSTATUS::XChain(int fast){
	if (!active) return 0;
	if (pm_go.opprint & 1){
		cout << "try XChain for digit " << digit + 1 << endl;
		char ws[82];
		cout << elims.String3X(ws) << " elims � voir" << endl;
	}
	// start from bi values seen by an elimination
	int xce, xce1, tback[20], iret = 0, store_start = pm_go.nstore_xlc;
	BF128 we = elims; // we still to study
	while ((xce = we.getFirst128()) >= 0){// next cell to elim
		we.clearBit(xce);
		int ce = From_128_To_81[xce];
		BF128 wwce = cell_z3x[ce]; wwce &= pmbiv; //possible starts
		if (wwce.Count() < 2) continue;
		//if (pm_go.opprint2 & 4)cout << "pour elim " << cf.pt << endl;
		while (wwce.Count() > 1){// take any seen bi value as start
			xce1 = wwce.getFirst128();// take and clear the first cell
			wwce.clearBit(xce1);
			int npas = R65Xexpand(xce, xce1, 0, tback,we);// we dummy argument
			if (!npas)continue;
			if (npas < maxpas){
				pm_go.nstore_xlc = store_start;// clean file of store xchain
				maxpas = npas;
			}
			if (npas <= 6 || fast){// do elims set iret to 1
				pm_go.nstore_xlc = 0; //clean longer if any
				BF128 clean;
				if (CleanChain(tback, npas,clean)){
					we -= clean;
					iret = 1;
					if (pm_go.opprint & 2) {
						cout << "chain back npas=" << npas<< " digit="<<digit+1 << endl;
						for (int i = 0; i <= npas; i++) {
							int xcell = tback[i], cell = From_128_To_81[xcell];
							if (i & 1) cout << "~";
							if (cell >= 0 && cell < 81)
								cout << cellsFixedData[cell].pt << " ";
							else cout << cell << " ";
						}
						cout << endl;
					}
					//break;
				}
			}
			else if (!iret){// nothing to store if active 
				STORE_XLC & ss = pm_go.store_xlc[pm_go.nstore_xlc++];
				ss.dig = digit;
				ss.loop = 0;
				memcpy(ss.t, tback, (npas + 1) * 4);
				ss.nt = npas;
				ss.rating = pm_go.hint.ChainLengthAdjusted(66, npas);
			}

		}

	}
	return iret;
}
struct XCOM{// data for x search one for all digits
	struct XCN {// Nishio contradiction
		GINT16	tret1[50], tret2[50],elim;
		int hintrating, nret1, nret2;
		void Print() {
			cout << "path1\t ";
			for (int i = 0; i < nret1; i++) {
				GINT16 x = tret1[i];
				if (x.u8[1])cout << "~";
				cout << cellsFixedData[x.u8[0]].pt << " ";
			}
			cout << endl;
			cout << "path2\t ";
			for (int i = 0; i < nret2; i++) {
				GINT16 x = tret2[i];
				if (x.u8[1])cout << "~";
				cout << cellsFixedData[x.u8[0]].pt << " ";
			}
			cout << endl;
		}
	}xc,txc[10];
	int fast, hitmode, ntcand,digit,cell,old_rating,clear_done;
	int hintrating, nelims,opprint;
	GINT16 tex[2][81];// pointer to source cell/sign or unit/2
	GINT16	 tcand[150],telims[10];
	PM3X seen_elims;
	void Init(int f,int oldr=0){
		opprint = pm_go.opprint & 2;
		if (oldr)old_rating = oldr;
		else old_rating = pm_go.rat_er;
		if (old_rating < 76)old_rating = 76;// minimum expected do it always
		clear_done = 0;
		fast = f;
		hitmode = 0;
		hintrating = 200;
		nelims = 0;
	}
	void AddElim(int dig, int cell, int rr){
		if (rr > hintrating)return;
		if (rr < hintrating){
			hintrating = rr;
			nelims = 0;
			seen_elims.SetAll_0();
		}
		if (nelims >= 10) return;
		if (seen_elims.On_c(dig, cell))return;//redundancy
		seen_elims.Set_c(dig, cell);
		if (opprint < 2) {
			cout << "store  " << dig + 1 << cellsFixedData[cell].pt << " rating=" << rr << endl;
			xc.Print();
		}
		if (opprint & 2) {// store chains for later print
			txc[nelims] = xc;
		}
		telims[nelims++].u16 = (uint16_t)(cell | (dig << 8));
	}

	void Store1( int c_elim, int c_contradiction, XSTATUS * xst){
		int d_elim = xst->digit;
		if (zhou_solve.IsOffCandidate_c(d_elim, c_elim))return;
		GINT16 x; x.u16 = (uint16_t)c_contradiction;
		xc.nret1 = XBackNishio(x, xc.tret1, xst);
		x.u8[1] = 1;
		xc.nret2 = XBackNishio(x, xc.tret2, xst);
		int rr = pm_go.hint.ChainLengthAdjusted(75, xc.nret1+xc.nret2);
		if (rr <= old_rating){
			if (opprint)cout << "clear "<<d_elim+1 << cellsFixedData[c_elim].pt << endl;
			xc.Print();
			zhou_solve.ClearCandidate_c(d_elim, c_elim);
			clear_done = 1;
			return;
		}
		AddElim(d_elim, c_elim, rr);
	}
	void Store2( int n1,GINT16 * t1,int c_elim, XSTATUS * xst){
		int d_elim = xst->digit;
		if (zhou_solve.IsOffCandidate_c(d_elim, c_elim))return;
		xc.nret1 = n1;
		memcpy(xc.tret1, t1, 2 * xc.nret1);
		GINT16 x; x.u16 = (uint16_t)(c_elim + 0x100); // start path2 with elim in off mode
		xc.nret2 = XBackNishio(x, xc.tret2, xst);
		int rr = pm_go.hint.ChainLengthAdjusted(75, xc.nret1 + xc.nret2 + 1);
		if (rr <= old_rating){
			if (opprint)cout << "clear " << cellsFixedData[c_elim].pt << endl;
			xc.Print();
			zhou_solve.ClearCandidate_c(d_elim, c_elim);
			clear_done = 1;
			return;
		}
		AddElim(d_elim, c_elim, rr);
	}
	int XBackNishio(GINT16 x, GINT16 * tretr, XSTATUS * xst);
}xcom;


int XCOM::XBackNishio(GINT16 x0, GINT16 * tretr,XSTATUS * xst){
	BF128 back_bf; back_bf.SetAll_0(); // no possible conflict in the way back
	int itret = 1, itret1 = 0 ;
	GINT16 tretw[50];
	tretw[0] = x0;
	back_bf.Set_c(x0.u8[0]);
	while (itret1 < itret){// && itret < 100 ) { // solve each entry back
		GINT16 x = tretw[itret1], y = tex[x.u8[1]][x.u8[0]];
		if (y.u16 == tcand[0].u16) { itret1++;	continue; }  // start point
		if (!(y.u8[1] & 2)) { // this is direct
			if (back_bf.On_c(y.u8[0])){ itret1++;	continue; }
			back_bf.Set_c(y.u8[0]);
			tretw[itret++] = y;
			itret1++;
			continue;
		}
		// now this comes from a set last in region
		int unit = y.u8[0] ,ucell; 
		BF128 w = units3xBM[unit];   w &= xst->pm;		w.Clear_c(x.u8[0]);
		while ((ucell = w.getFirstCell()) >= 0){
			w.Clear_c(ucell);
			if (back_bf.Off_c(ucell)) 
				tretw[itret++].u16 = (uint16_t)(ucell + 0x100);// add cell off
			back_bf.Set_c(ucell);
		}
		itret1++;
	}
	tretw[itret++].u16 = tcand[0].u16;//add the start in the way back
	back_bf.Set_c(tcand[0].u8[0]);
	// reset tretr in rigth order no contradiction
	if (tretr){
		int nb = 0;
		for (int i = 0; i<ntcand; i++){
			GINT16 x = tcand[i];
			if (back_bf.On_c(x.u8[0])) {
				if (x.u8[0] == x0.u8[0]) {// skip false sign
					if (x.u8[1] == x0.u8[1])tretr[nb++] = x0;
				}
				else tretr[nb++] = x;
			}
		}
	}
	return itret;
}
int  XSTATUS::XexpandNishio(int cell1){
	int diagloc = 0;
	//if (digit == 7) diagloc = 1;
	if(diagloc)cout << "try expand " << cellsFixedData[cell1].pt << endl;
	int ucount[27],used_sets=0;
	for (int i = 0; i < 27; i++){
		BF128 wu = units3xBM[i]; wu &= pm;
		ucount[i] = wu.Count();
	}
	expon.SetAll_0();	expoff = expon;	expon.Set_c(cell1);
	npas = 0;
	xcom.tcand[0].u16 = (uint16_t)cell1;// cell,digit,source,
	xcom.ntcand =  1;  
	int   ntd = 0, ntp,wcell2,tu[3];
	while (npas++ < 10){// safety, should never go so far
		if (diagloc)cout << "npas= " << npas << endl;
		ntp = ntd;
		ntd = xcom.ntcand;
		BF128 onold = expon;
		for (int i = ntp; i < ntd; i++){
			GINT16 wc = xcom.tcand[i];
			int wcell = wc.u8[0];
			if (diagloc>1 )				cout << cellsFixedData[wcell].pt<< endl;
			if (!(npas & 1)){// off to on
				GINT16 * texon = xcom.tex[0];
				CELL_FIX & cf = cellsFixedData[wcell];
				cf.GetRegions(tu);
				for (int iu = 0; iu < 3; iu++){// process each set
					int unit = tu[iu];
					BF128 wu = units3xBM[unit]; wu &= pm;
					int  nn_start = ucount[unit];
					BF128 w81 = wu - expoff;
					int nbc = w81.Count();
					if (nbc > 1)continue; // not last in region
					if (nbc == 1){
						wcell2 = w81.getFirstCell();
						if (onold.On_c(wcell2)) continue;
						if (expon.Off_c(wcell2)) {
							expon.Set_c(wcell2);
							if (diagloc>1)	cout<<"<add "<< cellsFixedData[wcell2].pt << endl;
							xcom.tcand[xcom.ntcand++].u16 = (uint16_t)wcell2;// on status
							if (nn_start<3)	texon[wcell2] = wc;// direct 
							else	texon[wcell2].u16 = (uint16_t)(unit + 0x200); //or refer to set  
							if (expoff.On_c(wcell2)){// contradiction reached
								if (xcom.fast) return 2;
								xcom.Store1( cell1,wcell2,this);
							}
						}
						else if (texon[wcell2].u8[1]&2){// use smaller if possible
							if (diagloc>1){
								cout << cellsFixedData[wcell].pt 
								<< " try smaller " << cellsFixedData[wcell2].pt 
								<< " unit=" << unit<<" count="<<nn_start << endl;
								cout << " unitold " << (int)texon[wcell2].u8[0] << " oldcount="
									<< ucount[texon[wcell2].u8[0]] << endl;
							}
							if (ucount[texon[wcell2].u8[0]]>nn_start)
								texon[wcell2].u8[0] = (uint8_t)unit;
							// could be in serate nn_start>2 ????
						}
						continue;
					}
					// nbc=0 empty set force contradiction
					int bit = 1 << unit;
					if (used_sets & bit) continue; // already done
					used_sets |= bit;
					if (diagloc)	cout<<"processing nbc=0 unit="<<unit+1  << endl;
					int rncand = xcom.ntcand++;
					if (xcom.fast) return 2;
					wu.Clear_c(wcell);
					while ((wcell2 = wu.getFirstCell()) >= 0){
						wu.Clear_c(wcell2);
						xcom.tcand[rncand].u16 = (uint16_t)wcell2; // on status
						if (nn_start<3)	texon[wcell2] = wc;// direct 
						else texon[wcell2].u16 = (uint16_t)(unit + 0x200); //or refer to set 
						if (diagloc)	cout << " try store1 " << cellsFixedData[wcell2].pt << endl;
						xcom.Store1( cell1, wcell2, this);
					}
				}// end iu

			}//end off to on
			else {// on to off
				GINT16 * texoff = xcom.tex[1];
				BF128 wz = cell_z3x[wcell], wzcont = wz & onold;
				if (wzcont.isNotEmpty()){// contradiction
					while ((wcell2 = wzcont.getFirstCell()) >= 0){
						wzcont.Clear_c(wcell2);
						xcom.tcand[xcom.ntcand++].u16 = (uint16_t)(wcell2 + 0x100);// off status
						texoff[wcell2] = wc;
						xcom.Store1( cell1, wcell2, this);
					}
					continue;
				}
				wz &= (pm - expoff);
				while ((wcell2 = wz.getFirstCell()) >= 0){
					wz.Clear_c(wcell2);
					texoff[wcell2] = wc;
					xcom.tcand[xcom.ntcand++].u16 = (uint16_t)(wcell2 + 0x100); // off status
					expoff.Set_c(wcell2);
				}
			}//end on to off
		}// end for i
		if (ntd == xcom.ntcand)break;// empty step
	}// end while
	return 0;
}

int XSTATUS::Nishio1(){// this is for a given digit
	if (!active) return 0;
	int iret = xcom.clear_done=0;
	if (pm_go.opprint2 & 8){
		cout << "try Nishio1 for digit " << digit + 1 << endl;
		char ws[82];
		cout << elims.String3X(ws) << " elims to study" << endl;
		zhou_solve.DebugDigit(digit);
	}
	int tp[80], ntp = elims.Table3X27(tp);
	for (int icell = 0; icell < ntp; icell++){
		int cell1 = tp[icell], irexpand = XexpandNishio(cell1);
		if (irexpand == 2){// contradiction fast 
			zhou_solve.ClearCandidate_c(digit, cell1);
			xcom.clear_done = 1;
			iret = 1;
		}
	}
	return iret | xcom.clear_done;
}

int XSTATUS::Nishio2(){// must also consider bi value multi chains seen as x->~a + ~x->~a
	if (!active) return 0;
	int diagloc = pm_go.opprint2 & 8;
	if (diagloc){
		cout << "try Nishio2 for digit " << digit + 1 << endl;
		char ws[82];
		cout << elims.String3X(ws) << " elims to see" << endl;
	}
	int  iret = 0;

	for (int unit = 0; unit < 27; unit++){
		GINT16	tpath1[10][50],wd;
		int ntpath1[10];
		if (bivsets.Off(unit))continue;
		if (diagloc) cout << "unit " << unit << endl;
		BF128 wu = units3xBM[unit]; wu &= pm; // 2 cells in 3x27 mode
		if ((wu&elims).isNotEmpty()) continue; // is direct
		int tp[3];  wu.Table3X27(tp);// 2 cells in table
		XexpandNishio(tp[0]); // no elimination expected  in expand
		BF128 offt = expoff&elims;
		if (offt.isEmpty()) continue;
		int tpe[50], ntpe = offt.Table3X27(tpe);
		if (ntpe>10) ntpe = 10; // safety measure to protect tables
		for (int i = 0; i < ntpe; i++){// store path1 for elims
			wd.u16 = (uint16_t)(tpe[i] + 0x100);
			ntpath1[i] = xcom.XBackNishio(wd, tpath1[i], this);
		}
		XexpandNishio(tp[1]);  // no elimination expected in expand
		offt &= expoff;
		if (offt.isEmpty()) continue;
		if (xcom.fast){
			iret += zhou_solve.CleanCellsForDigits(offt, 1 << digit);
			xcom.clear_done = 1;
			continue;
		}
		// now collect paths
		if (diagloc){
			char ws[82];
			cout << offt.String3X(ws) << "to clean" << endl;
		}
		for (int i = 0; i < ntpe; i++){// catch path for still there
			int cell = tpe[i];
			if (offt.Off_c(cell))continue;//not in second path
			xcom.Store2(ntpath1[i], tpath1[i], cell, this);
		}
	}

	/*
	for (int unit = 0; unit<27; unit++){
	tretx1[10][30], ntretx1[10],
	tretx2[30], ntretx2;
	if (ntpe>10) ntpe = 10; // safety measure to protect tables
	for (int ie = 0; ie<ntpe; ie++)
	ntretx1[ie] = XBackNishio(tpe[ie] + (1 << 7), tretx1[ie]);
	XexpandNishio(tp[0]); // redo it
	for (int ie = 0; ie<ntpe; ie++){
	ntretx2 = XBackNishio(tpe[ie] + (1 << 7), tretx2);
	int rating = parent->hint.ChainLengthAdjusted(75, ntretx1[ie] + ntretx2 + 1);
	bds.SetCurrentElim();
	if (parent->hint.AddCand(curdig, tpe[ie], rating)){// need to print the situation
	if (parent->opprint){
	}
	}
	}
	}
	*/

	return iret;
}

void STORE_XLC::Print() {//[From_128_To_81[xce1]];
	cout << "Xprint loop=" << loop  << " digit=" <<dig+1<< endl;
	for (int i = 0; i <= nt; i++) {
		if ((i+loop) & 1) cout << "~" ;
		cout << cellsFixedData[From_128_To_81[t[i]]].pt << " ";
	}
	cout <<endl;
}

//============================================= YLSEARCH

int YLSEARCH::Search(int fast ){// search using zh_g.digit_sol as compulsory 
	//cout << "ylsearch start" << endl;
	locdiag = 0;
	if (pm_go.opprint & 1) locdiag = 1;
	BF128 lastloop; lastloop.SetAll_0();
	pm_go.nstore_yl = 0;
	maxpas = 15;
	mode = 0;
	int iret = 0,tunit[3];
	
	for (idig = 0; idig < 9; idig++){
		BF128 wdok = zh_g.pairs & zh_g2.digit_sol[idig];// start to consider
		while ((xcell1 = wdok.getFirst128()) >= 0){
			wdok.clearBit(xcell1);
			int cell1 = From_128_To_81[xcell1];
			CELL_FIX  &cf=cellsFixedData[cell1];
			tunit[0] = cf.el; tunit[1] = cf.plu; tunit[2] = cf.ebu;
			BF128 wd1 = cell_z3x[cell1]; wd1 &= zh_g2.pm.pmdig[idig];
			for (int iu = 0; iu < 3; iu++){// try a unit with cleaning potential
				int unit = tunit[iu];
				BF128 wd1u = wd1; wd1u &= units3xBM[unit];
				if (wd1u.Count() < 2) continue; // min cell1 + 2 digits in the unit 
				BF128 wd1up = wd1u&zh_g.pairs;
				while ((xcell2 = wd1up.getFirst128()) >= 0){ // A pair of cells as start
					wd1up.clearBit(xcell2);// usually only one cell in wd1up

					if (Expand()){
						loop.SetAll_0();
						for (int i = 0; i < ncells; i++) loop.Set_c(tback[i].u16[0]);
						if (loop == lastloop)continue;
						if (locdiag){
							PrintTback(" yloop seen ");
						}
						if (ncells < maxpas){
							maxpas = ncells;
							pm_go.nstore_yl = 0;
						}
						lastloop = loop;
						if (ncells == 4|| fast){// apply the loop
							if (CleanLoop()){
								PrintTback(" active yloop ");
								iret = 1;
							}
						}
						else {
							for (int i = 0; i < pm_go.nstore_yl; i++){
								if (loop == pm_go.store_yl[i].loop){
									goto skipit;
								}
							}
							pm_go.store_yl[pm_go.nstore_yl++] = *this; //or store the loop
						}
					}
				skipit:;
				}
			}
		}

	}
	if (locdiag) cout << "end ylsearch nstore=" << pm_go.nstore_yl << endl;
		return iret;
}
int YLSEARCH::Expand(){//search start idig;xcell1;xcell2 end 
	c1 = From_128_To_81[xcell1];	c2 = From_128_To_81[xcell2];
	if (locdiag&2)		cout << "yl expand dig="<<idig+1<<" " << cellsFixedData[c1].pt
		<< " " << cellsFixedData[c2].pt << endl;
	ncells = 1;// start with 
	uint32_t d2;
	BF128 used_cells;	used_cells.SetAll_0();	
	used_cells.Set(xcell1); used_cells.Set(xcell2);
	GINT64 t[160];// cell,digit on,digit off,source
	int   nt = 2, ntd = 1, ntp, xcell;
	t[0].u64 = c1;	// source to 0
	t[1].u64 = c2 | (idig << 16);	// source to 0
	t[0].u16[2] = (uint16_t)idig;
	while (ncells++ < maxpas){ // safety, should never go so far
		ntp = ntd;
		ntd = nt;
		for (int i = ntp; i < ntd; i++){
			GINT64 &c = t[i];// source cell
			int dc = zh_g2.dig_cells[c.u16[0]];
			dc ^= 1 << c.u16[1];// clear digit off
			bitscanforward(d2, dc); // catch the second digit
			c.u16[2]=(uint16_t)d2;// store it for later use
			BF128 w2 = zh_g2.pm.pmdig[d2] & zh_g.pairs; 
			w2 &= cell_z3x[c.u16[0]];// bi value cells same digit seen, no way back
			if (w2.On(xcell1) && t[0].u16[2] != (uint16_t)d2){// it is a loop
				t[0].u16[1] = (uint16_t)d2;
				tback[ncells] = t[0];
				register int Ri = i, Rn = ncells;
				while (--Rn >= 0) {
					tback[Rn] = t[Ri];
					Ri = t[Ri].u16[3];
				}				
				return ncells;
			}
			w2 -= used_cells;
			if (w2.isEmpty()) continue; // dead branch;
			while ((xcell = w2.getFirst128()) >= 0){// add new cells
				used_cells.Set(xcell);
				w2.clearBit(xcell);
				GINT64 &tw = t[nt++];
				tw.u64 = From_128_To_81[xcell];
				tw.u16[1] = (uint16_t) d2;
				tw.u16[3] = (uint16_t)i; 
			}
		}
		if (nt == ntd) return 0;
	}
	return 0;
}

int YLSEARCH::ExpandOut(){//search start c1 target c2
	struct SPOT{
		BF128 used_cells,wcells;
		int mycell,cell,digit,ispot;
		uint32_t d2;
		inline void Init(int ca, int idig){
			ispot = 0;
			used_cells.SetAll_0();
			used_cells.Set_c(ca); // lock start cell
			//used_cells.Set_c(cb); // lock target
			mycell = ca;
			digit = idig;
			FindWcells();
		}
		inline int Nextcell(){
			cell = wcells.getFirstCell();
			if (cell < 0)return 0;
			wcells.Clear_c(cell);
			return 1;
		}
		inline void Copy(SPOT * sp){
			*this = *sp;
			mycell = cell;
			ispot++;
			used_cells.Set_c(cell);
			digit = d2;
			FindWcells();
		}
		void FindWcells(int diag = 0){
			int dc = zh_g2.dig_cells[mycell];
			dc ^= 1 << digit;// clear digit off
			bitscanforward(d2, dc); // catch the second digit
			wcells = zh_g2.pm.pmdig[d2] & zh_g.pairs;
			wcells &= cell_z3x[mycell];// bi value cells same digit seen, no way back
			wcells -= used_cells;
			if (diag){// || ispot<4){
				char ws[82];
				cout << " find " <<  d2 + 1 << cellsFixedData[mycell].pt << endl;
				cout << wcells.String3X(ws) << endl;
			}
		}
		int Store(SPOT * start, GINT64 *t){
			SPOT *sw = start;
			int n = 0;
			while (sw <= this){
				GINT64 &tw = t[n++];
				tw.u16[0] = (uint16_t)sw->mycell;
				tw.u16[1] = (uint16_t)sw->digit;
				tw.u16[2] = (uint16_t)sw->d2;
				sw++;
			}
			return n;
		}
		void Diag1(int i, int ph){
			cout << i << " phase" << ph << " -" << digit + 1 << " +" << d2 + 1 << cellsFixedData[cell].pt << endl;
		}
		void PPath(SPOT * start){
			SPOT *sw = start;
			cout << "(-+)";
			while (sw < this){
				cout << " " << sw->digit + 1 << sw->d2 + 1 << cellsFixedData[sw->mycell].pt;
				sw++;
			}
			cout << " -+" << digit + 1  << cellsFixedData[mycell].pt << endl;
		}

	}spots[30],*s,*sp,*slim,*slim1,*slim2;
	//________________________________________________________________________________
	diag = 0;//	if (locdiag & 8)diag = 1;
	//if (diag  && idig==1) 	diag = 2;
	if (diag){
		cout << "entry expand out"<< idig + 1 << cellsFixedData[c0].pt << " "
			<< cellsFixedData[c1].pt << " " << cellsFixedData[c2].pt
			<< " c0,c1,c2 go yloopout maxpas=" << maxpas << endl;
	}
	ncells = 0;
	s = spots;	s->Init(c1,idig);
	slim = &spots[(maxpas - 3)];	slim2 = &spots[maxpas];
	goto next;
nextspot:
	sp=s++;	s->Copy(sp);// copy previous and find possible next cells
	if (s->wcells.On_c(c2)){// ok or skip it
		if ((int)s->d2 == idig) goto back;// not the right value
		slim1 = s;
		s->cell = c2;
		s->used_cells.Clear_c(c1);// reset start cell as possible
		sp = s++;	s->Copy(sp);// copy previous and find possible next cells
		if (diag){
			cout << s->ispot << " target found, goto phase 2" << endl;
			s->PPath(spots);
		}
		SPOT * sslim = s + 3;
		if (sslim<slim)slim = sslim;
		goto next2; 
	nextspot2:
		sp = s++;	s->Copy(sp);// copy previous and find possible next cells
		if (s->wcells.On_c(c1)){// ok or skip it
			if ((int)s->d2 != idig) goto back2;// not the right value
			//<= same process, (not so common) store the path in tback
			maxpas = s->ispot;
			slim = &spots[(maxpas - 3)];	slim2 = &spots[maxpas];
			if (diag)cout << s->ispot << " loop" << endl;
			s->cell = c1;
			sp = s++;	s->Copy(sp);// copy previous and find possible next cells
			if (diag)s->PPath(spots);
			ncells=s->Store(spots, tback)-1;
			s--;
			goto back2;
		}
		if (s >= slim2)goto back2;
	next2:
		while (s->Nextcell())goto nextspot2;
	back2:if (--s > slim1)goto next2;
		goto back;
	}
	if (s >= slim)goto back;
next:
	while (s->Nextcell())goto nextspot;
back:if (--s >= spots)goto next;
	return ncells;
}

int YLSEARCH::SearchOut(int fast){// search using zh_g.digit_sol as compulsory 
	// long loops clearing out of a region
	//cout << "ylsearchout start" << endl;
	maxpas = 12;
	locdiag = 0;
	if (pm_go.opprint & 1) locdiag = 1;
	if (zh_g.pairs.Count() < 7) return 0;// minimm with no XY wing
	//maxpas = 20;// don't do that would delete ylsearch stored
	mode = 1;
	int iret = 0,  xclean, tpstart[200], ntpstart = 0,xc1,xc2;
	for (idig = 0; idig < 9; idig++){// this is the digit to clear
		BF128 wdok = zh_g.pairs & zh_g2.pm.pmdig[idig]; 
		BF128 wdok_true = wdok & zh_g2.digit_sol[idig];// start to consider
		BF128 wdok_false = wdok -wdok_true;//possible second cell
		if (wdok_true.isEmpty() || wdok_false.isEmpty()) continue;
		BF128 dclean = zh_g2.pm.pmdig[idig] - zh_g2.digit_sol[idig];// possible clean out
		while ((xclean = dclean.getFirst128()) >= 0){
			dclean.clearBit(xclean);
			c0 = From_128_To_81[xclean];
			BF128 dok1_true = cell_z3x[c0]; dok1_true &= wdok_true;
			while ((xc1 = dok1_true.getFirst128()) >= 0){
				dok1_true.clearBit(xc1);
				c1 = From_128_To_81[xc1];
				BF128 dok1_false = cell_z3x[c0]; dok1_false &= wdok_false;
				dok1_false-= cell_z3x[c1];// forget same unit already studied
				while ((xc2 = dok1_false.getFirst128()) >= 0){
					dok1_false.clearBit(xc2);
					c2 = From_128_To_81[xc2];
					int w = c1 | (c2 << 16);
					for (int i = 0; i < ntpstart; i++) if (w == tpstart[i]) goto skipc2;
					tpstart[ntpstart++] = w;
					if (0 ){
						cout <<idig+1<< cellsFixedData[c0].pt << " " 
							<< cellsFixedData[c1].pt << " "	<< cellsFixedData[c2].pt
							<< " c0,c1,c2 go yloopout maxpas=" << maxpas << endl;
					}
					//if (1) continue;
					if (ExpandOut()){
						loop.SetAll_0();
						for (int i = 0; i < ncells; i++) loop.Set_c(tback[i].u16[0]);
						if (locdiag & 8){
							PrintTback(" yloop out seen ");
						}
						if (ncells < maxpas){maxpas = ncells;	pm_go.nstore_yl = 0;}
						if (ncells <8 || fast){// apply the loop
							if (CleanLoopOut()) {
								PrintTback(" yloop out active ");
								return 1; 	// should always be "yes"	
							}
						}
						else {
							for (int i = 0; i < pm_go.nstore_yl; i++){
								if (loop == pm_go.store_yl[i].loop){
									pm_go.store_yl[i].mode = 1; // force mode to out
									goto skipc2;
								}
							}
							pm_go.store_yl[pm_go.nstore_yl++] = *this; //or store the loop
						}
					skipc2:;
					}// end c2
				}// end c1
			}// end while xclean
		//next0:;
		}
	}
	if (locdiag)		cout << " searchOut  end nstore_y = " << pm_go.nstore_yl << endl;
	return iret;
}

int YLSEARCH::CleanLoop(){
	//if (pm_go.opprint2 & 4) cout << "clean yloop mode=" << mode << endl;
	if (mode)return CleanLoopOut();
	int iret = 0;
	for (int icell = 0; icell < ncells; icell++){
		GINT64 w1 = tback[icell], w2 = tback[icell+1];
		int idig = w1.u16[2]; // "on" digit in the first cell
		//if (pm_go.opprint2 & 4) cout << "dig=" << idig + 1
			//<< " " << cellsFixedData[w1.u16[0]].pt
			//<< " " << cellsFixedData[w2.u16[0]].pt << endl;
		BF128 clean = cell_z3x[w1.u16[0]];
		clean &= cell_z3x[w2.u16[0]];
		clean &= zhou_solve.FD[idig][0];//this is to clean
		if (clean.isNotEmpty()){
			zhou_solve.FD[idig][0]-=clean;
			iret = 1;
		}
	}
	return iret;
}
int YLSEARCH::Is_To_Clean(int rating){
	return (pm_go.hint.ChainLengthAdjusted(65, 2 * ncells) <= rating);
}
int YLSEARCH::CleanLoopOut(){
	int iret = 0;
	for (int i1 = 0; i1 < ncells-3; i1++){// no XY wing  4 is a minimum
		GINT64 w1 = tback[i1];
		BF128 z1 = cell_z3x[w1.u16[0] ];
		for (int i2 = i1+3; i2 < ncells ; i2++){
			GINT64 w2 = tback[i2];
			BF128 z2 = cell_z3x[w2.u16[0]]; z2 &= z1;
			if (z2.isEmpty()) continue;
			int td[2], n = 0;
			if (w1.u16[1] == w2.u16[2])td[n++] = w1.u16[1];
			if (w1.u16[2] == w2.u16[1])td[n++] = w1.u16[2];
			for (int id = 0; id < n ; id++){// digits to check
				int idig = td[id]; // "on" digit in the first cell
				BF128 clean = z2 & zhou_solve.FD[idig][0];//this is to clean
				if (clean.isNotEmpty()){
					zhou_solve.FD[idig][0] -= clean;
					iret = 1;
				}
			}
		}
	}
	return iret;
}

void YLSEARCH::PrintTback(const char * lib){
	for (int i = 0; i < ncells; i++){
		cout << cellsFixedData[tback[i].u16[0]].pt << " -> ";
	}
	cout << cellsFixedData[tback[ncells].u16[0]].pt<<" "<<lib << " start on dig " << idig + 1 << endl;
}
//============================================= XYSEARCH
void  XYSEARCH::AddUnit(int unit, int source){
	//cout << "addunit" << endl;
	BF128 w = units3xBM[unit]; w &= wb;
	if (w.isEmpty()) return;
	int cc = w.getFirstCell();
	//cout << "do add unit=" << unit << "digit=" << digit + 1 << cellsFixedData[cc].pt << endl;
	if (used_on_digits.Off_c(digit, cc)){
		Addt(cc, digit, source);
	}
	used_on_digits.Set_c(digit, cc);
	wb.Clear_c(cc);
}

void XYSEARCH::OffToOn(int i){
	if (pairs.On_c(cell)){
		int dig = zh_g2.dig_cells[cell] ^ (1 << digit);
		bitscanforward(d2, dig);
		if (used_on_digits.Off_c(d2, cell)){
			Addt(cell, d2, i);
			used_on_digits.Set_c(d2, cell);
		}
	}
	CELL_FIX & cf = cellsFixedData[cell];
	BF32 &bsw = dig_bivsets[digit];
	wb = cell_z3x[cell]; wb &= dbiv.pmdig[digit];
	if (bsw.On(cf.el)) AddUnit(cf.el, i);
	if (bsw.On(cf.plu)) AddUnit(cf.plu, i);
	if (bsw.On(cf.ebu)) AddUnit(cf.ebu, i);
}

void XYSEARCH::OffToOn_Dyn(int i){

	if (pairs.On_c(cell)){
		int dig = zh_g2.dig_cells[cell] ^ (1 << digit);
		bitscanforward(d2, dig);
		if (used_on_digits.Off_c(d2, cell)){
			tex[d2][cell] = nt;// priority to direct
			Addt(cell, d2, i);
			used_on_digits.Set_c(d2, cell);
		}
		else{// check to offset a last on same step
			int oldi = tex[d2][cell];
			GINT64 w = t[oldi];
			if( (w.u16[2] & 0x3000)&& oldi>= ntd){// old was not direct same step
				t[oldi].u16[2] = (uint16_t)i;
			}
		}
	}
	else{// and now last in cell or empty cell
		int nfree = 0, free;
		int dig = zh_g2.dig_cells[cell] ^ (1 << digit);
		while ( dig){
			bitscanforward(d2, dig);
			dig ^= 1 << d2;
			if (used_on_digits.Off_c(d2, cell)){//not yet on
				if (used_off_digits.Off_c(d2, cell)){//not yet off
					if (nfree)goto exit_last_in_cell;// more than one not off
					nfree++;
					free = d2;
				}
			}
			else goto exit_last_in_cell;
		}
		if (nfree){// true last in cell put it on if not yet
			//cout << "add last in cell " << endl;
			if (used_on_digits.Off_c(free, cell)){
				tex[free][cell] = nt;// first
				AddLastInCell(cell, free);
				used_on_digits.Set_c(free, cell);
			}
		}
		else{// empty cell (no "on") set "on" all "off"  
			int digs = zh_g2.dig_cells[cell] ^ (1 << digit);;
			while ( digs){
				bitscanforward(d2, digs);
				digs ^= 1 << d2;
				if (used_on_digits.Off_c(d2, cell)){
					tex[d2][cell] = nt;// first
					AddLastInCell(cell, d2);
					used_on_digits.Set_c(d2, cell);
				}
			}
		}
	exit_last_in_cell:;
	}
	CELL_FIX & cf = cellsFixedData[cell];
	int tu[3];
	cf.GetRegions(tu);
	for (int iu = 0; iu < 3; iu++){// process each set
		int unit = tu[iu];
		BF128 wu = units3xBM[unit]; wu &= zh_g2.pm.pmdig[digit];
		int  nn_start = wu.Count();
		BF128 w81 = wu - used_off_digits.pmdig[digit];
		if ((w81&used_on_digits.pmdig[digit]).isNotEmpty()) continue;// no on
		int nbc = w81.Count(),wcell2;
		if (nbc > 1)continue; // not last in region
		if (nbc == 1){
			wcell2 = w81.getFirstCell();
			if (nn_start == 2){// bi value
				if (used_on_digits.Off_c(digit, wcell2)){
					tex[digit][wcell2] = nt;// priority to direct
					Addt(wcell2, digit, i);
					used_on_digits.Set_c(digit, wcell2);
				}
				else{// offset not direct same step
					int oldi = tex[digit][wcell2];
					GINT64 w = t[oldi];
					if ((w.u16[2] & 0x3000) && oldi >= ntd){// old was not direct same step
						t[oldi].u16[2] = (uint16_t)i;
					}
				}
			}
			else {//last in region
				if (used_on_digits.Off_c(digit, wcell2)){
					tex[digit][wcell2] = nt;// first
					AddLastInUnit(wcell2, digit,unit);
					used_on_digits.Set_c(digit, wcell2);
				}
				else if(nn_start==3){//offset not direct if smaller size same step
					int oldi = tex[digit][wcell2];
					GINT64 w = t[oldi];
					if ((w.u16[2] & 0x2000) && oldi >= ntd){// old was not direct same step
						t[oldi].u16[2] = (uint16_t)(0x2000 | unit); 
					}
				}
			}
		}
		else {//empty unit set(no "on)  "on" all  
			wu.Clear_c(cell);
			while ((wcell2 = wu.getFirstCell()) >= 0){
				wu.Clear_c(wcell2);
				if (used_on_digits.Off_c(digit, wcell2)){
					tex[digit][wcell2] = nt;// first
					AddLastInUnit(wcell2, digit, unit);
					used_on_digits.Set_c(digit, wcell2);
				}
			}
		}
	}
}

void XYSEARCH::OnToOff(int i){
	if (cells_all.On_c(cell)){//all cells with biv or pair
		int digs = zh_g2.dig_cells[cell] ^ (1 << digit);// can be more than one
		while ( digs){
			bitscanforward(d2, digs);
			digs ^= 1 << d2;
			if (dbiv.pmdig[d2].Off_c(cell)) continue;
			if (used_off_digits.On_c(d2, cell))continue;
			Addt(cell, d2, i);
			used_off_digits.Set_c(d2, cell);
		}

	}
	BF128 wb = cell_z3x[cell],
		wb1 = (pairs & zh_g2.pm.pmdig[digit] )| dbiv.pmdig[digit];
	wb &= wb1;
	wb -= used_off_digits.pmdig[digit];
	used_off_digits.pmdig[digit] |= wb;
	while ((c2 = wb.getFirstCell()) >= 0){
		wb.Clear_c(c2);
		Addt(c2, digit, i);
	}
}
void XYSEARCH::OnToOff_Dyn(int i){// no bi value filter
	int digs = zh_g2.dig_cells[cell] ^ (1 << digit);// can be more than one
	while ( digs){
		bitscanforward(d2, digs);
		digs ^= 1 << d2;
		if (used_off_digits.On_c(d2, cell))continue;
		if ((cell == dcell) && ((int)d2 == ddig)) continue;
		Addt(cell, d2, i);
		used_off_digits.Set_c(d2, cell);
	}
	CELL_FIX & cf = cellsFixedData[cell];
	BF128 wb = cell_z3x[cell];
	wb &= zh_g2.pm.pmdig[digit];
	wb -= used_off_digits.pmdig[digit];
	wb.Clear_c(dcell);// be sure not to use it
	used_off_digits.pmdig[digit] |= wb;
	while ((c2 = wb.getFirstCell()) >= 0){
		wb.Clear_c(c2);
		Addt(c2, digit, i);
	}
}

int XYSEARCH::CleanXYChain(){
	int iret = 0;
	if (mode)iret += pm_go.CleanOr(	tback[0].u16[1], tback[0].u16[0],
	tback[nsteps].u16[1], tback[nsteps].u16[0]); //chain Apply it directly
	else{// XY loop must consider each pair
		tback[nsteps+1] = tback[0];
		for (int i = 1; i <= nsteps; i+=2){
			int c1 = tback[i].u16[0], d1 = tback[i].u16[1];
			for (int j =i+ 1; j <= nsteps+1; j+=2){
				int c2 = tback[j].u16[0], d2 = tback[j].u16[1];
				iret += pm_go.CleanOr(d1, c1, d2, c2);
			}
		}
	}
	if(iret && (pm_go.opprint & 2))cout << "CleanXYChain() return ="<<iret << endl;
	return iret;
}

int XYSEARCH::Expand_true_false(){// start is {cell c1 digit idig} false exit loop or chain
	// in fact c1 is true in the solution
	// one step is �=b-c all belonging to bi-values 
	int diag = 0;
	//if (fastmode && idig == 4 && c1 >63)		diag = 1;
	int ichain, c1digs = zh_g2.dig_cells[c1];;
	mode=nsteps = 0;// start with 1 step and mode loop
	nt = 1;	t[0].u64 = c1 | (idig << 16);	// source to 0
	used_off_digits.SetAll_0();
	used_on_digits.SetAll_0();
	used_off_digits.Set_c(idig, c1);
	BF128 zc1_idig = cell_z3x[c1]; 
	zc1_idig &= zh_g2.pm.pmdig[idig];
	if (diag) cout << "start xyexpand " << idig + 1 << cellsFixedData[c1].pt <<
		" maxpas=" << maxpas << endl;
	int  i; 
	ntd = 0;
	while (nsteps++ < maxpas){ // safety, should never go so far
		ntp = ntd;
		ntd = nt;
		if (diag) cout << "step " << nsteps << " ntp="<<ntp<<" ntd="<<ntd << endl;
		for (i = ntp; i < ntd; i++){
			cell = t[i].u16[0];		 digit = t[i].u16[1];
			if (diag){
				if (nsteps & 1) cout <<"~";
				cout << digit + 1 << cellsFixedData[cell].pt << " go" << endl;
			}
			if (nsteps & 1)				OffToOn(i);
			else OnToOff(i); 
		}
		if (diag) cout << "start exit xyexpand " << "step " << nsteps 
			<< " nt=" << nt		<< endl;
		if (nt == ntd) 	return 0;
		if (!(nsteps & 1)) continue;// nothing to do after on to off
		if (nsteps < 3)continue;
		ichain = 0;
		for (i = ntd; i < nt; i++){// added in this step priority to loop
			cell = t[i].u16[0];		 digit = t[i].u16[1];
			if (cell == c1){
				if (digit != idig)goto backloop; //loop in cell not bi value
				continue;//contradiction seen
			}
			BF128 z = cell_z3x[cell];
			if (digit == idig){
				if (z.On_c(c1))goto backloop; // loop in unit
				z &= zc1_idig;
				if (z.isNotEmpty())ichain = i; // active xy chain
			}
			else{
				if (z.On_c(c1)){
					if (c1digs & (1 << digit))ichain = i;
					if (zh_g2.dig_cells[cell] & (1 << idig))ichain = i;
				}

			}
		}
		if (ichain) goto backchain;
	}
	return 0;
backchain:mode = 1;
	if (diag) cout << "backchain " << endl;
	cell = t[ichain].u16[0];		 digit = t[ichain].u16[1];
	i = ichain;
backloop:
	if (diag) cout << "backloop nsteps="<<nsteps << endl;
	register int Ri = i, Rn = nsteps + 1;
	while (--Rn >= 0) {
		if (diag)cout << "rn=" << Rn << " ri=" << Ri << endl;
		tback[Rn].u32[0] = t[Ri].u32[0];
		Ri = t[Ri].u16[2];
	}
	if (diag) cout << "return backloop " << endl;
	return nsteps + mode;
}

void XYSEARCH::PrintTback(){
	cout << " found xy loop/chain mode=" << mode << endl;
	PrintBackCom("path ", tback, nsteps + 1, 0);
}
void  XYSEARCH::PrintBackCom(const char * lib, GINT64 * ptback, int nback, int pmode){
	cout << lib << " ";
	for (int i = 0; i < nback; i++){
		GINT64 w = ptback[i];
		int sign = w.u16[3];
		switch (pmode){// 0 base ;1 dyn; 2 dynplus
		case 0:if(!(i&1))sign ^= 1;// start with a off status
		case 1:
			if (sign & 1)cout << "~";
			break; 
		case 2: 
			if (sign & 0x1000)cout << "~";
		}
		cout << w.u16[1] + 1 << cellsFixedData[w.u16[0]].pt<<" ";
	}
	cout << endl;
}
void XYSEARCH::DebugT(){
	cout << "\nsummary of expand nt="<<nt << endl;
	int curstep = -1;
	for (int i = 0; i < nt; i++){
		GINT64 w = t[i];
		int wc = w.u16[0], wd = w.u16[1], ws = w.u16[3], source = w.u16[2];
		if (ws != curstep){
			curstep = ws;
			cout << "\nstep " << curstep << "->";
			cout << wd + 1 << cellsFixedData[wc].pt << ";0x" << hex << source << dec << " ";
		}

	}
	cout << endl;
}
void XYSEARCH::Init(){//Collect bi values
	pairs = zh_g.pairs;  
	cells_all.SetAll_0(); cells_biv_all.SetAll_0();
	dbiv.SetAll_0();
	cells_biv_true = pairs;
	memset(dig_bivsets, 0, sizeof dig_bivsets);
	memset(dig_sets3, 0, sizeof dig_sets3);
	for (int idig = 0; idig < 9; idig++){
		int * wds = dig_sets[idig];
		memcpy(wds, zh_g2.dig_rows[idig], 36);
		memcpy(&wds[9], zh_g2.dig_cols[idig], 36);
		memcpy(&wds[18], zh_g2.dig_boxes[idig], 36);
		BF128 & pmb = dbiv.pmdig[idig];
		for (int iu = 0; iu < 27; iu++){
			int nc = _popcnt32(wds[iu]);
			if (nc == 2){
				dig_bivsets[idig].Set(iu);
				pmb |= units3xBM[iu];
			}
			else if (nc>2) dig_sets3[idig].Set(iu);
		}
		pmb &= zh_g2.pm.pmdig[idig];
		if (idig)cells_biv_all |= (pmb & cells_all);
		cells_all |= pmb;
		dig_b_true[idig] = pmb & zh_g2.digit_sol[idig];
		cells_biv_true |= dig_b_true[idig];
	}
}
void XYSEARCH::Init2(){// if multi_chains level reached
	BF128 wbiv = cells_biv_all | pairs,singles=cells_all-cells_biv_all;
	if(0){
		dbiv.Print(" bi-values status");
		char ws[82];
		cout << wbiv.String3X(ws) << " wbiv" << endl;
		cout << singles.String3X(ws) << " singles" << endl;

	}
	int cell;
	for (int i = 0; i < 9; i++){
		BF128 wd = zh_g2.pm.pmdig[i], wwd = wd,
			wdb = dbiv.pmdig[i] | (pairs & wd);
		while ((cell = wwd.getFirstCell()) >= 0){// check each candidate
			wwd.Clear_c(cell);			// must have a digit pair seen 
			BF128 seen =cell_z3x[cell];
			if ((seen&wdb).isEmpty())wd.Clear_c(cell);
		}
		active_all.pmdig[i] = wd;
		active_unit.pmdig[i] =wd;
		active_unit.pmdig[i] |= singles - dbiv.pmdig[i];
		active_unit.pmdig[i] |= cells_biv_all & zh_g2.pm.pmdig[i];

	}
}
void XYSEARCH::InitCandidatesTable(){
	ntcands = 0;
	int nb = 0;
	for (idig = 0; idig < 9; idig++){
		BF128 wd = zh_g2.pm.pmdig[idig];
		while ((cell = wd.getFirstCell()) >= 0){
			wd.Clear_c(cell);
			GINT w; w.u32 = cell + (idig << 8); 
			//if (dbiv.On_c(idig, cell)				|| pairs.On_c(cell))	w.u16[1] = ++nb;// index to off_store
			w.u16[1] = (uint16_t)(nb++); // here a store all version, multi chains in use
			ind_pm[idig][cell] = ntcands;// direct index to tcand
			tcands[ntcands++] = w;
		}
	}
	if (0){
		cout << "candidates table ntcands=" << ntcands << endl;
		for (int icand = 0; icand < ntcands; icand++){
			GINT w = tcands[icand];
			cout << w.u8[1] + 1 << cellsFixedData[w.u8[0]].pt << "\t" << w.u16[1] << endl;
		}
	}
}

/*

in clearing mode an AIC chain   x-a=b ...  e=a-x has one of the solution a/e
conflict can be unit or cell not pair
In multi chain cell abc.. or region abc.. one belong to the solution
in this chain a-x=y....z-e (e to clear) x is in the solution
for the rest as 
b-x=y...z-e 
x or z is in the solution, but we don't know wich one
*/
int XYSEARCH::Search(int fast){// search using zh_g.zerobased_sol[81] as digit
	// data to store and check found chains
	SearchInit(fast);
	GINT wstore[50];// chain to store or back
	BF128 wsloop, tsloop[20];
	int ntsloop = 0;
	locdiag = 0;	if (pm_go.opprint & 1) locdiag = 1;
	if (locdiag) {
		cout << "xysearch start fast =" << fast << endl;
	}
	pm_go.gintbuf.Init();
	maxpas = 35;// don't do that would delete ylsearch stored
	int iret = 0,   xc1;
	Init();// find all true bivalues and all "false" bivalue
	BF128 wbon = cells_biv_true;// expand all trues in bi values
	while ((xc1 = wbon.getFirst128()) >= 0){
		wbon.clearBit(xc1);
		c1 = From_128_To_81[xc1]; idig = zh_g2.zerobased_sol[c1];
		if (locdiag>1) cout << "start xyexpand " << idig + 1 << cellsFixedData[c1].pt << endl;
		if (Expand_true_false()){
			if (locdiag && fastmode) cout << "back xyexpand ok nsteps= "<<nsteps<<endl;
			if ((nsteps + 2) < maxpas) maxpas = nsteps + 2;
			if (locdiag)PrintTback();
			if (fast)iret += CleanXYChain();
			else{//apply if ok or store it for later use 
				int length = nsteps + mode+1, rating = pm_go.hint.ChainLengthAdjusted(70, length);
				if (0 &&rating <= 71){
					PrintTback();
					cout << "rating <=71 mode=" << mode << " nsteps=" << nsteps << " rating=" << rating << endl;
				}
				if (rating < maxrating) {
					if (locdiag)cout << " maxrating set to =" << rating << endl;
					maxrating = rating;
					ntsloop = 0;
					pm_go.gintbuf.Init(); // clean all stored
				}
				if (rating == 70) {
					int ir= CleanXYChain();
					if (ir) {
						PrintTback();
						iret = 1;
					}
				}
				else if (rating == maxrating) {// store it for later use
					// need to store tback, nsteps mode
					wsloop.SetAll_0();
					wstore[0].u16[1] = (uint16_t)mode;
					wstore[0].u16[0] = (uint16_t)(nsteps + 2);//total length to store
					for (int i = 0; i <= nsteps; i++){
						GINT64 w = tback[i];
						wstore[i + 1].u32=w.u32[0];
						wsloop.Set_c(w.u16[0]); //just flag cells involved for a loop 
					}
					if (!mode){// if loop no redundancy
						for (int i = 0; i < ntsloop; i++)
							if (wsloop == tsloop[i])goto nextxc1;
						tsloop[ntsloop++] = wsloop;
					}
					pm_go.gintbuf.Store(wstore);
				}
			}
		nextxc1:;
		}
	}
	if (locdiag>1)cout << " exit search iret=" << iret<<endl<<endl;
	if (iret){
		pm_go.hint.rating_done = (USHORT)maxrating;
		return 1;
	}
	if (fast || maxrating == 100)return iret;// no chain/loop in wait state
	if (!pm_go.gintbuf.InitGetback()) return 0;// should never be maxrating hit
		while (pm_go.gintbuf.GetBack(wstore)){
		mode = wstore[0].u16[1] ;
		nsteps = wstore[0].u16[0] -2;//total length to store
		for (int i = 1; i <= nsteps+1; i++){
			GINT w = wstore[i];
			tback[i- 1].u32[0] = w.u32;
		}
		if (locdiag)PrintTback();
		int ir = CleanXYChain();
		if (ir) {
			PrintTback();
			iret = 1;
		}
	}
	pm_go.hint.rating_done = (USHORT)maxrating;
	if (iret){
		if (locdiag>1)cout << "exit final rating=" << maxrating << endl;
	}
	return iret;
}

int XYSEARCH::SearchMulti(int fast){
	opprint = pm_go.opprint & 2;
	//if (opprint)zhou_solve.ImageCandidats();
	SearchInit(fast);
	int cell, iret = 0;;
	Init2();
	int locdiag = 0;
	//if (pm_go.cycle == 5 && opprint)locdiag = 1;
	if(locdiag){
		active_all.Print(" dig_active all");
		active_unit.Print(" dig_active unit");
	}
	BF128 w3 = zh_g2.cells_unsolved_e - pairs;
	while ((cell = w3.getFirstCell()) >= 0){// see cells to process 
		w3.Clear_c(cell);
		int digs = zh_g2.dig_cells[cell], n = 0; 
		for (int idig = 0, bit = 1; idig < 9; idig++, bit <<= 1){
			if (!(digs & bit))continue;
			if (active_all.Off_c(idig,cell)){
				if (n) goto nextcell;
				n++; 
			}
		}
		iret += MultiCell(cell);
	nextcell:;
	}
	for (int idig = 0; idig < 9; idig++){// try now regions
		BF128 wd = active_unit.pmdig[idig], pm = zh_g2.pm.pmdig[idig];
		BF32 ds3 = dig_sets3[idig];
		for (int iu = 0; iu < 27; iu++)if (ds3.On(iu)){
			BF128 wiu = units3xBM[iu]; wiu &= pm;
			BF128 wiud = wiu&wd, wiun = wiu-wiud , wseen = pm - wiu;
			int n = wiun.Count(), cell2;
			if (!n)goto tryiu;
			if (n > 3) goto nextiu;
			while ((cell2 = wiun.getFirstCell()) >= 0){
				wiun.Clear_c(cell2);
				wseen &= cell_z3x[cell2];
			}
			if (wseen.isEmpty()) goto nextiu;
		tryiu:
			iret += MultiUnit(idig,iu);
		nextiu:;
		}
	}
	if (iret) return 1;
	if (ntelims){// do stored elim(s) if any
		for (int i = 0; i < ntelims; i++){
			GINT w = telims[i];
			zhou_solve.ClearCandidate_c(w.u16[1], w.u16[0]);
		}
	}
	return ntelims;
}
void XYSEARCH::StartMulti( int dig, int cell){
	cleanstart.SetAll_0();
	cleanstart.pmdig[dig] = cell_z3x[cell];
	cleanstart.pmdig[dig] &= zh_g2.pm.pmdig[dig];// cleang.pmdig[dig];
	for (int i = 0; i < 9; i++)if (i - dig){
		if (zhou_solve.IsOnCandidate_c(i, cell))
			cleanstart.pmdig[i].Set_c(cell);
	}
}

int XYSEARCH::MultiUnit(int udigit, int unit){
	int diagloc = 0;
	//if (pm_go.cycle == 5 && opprint&&udigit == 3 && unit==24) diagloc = 2;
	locdiag =  diagloc;
	BF128 wiu = units3xBM[unit]; wiu &= zh_g2.pm.pmdig[udigit];
	int ncells = wiu.Count(),wcell;
	if (diagloc){
		cout << "start region for dig " << udigit + 1 << " unit" << unit
			<< " ncells=" << ncells << endl;
	}
	cleang = zh_g2.pm;
	npaths = 0;
	BF128 wiuw = wiu;
	while ((wcell = wiuw.getFirstCell()) >= 0){
		wiuw.Clear_c(wcell);
		PATH &wp = paths[npaths++];
		wp.dig = udigit; wp.cell = wcell;
		if (active_unit.On_c(wp.dig,wp.cell)){
			if (diagloc > 1)cout << "go cell " << cellsFixedData[wp.cell].pt << endl;
			StartMulti(wp.dig, wp.cell);
			used_off_digits.SetAll_0();
			used_on_digits.SetAll_0();
			used_on_digits.Set_c(wp.dig,wp.cell);
			t[0].u64 = wp.cell + (wp.dig << 16);
			nt = 1;
			if (diagloc > 1)cleanstart.Print("cleanstart at call");
			Expand_Multi(cleanstart);
			cleang &= cleanstart;
			if (diagloc > 1){
				cleanstart.Print("cleanstart");
				cleang.Print("cleang");
			}
			if (cleang.IsEmpty())return 0;
			wp.nt = nt;
			memcpy(wp.t, t, nt * 8);
		}
		else { // cleaning must be seen by this digit (no cell cleaning)
			if (diagloc > 1)cout << "go direct cell " << cellsFixedData[wp.cell].pt << endl;
			StartMulti(wp.dig, wp.cell);
			cleang &= cleanstart;
			wp.nt = 0;
		}
	}
	if (diagloc>1){
		cleang.Print("cleang ");
		cout << "call D0_Clean " << endl;
	}
	return Do_Clean();//clean or store the final ratings
}
int XYSEARCH::MultiCell(int c0){
	int diagloc = 0;
	if (pm_go.opprint2 & 8) diagloc = 1;
	//if (c0 == 68)diagloc = 2;
	locdiag = 0;// diagloc;
	if (diagloc>1)cout << "start Multicell for " << cellsFixedData[c0].pt << endl;
	cleang = zh_g2.pm;
	if (diagloc>1)	cleang.Print("cleang at start");
	int digs = zh_g2.dig_cells[c0]; 
	npaths = 0;
	for (int idig = 0, bit = 1; idig < 9; idig++, bit <<= 1){
		if (!(digs & bit))continue;
		PATH &wp = paths[npaths++];
		wp.dig = idig; wp.cell = c0;
		cleanstart.SetAll_0();
		cleanstart.pmdig[idig] = cell_z3x[c0];
		cleanstart.pmdig[idig] &= cleang.pmdig[idig];
		if (diagloc>1)cout << "dig " << idig + 1 << endl;
		if (active_all.On_c(idig,c0)){
			if (diagloc>1)cout << "go dig " << idig + 1 << endl;
			nt = 0;
			used_off_digits.SetAll_0();
			used_on_digits.SetAll_0();
			used_on_digits.Set_c(idig, c0);
			t[nt++].u64 = c0 + (idig << 16);
			Expand_Multi(cleanstart);
			cleang &= cleanstart;
			if (diagloc>1){
				cleanstart.Print("clean start");
				cleang.Print("cleang ");
				if (cleang.IsEmpty()) cout << "empty cleang" << endl;
			}
			if (cleang.IsEmpty())return 0;
			wp.nt = nt;
			memcpy(wp.t, t, nt * 8);
		}
		else { // cleaning must be seen by this digit (no cell cleaning)
			cleang &= cleanstart;
			wp.nt = 0;
		}
	}	
	if (diagloc>1){
		cleang.Print("cleang ");
		cout << "call D0_Clean " << endl;
	}
	return Do_Clean();//clean or store the final ratings
}

void XYSEARCH::PrintBackMulti(int elim_dig, int elim_cell){
	BUILDSTRING & wp = pm_go.bdsp[0];
	if (wp.NoFree()) return; // safety code must have room to store
	cout << "Print back multi elim " << elim_dig + 1 << cellsFixedData[elim_cell].pt << endl;
	int nstore = 0;
	GINT64 * pstore = wp.GetFree();
	BF128 seen = cell_z3x[elim_cell]; seen &= zh_g2.pm.pmdig[elim_dig];
	for (int ipath = 0; ipath < npaths; ipath++){
		PATH &pth = paths[ipath];
		cout << pth.dig + 1 << cellsFixedData[pth.cell].pt << " ";
		if ((!pth.nt) || (pth.dig == elim_dig && seen.On_c(pth.cell))){// seen direct
			goto next_ipath;
		}
		for (int it = 0; it < pth.nt; it++){
			GINT64 wt = pth.t[it];
			int cell = wt.u16[0], dig = wt.u16[1], pas = wt.u16[3];
			if (pas & 1) continue;// look for on status
			if (cell==elim_cell ||
				(elim_dig == dig &&seen.On_c(cell))){
				// find way back and print it
				//cout << "it=" << it << " npas=" << pas << endl;
				register int Ri = it, Rn = pas;
				while (--Rn >= 0) {
					tback[Rn].u32[0] = pth.t[Ri].u32[0];
					Ri = pth.t[Ri].u16[2];
				}
				for (int i = 0; i < pas; i++){
					if(! (i & 1))cout << "~";
					cout << tback[i].u16[1] + 1 << cellsFixedData[tback[i].u16[0]].pt << " ";
				}
				goto next_ipath;
			}
		}
		cout << "xy do_clean elim missing in path" << ipath << endl;// should never be
	next_ipath:
		cout << "~" << elim_dig + 1 << cellsFixedData[elim_cell].pt << endl;
	}

}
int XYSEARCH::Do_Clean(){
	if (cleang.IsEmpty()) return 0;
	if(locdiag)cout << " entry clean fastmode= " << fastmode << endl;
	// cleang contains potential eliminations look for "length"
	if (fastmode){// if fast mode do all in once
		if (opprint) cleang.Print("multi chain fast cleaning");
		return zhou_solve.Clean(cleang);
	}
	int iret = 0;
	for (int idig = 0; idig < 9; idig++){
		BF128 w = cleang.pmdig[idig];
		if (w.isEmpty())continue;
		int cell;
		while ((cell = w.getFirstCell()) >= 0){
			w.Clear_c(cell);
			if (locdiag)cout << "test clean elim" << idig + 1 << cellsFixedData[cell].pt << endl;
			BF128 seen = cell_z3x[cell]; seen &= zh_g2.pm.pmdig[idig];
			int length = 0;
			for (int ipath = 0; ipath < npaths; ipath++){
				PATH &pth = paths[ipath];
				if ((!pth.nt)||(pth.dig== idig && seen.On_c(pth.cell))){// seen direct
					length += 2;
					goto next_ipath;
				}
				for (int it = 0; it < pth.nt; it++){
					GINT64 wt = pth.t[it];
					int wcell = wt.u16[0], wdig = wt.u16[1], pas = wt.u16[3];
					if (locdiag && it==0)cout << "start" << wdig + 1 << cellsFixedData[wcell].pt 
						<<" pas="<<pas<< endl;
					if (pas & 1) continue;// look for on status
					if (cell ==wcell ||
						(idig == wdig &&seen.On_c(wcell))){
						length += pas + 2;
						goto next_ipath;
					}
				}
				cout << "xy do_clean elim missing in path" << ipath << endl;// should never be
				goto next_elim;
			next_ipath:;
			}
			int rating = pm_go.hint.ChainLengthAdjusted(80, length);
			if (rating <= pm_go.rat_er){// then do it
				if (zhou_solve.IsOnCandidate_c(idig, cell)){
					zhou_solve.ClearCandidate_c(idig, cell);
					if (opprint){
						cout << "cleaned rating=" << rating << endl;
						PrintBackMulti(idig, cell);
					}
					ntelims = 0;
					maxrating = pm_go.rat_er;
					iret = 1;
					continue;
				}
			}
			if (rating > maxrating)continue;
			if (rating < maxrating){
				maxrating = rating;
				ntelims = 0;
			}
			if (ntelims < 40){// must continue to look for smaller
				if (opprint){
					cout << "stored rating=" << rating << endl;
					PrintBackMulti(idig, cell);
				}
				telims[ntelims++].u32 = cell | (idig << 16);
			}
		}
	next_elim:;
	}

	return iret;
}

int XYSEARCH::AddElim(int d, int c, int rating){
	if (rating <= pm_go.rat_er || fastmode){// then do it
		ntelims = 0;
		maxrating = pm_go.rat_er;
		if (zhou_solve.IsOnCandidate_c(d, c)){
			zhou_solve.ClearCandidate_c(d, c);
			if (opprint)	cout << "immediate elim " << d + 1 << cellsFixedData[c].pt << " rating=" << rating
				<< "  pm_go.rat_er=" << pm_go.rat_er << " nt=" << nt << endl;
			elim_done = 1;
			return 1;
		}
		return 0;
	}
	if (rating > maxrating) return 0;
	if (elim_done)return 0;
	if (opprint)	cout << "addelim " << d + 1 << cellsFixedData[c].pt << " rating=" << rating
		<< "  pm_go.rat_er=" << pm_go.rat_er << " nt=" << nt << " ntelim=" << ntelims << endl;
	if (rating < maxrating){
		maxrating = rating;
		ntelims = 0;
		elim_stored.SetAll_0();
		if (opprint)cout<< "reset elim_count to 0" << endl;
	}
	if (ntelims < 40 && elim_stored.Off_c(d,c)){// must continue to look for smaller
		telims[ntelims++].u32 = c | (d << 16);
		elim_stored.Set_c(d, c);
		return 2;
	}
	return 0;
}


void XYSEARCH::Expand_Multi(PM3X & cleanstart){// start is t;nt falses
	nsteps = 0;// start with 1 step and mode loop
	int  i, ntd = 0, ntp;
	while (nsteps++ <20){ // safety, should never go so far
		if (locdiag) cout << "expand npas="<<nsteps <<" nt="<<nt<< endl;
		ntp = ntd;		ntd = nt;
		for (i = ntp; i < ntd; i++){
			cell = t[i].u16[0];		 digit = t[i].u16[1];
			if (locdiag) cout << digit + 1 << cellsFixedData[cell].pt << " processed"<<endl;
			if (nsteps & 1) OnToOff(i);   else 	 OffToOn(i);
		}
		if (nt == ntd) 	return;
		if (nsteps & 1){
			if (locdiag){
				cout << "added off\t";
				for (i = ntd; i < nt; i++) cout << t[i].u16[1] + 1 << cellsFixedData[t[i].u16[0]].pt << " ";
				cout << endl;
			}
			continue;// nothing to do after on to off
		}
		for (i = ntd; i < nt; i++){// added in this step
			cell = t[i].u16[0];		 digit = t[i].u16[1];
			if (locdiag) cout << digit + 1 << cellsFixedData[cell].pt<<" added true" << endl;
			BF128 wb = cell_z3x[cell]; wb &= zh_g2.pm.pmdig[digit];
			cleanstart.pmdig[digit] |= wb;
			int digs = zh_g2.dig_cells[cell];
			for (int id = 0,bit=1; id < 9; id++,bit<<=1) 
				if( (id - digit) && (digs & bit)){
					cleanstart.pmdig[id].Set_c(cell);
				}
		}
	}
}
//____________________ XY DYN base 85


void XYSEARCH::ExpandDynamic(GINT cand){// start with cand on
	ddig = cand.u8[1];
	dcell = cand.u8[0];
	int dind = cand.u16[1];
	int diag = 0;
	//if (dcell == 34) diag = 1;
	//if (pm_go.cycle==15 && maxpas>10 && ddig ==2  && dcell==30 )		diag = 2;
	//if (pm_go.cycle == 16 && maxpas>6 &&  ddig == 1 && dcell == 5)diag = 3;
	nsteps = is_contradiction = 0;// start with 1 step 
	//if (zh_g.zerobased_sol[c1] == idig)is_contradiction = 2;// skip test if valid
	if (zh_g2.zerobased_sol[dcell] == idig)is_contradiction = 2;// skip test if valid
	nt = 1;	t[0].u64 = dcell | (ddig << 16);	// source to 0
	used_off_digits.SetAll_0();	used_on_digits.SetAll_0();
	used_on_digits.Set_c(ddig, dcell);
	if (diag ) cout << "start xyexpand " << ddig + 1 << cellsFixedData[dcell].pt
		<< " dind=" <<dind<<" maxpas="<<maxpas<< endl;
	ntd = 0;
	while (nsteps++ < maxpas){ 
		ntp = ntd;
		ntd = nt;
		if (diag>1) cout << "step " << nsteps << " ntp=" << ntp << " ntd=" << ntd << endl;
		for (int i = ntp; i < ntd; i++){
			cell = t[i].u16[0];		 digit = t[i].u16[1];
			if (is_contradiction == 1) {// lock expand if on and off
				if (used_off_digits.On_c(digit, cell)
					&& used_on_digits.On_c(digit, cell)) continue;
			}
			if (diag > 1)cout << "go for " << digit + 1 << cellsFixedData[cell].pt << endl;
			if (nsteps & 1)		 OnToOff_Dyn(i); 		else OffToOn_Dyn(i);
			if (nt > 350) break;
		}
		if (nt == ntd) 	break;
		if (!is_contradiction){
			contradiction = used_off_digits;  contradiction &= used_on_digits;
			if (!contradiction.IsEmpty()){
				if (diag) {
					cout << cand.u8[1] + 1 << cellsFixedData[cand.u8[0]].pt << " contradiction for npas=" << nsteps << " nt=" << nt << endl;
					contradiction.Print("contradiction ");
					if (diag > 2)DebugT();
				}
				is_contradiction = 1;
				//if (maxpas > nsteps + 2)maxpas = nsteps + 2;
				if (fastmode) {
					AddElim(ddig, dcell, 85);
					if (diag)cout << "85 fastelim " << ddig + 1 << cellsFixedData[dcell].pt << endl;
					off_status[dind] = used_off_digits;
					return;
				}
				DynamicSolveContradiction(cand,contradiction);
			}
		}
		if (diag > 1 && (nsteps & 1)) {
			cout << "end of step=" << nsteps << " nt=" << nt << endl;
			used_off_digits.Print("dyn off status end of step");
		}
		if ((maxpas== 6 && nt > 150)|| nt>300)break;
	}
	if (diag) {
		cout << "expand closed nt=" << nt << " off count" << used_off_digits.Count() << endl;
		used_off_digits.Print("dyn off status ");
	}
	off_status[dind] = used_off_digits;// store off status 

}
int XYSEARCH::ExpandDynamicToElim(GINT cand,GINT target){// start with cand on
	int tdig = target.u16[1], tcell = target.u16[0];
	ddig = cand.u16[1];
	dcell = cand.u16[0];
	int locdiag = 0;
	//if (pm_go.cycle == 5 && ddig == 7 && dcell == 19 && tdig ==4 && tcell== 20)		locdiag = 1;
	if (locdiag)cout << "expand to elim diag mode target " << cand.u16[1] + 1 << cellsFixedData[cand.u16[0]].pt
		<< " to "<< tdig + 1 << cellsFixedData[tcell].pt << endl;
	//if( tcell == 33) locdiag = 1;
	nsteps = 0;
	nt = 1;	
	t[0].u64 = dcell | (ddig << 16);	// source to 0
	used_off_digits.SetAll_0();	used_on_digits.SetAll_0();
	used_on_digits.Set_c(ddig, dcell);
	ntd = 0;
	while (nsteps++ < 40){// should never pass the target
		ntp = ntd;
		ntd = nt;
		for (int i = ntp; i < ntd; i++){
			cell = t[i].u16[0];		 digit = t[i].u16[1];
			if (nsteps & 1){
				OnToOff_Dyn(i);
				if (used_off_digits.On_c(tdig, tcell)) return 1;
			}
			else OffToOn_Dyn(i);
			if (nt > 350)return  0;// TEMP SAFETY TEST
		}
		if (nt > 350) {
			if (locdiag) {
				cout << "limit reached  npas=" << nsteps << " nt=" << nt << endl;
				if (nsteps & 1)used_off_digits.Print("off status");
				else used_on_digits.Print("on status");
			}
			return 0;
		}
		if (locdiag ){
			cout << "end npas=" << nsteps << " nt=" << nt<< endl;
			if (nsteps&1)used_off_digits.Print("off status");
			else used_on_digits.Print("on status");
		}
		if (nt == ntd ) 	break;// should never be
	}
	return 0;
}

void XYSEARCH::SearchDynPass(int nmax){	// try a  pass limited to nmax steps
	nind = 0;
	maxpas = nmax;
	for (int icand = 0; icand < ntcands; icand++)// all candidates processed 
		ExpandDynamic(tcands[icand]);
	if (opprint) {
		cout << "search pass end phase 1 elim_done status=" << elim_done 
			<<"  maxrating="  << maxrating
			<< endl;
	}
	if (elim_done) return;
	if (opprint)cout << "try cells bi values" << endl;
	// try all bi values in mode x->~a and y->~a adding one in length
	BF128 wp = pairs;
	uint32_t  dc1,dc2;
	while ((cell = wp.getFirstCell()) >= 0){
		if (opprint){
			cout << "cells "<<cellsFixedData[cell].pt << endl;
		}
		wp.Clear_c(cell);
		int digs = zh_g2.dig_cells[cell];
		bitscanforward(dc1, digs);
		bitscanreverse(dc2, digs);
		int i1 = ind_pm[dc1][cell], i2 = ind_pm[dc2][cell];// pointers to tcands
		GINT cand1 = tcands[i1], cand2 = tcands[i2];
		PM3X welims = off_status[cand1.u16[1]]; welims &=off_status[cand2.u16[1]];
		if (welims.IsEmpty()) continue;
		DynamicSolveContradiction(dc1, cell, dc2, cell, welims);
	}
	if (opprint)cout << "try unit bi values" << endl;
	for (int id = 0; id < 9; id++){
		//if (nmax > 10)cout << "digit=" << id + 1 << endl;
		for (int iu = 0; iu < 27; iu++){
			if (dig_bivsets[id].Off(iu))continue;
			//if (nmax > 10)cout << "unit=" << iu << endl;
			BF128 wu = units3xBM[iu]; wu &= zh_g2.pm.pmdig[id];
			int cell1 = wu.getFirstCell();
			wu.Clear_c(cell1);
			int cell2 = wu.getFirstCell();
			int i1 = ind_pm[id][cell1], i2 = ind_pm[id][cell2];// pointers to tcands
			GINT cand1 = tcands[i1], cand2 = tcands[i2];
			PM3X welims = off_status[cand1.u16[1]]; welims &= off_status[cand2.u16[1]];
			if (welims.IsEmpty()) continue;
			DynamicSolveContradiction(id, cell1, id, cell2, welims);
		}
	}

	//if (nmax == 6)SearchDynPassMulti(6);
	//else 
	SearchDynPassMulti(nmax);
}

void XYSEARCH::SearchDynPassMulti(int nmax){// try multi chains if nothing low
	if (elim_done) return;
	if (ntelims && maxrating <= 88) return;
	if (opprint)cout << "try dyn multi cells not bi values" << endl;
	// try all bi values in mode x->~a and y->~a adding one in length
	BF128 wp = zh_g2.cells_unsolved_e-pairs;
	GINT target,p;
	GINT64 tbn[9][400];
	int ntbn[9], nx,xcell;
	PM3X welims;
	while ((xcell = wp.getFirstCell()) >= 0){
		wp.Clear_c(xcell);
		int digs = zh_g2.dig_cells[xcell], ndigs=_popcnt32(digs);
		if (nmax < 8 && ndigs>3)continue;
		welims.SetAll_1();
		while ( digs){
			bitscanforward(d2, digs);
			digs ^= 1 << d2;
			int i1 = ind_pm[d2][xcell], coff = tcands[i1].u16[1];
			welims &= off_status[coff];
		}
		if (welims.IsEmpty()) continue;
		if (fastmode){// do it in bloc and return
			if (opprint) {
				cout << "cell multi analysis " << cellsFixedData[xcell].pt << endl;
				welims.Print(" elims seen ");
			}
			zhou_solve.Clean(welims);
			elim_done = 1;
			continue;
		}
		for (int id = 0; id < 9; id++) if (welims.pmdig[id].isNotEmpty()){
			BF128 elimd = welims.pmdig[id];
			int elim_cell;
			while ((elim_cell = elimd.getFirstCell()) >= 0){
				elimd.Clear_c(elim_cell);
				target.u32 = elim_cell | (id << 16);
				int length = 0, n = 0;
				digs = zh_g2.dig_cells[xcell];
				while (digs){
					bitscanforward(d2, digs);
					digs ^= 1 << d2;
					p.u32 = xcell | (d2 << 16);
					if (!ExpandDynamicToElim(p, target)) {// redo expansion should work
						if (opprint) {
							cout << "anomaly in cell redo " << d2 + 1 << cellsFixedData[xcell].pt
								<< " to target " << id + 1 << cellsFixedData[elim_cell].pt << endl;
						}
						goto exitcell;
					}
					nx=ntbn[n] = BackDynamicOff(target);
					if (locdiag){
						cout << "nx=" << nx  << endl;
						PrintBackCom("locdiag on path ", tback, nx, 1);
					}
					length += nx;
					memcpy(tbn[n++], tback, nx * 8);

				}
				int rating = pm_go.hint.ChainLengthAdjusted(85, length);
				if (AddElim(id, elim_cell, rating)){
					if (opprint){
						cout << "cleaned or stored rating " << rating << endl;
						for (int ip = 0; ip < n; ip++)
							PrintBackCom("", tbn[ip], ntbn[ip], 1);
					}
				}

			}
		}
	exitcell:;
	}
	if (opprint)cout << "try units not bi values" << endl;
	for (int id1 = 0; id1 < 9; id1++){
		for (int iu = 0; iu < 27; iu++){
			if (dig_bivsets[id1].On(iu))continue;
			BF128 wu = units3xBM[iu]; wu &= zh_g2.pm.pmdig[id1];
			if (wu.isEmpty())continue;
			int tcu[10], ntcu = wu.Table3X27(tcu);
			welims.SetAll_1();
			for (int icu = 0; icu < ntcu; icu++){
				int xcell = tcu[icu]	;
				int i1 = ind_pm[id1][xcell], coff = tcands[i1].u16[1];
				welims &= off_status[coff];
			}
			if (welims.IsEmpty()) continue;
			if (0 && opprint) {
				cout << "digit " << id1 + 1 << " unit " << iu  << " multi analysis"<< endl;
				welims.Print(" unit elims seen ");
			}
			if (fastmode){// do it in bloc and return
				zhou_solve.Clean(welims);
				elim_done = 1;
				continue;
			}
			for (int id = 0; id < 9; id++) if (welims.pmdig[id].isNotEmpty()){
				BF128 elimd = welims.pmdig[id];
				int elim_cell;
				while ((elim_cell = elimd.getFirstCell()) >= 0){
					elimd.Clear_c(elim_cell);
					target.u32 = elim_cell | (id << 16);
					if (locdiag)cout << "try unit elim" << id + 1 << cellsFixedData[elim_cell].pt << endl;
					int length = 0, n = 0,xcell;
					for (int icu = 0; icu < ntcu; icu++){
						xcell = tcu[icu];
						p.u32 = xcell | (id1 << 16);
						if (!ExpandDynamicToElim(p, target)) {// redo expansion should work
							if (opprint) {
								cout << "trying unit elim" << id + 1 << cellsFixedData[elim_cell].pt << endl;
								cout << "anomaly in unit/cell redo start "
									<< id1 + 1 << cellsFixedData[xcell].pt << endl;
								//fout1 << "anomaly in unit/cell redo" << endl;
							}
							goto exitunit;;
						}
						nx = ntbn[n] = BackDynamicOff(target);
						if (locdiag){
							cout << "nx=" << nx << endl;
							PrintBackCom("locdiag on path ", tback, nx, 1);
						}
						length += nx;
						memcpy(tbn[n++], tback, nx * 8);

					}
					int rating = pm_go.hint.ChainLengthAdjusted(85, length);
					if (AddElim(id, elim_cell, rating)){
						if (opprint){
							cout << "cleaned or stored rating " << rating << endl;
							for (int ip = 0; ip < n; ip++)
								PrintBackCom("", tbn[ip], ntbn[ip], 1);
						}
					}
				}
			}
		exitunit:;
		}
	}
}

int XYSEARCH::SearchDyn(int fast){
	opprint = pm_go.opprint;
	if (opprint)cout << "85 search dyn fast 0/1= "<<fast << endl;
	SearchInit(fast);
	InitCandidatesTable();// build the table of candidates 
	if (fast) {
		maxpas = 35;
		PM3X welims;
		if (opprint)cout << "85 dyn fast 1" << endl;
		for (int icand = 0; icand < pm_go.xysearch.ntcands; icand++) {// all candidates processed 
			GINT wc = tcands[icand];
			if ((char)wc.u8[1] == zh_g2.zerobased_sol[wc.u8[0]]) continue;
			ExpandDynamic(wc);
		}
		if (elim_done) return 1;
		for (int icand = 0; icand < pm_go.xysearch.ntcands; icand++) {// all candidates processed 
			GINT wc = tcands[icand];
			if ((char)wc.u8[1] == zh_g2.zerobased_sol[wc.u8[0]]) 
			ExpandDynamic(wc);// expand the good candidates
		}
		if (opprint)cout << "try fast  cells bi values" << endl;
		// try all bi values in mode x->~a and y->~a adding one in length
		BF128 wp = pairs;
		uint32_t  dc1, dc2;
		while ((cell = wp.getFirstCell()) >= 0) {
			wp.Clear_c(cell);
			int digs = zh_g2.dig_cells[cell];
			bitscanforward(dc1, digs);
			bitscanreverse(dc2, digs);
			int i1 = ind_pm[dc1][cell], i2 = ind_pm[dc2][cell];// pointers to tcands
			GINT cand1 = tcands[i1], cand2 = tcands[i2];
			welims = off_status[cand1.u16[1]];
			welims &= off_status[cand2.u16[1]];
			if (welims.IsEmpty()) continue;
			if (opprint) welims.Print("elims fast cell bi values");
			zhou_solve.Clean(welims);
			elim_done = 1;
		}
		if (opprint)cout << "try fast unit bi values" << endl;
		for (int id = 0; id < 9; id++) {
			for (int iu = 0; iu < 27; iu++) {
				if (dig_bivsets[id].Off(iu))continue;
				BF128 wu = units3xBM[iu]; wu &= zh_g2.pm.pmdig[id];
				int cell1 = wu.getFirstCell();
				wu.Clear_c(cell1);
				int cell2 = wu.getFirstCell();
				int i1 = ind_pm[id][cell1], i2 = ind_pm[id][cell2];// pointers to tcands
				GINT cand1 = tcands[i1], cand2 = tcands[i2];
				welims = off_status[cand1.u16[1]]; 
				welims &= off_status[cand2.u16[1]];
				if (welims.IsEmpty()) continue;
				zhou_solve.Clean(welims);
				elim_done = 1;
				if (opprint) welims.Print("elims fast unit bi values");
			}
		}
		if (elim_done) return 1;
		if (opprint)cout << "try fast multi cells" << endl;
		// try all bi values in mode x->~a and y->~a adding one in length
		wp = zh_g2.cells_unsolved_e - pairs;
		int  xcell;
		while ((xcell = wp.getFirstCell()) >= 0) {
			wp.Clear_c(xcell);
			int digs = zh_g2.dig_cells[xcell];
			if (_popcnt32(digs) <3)continue;
			welims.SetAll_1();
			while (digs) {
				bitscanforward(d2, digs);
				digs ^= 1 << d2;
				int i1 = ind_pm[d2][xcell], coff = tcands[i1].u16[1];
				welims &= off_status[coff];
			}
			if (welims.IsEmpty()) continue;
			zhou_solve.Clean(welims);
			if (opprint) welims.Print("elims fast multi cells");
			elim_done = 1;
		}
		if (opprint)cout << "try fast milti units" << endl;
		for (int id1 = 0; id1 < 9; id1++) {
			for (int iu = 0; iu < 27; iu++) {
				if (dig_bivsets[id1].On(iu))continue;
				BF128 wu = units3xBM[iu]; wu &= zh_g2.pm.pmdig[id1];
				if (wu.isEmpty())continue;
				int tcu[10], ntcu = wu.Table3X27(tcu);
				welims.SetAll_1();
				for (int icu = 0; icu < ntcu; icu++) {
					int xcell = tcu[icu];
					int i1 = ind_pm[id1][xcell], coff = tcands[i1].u16[1];
					welims &= off_status[coff];
				}
				if (welims.IsEmpty()) continue;
				zhou_solve.Clean(welims);
				if (opprint) welims.Print("elims fast multi units");
				elim_done = 1;
			}
		}
		return elim_done;
	}
	SearchDynPass(6);// try a first pass limited to 6 steps
	if (elim_done) return 1;
	if (ntelims && maxrating <= 88){// do it if small enough 
		pm_go.hint.rating_done = (USHORT)maxrating;
		for (int i = 0; i < ntelims; i++){
			GINT w = telims[i];
			zhou_solve.ClearCandidate_c(w.u16[1], w.u16[0]);
		}
		return 1;
	}
	if (opprint) {
		cout << "search phase 1 closed ntelims="<< ntelims	<< endl;
	}
	//if (ntelims) SearchDynPass(12);//  just secure the found rating
	//else 
	SearchDynPass(35);// try a second  pass "no limit"
	if (elim_done) return 1;
	if (ntelims ){// do it  
		pm_go.hint.rating_done = (USHORT)maxrating;
		for (int i = 0; i < ntelims; i++){
			GINT w = telims[i];
			zhou_solve.ClearCandidate_c(w.u16[1], w.u16[0]);
		}
		return 1;
	}
	return 0;
}
void XYSEARCH::DynamicSolveContradiction(GINT cand,PM3X cont){// find path and elims low rating a-> x/~x
	if (fastmode){// do it in bloc and return
		zhou_solve.ClearCandidate_c(cand.u8[1], cand.u8[0]);
		elim_done = 1;
		return;
	}
	int diag = 0;
	//if (pm_go.cycle == 14 &&cand.u8[1] == 8 && cand.u8[0] == 15)diag = 1;
	if (diag) cout << "solve cont for " << cand.u8[1] + 1 << cellsFixedData[cand.u8[0]].pt << endl;
	for (int id = 0; id < 9; id++) if (cont.pmdig[id].isNotEmpty()){
		BF128 elimd = cont.pmdig[id];
		int elim_cell;
		while ((elim_cell = elimd.getFirstCell()) >= 0){
			elimd.Clear_c(elim_cell);
			GINT64 target_on, target_off;
			int n = 0;
			for (int i = 0; i < nt; i++){// locate targets 
				GINT64 w = t[i];
				if (w.u16[1] != id)continue;
				if (w.u16[0] != elim_cell)continue;
				n++;
				if (w.u16[3] & 1)target_off = w; else target_on = w;
			}
			if (n != 2) continue;// should never be
			if (diag){
				cout << "target_on" << target_on.u16[1] + 1 << cellsFixedData[target_on.u16[0]].pt
					<< " step" << target_on.u16[3] << " source=" << target_on.u16[2] << endl;
				cout << "target_off" << target_off.u16[1] + 1 << cellsFixedData[target_off.u16[0]].pt
					<< " step" << target_off.u16[3] << " source=" << target_off.u16[2] << endl;

			}
			int n1 = BackDynamic(target_on, t, nt);
			GINT64 t1b[200];
			memcpy(t1b, tback, n1 * 8);
			int n2 = BackDynamic(target_off, t, nt);
			int rating = pm_go.hint.ChainLengthAdjusted(85, n1 + n2);
			//cout << "rating " << rating<<endl<<endl;
			if (AddElim(tback[0].u16[1], tback[0].u16[0], rating)){
				if (opprint){
					cout << "cleaned or stored rating " << rating <<" ntelim="<<ntelims<< endl;
					PrintBackCom("off path ", tback, n2, 1);
					PrintBackCom("on  path ", t1b, n1, 1);
					cout << "done" << endl;
				}
			}
		}
	}
}
void XYSEARCH::DebugDynamicSolveContradiction(const char * lib, GINT cand, GINT targ) {
	cout << "debug contradiction error " << lib << endl;
	cout << "start " << cand.u16[1] + 1 << cellsFixedData[cand.u16[0]].pt << endl;
	cout <<"target " << targ.u16[1] + 1 << cellsFixedData[targ.u16[0]].pt << endl;
	cout << "n expand=" << nt << " nback="<<nback<< endl;
	PrintBackCom("back path ", tback, nback, 1);
	cout << "i\tcand\tsource\tpas" << endl;
	for (int i = 0; i < nt; i++) {
		GINT64 w = t[i];
		int c = w.u16[0], d = w.u16[1], s = w.u16[2], p = w.u16[3];
		cout << i << "\t" << d + 1 << cellsFixedData[c].pt;
		if(s & 030000)		cout<< "\t0" << oct << s << dec;
		else cout <<"\t"<<s;
		cout << "\t" << p << endl;
	}
}
/* this is for the serate mode x-> ~a and ~x -> � 
   here a bi value at start ~x -> y 
   x is dig1 cell1
   y is dig2 cell2
*/
void XYSEARCH::DynamicSolveContradiction(int dig1, int cell1, int dig2, int cell2, PM3X cont){
	if (fastmode){// do it in bloc and return
		zhou_solve.Clean(cont);
		elim_done = 1;
		return;
	}
	int locdiag = 0;
	//if (pm_go.cycle == 13 && dig1==2 &&dig2==2 && cell1>=72)		locdiag = 1;
	//if (dig1 != dig2 && cell1 == 62) locdiag = 1;
	if (locdiag){
		cout << "DynamicSolveContradiction " << dig1 + 1 << cellsFixedData[cell1].pt << " "
			<< dig2 + 1 << cellsFixedData[cell2].pt  << endl;
		cont.Print("for elims");

	}
	GINT p1, p2,target;
	p1.u32 = cell1 | (dig1 << 16); 
	p2.u32 = cell2 | (dig2 << 16);
	for (int id = 0; id < 9; id++) if (cont.pmdig[id].isNotEmpty()){
		BF128 elimd = cont.pmdig[id];
		int elim_cell;
		while ((elim_cell = elimd.getFirstCell()) >= 0){
			elimd.Clear_c(elim_cell);
			target.u32 = elim_cell | (id <<16);
			if (locdiag) cout << "to elim " << id + 1 << cellsFixedData[elim_cell].pt << endl;
			if(!ExpandDynamicToElim(p1,target)) continue;// redo expansion should work
			int n1 = BackDynamicOff(target);
			if (p1.u16[0] != tback->u8[0]) {
				if (opprint) {
					cerr << " DynamicSolveContradiction illegal way back p1" << endl;
					DebugDynamicSolveContradiction("way back p1", p1, target);
				}
				continue;;
			}
			if (locdiag){
				cout << "n1=" << n1 << endl;
				PrintBackCom("locdiag on path ", tback, n1, 1);
			}
			if (!n1) continue; // should never be
			GINT64 t1b[200];
			memcpy(t1b, tback, n1 * 8);
			if (!ExpandDynamicToElim(p2, target)) continue;// redo expansion should work
			int n2 = BackDynamicOff(target);
			if (p2.u16[0] != tback->u8[0]) {
				if (opprint) {
					DebugDynamicSolveContradiction("way back p2", p2, target);
					cerr << " DynamicSolveContradiction illegal way back p2" << endl;
				}
				continue;;
			}
			if (!n2) {
				if (opprint) {
					DebugDynamicSolveContradiction("way back p2", p2, target);
					cerr << "DynamicSolveContradiction pas trouve cible nt=" << nt << endl;
				}
				continue;;
			}
			if (locdiag){
				cout << "n2=" << n2 << endl;
				PrintBackCom("locdiag off path ", tback, n2, 1);
			}
			if (!n2) continue; // should never be
			int rating = pm_go.hint.ChainLengthAdjusted(85, n1 + n2);
			if (AddElim(id, elim_cell, rating)){
				if (opprint){
					cout << "cleaned or stored rating " << rating << " ntelim=" << ntelims << endl;
					PrintBackCom("off path ", tback, n2, 1);
					PrintBackCom("on  path ", t1b, n1, 1);
				}
			}
		}
	}
}
int XYSEARCH::BackDynamic(GINT64 target, GINT64 * tb, int ntb) {
	int diag = 0;
	//if (pm_go.cycle == 14 && target.u16[1] == 5 && target.u16[0] == 31)diag = 1;

	PM3X back_bf; back_bf.SetAll_0(); // no possible conflict in the way back
	int itret = 1, itret1 = 0;
	GINT64 tretw[300];// cell;digit;source/last in;step/sign
	tretw[0] =target;
	back_bf.Set_c(target.u16[1], target.u16[0]);
	while (itret1 < itret){// && itret < 100 ) { // solve each entry back
		GINT64 x = tretw[itret1];
		if (diag){
			cout << "BackDynamic " << x.u16[1]+1 << cellsFixedData[x.u16[0]].pt
				<< " step " << x.u16[3] << " source=0x" << hex << x.u16[2] << dec << endl;
		}
		if (!x.u16[3]) { itret1++;	continue; }  // start point
		if (!(x.u16[2] & 0x3000)) { // this is direct
			GINT64 y = tb[x.u16[2]];
			if (diag){
				cout << "source direct BackDynamic " << y.u16[1] + 1 << cellsFixedData[y.u16[0]].pt
					<< " status  " << back_bf.On_c(y.u16[1], y.u16[0] )<< endl;
			}
			if (back_bf.On_c(y.u16[1], y.u16[0])){ itret1++;	continue; }
			back_bf.Set_c(y.u16[1], y.u16[0]);
			tretw[itret++] = y;
			itret1++;
			continue;
		}
		// now this comes from a set last in cell/region
		int xcell = x.u16[0], xdig = x.u16[1];
		switch (x.u16[2] >> 12){
		case 1:{// last in cell
			int digs = zh_g2.dig_cells[xcell] ^ (1 << xdig);
			while (digs){
				bitscanforward(d2, digs);
				digs ^= 1 << d2;
				if (back_bf.On_c(d2, xcell))	continue;
				back_bf.Set_c(d2, xcell);
				for (int i = 0; i < ntb; i++){// must exist
					GINT64 w = tb[i];
					if (w.u16[0] == xcell && w.u16[1] == d2 && (w.u16[3] & 1))
						tretw[itret++] = w;
				}
			}
			break;
		}
		case 2:{// last in region
			int unit = x.u8[4], ucell;
			BF128 wu = units3xBM[unit]; wu &= zh_g2.pm.pmdig[xdig];
			if (diag){
char ws[82];
cout << wu.String3X(ws) << " wu to backload for last" << endl;
			}
			while ((ucell = wu.getFirstCell()) >= 0) {
				wu.Clear_c(ucell);
				if (back_bf.On_c(xdig, ucell))	continue;
				back_bf.Set_c(xdig, ucell);
				//cout << " look for cell " << cellsFixedData[ucell].pt << endl;
				for (int i = 0; i < ntb; i++) {// must exist
					GINT64 w = tb[i];
					if (w.u16[0] == ucell && w.u16[1] == xdig && (w.u16[3] & 1)) {
						tretw[itret++] = w;
						break;
					}
				}
			}

		}
		}//  end switch
		itret1++;
	}
	// send it back increasing order of step
	int stepl = tretw[0].u16[3];
	int nb = 0;
	for (int ist = 0; ist <= stepl; ist++) {
		for (int j = 0; j < itret; j++) if (tretw[j].u16[3] == ist)
			tback[nb++] = tretw[j];
	}
	nback = nb;
	return nb;
}
int XYSEARCH::BackDynamicOff(GINT target) {
	// locate target in the last step (must be a on to off step
	for (int i = ntd; i < nt; i++) {
		if (t[i].u32[0] == target.u32)
			return BackDynamic(t[i], t, nt);
	}
	return 0;
}
/*
DynamicForcingChain=85,
DynamicForcingChainPlus=90,
NestedForcingChain=95,
NestedLevel3=100,
NestedLevel4=105,
NesttedLevel5=110

*/
//================= SETS_LINKS
/*
struct SETS_LINKS {// attempt to have a new design
	BF128 dig_sets[40], dig_links[40], cells_sets,cells_links;
	uint32_t cells[81];// cell valid digits
	GINT assigned_cands[40];
	uint32_t ndsets,ndlinks,nassigned;
	//dig_set 3X + digit
	void Assign(int cell, int dig, SETS_LINKS o);
};
*/
int SETS_LINKS::Assign(int cell, int digit, SETS_LINKS & o) {
// code assuming sets are cells pm  or digit/unit pm
	*this = o;// copy source
	GINT & a = assigned_cands[nassigned++];
	a.u16[0] = digit;
	a.u16[1] = cell;
	ndsets = 0;
	for (uint32_t i = 0; i < o.ndsets; i++) {
		BF128 w = o.dig_sets[i];
		int dig = w.bf.u32[3] ;
		if (dig==digit && w.On_c(cell))continue;
		dig_sets[ndsets++] = w;
	}
	cells_sets.Clear_c(cell);
	// we have the new status of sets, 

	// this is a solution if no more set to process
	if ((!ndsets) && cells_sets.isEmpty() ){
		return 1;
	}
	
	//clear links effect
	if (cells_links.On_c(cell)) {
		cells_links.Clear_c(cell);
		for (uint32_t i = 0; i < ndsets; i++)
			dig_sets[i].Clear_c(cell);
	}

	// copy untouched digit links, apply touched l
	ndlinks = 0;
	for (uint32_t i = 0; i < o.ndlinks; i++) {
		BF128 w = o.dig_links[i];
		int dig = w.bf.u32[3];
		if (dig == digit && w.On_c(cell)) {
			// clean sets of the digit
			for (uint32_t j= 0; j < ndsets; j++) {
				BF128 w = dig_sets[j];
				int dig = w.bf.u32[3] & 0x1ff;
				if (dig == digit ) 
				dig_sets[j] -= w;
				dig_sets[j].bf.u32[3]= dig;// retore dig destroyed
			}
			// clean cells sets
			BF128 wc = cells_sets & w;// cells touched by the linkset
			int wc_cell;
			while ((wc_cell = wc.getFirstCell() >= 0)) {
				wc.Clear_c(cell);
				cells[wc_cell] &= (~(1 << dig));
			}
		}
		else 	dig_links[ndlinks++] = w;
	}
	return 0;
}
void SETS_LINKS::LoopNextSet() {// select best set and assign
	int tc[50], ntc = cells_sets.Table3X27(tc);
	int minset = 100, set_mode, set_ind;
	for (int i = 0; i < ntc; i++) {// try a cell set
		int dcell = tc[i],nd=_popcnt32(dcell);
		if (nd < minset) {
			minset = nd;
			set_mode = 0;
			set_ind = i;
		}
	}
	for (uint32_t i = 0; i < ndsets; i++) {// try a dig/unit set
		BF128 w = dig_sets[40]; 
		int nc = w.Count96();
		if (nc < minset) {
			minset = nc;
			set_mode = 1;
			set_ind = i;
		}
	}
	if (minset > 10) return; // would be a bug
	if (set_mode) {// it is a dig/unit set
		BF128  w = dig_sets[set_ind];
		int dig = w.bf.u32[3];
 		int tc2[50], ntc2 = w.Table3X27(tc);
		for (int ic = 0; ic < ntc2; ic++) {
			SETS_LINKS sl;
			if (sl.Assign(tc2[ic], dig, *this)) continue;// closed
			sl.LoopNextSet();
		}
	}
	else {// it is a cell set 
		int cell = tc[set_ind], d = cells[cell];
		for (int idig = 0; idig < 9; idig++) if (d & (1 << idig)) {
			SETS_LINKS sl;
			if(sl.Assign(cell, idig, *this)) continue;// closed
			sl.LoopNextSet();
		}
	}
}

int SETS_LINKS::CheckCover() {
	// compare  digit set pm and digit links pm
	for (int idig = 0; idig < 9; idig++) {
		BF128 wd = zhou_solve.FD[idig][0]& cells_sets,wd2=wd;
		for (uint32_t iset = 0; iset < ndsets; iset++) {
			BF128 w = dig_sets[iset];
			if (w.bf.u32[3] != idig)continue;
			w.bf.u32[3] = 0;
			wd2 |= w & wd;
			wd |= w;
		}
		// now digit pm and multiple points in wd2
		BF128 wdl = zhou_solve.FD[idig][0] & cells_links, wd2l = wdl;
		for (uint32_t iset = 0; iset < ndlinks; iset++) {
			BF128 w = dig_links[iset];
			if (w.bf.u32[3] != idig)continue;
			w.bf.u32[3] = 0;
			wd2l |= w & wdl;
			wdl |= w;
		}
		if ((wd - wdl).isNotEmpty() ){// missing cover

		}
	}
}

void TB_MULT_9P::Init(int p9) {// first multi row/col/box status
	memset(this, 0, 3* sizeof rows);
	for (int i = 0; i < 9; i++) {
		int bit = 1 << i;
		if (p9 & bit) {
			rows |= units3xBM[i];
			cols |= units3xBM[i + 9];
			boxes |= units3xBM[i + 18];
		}
	}

}
//==========================


struct VLOOP {      // collecting the sk loop pattern for later action
	struct VBOX {
		int cellsr[2], cellsc[2], digsr, digsc, ndigsr, ndigsc;
		void Debug(int i);
	}tvbox[5];// one more for loop
	BF128 cmap, crows, ccols;
	int box4 ,tbox[9], ntbox ,rows[2],columns[2];
	int seq_digs[9], seq_ndigs[9], seq_com[9],seq_sol[8];// one more for loops
	int dig_cells[81];
	USHORT mainbeltscleared, seen;       // 8 or 13 could be less if virus chain
	int Locate_Loop();
	int FirstAction();
	int SecondAction(int do_it);
	int SecondBilocations();
	void GenNotValidEffect();
}vloop;

void VLOOP::VBOX::Debug(int i) {
	cout << i << " boxdebug ncdigs=" << ndigsr << "," << ndigsc 
	 << oct << " digsr 0" << digsr << " disgc 0" << digsc << dec << endl;
}
int VLOOP::Locate_Loop() {
	// build a full dig_cells 
	memset(dig_cells, 0, sizeof dig_cells);
	for (int idig = 0,bit=1; idig < 9; idig++,bit<<=1) {
		BF128 w = zhou_solve.FD[idig][0];
		w.bf.u32[3] = 0;
		int tc[80], ntc = w.Table3X27(tc);
		for (int i = 0; i < ntc; i++) dig_cells[tc[i]] |= bit;
	}
	// try 2 rows 2 columns as base set (2 bands 2 stacks)
	BF128 cmap = zhou_solve.cells_unsolved;
	for (int ir = 0; ir < 36; ir++) {
		R0SEARCH & rs = pm_go.r0search;
		if (!floors2d_2bands[ir]) continue;
		for (int jc = 0; jc < 36; jc++) {
			if (!floors2d_2bands[jc]) continue;
			BF128 corners = rs.tb9p2[ir].rows&rs.tb9p2[jc].cols;
			// separate analysis if corners are not empty
			if ((corners & cmap).isNotEmpty()) continue;

			crows = rs.tb9p2[ir].rows -corners;
			ccols = rs.tb9p2[jc].cols -corners;
			box4 = rs.fl2d_rboxes[ir] & rs.fl2d_cboxes[jc];
			ntbox = 0;
			BitsInTable32(tbox, ntbox, box4);
			// exchange box index 2;3 to have a "loop" order 1-2-3-4-1...
			int temp = tbox[2];	tbox[2]= tbox[3]; tbox[3]=temp;
			if (sgo.vx[2]) {
				char ws[82];
				cout << corners.String3X(ws) << " empty corners boxes " 
					<<	tbox[0] << tbox[1] << tbox[2] << tbox[3] << endl;
			}
			// collect cells in box, digits in bos check count dig= count cells
			for (int ibox = 0; ibox < 4; ibox++) {
				int box = tbox[ibox] + 18;
				BF128 bbf= units3xBM[box];
				VBOX & vb=tvbox[ibox];
				BF128 cboxr = crows & bbf,cboxc=ccols&bbf;
				if((cboxr&cmap).isEmpty())goto exit_no;// maxi one given
				if ((cboxc&cmap).isEmpty())goto exit_no;// maxi one given
				cboxr.Table3X27(vb.cellsr);
				cboxc.Table3X27(vb.cellsc);
				vb.digsr =vb.digsc = 0;
				int * dc = dig_cells;// use cells inclufing givrn/assigned
				for (int i = 0; i < 2; i++)vb.digsr |= dc[vb.cellsr[i]];
				for (int i = 0; i < 2; i++)vb.digsc |= dc[vb.cellsc[i]];
				vb.ndigsr = _popcnt32(vb.digsr); if (vb.ndigsr > 4)goto exit_no;
				vb.ndigsc = _popcnt32(vb.digsc); if (vb.ndigsc > 4)goto exit_no;
			}
			if (sgo.vx[2])
				for (int i = 0; i < 4; i++) tvbox[i].Debug(i);		

			tvbox[4] = tvbox[0]; // to have always a +1 
			for (int ibox = 0, iseq = 0; ibox < 4; ibox++) {
				VBOX & vb = tvbox[ibox];
				if (ibox & 1) {// row to col
					seq_ndigs[iseq] = vb.ndigsr; seq_digs[iseq++] = vb.digsr;
					seq_ndigs[iseq] = vb.ndigsc; seq_digs[iseq++] = vb.digsc;
				}
				else {// col to row
					seq_ndigs[iseq] = vb.ndigsc; seq_digs[iseq++] = vb.digsc;
					seq_ndigs[iseq] = vb.ndigsr; seq_digs[iseq++] = vb.digsr;
				}
			}
			if (sgo.vx[2]) {
				cout << " digs sequence ";
				for (int i = 0; i < 8; i++)
					cout << oct <<" 0"<< seq_digs[i];
				cout << endl;
			}
			seq_digs[8] = seq_digs[0];
			// check now if eq_digs seq_cells can produce a loop
			int iknown = -1;
			for (int i = 0; i < 8; i++) {
				if (iknown >= 0) {// loop check direct, common forced
					int dcom = seq_digs[i] & ~seq_sol[i - 1];
					if (seq_ndigs[i + 1] == 3) {
						dcom &= seq_digs[i + 1];
						if(!dcom)goto exit_no;
					}
					else if (dcom & ~seq_digs[i + 1]) goto exit_no;
					seq_sol[i] = dcom;
					seq_com[i] = dcom;
				}
				else {
					seq_sol[i] = 0;
					int dcom = seq_digs[i] & seq_digs[i + 1],
						cc = _popcnt32(dcom);
					if (cc < 1) goto exit_no;
					if (seq_ndigs[i] == 4 && seq_ndigs[i + 1] == 4) {
						if (cc < 2)goto exit_no;
						if (cc == 2) {// store and check if ok with past
							seq_sol[i] = dcom;
							iknown = i;
						}
					}
					seq_com[i] = dcom;
				}
			}
			if (iknown < 0) goto exit_no; // should never be ??
			if (sgo.vx[2]) {
				cout << "end comdirect iknown=" << iknown << " com sequence";
				for (int i = 0; i < 8; i++)
					cout << oct << " 0" << seq_com[i]<<dec;
				cout << endl;
				cout << "end direct sol  sequence";
				for (int i = 0; i < 8; i++)
					cout << oct << " 0" << seq_sol[i]<<dec;
				cout << endl;
			}
			if (iknown > 0) {// must solve right to left 
				for (int i = iknown - 1; i >= 0; i--) {
					int dcom = seq_digs[i+1] & ~seq_sol[i + 1];
					if (seq_ndigs[i ] == 3) {
						dcom &= seq_digs[i];
						if (!dcom)goto exit_no;
					}
					else 	if (dcom & ~seq_digs[i]) goto exit_no;
					seq_sol[i] = dcom;
				}
			}
			if (sgo.vx[2]) {
				cout << "final sol  sequence ";
				for (int i = 0; i < 8; i++)
					cout << oct << " 0" << seq_sol[i]<<dec;
				cout << endl;
			}
			int nr = 0, nc = 0,pr= floors_2d[ir],pc= floors_2d[jc];
			for (int i = 0,bit=1; i < 9; i++,bit<<=1) {
				if (pr&bit)rows[nr++] = i;
				if (pc&bit)columns[nc++] = i;
			}
			// store rows cols
			return 1; // stop at first vloop
		exit_no:;
		}
	}
	return 0;
}

//============================ R0Search

// constructor building more tables

R0SEARCH::R0SEARCH() {
	for (int ifl = 0; ifl < 36; ifl++)		tb9p2[ifl].Init(floors_2d[ifl]);
	for (int ifl = 0; ifl < 84; ifl++)		tb9p3[ifl].Init(floors_3d[ifl]);
	for (int ifl = 0; ifl < 126; ifl++)		tb9p4[ifl].Init(floors_4d[ifl]);
	for (int ifl = 0; ifl < 126; ifl++)		tb9p5[ifl].Init(0x1ff^floors_4d[ifl]);
	// init row and columns bos hits 
	for (int ifl = 0; ifl < 36; ifl++) {
		register int r = floors_2d[ifl], dr = 0, dc = 0;
		if (r & 7) { dr |= 7; dc |= 0111; }
		if (r & 070) { dr |= 070; dc |= 0222; }
		if (r & 0700) { dr |= 0700; dc |= 0444; }
		fl2d_rboxes[ifl] = dr;
		fl2d_cboxes[ifl] = dc;
	}

}
void R0SEARCH::CollectPerms(int pr) {
	BF128 cells_unsolved = zhou_solve.cells_unsolved;
	memset(nperms, 0, sizeof nperms);
	for (int idig = 0; idig < 9; idig++) {
		BF128 wd = zhou_solve.FD[idig][0];
		if ((wd&cells_unsolved).isEmpty())continue;
		nperms[idig] = zh1d_g.Go(wd, tperms_digits[idig]);
	}
	if (pr) {
		cout << "n perms per digit \t";
		for (int i = 0; i < 9; i++) cout << nperms[i] << " ";
		cout << endl;
	}
	if (pr > 1) {
		char ws[82];
		cout << " table paires par digit" << endl;
		for (int i = 0; i < 9; i++) {
			cout << "digit " << i + 1 << endl;
			BF128 * t = tperms_digits[i];
			for (uint32_t j = 0; j < nperms[i]; j++) {
				cout << t[j].String3X(ws) << endl;
			}
		}
	}

}
int MULTISEARCH::Shrink(int rd2, BF128 *td2, uint32_t ntd2) {
	int n = 0;
	for (uint32_t i = 0; i < ntd2; i++) {
		if ((mask&td2[i]).isNotEmpty()) continue;
		tperms[rd2][n++] = td2[i];
	}
	nperms[rd2] = n;
	return n;
}
void MULTISEARCH::GetDigitPm(int digit, BF128 & pmout, BF128 & pmsolved) {
	pmout.SetAll_0(); // debugging false digit
		for (uint32_t i = 0; i < ind; i++) {
		if (digssols[i] == digit) {
			pmout = sol[i];
			pmsolved |= pmout;
			return;
		}
	}
	for (uint32_t i = 0; i < ntsd; i++) {
		if (tsd[i].u8[1] != digit) continue;
		int np = nperms[i];
		if (!np) return; // empty 
		BF128 * wp = tperms[i];
		pmout = wp[0];
		for (int j = 0; j < np; j++)	pmout |= wp[j];
		return;
	}
}
int R0SEARCH::IsElims(uint32_t floorse, int mode) {
	floors = floorse;
	cumsols.SetAll_0();	elims.SetAll_0();
	infield.SetAll_0();	outfield.SetAll_0();
	BF128 uns= zhou_solve.cells_unsolved;
	ntin = 0;
	for (int i = 0; i < 9; i++) {
		int bit = 1 << i;
		if (floors & bit) {
			tfloors[ntin] = i;
			BF128 w= zh_g2.pm.pmdig[i];
			pmfloors[ntin] = w;
			infield |= w;
			GINT & t = tin[ntin++];
			t.u16[0] = i;
			t.u16[1] = nperms[i];
			if (!nperms[i]) return 0;// one digit solved
		}
		else outfield |= zh_g2.pm.pmdig[i];
	}
	ndigits = ntin;
	lastindex = ntin - 1;
	infield &= uns;
	infield_and_sols = infield;
	outfield &= infield;// infield with extra digits
	in_no_out = infield - outfield;
	Sort_uint32_t(&tin[0].u32, ntin);
	// Build the first subtable
	MULTISEARCH ms;
	int dig = tin[0].u16[0], np = tin[0].u16[1];
	BF128 * tp = tperms_digits[dig];
	//cout << " start with digit " << dig + 1 << " nperms=" << np << endl;
	for (int ip = 0; ip <  np; ip++) {
		//cout << "index 0 perm ip=" << ip << endl;
		BF128 wp = tp[ip];// perm to apply for the first digit
		ms.AddMask(wp, 0,dig);
		ms.ntsd = 0;
		for (uint32_t id2 = 1; id2 < ntin; id2++) {
			int dig2 = tin[id2].u16[0],
				nn = ms.Shrink(id2 - 1, tperms_digits[dig2], nperms[dig2]);
			//cout << "id2=" << id2 << " nn=" << nn << endl;
			ms.Add(dig2, nn);
		}
		if(lastindex==1)IsNextLast(ms, 1);
		else IsNext(ms, 1);
		//if (1) break;
	}
	if (0) {
		char ws[82];
		cout << infield_and_sols.String3X(ws) << " always in sol " << endl;
	}
	for (int i = 0; i < 9; i++) {
		int bit = 1 << i;
		if (floors & bit) 
			elims.pmdig[i]= zh_g2.pm.pmdig[i]-cumsols.pmdig[i] ;
		else elims.pmdig[i] = zhou_solve.FD[i][0] & infield_and_sols;
	}
	int cc= elims.Count();
	if(cc)	{ 
		cout << "active multifloors " << endl;
		DebugFloors();
		cout << "elims" << endl;
		elims.PrintCells();
	}
	if (!mode)return cc;
	// try to find a clearing logic
	if (cc)TryRank0Logic();
	return cc;
}
int R0SEARCH::IsNext(MULTISEARCH & mso, int index) {
	MULTISEARCH ms;
	ms.Init(mso);
	Sort_uint32_t(&mso.tsd[0].u32, mso.ntsd);
	GINT w = mso.tsd[0];// lowest number of perms
	int dig = w.u8[1], rdig = w.u8[0], np = w.u16[1];
	BF128 * tp = mso.tperms[rdig];
	//cout << "index " << index << " continue with digit " << dig + 1 << " nperms=" << np << " index="<<index  << endl;
	for (int ip = 0; ip < np; ip++) {
		//cout << "index " << index << "perm ip=" << ip << endl;
		BF128 wp = tp[ip];// perm to apply for the first digit
		ms.AddMask(wp, index,dig);
		ms.ntsd = 0;
		for (uint32_t id2 = 1; id2 < mso.ntsd; id2++) {
			int rdig2 = mso.tsd[id2].u8[0],
				nn = ms.Shrink(id2 - 1, mso.tperms[rdig2], mso.nperms[rdig2]);
			//cout << "id2=" << id2 << " nn=" << nn << " digit="<< mso.tsd[id2].u8[1]+1 << endl;
			ms.Add(mso.tsd[id2].u8[1], nn);
		}
		//if(index==2)DebugFloors2(ms);
		uint32_t nindex = index + 1;
		if (nindex <lastindex) IsNext(ms, nindex);
		else IsNextLast(ms, nindex);
	}
	return 0;
}
int R0SEARCH::IsNextLast(MULTISEARCH & mso, int index) {
	GINT w = mso.tsd[0];// lowest number of perms
	int dig = w.u8[1], rdig = w.u8[0], np = w.u16[1];
	BF128 * tp = mso.tperms[rdig],wor;
	//cout<<"index "<<index << " continue last  digit " << dig + 1 << " nperms=" << np << " index=" << index << endl;
	mso.OrOlds(wor);// logical or of previous digits 

	for (int ip = 0; ip < np; ip++) {
		//cout << "last perm ip=" << ip << endl;
		BF128 wp = tp[ip];// perm to apply for the first digit
		BF128 wor2 = wor | wp;
		if ((in_no_out - wor2).isNotEmpty()) 
			continue;// empty cell
		// this is a valid set of digit solutions

		infield_and_sols &= wor2;// needed to clear outfield

		for (uint32_t i = 0; i < mso.ind; i++) {
			int digi = mso.digssols[i];
			cumsols.pmdig[digi] |= mso.sol[i];
		}
		cumsols.pmdig[dig] |= wp;

		if (0) {
			char ws[82];
			BF128 orsol = wp;
			for (uint32_t i = 0; i < mso.ind; i++) {
				int digi = mso.digssols[i];
				//cout << mso.sol[i].String3X(ws) << " dig " << digi + 1 << endl;
				orsol |= mso.sol[i];
			}
			orsol &= infield;
			cout <<orsol.String3X(ws) << " orsol"  << endl;
		}
	}
	return 0;
}

void R0SEARCH::DebugFloors() {
	cout << "PM map " << endl << endl;
	int dig_cells[81];
	memset(dig_cells, 0, sizeof dig_cells);
	for (int idig = 0; idig < 9; idig++) {
		int bit = 1 << idig;
		if (!(floors & bit)) continue;
		BF128 wd = zhou_solve.FD[idig][0];
		for (int i = 0; i < 81; i++) {
			if (wd.Off_c(i))continue;
			dig_cells[i]|=bit;
		}
	}
	int i, j, l, lcol[9], tcol = 0;
	for (i = 0; i < 9; i++) {  // attention ici i indice colonne
		lcol[i] = 2;    // 2  mini tous chiffres impos�s
		for (j = 0; j < 9; j++) {
			l = _popcnt32(dig_cells[9 * j + i])+1;// room for '+'
			if (l > lcol[i])       lcol[i] = l;
		}
		tcol += lcol[i];
	}
	for (i = 0; i < 9; i++) { // maintenant indice ligne
		if ((i == 3) || (i == 6)) {
			for (int ix = 0; ix < (tcol + 10); ix++)       cout << (char)'-';
			cout << endl;
		}
		for (j = 0; j < 9; j++) {
			if ((j == 3) || (j == 6))cout << "|";
			int cell = 9 * i + j, digs = dig_cells[cell], ndigs = _popcnt32(digs), plus = 0;
			if (outfield.On_c(cell)) { plus = 1; ndigs++; }
			if (ndigs) {
				for (int id = 0; id < 9; id++)if (digs & (1 << id))
					cout << id + 1;
				if (plus) cout << "+"; 
			}
			else {		cout << "-"; ndigs = 1;		}
			cout << Blancs(lcol[j] + 1 - ndigs, 1);
		} // end for j
		cout << endl;
	} // end for i
	cout << endl;

}
void R0SEARCH::DebugFloors2(MULTISEARCH & ms) {
	cout << "PM map  from multi search " << endl << endl;
	int dig_cells[81];
	memset(dig_cells, 0, sizeof dig_cells);
	BF128 wdsolved; wdsolved.SetAll_0();
	for (int idig = 0; idig < 9; idig++) {
		int bit = 1 << idig;
		if (!(floors & bit)) continue;
		BF128 wd;
		ms.GetDigitPm(idig, wd,wdsolved);
		for (int i = 0; i < 81; i++) {
			if (wd.Off_c(i))continue;
			dig_cells[i] |= bit;
		}
	}
	int i, j, l, lcol[9], tcol = 0;
	for (i = 0; i < 9; i++) {  // attention ici i indice colonne
		lcol[i] = 2;    // 2  mini tous chiffres impos�s
		for (j = 0; j < 9; j++) {
			l = _popcnt32(dig_cells[9 * j + i]) + 1;// room for '+'
			if (l > lcol[i])       lcol[i] = l;
		}
		tcol += lcol[i];
	}
	BF128 nsout = outfield - wdsolved;// forget assigned digits
	for (i = 0; i < 9; i++) { // maintenant indice ligne
		if ((i == 3) || (i == 6)) {
			for (int ix = 0; ix < (tcol + 10); ix++)       cout << (char)'-';
			cout << endl;
		}
		for (j = 0; j < 9; j++) {
			if ((j == 3) || (j == 6))cout << "|";
			int cell = 9 * i + j, digs = dig_cells[cell], ndigs = _popcnt32(digs), plus = 0;
			if (nsout.On_c(cell)) { plus = 1; ndigs++; }
			if (ndigs) {
				for (int id = 0; id < 9; id++)if (digs & (1 << id))
					cout << id + 1;
				if (plus) cout << "+";
			}
			else { cout << "-"; ndigs = 1; }
			cout << Blancs(lcol[j] + 1 - ndigs, 1);
		} // end for j
		cout << endl;
	} // end for i
	cout << endl;

}
//=========== truth/links search in R0search
void R0SEARCH::BuildRelativetables() {
	memset(dig_cells, 0, sizeof dig_cells);
	memset(cells_count, 0, sizeof cells_count);
	for (int icell = 0; icell < 81; icell++) {
		if (infield.Off_c(icell))continue;
		uint32_t d = 0,w = zh_g2.dig_cells[icell];
		for (uint32_t i = 0; i < ndigits; i++) {
			int bit = 1 << tfloors[i];
			if (w&bit)d |= 1 << i;
		}
		dig_cells[icell] = d;
		cells_count[icell] = _popcnt32(d);
	}
	memset(unit_sets_count, 0, sizeof unit_sets_count);
	for (int iu = 0; iu < 27; iu++) {
		BF128 w = units3xBM[iu];
		for (uint32_t id = 0; id < ndigits; id++) {
			int dig = tfloors[id];
			if ((w&pmfloors[id]).isNotEmpty())
				unit_sets_count[iu]++;
		}
	}
	if (0) {
		cout << " check units count" << endl;
		for (int iu = 0; iu < 27; iu++)
			cout << unit_sets_count[iu] << " ";
		cout <<endl;
	}
}
int R0SEARCH::CROSS::Build(R0SEARCH &r, int i) {
	iu = i;
	cross = r.infield & units3xBM[iu];
	crosstruth = cross & r.truths_basis_pm;
	cross_more = (cross - crosstruth) & r.in_no_out;
	if (crosstruth.isEmpty()) return 0;
	ncrosstruth = crosstruth.Count();
	nlsets = r.unit_sets_count[iu];
	ntcross = crosstruth.Table3X27(tcross);
	ntmore = cross_more.Table3X27(tmore);
	digs_cross = ntmore2 = 0;
	for (int i = 0; i < ntcross; i++)
		digs_cross |= r.dig_cells[tcross[i]];
	for (int i = 0; i < ntmore; i++) {
		if (r.dig_cells[tmore[i]] & ~digs_cross) continue;
		tmore2[ntmore2++] = tmore[i];
	}
	min_links = nlsets - 2* ntmore2;// dummy links-sets
	if (ntcross < min_links) min_links = ntcross;
	return min_links;
}

void R0SEARCH::CROSS::BuildElims(R0SEARCH &r, PM3X & elims) {
	BF128 linkcells;
	if (min_links == nlsets - 2*ntmore2) {
		BF128 w = cross - crosstruth;
		if (ntmore2) {
			for (int i = 0; i < ntmore2; i++)
				w.Clear_c(tmore2[i]);
		}
		if(w.isNotEmpty())
			for (uint32_t i = 0; i < r.ndigits; i++) {
				int digit = r.tfloors[i];
				elims.pmdig[digit] |= zhou_solve.FD[digit][0] & w;
			}
	}
	if (min_links != ntcross) return;
	BF128 w= crosstruth - r.in_no_out;
	if (w.isEmpty()) return;
	int cell;
	while ((cell = w.getFirstCell()) >= 0) {
		w.Clear_c(cell);
		int digs = zh_g2.dig_cells[cell]& ~r.floors;
		for (int i = 0, bit = 1; i < 9; i++, bit <<= 1)
			if (digs & bit)
				elims.pmdig[i].Set_c(cell);
	}
}
void R0SEARCH::CROSS::PrintLinks(R0SEARCH &r) {
	BF128 linkcells;
	if (min_links == nlsets - 2 * ntmore2) {
		BF128 w = cross;
		if (ntmore2) {
			for (int i = 0; i < ntmore2; i++)
				w.Clear_c(tmore2[i]);
		}
		if (w.isNotEmpty()) {
			for (uint32_t i = 0; i < r.ndigits; i++) {
				int digit = r.tfloors[i];
				if((w&r.pmfloors[i]).isNotEmpty())
					cout << digit + 1;
			}
			if (iu > 8) 	cout << "C" << iu - 8 << " ";
			else cout << "R" << iu + 1 << " ";
		}
	}
	if (min_links != ntcross) return;
	BF128 w = crosstruth ;
	if (w.isEmpty()) return;
	int cell;
	while ((cell = w.getFirstCell()) >= 0) {
		w.Clear_c(cell);
		cout << cellsFixedData[cell].pt << " ";
	}
	
}
int R0SEARCH::Try_RC(TB_MULT_9P &t9p, int p9, int mode) {
	cout << "entry rc mode " << mode << " perm 0" << oct << p9 << dec << endl;
	
	truths_basis_pm = infield;
	truths_basis_pm &= (mode) ? t9p.cols : t9p.rows;
	ntruth = nlinks = 0;// = ntu_links = ntu_sets = 0;
	{	
		int iu = (mode) ? 9 : 0, iuf = iu + 9;
		for (int bit = 1; iu < iuf; iu++, bit <<= 1)
			if (p9 & bit) ntruth += unit_sets_count[iu];
	}

	if (1) {
		char ws[82];
		cout << "ntruth=" << ntruth << endl;
		cout << truths_basis_pm.String3X(ws) << " truths cells" << endl;
	}
	int iu = (mode) ? 0 : 9,iuf=iu+9; 
	CROSS tcr[9];
	int ntcr = 0;
	for (; iu < iuf; iu++) {// crossing units
		CROSS w;
		int ir=w.Build(*this, iu);
		if (ir) { // crossing unit active 
			nlinks += ir;
			tcr[ntcr++] = w;
		}
	}
	if (nlinks > ntruth) return 0;
	cout << "rank 0 logic expected dummy ntruth/nlinks " 
		<< ntruth << "/" << nlinks << endl;
	PM3X e; e.SetAll_0();
	for (int itcr = 0; itcr < ntcr; itcr++)
		tcr[itcr].BuildElims(*this, e);
	if (e.IsEmpty()) return 0;
	cout << "elim" << endl;
	e.PrintCells();
	cout << "truths links status" << endl;
	cout << "truths={ ";
	{ // standard truths
		int iu = (mode) ? 9 : 0, iuf = iu + 9;
		for (int bit = 1; iu < iuf; iu++, bit <<= 1)
			if (p9 & bit) {
				BF128 w = units3xBM[iu];
				for (uint32_t id = 0; id < ndigits; id++) {
					int dig = tfloors[id];
					if ((w&pmfloors[id]).isNotEmpty())
						cout << dig + 1;
				}
				if (mode) cout <<"C"<<iu-8<< " ";
				else cout << "R" << iu+1  << " ";
			}
	}
	{// added truths in linksets
		for (int i = 0; i < ntcr; i++) if (tcr[i].ntmore2) {
			for (int j = 0; j < tcr[i].ntmore2; j++)
				cout << cellsFixedData[tcr[i].tmore2[j]].pt << " ";
		}
	}
	cout << "}" << endl;
	cout << "links={ ";
	for (int i = 0; i < ntcr; i++)	tcr[i].PrintLinks(*this);
	cout << "}" << endl;
	return 1;
}
int R0SEARCH::TryRank0Logic() {
	cout << "try rank 0 logic " << endl;
	BuildRelativetables();
	for (int i = 0; i < 84; i++) {// try 3 rows or 3 colums as base sets
		int ir = Try_RC(tb9p3[i], floors_3d[i], 0);
		if (ir) return 1;
		ir = Try_RC(tb9p3[i], floors_3d[i], 1);
		if (ir) return 1;
	}
	for (int i = 0; i < 126; i++) {// try 4 rows or 4 colums as base sets
		if (floors_4d[i] != 0132)continue;
		int ir = Try_RC(tb9p4[i], floors_4d[i], 0);
		if (ir) return 1;
		ir = Try_RC(tb9p4[i], floors_4d[i], 1);
		if (ir) return 1;
	}
	// try 2 rows 2 columns as base set (2 bands 2 stacks)
	for (int i = 0; i < 36; i++) {
		if (!floors2d_2bands[i]) continue;
		for (int j = 0; j < 36; j++) {
			if (!floors2d_2bands[j]) continue;
		}
	}

	for (int i = 0; i < 126; i++) {// try 5 rows or 5 colums as base sets
		int ir = Try_RC(tb9p5[i],0x1ff^ floors_4d[i], 0);
		if (ir) return 1;
		ir = Try_RC(tb9p5[i], 0x1ff ^ floors_4d[i], 1);
		if (ir) return 1;
	}

	return 0;
}


	/*



	// then try to add a set in extra rows when it is possible
	// risk of overlapping rows col box
	// no cell truth  in the set
//	parent->ImagePoints(cells_sets); EE.Enl (" cells setss before adds");
	for(int j=0;j<9;j++) if(floors.On(j)){// for floor digits
		// create the link group for that digit
		for(int i=0;i<9;i++){
			if(rcbm.On(i)) continue; //only extra rows or columns
			BF81 wset(cellsInHouseBM[i+dtruth],parent->c[j]);
			if(wset.IsEmpty()) continue;
			if((wset & cells_sets).IsNotEmpty()) continue; // no cell added
			wset-= link_candidates.bfc[j];
			if(wset.IsNotEmpty()) continue; // must cover all
			added_truths.SetRegion(j,i+dtruth);
//			EE.E(" added  dig ");EE.E(j+1); EE.E(" unit ");EE.Enl(i+dtruth+1);
			ntruth++;
		}
	}

	rank=nlinks-ntruth;
	if(rank>1) return 0;
	if(rank<1 && (opt & 2)) return 0;
	if(rank>0 && (opt & 1)) return 0;
	// check for elims can be rank 0 or rank 1
	return CheckElims();
}
*/
//============================== R0SEARCH


/*
int PM_GO::R0SEARCH::TryRowColBand(USHORT * tu,USHORT ntu,USHORT dtruth){
	PM_DATA & myd=parent->zpmd[0];

	int dlink=9-dtruth,ib0=CheckBand(tu,ntu);
	if(!ib0) return 0; else ib0--;
	BF81 wband0=cellsInBandBM[(dtruth)? ib0+3 : ib0];

	ntruth=nlinks=ntu_links=ntu_sets=0;
	cells_sets.SetAll_0();
	cellstruth=cells_link=cells_sets;
	// build cellstruths
	for(int i=0;i<ntu;i++){
		int itr=dtruth+tu[i],ntr=unit_sets[itr];
		tu_sets[ntu_sets++]=itr;
		if(!ntr) return 0; //no empty set
		cellstruth |=cellsInHouseBM[itr];
		ntruth+=ntr;
	}
	cellstruth &= cellsf;
	if(ntruth<6) return 0;
	wband0 &= cellstruth;
	// now cross rows or columns band by band
	for(int ib=0;ib<3;ib++){// three lots of 3 row or columns
		BF81 cells_sets3,cells_link3;
		USHORT nlinks3=0,ntruth3=0,tu_links3[3],ntu_links3=0;
		for(int i=0,il=dlink+3*ib;i<3;i++,il++){
			BF81 ww=cellsInHouseBM[il],wwp=ww;
			BF16 digits,digitsp;
			ww&= cellstruth;
			wwp&=cellspuref-ww;
			USHORT tp[10],ntp=ww.String(tp),tpp[10],ntpp=wwp.String(tpp);
			if(!ntp) continue;
			for(int ip=0;ip<ntp;ip++)
				digits |= myd.dig_cells[tp[ip]];
			digits &=floors;
			for(int ip=0;ip<ntpp;ip++)
				digitsp |= myd.dig_cells[tpp[ip]];
			digitsp &=floors;
			digitsp |=digits; // can not be used without cross links
			int nd=digits.bitCount(),ndp=digitsp.bitCount(),
				nlinksp= ndp-ntpp,nlinksmin=(nd<= nlinksp)? nd:nlinksp;
			if(ntp<=nlinksmin){
				nlinks3+=ntp;
				cells_link3|=ww;
			}
			else{
				nlinks3+= nd;
				tu_links3[ntu_links3++]=il;
				if(nlinksp<nd){
					nlinks3+=(ndp-nd);
					ntruth3+=ntpp;
					cells_sets3|=wwp;
				}
			}
		}
		// compare to a box cover + cells
		BF81 wband=cellsInBandBM[(dlink)? ib+3:ib],wband1;
		wband &= cellstruth; // cells in cross band belonging to the map of truths
		wband1=wband & wband0; // cells in box belonging to the band
		wband -= wband1; // now cells outside the box
		USHORT tp1[10],ntp1=wband1.String(tp1); // cells in box to find digits
		BF16 dig_box;
		for(int i=0;i<ntp1;i++)
			dig_box |= myd.dig_cells[tp1[i]];
		int n_with_box=dig_box.bitCount()+wband.Count();
		if(n_with_box<(nlinks3-ntruth3)){ // then take the box
			nlinks+=n_with_box;
			tu_links[ntu_links++] = (dtruth)? 3*ib0 + ib + 18 :3*ib+ib0+18; // add the box
			cells_link |= wband; // add cells outside the box
		}
		else{ // apply the 3 rows or columns
			nlinks+= nlinks3;
			ntruth+= ntruth3;
			cells_sets |= cells_sets3;
			cells_link |= cells_link3;
			for(int i=0;i<ntu_links3;i++)
				tu_links[ntu_links++]=tu_links3[i];
		}


		if(nlinks>(ntruth+1))
			break; // stop as soon as possible keep open rank 1
	}
	rank=nlinks-ntruth;
	if(rank>1) return 0;
	if(rank<1 && (opt & 2)) return 0;
	if(rank>0 && (opt & 1)) return 0;
	// check for elims can be rank 0 or rank 1
	return CheckElims();
}
*/

int R0SEARCH::GoX(int pr, int pc) {
	truths_basis_pm = infield;
	truths_basis_pm &= (tb9p2[pc].cols | tb9p2[pr].rows);
	BF128 corners = tb9p2[pc].cols&tb9p2[pr].rows;
	// separate analysis if corners are not empty
	if ((corners & truths_basis_pm).isNotEmpty()) return 0;
	ntruth = nlinks = 0;// = ntu_links = ntu_sets = 0;
	int vu = floors_2d[pr] | (floors_2d[pc]<<9);
	for (int iu = 0, bit = 1; iu < 18; iu++, bit <<= 1)	if (vu & bit) {
		if (!unit_sets_count[iu]) return 0;
		ntruth += unit_sets_count[iu];
	}
	int crossing_rows = 0x1ff ^ fl2d_rboxes[pr];
	int crossing_cols = 0x1ff ^ fl2d_rboxes[pc];// here in 0-8 mode
	// now 4 boxes, 3 rows, 3 cols for linksets
	int icross_band= floors2d_band_stack_free[pr],
		icross_stack=3+ floors2d_band_stack_free[pc];
	BF128 cross_band = infield & band3xBM[icross_band],
		cross_stack = infield & band3xBM[icross_stack],
		cross2 = cross_band & cross_stack;
	// cross2 potential eliminations rank 1
	if ((cross2&in_no_out).isNotEmpty()) return 0;// not here,triple points
	// now all potential cells to add are in areas with no double cover.
	int cross_u_bf = crossing_rows | (crossing_cols << 9)
		| ((fl2d_rboxes[pr] & fl2d_cboxes[pc])<<18);
	CROSS tcr[10];
	int ntcr = 0;
	for (int iu = 0, bit = 1; iu < 27; iu++, bit <<= 1) {
		if (!cross_u_bf&bit)continue;
		CROSS w;
		int ir = w.Build(*this, iu);
		if (ir) { // crossing unit active
			nlinks += ir;
			tcr[ntcr++] = w;
		}
	}
	int rank = nlinks - ntruth;
	if (rank > 1) return 0;
	if (rank == 1) {
		if (cross2.isEmpty())return 1;
	}
	cout << "rank 0 logic expected dummy ntruth/nlinks "
		<< ntruth << "/" << nlinks << endl;
	PM3X e; e.SetAll_0();
	for (int itcr = 0; itcr < ntcr; itcr++)
		tcr[itcr].BuildElims(*this, e);
	if (rank == 1) {
		for (int idig = 0; idig < 9; idig++)
			e.pmdig[idig] &= cross2;
	}
	if (e.IsEmpty()) return 0;
	cout << "elim" << endl;
	e.PrintCells();
	cout << "truths links status" << endl;
	cout << "truths={ ";
	{ // standard truths
		for (int iu = 0, bit = 1; iu < 18; iu++, bit <<= 1)	if (vu & bit) {
			BF128 w = units3xBM[iu];
			for (uint32_t id = 0; id < ndigits; id++) {
				int dig = tfloors[id];
				if ((w&pmfloors[id]).isNotEmpty())
					cout << dig + 1;
			}
			if (iu>8) cout << "C" << iu - 8 << " ";
			else cout << "R" << iu + 1 << " ";
		}
		
	}

	{// added truths in linksets
		for (int i = 0; i < ntcr; i++) if (tcr[i].ntmore2) {
			for (int j = 0; j < tcr[i].ntmore2; j++)
				cout << cellsFixedData[tcr[i].tmore2[j]].pt << " ";
		}
	}
	cout << "}" << endl;
	cout << "links={ ";
	for (int i = 0; i < ntcr; i++)	tcr[i].PrintLinks(*this);
	cout << "}" << endl;


	return 0;
}
/* 







void PM_GO::R0SEARCH::Add(BFSETS & ss,USHORT unit){
	PM_DATA & myd=parent->zpmd[0];
	for(int i=0;i<9;i++) if(floors.On(i) &&  myd.dig_reg[i][unit].f)
			ss.SetRegion(i,unit);
}
USHORT r0boxes_table[9][4]= // 4 boxes in square
	  {{0,1,4,3},     {0,1,7,6},	     {3,4,7,6},
	  {0,2,5,3},      {0,2,8,6},	     {3,5,8,6},
	  {1,2,5,4},      {1,2,8,7},		 {4,5,8,7}};
USHORT r0t4r[3][6]=  {{0,1,2,3,4,5} , {0,1,2,6,7,8} ,{3,4,5,6,7,8}};
USHORT r0t4c[3][6]=  {{9,10,11,12,13,14} , {9,10,11,15,16,17} ,{12,13,14,15,16,17}};
int PM_GO::R0SEARCH::GoForBox4(){
	PM_DATA & myd=parent->zpmd[0];
	for(int i4=0;i4<9;i4++){
//		USHORT tu[4];  tu[0]=row1;tu[1]=row2;tu[2]=col1+9;tu[3]=col2+9;
		ntruth=nlinks=ntu_links=ntu_sets=0;
		cells_sets.SetAll_0();
		cellstruth=cells_link=cells_sets;
		USHORT * t4b=r0boxes_table[i4],
			   * t4r=r0t4r [i4%3],
			   * t4c=r0t4c [i4/3],
				aigok=1;
		// build boxes truths
		for(int j=0;j<4;j++){ // boxes as sets
			int jb=t4b[j]+18;
			BF81 wel(cellsInHouseBM[jb]);
			wel &= cellsf;
			for(int dg=0;dg<9;dg++) if(floors.On(dg)){
//				if((wel & parent->c[dg]).IsNotEmpty())
//					tu_sets[ntu_sets++]=jb;
			}
			// and linkset if possible
//			for(int ip=0;ip<9;ip++){
//				int i81=cellsInGroup[jb][ip];
//				if( gsv->m_cells_biv.On(i81) || gsv->m_and.On(i81))
//					AddLinkCell(i81);
//			}
		}
	}
	return 0;
}

void PM_GO::R0SEARCH::GoForCellsPrepare(){
	PM_DATA & myd=parent->zpmd[0];
	for(int i=0,j=indtcomb9[2];i<84+126;i++,j++){
		COMB9 & t9= tcomb9[j][0]; // 3 or 4 rows cols
		for(int k=0;k<2;k++){
			R0S_LOT4 & lx=rclot[i][k];
			lx.c81.SetAll_0();
			lx.nrc4=t9.nt;
			lx.rcbm.f=0;
			for(int k2=0;k2<t9.nt;k2++){
				USHORT ww=t9.t[k2];
				lx.rc4[k2]=ww;
				lx.rcbm.Set(ww);
				if(k) ww+=9;
				lx.c81 |=cellsInHouseBM[ww]; // row or column
			}
			lx.c81 &=myd.unknown;
		}
	}
	// for digits shared 2/7 to 4/5 prepare the BF 81
	myd.PrepareSubGrids();

}
*/
/*
int PM_GO::R0SEARCH::GoForCells63(){
	PM_DATA & myd=parent->zpmd[0];
	GoForCellsPrepare();
	diag=0;
	for(int i=0;i<84;i++){ // 3x3
		R0S_LOT4 & lr=rclot[i][0];
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=0;j<84;j++){
			R0S_LOT4 & lc=rclot[j][1];
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<9) continue;
			if(GoForCells(lr,lc)) return 1;
		}
	}
	for(int i=0;i<84;i++){// 3 rows 4 cols
		R0S_LOT4 & lr=rclot[i][0];
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=84;j<84+126;j++){
			R0S_LOT4 & lc=rclot[j][1];
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<12) continue;
			if(GoForCells(lr,lc)) return 1;
		}
	}
	for(int i=84;i<84+126;i++){ // 3 cols 4 rows
		R0S_LOT4 & lr=rclot[i][0];
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=0;j<84;j++){
			R0S_LOT4 & lc=rclot[j][1];
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<12) continue;
			if(GoForCells(lr,lc)) return 1;
		}
	}
	for(int i=84;i<84+126;i++){
		R0S_LOT4 & lr=rclot[i][0] ;
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=84;j<84+126;j++){ // 4x4 and 4x5
			R0S_LOT4 & lc=rclot[j][1];
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<16) continue;
			if(GoForCells(lr,lc)) return 1;
		}
	}

//	EE.Enl("go 4x5");

	for(int i=84;i<84+126;i++){
		R0S_LOT4 lr=rclot[i][0],lrr=lr;
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=84;j<84+126;j++){ // 4x4 and 4x5
			R0S_LOT4 lc=rclot[j][1],lcr=lc;
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<16) continue;
			// now build 4x5 and 5x4 fromd existing 4x4
			int lastrow=lr.rc4[3],lastcol=lc.rc4[3];
			lrr.nrc4=lcr.nrc4=5;
			for(int irow=lastrow+1;irow<9;irow++){// add a row
				lr=lrr; // start status
				lr.rc4[4]=irow;
				lr.rcbm.Set(irow);
				lr.c81 |= cellsInHouseBM[irow];
				lr.c81 &=myd.unknown;
				cells_base=lr.c81 &lc.c81;
				if(cells_base.Count()<20) continue;
				if(GoForCells(lr,lc)) return 1;
			}
			lr=lrr; lr.nrc4=4; // restore lr at start
			for(int icol=lastcol+1;icol<9;icol++){// add a row
				lc=lcr; // start status
				lc.rc4[4]=icol;
				lc.rcbm.Set(icol);
				lc.c81 |= cellsInHouseBM[icol+9];
				lc.c81 &=myd.unknown;
				cells_base=lr.c81 & lc.c81;
				if(cells_base.Count()<20) continue;
				if(GoForCells(lr,lc)) return 1;
			}
		}
	}
	return 0;
}
*/
/*
int PM_GO::R0SEARCH::GoForCells(){
	PM_DATA & myd=parent->zpmd[0];
	GoForCellsPrepare();
	diag=0;
	for(int i=0;i<84;i++){ // 3x3
		R0S_LOT4 & lr=rclot[i][0];
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=0;j<84;j++){
			R0S_LOT4 & lc=rclot[j][1];
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<7) continue;
			if(GoForCells(lr,lc)) return 1;
		}
	}
	for(int i=0;i<84;i++){// 3 rows 4 cols
		R0S_LOT4 & lr=rclot[i][0];
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=84;j<84+126;j++){
			R0S_LOT4 & lc=rclot[j][1];
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<10) continue;
			if(GoForCells(lr,lc)) return 1;
		}
	}
	for(int i=84;i<84+126;i++){ // 3 cols 4 rows
		R0S_LOT4 & lr=rclot[i][0];
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=0;j<84;j++){
			R0S_LOT4 & lc=rclot[j][1];
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<10) continue;
			if(GoForCells(lr,lc)) return 1;
		}
	}
	for(int i=84;i<84+126;i++){
		R0S_LOT4 & lr=rclot[i][0] ;
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=84;j<84+126;j++){ // 4x4 and 4x5
			R0S_LOT4 & lc=rclot[j][1];
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<14) continue;
			if(GoForCells(lr,lc)) return 1;
		}
	}

//	EE.Enl("go 4x5");

	for(int i=84;i<84+126;i++){
		R0S_LOT4 lr=rclot[i][0],lrr=lr;
		if((lr.rcbm & myd.emptyrows).f) continue;
		for(int j=84;j<84+126;j++){ // 4x4 and 4x5
			R0S_LOT4 lc=rclot[j][1],lcr=lc;
			cells_base=lr.c81 &lc.c81;
			if(cells_base.Count()<14) continue;
			// now build 4x5 and 5x4 fromd existing 4x4
			int lastrow=lr.rc4[3],lastcol=lc.rc4[3];
			lrr.nrc4=lcr.nrc4=5;
			for(int irow=lastrow+1;irow<9;irow++){// add a row
				lr=lrr; // start status
				lr.rc4[4]=irow;
				lr.rcbm.Set(irow);
				lr.c81 |= cellsInHouseBM[irow];
				lr.c81 &=myd.unknown;
				cells_base=lr.c81 &lc.c81;
				if(cells_base.Count()<18) continue;
				if(GoForCells(lr,lc)) return 1;
			}
			lr=lrr; lr.nrc4=4; // restore lr at start
			for(int icol=lastcol+1;icol<9;icol++){// add a row
				lc=lcr; // start status
				lc.rc4[4]=icol;
				lc.rcbm.Set(icol);
				lc.c81 |= cellsInHouseBM[icol+9];
				lc.c81 &=myd.unknown;
				cells_base=lr.c81 & lc.c81;
				if(cells_base.Count()<18) continue;
				if(GoForCells(lr,lc)) return 1;
			}
		}
	}
	return 0;
}

int PM_GO::R0SEARCH::GoForCells(R0S_LOT4 & lr,R0S_LOT4 & lc){
	PM_DATA & myd=parent->zpmd[0];
	BF16 *dcells=myd.dig_cells,ordig,ordigmissing;
	BF81 rbase=cells_base;
	USHORT tp[20];
	ntruths_cells=cells_base.String(tp);
	USHORT rtruths=ntruths_cells;
	for(int i=0;i<ntruths_cells;i++){// find possible digits
		ordig |= dcells[tp[i]];
	}
	ordigmissing.f=0x1ff ^ ordig.f;
	int lim=(ordig.bitCount()>7)?4:3;
	for(int icomb=indtcomb9[1];icomb<indtcomb9[lim];icomb++){// 2/7 3/6 4/5
		COMB9 * tt=tcomb9[icomb];
		if((tt[0].bm & ordigmissing).f) continue; // ignore one digit missing left
		BF16 rightdigs=tt[1].bm & ordig;
		ncovers=0; // try rows + columns
		cells_base=rbase;
		ntruths_cells=rtruths;
		GoForCellsRC(lr,tt[0].bm,0);// row cover
		if(ncovers <ntruths_cells ){
			GoForCellsRC(lc,rightdigs,1);//columns cover
//			EE.E("go2 covers=");EE.Enl(ncovers);
			if(ncovers==ntruths_cells){
//				EE.Enl("check");
				if( CheckElims(ordig,lr,lc)) return 1;
			}
		}

		ncovers=0; // try columns + rows
		cells_base=rbase;
		ntruths_cells=rtruths;
		GoForCellsRC(lc,tt[0].bm,1);// columns cover
		if(ncovers <ntruths_cells ){
			GoForCellsRC(lr,rightdigs,0);//rows cover
			if(ncovers==ntruths_cells){
				if( CheckElims(ordig,lr,lc)) return 1;
			}
		}
	}
return 0;
}

void PM_GO::R0SEARCH::GoForCellsRC(R0S_LOT4 & lx,BF16 digpat,int moderc){
	PM_DATA & myd=parent->zpmd[0];
	BF81 morecells=lx.c81-cells_base;
	USHORT tp[100],ntp=morecells.String(tp);
	for(int i=0;i<ntp;i++) {
		USHORT cell=tp[i];
		BF16 ww=myd.dig_cells[cell]-digpat;
		if(ww.f) morecells.Clear(cell); // not a pure cell of the multifloor
	}

	for(int iband=0;iband<3;iband++){
		BF16 rcpatb=lx.rcbm;
		rcpatb.f &= (7<<(3*iband)); // pattern for rows or columnsband
		if(!rcpatb.f  ) continue; // nothing in the band

		// look in rows or columns to see if it's good to add
		BF81 moreband(cellsInBandBM[iband+(moderc? 3:0)],morecells);
		if(moreband.IsNotEmpty()){
			for(int irc=3*iband;irc<3*(iband+1);irc++) if(rcpatb.On(irc)){// valid row or column
				int rowcol=irc +(moderc? 9:0),ntpmore=0;
				BF81 moreirc(cellsInHouseBM[rowcol],moreband);
				if(moreirc.IsEmpty()) continue;
				BF16 digmore,digirc;
				for(int ip=0;ip<9;ip++){
					int cell=cellsInGroup[irc][ip];
					BF16 & wd=myd.dig_cells[cell];
					if(cells_base.On(cell))
						digirc |=wd;
					else if(moreirc.On(cell)){
						ntpmore++;
						digmore |=wd;
					}
				}				// we forget a limit possible partial action very low probability
				if((digmore-digirc).bitCount()<ntpmore){
					cells_base |= moreirc;
					ntruths_cells +=ntpmore;
				}
			}
		}
		int ibandew=(moderc)? iband+3:iband;
		for(int dig=0;dig<9;dig++)if(digpat.On(dig)){
			BF81 wwdig=cells_base & parent->c[dig],
				wwb(cellsInBandBM[ibandew],wwdig);
			if(wwb.IsEmpty()) continue; // no cover need in that band
			int nbox=0,nrc=0,tbox[3],trc[4];
			for(int ibox=0;ibox<3;ibox++){
				int iboxw=(moderc)? iband+3*ibox+18:3*iband+ibox+18 ;
				BF81 wwbb(cellsInHouseBM[iboxw],wwb);
				if(wwbb.IsNotEmpty()) tbox[nbox++]=iboxw;
			}
			for(int irc=0;irc<lx.nrc4;irc++){
				int ircw=lx.rc4[irc]+((moderc)? 9: 0);
				if((ircw/3)-ibandew) continue; // not in the band/stack
				BF81 wwbb(cellsInHouseBM[ircw],wwb);
				if(wwbb.IsNotEmpty()) trc[nrc++]=ircw;
			}
			if(nbox<nrc) {// use the box cover
				for(int j=0;j<nbox;j++)
					tcovers[ncovers++]=(dig<<7)+tbox[j];
			}
			else // use the row column cover
				for(int j=0;j<nrc;j++)
					tcovers[ncovers++]=(dig<<7)+trc[j];
		}
		if(ncovers>ntruths_cells) return; // stop as soon as possible
	}

}
*/
/*
new process  "8"
98.7.....7.6...8...54......6..8..3......9..2......4..1.3.6..7......5..9......1..4 # GP;H1521

	 12 Truths = {3B589 6B569 7B568 8B689}
	 12 Links = {7r4 8r7 3c4 6c7 69n5 58n6 69n8 58n9}
   look in two bands
   4 boxes in square
   or 4 rows/cols and 2 boxes on one end as link
   boxes have 3 empty cells
   une ligne et une colonne par bande trois connus
*/

//========================= BUILDSTRING


//=========================  PM_GO  processing a file

//==========================================================================PM_GO::HINT
//               0  1  2  3   4   5   6   7   8   9   10
int  steps[] = { 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128,
//11 12    13   14   15    16   17    18    19    20    21    22 
192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144, 8192 };

int PM_GO::HINT::ChainLengthAdjusted(int base, int length){
	if (length < 2 || length>8192)		return 0; // should not happen
	int wrating = 300, lg = length - 2;
	for (int ii = 0; ii<22; ii++)
		if (lg <= steps[ii]) {
			wrating = base + ii;
			break;
		}
	return wrating;
}
int PM_GO::HINT::MaxLengthForRating(int base){
	int index = rating - base;
	if (index<0 || index > 10) return 0;
	return steps[index] + 2;
}

void PM_GO::HINT::Add(PM3X & elime, USHORT rate){
	if (elime.IsEmpty()) return;
	if (rate>rating) { elime.SetAll_0(); return; }
	if (rate<rating){
		rating = rate;
		pmelims = elime;
		if (rating>62)parent->bdsp[0].Init();
	}
	else
		pmelims |= elime;
	elime.SetAll_0();
}

int PM_GO::HINT::AddCand(USHORT dig, USHORT cell, USHORT rate){
	if (rate <= parent->rat_er){// already active, keep it 
		if (rating>parent->rat_er){// but check for higher rating there (xy chains)
			pmelims.SetAll_0();
			if (rating>62)		parent->bdsp[0].SetCurrentAsFirst();
		}
		if (pmelims.On(dig, cell)){ parent->bdsp[0].ClearCurrent(); return 0; }
		pmelims.Set(dig, cell);
		rating = (USHORT)parent->rat_er; // just to say something hapenned
		return 1;
	}
	if (rate>rating) { parent->bdsp[0].ClearCurrent(); return 0; }
	//	if(rate>65)EE.Enl("add rating accepted");
	if (rate == rating && pmelims.On(dig, cell)){ parent->bdsp[0].ClearCurrent(); return 0; }
	if (rate<rating){
		//		if(rate>65)EE.Enl("add rating acceptedzero reset");
		rating = rate;
		pmelims.SetAll_0();
		if (rating>62)		parent->bdsp[0].SetCurrentAsFirst();
	}
	pmelims.Set(dig, cell);
	return 1;
}
//===========================================================================PM_GO
PM_GO::PM_GO(){
	/*
	t2cells.parent = r0search.parent = xysearch.yls.goparent = this;
	nested.tpmd = &zpmd[1]; //main data and dynam area for basic work
	nested.drcells = zpmd[1].dig_cells;
	rank0_min = 2;
	rank0_max = 5;
	myd_at_start = &zpmd[0];
	c = zpmd[0].pm.bfc;
	*/
	// setup buildstring permanent areas
	gintbuf.Set(gintbuffer, GINTBUFSIZE1);
	bdsp[0].SetUp(builstr1, buildstringsize1);
	bdsp[1].SetUp(builstr2, buildstringsize2);
	for (int i = 0; i<17; i++) ratfound[i] = 0;// used in low ratings compressed
	sgiven_ok = 0; // initial symmetry of given to nothing
	is_valid_puzzle = 1; // usually a valid puzzle processed 
}
int PM_GO::CleanOr(int d1, int c1, int d2, int c2){
	if (opprint & 1)cout << "pm_go cleanor " << d1 + 1 << cellsFixedData[c1].pt << " "
		<< d2 + 1 << cellsFixedData[c2].pt << endl;
	int digits = (1 << d1) | (1 << d2);
	BF128 clean = cell_z3x[c1];
	if (d1 == d2){
		int digits = 1 << d1;
		clean &= cell_z3x[c2];
		return zhou_solve.CleanCellsForDigits(clean, digits);
	}
	else if (c1 == c2) {
		int digs_c = zh_g2.dig_cells[c1];
		if (_popcnt32(digs_c) < 3) return 0;
		digs_c &= (~digits);		
		return zhou_solve.CleanCellForDigits(c1, digs_c);
	}
	else {// must be cell + digit
		if (clean.Off_c(c2)) return 0;// cells must be same unit
		int digs = zh_g2.dig_cells[c1];
		if( (digs & (1 << d2))&& zhou_solve.FD[d2][0].On_c(c1)) {
			zhou_solve.ClearCandidate_c(d2, c1);
			return 1;
		}
		digs = zh_g2.dig_cells[c2];
		if ((digs & (1 << d1)) && zhou_solve.FD[d1][0].On_c(c2)) {
			zhou_solve.ClearCandidate_c(d1, c2);
			return 1;
		}
		return 0;
	}
}
void PM_GO::Quickrate(int x) {// used in serate mode
	ratecycle = x;
	if (cycle == 1){
		//if (rat_ed<x)
		rat_ed = rat_ep = rat_er = x;
	}
	else 	if (!assigned){
		if (rat_ep<x)rat_ep = rat_er = x;
	}
	else if (rat_er<x) rat_er = x;
}

//___________________________  command 1xx not solve

void PM_GO::Solve120_MultiAnalysis() {
	cout << "entry 120" << endl;
	if (Solved_xx(44)) return;
	SetupActiveDigits();
	if (zh_g2.active_floor) {// done in Solved_xx
		cout << Char9out(zh_g2.active_floor) << " active digits" << endl;
		for(int idig=0;idig<9;idig++)
			if (zh_g2.elim_floor[idig].isNotEmpty()) {
				char ws[82];
				cout << zh_g2.elim_floor[idig].String3X(ws)
					<< " elim for digit " << idig + 1 << endl;
				zhou_solve.FD[idig][0] -= zh_g2.elim_floor[idig];
			}
		zh_g2.Pm_Status(&zhou_solve);
	}
	int aigstop = 0;
	zhou_solve.ImageCandidats();
	r0search.CollectPerms(1);// build the perms for unsolved digits
	for (int ifl = 0; ifl < 36; ifl++)
		aigstop += r0search.IsElims(floors_2d[ifl], 0);
	if (aigstop) return;
	for (int ifl = 0; ifl < 84; ifl++) 
		aigstop += r0search.IsElims(floors_3d[ifl], 1);
	if (aigstop) return;
	for (int ifl = 0; ifl < 126; ifl++) 
		aigstop += r0search.IsElims(floors_4d[ifl], 1);
	if (1 ||aigstop) return;
	for (int ifl = 0; ifl < 126; ifl++) 
		aigstop += r0search.IsElims(0x1ff ^ floors_4d[ifl], 1);
}
void PM_GO::Solve125_Find_Vloop() {
	if (sgo.vx[2])cout << "entry 125" << endl;
	if (Solved_xx(44)) return;
	if (sgo.vx[2])zhou_solve.ImageCandidats();
	if (vloop.Locate_Loop()) {
		fout1 << zh_g2.puz << ";vloop R/C;" << vloop.rows[0]+1 << vloop.rows[1]+1
			<<" "<< vloop.columns[0]+1 << vloop.columns[1]+1 << endl;
	}
}


struct EXOCET { // what is collected in the first step
	BF128 base, target,crossing_area,exocet,tabi_threat[9];
	int permbase, iu_base, cells_base[2], bs_base,bs_cross, bs_cross0, 
		digs,tdigs[4],ndigs,boxes[3],ntarget,mode,locked_digit,
		cross1,tabi[4],i2rc,abibf;
	int isExocet(BF128 b, int emode, int prc, int cross_rc);
	void SetAbiLoops();

};

int EXOCET::isExocet(BF128 b, int emode, int ei2prc, int cross_rc) {
	R0SEARCH & rs = pm_go.r0search;
	BF128 cmap = zhou_solve.cells_unsolved;
	// must be 3/4 digits
	cells_base[0] = b.getFirstCell();
	cells_base[1] = b.getLastCell();
	digs = zh_g2.dig_cells[cells_base[0]] | zh_g2.dig_cells[cells_base[1]];
	ndigs = _popcnt32(digs);
	if (ndigs < 3 || ndigs>4) return 0;
	base = b;
	i2rc = ei2prc;
	if (sgo.vx[2]) {
		cout << "try " << cellsFixedData[cells_base[0]].pt
			<< " " << cellsFixedData[cells_base[1]].pt 
			<<" digs 0"<<oct<<digs<<dec<< endl;
	}
	mode = emode;
	bs_cross0 = i2rc / 3;// to use in the perm loop
	bs_cross = bs_cross0 + 3* (1-mode);
	iu_base = cross_rc + 9 * mode;
	bs_base = iu_base / 3 ;
	cross1= floors2d_same_band_stack_third[i2rc]+9*(1-mode);
	crossing_area = cmap & band3xBM[bs_base];
	crossing_area -= units3xBM[iu_base];
	boxes[0] = cellsFixedData[cells_base[0]].ebu;
	if (mode) {// find 0ther boxes
		int b0 = cellsFixedData[cells_base[0]].pl / 3,n=1;
		for (int i = b0; i < 9; i += 3) 	if (i != boxes[0]-18) 
			boxes[n++] = i+18;
	}
	else  {
		int b0 = 3*(cellsFixedData[cells_base[0]].el / 3) , n = 1;
		for (int i = b0; i < b0+3; i ++) 	if (i != boxes[0]-18)
			boxes[n++] = i+18;
	}


	for (int ipcross = 0; ipcross < 36; ipcross++) {
		if (floors2d_band_stack_free[ipcross] != bs_cross0)continue;
		//================ this is a crossing to test
		if (mode) target = rs.tb9p2[ipcross].rows;
		else target = rs.tb9p2[ipcross].cols;
		target &= crossing_area & cmap;// can be 2;3 or 4 cells best is 2
		ntarget = target.Count();
		if (ntarget < 2) continue;
		if (sgo.vx[2]) {
			char ws[82];
			cout << target.String3X(ws) << " try target" << endl;
		}
		// all digit perms hitting base must hit target to have an exocet
		int nd = 0;
		for (int idig = 0; idig < 9; idig++) {
			if (!(digs&(1 << idig))) continue; // not a base digit
			//cout << "digit " << idig + 1 << " nperms= "<< rs.nperms[idig] << endl;
			tdigs[nd++] = idig;
			BF128 *tpw = rs.tperms_digits[idig];
			BF128 andperm = maskffff;
			for (uint32_t itp = 0; itp < rs.nperms[idig]; itp++) {
				BF128 w = tpw[itp];
				if ((w&base).isEmpty())continue;
				if ((w&target).isEmpty())goto next_ipcross;
				andperm &= w & target;
			}
			//cout << "exit digit ok" << endl;
			if (andperm == target) goto next_ipcross; // target forced by one digit
		}// end digit exocet granted ?? yes if ntarget=2 see if  more 
		exocet = base | target;
		if ((target&units3xBM[boxes[1]]).isEmpty()) goto next_ipcross;
		if ((target&units3xBM[boxes[2]]).isEmpty()) goto next_ipcross;
		if (sgo.vx[2]) {
			cout  << cellsFixedData[cells_base[0]].pt
				<< " " << cellsFixedData[cells_base[1]].pt
				<< " digs 0" << oct << digs << dec << endl;
			char ws[82];
			cout <<exocet.String3X(ws)<< "this is a valid exocet likely a JExocet" << endl;
			cout << "bs_base=" << bs_base << " iu_base=" << iu_base
				<< " bs_cross=" << bs_cross << "cross1=" << cross1 << endl;
			cout << " boxes " << boxes[0] << " " << boxes[1] << " " << boxes[2] << endl;
			cout << crossing_area.String3X(ws) << " crossing area" << endl;
		}
		
		if (ntarget > 2) {// check locked digits (one digit lockked)
			//cout << "check locked digit" << endl;
			locked_digit = 0;
			for (int ibox = 1; ibox < 3; ibox++) {
				int box = boxes[ibox];
				BF128 w = zhou_solve.cells_unsolved & units3xBM[box];
				if ((target&w).Count() != 2) continue;
				//cout << "check locked digit box " << box << endl;
				for (int idig = 0; idig < 9; idig++) {
					if (digs&(1 << idig)) continue;
					// use zhou_solve to have assigned
					BF128 fd_box = zhou_solve.FD[idig][0] & w;
					if ((fd_box - target).isEmpty()) {
						locked_digit |= 1 << ibox;
						//cout << "locked digit seen" << endl;
						break;
					}
				}
			}
		}
		SetAbiLoops();
		fout1 << zh_g2.puz << ";" << cellsFixedData[cells_base[0]].pt
			<< ";" << " " << cellsFixedData[cells_base[1]].pt << endl;

	next_ipcross:;
	}
	return 0; 
}

void EXOCET::SetAbiLoops() {
	R0SEARCH & rs = pm_go.r0search;
	memset(tabi, 0, sizeof tabi);
	memset(tabi_threat, 0, sizeof tabi_threat);
	abibf = 0;
	int p36 = floors2d_same_band_stack[i2rc]; // corresponding 36 perms
	BF128 threat_area= rs.tb9p2[p36].cols;
	int ibase = bs_base - 3 * mode, di = 9 * mode;
	if (mode) threat_area = rs.tb9p2[p36].rows;	
	threat_area &= zhou_solve.cells_unsolved;
	for (int ib=0,i = 0; ib < 3; ib++) {
		if (ib == ibase) {	i += 3; continue;	}
		for (int ir = 0; ir < 3; ir++, i++) {
			BF128 w = threat_area & units3xBM[i+di];
			if (w.Count() != 2) continue;
			tabi_threat[i] = w;
			abibf |= 1 << i;
		}
	}
	if (sgo.vx[2]>1) {
		char ws[82];
		cout << "threat map" << endl;
		for (int i = 0; i < 9; i++)
			cout << tabi_threat[i].String3X(ws) << " i=" << i << endl;
	}

	for (int i = 0; i < ndigs - 1; i++)for (int j = i + 1; j < ndigs; j++) {
		int dig1 = tdigs[i], dig2 = tdigs[j];// process the digits pair 
		if (sgo.vx[2] > 1)cout << "try pair " << dig1 + 1 << dig2 + 1 << " for abi exclusion" << endl;
		//___________________________ loop in perms stop at first valid no UR
		BF128 *tpw1 = rs.tperms_digits[dig1];
		for (uint32_t itp1 = 0; itp1 < rs.nperms[dig1]; itp1++) {
			BF128 w1= tpw1[itp1];// use perm hitting the base
			if ((w1&base).isEmpty())continue;
			BF128 *tpw2 = rs.tperms_digits[dig2];
			for (uint32_t itp2 = 0; itp2 < rs.nperms[dig2]; itp2++) {
				BF128 w2 = tpw2[itp2];// use perm hitting the base
				if ((w2&base).isEmpty())continue;
				if((w1&w2).isNotEmpty()) continue; // must be disjoint perms
				// note : not yet implementted
				// if 3 target cells and a locked digit (platinum blonde), 
				// can not be more than 2 assigned digits in the target
				BF128 pp = w1 | w2, ppt=pp&target;
				if ((ppt&units3xBM[boxes[1]]).isEmpty()) continue;
				if ((ppt&units3xBM[boxes[2]]).isEmpty()) continue;
				if (ppt.Count() > 2) {// check for locked digit(s)
					if(ntarget == 3 && locked_digit)continue;
				}
				for (int i = 0, bit = 1; i < 9; i++, bit <<= 1) {// check each UR threat
					if (!(abibf&bit)) continue;
					BF128 th = pp & tabi_threat[i];
					if (th.Count() == 2) goto exit_ur; // ignore perm with UR
				}// no ur found not an abi loop exclusion
				if (sgo.vx[2] > 1) {
					char ws[82];
					cout << pp.String3X(ws) << " perm valide" << endl;
					cout << ppt.String3X(ws) << " target view" << endl;
				}
				tabi[i ]|= 1 << j;
				tabi[j] |= 1 << i;
				goto next_idig2;
			exit_ur:;
			}
		}
	next_idig2:;
	}
	if (sgo.vx[2] > 1) {
		for (int i = 0; i < ndigs; i++) {
			if (tabi[i]) continue;
			cout << "digit " << tdigs[i] + 1 << " excluded through abi loop" << endl;
		}
	}
}

void PM_GO::Solve130_Find_JExocet() {
	if (sgo.vx[2] )cout << "entry 130" << endl;
	if (Solved_xx(44)) return;
	if (sgo.vx[2] )zhou_solve.ImageCandidats();
	//zh_g2.Pm_Status(&zhou_solve);
	BF128 cmap = zhou_solve.cells_unsolved,csmap=cmap^maskffff;
	R0SEARCH & rs = pm_go.r0search;
	rs.CollectPerms(sgo.vx[2]);// build the perms for unsolved digits
	// check all starts as 2 rows/cols in band/stack + col/row
	EXOCET we;
	for (int i2rc = 0; i2rc < 9; i2rc++) {// 9 pairs of row or columns
		int p36 = floors2d_same_band_stack[i2rc]; // corresponding 36 perms
		BF128 p36rbf = rs.tb9p2[p36].rows, p36cbf = rs.tb9p2[p36].cols;
		for (int icross_rc = 0; icross_rc < 9; icross_rc++) {
			BF128 w= p36rbf& units3xBM[icross_rc+9];// 2 rows 1 column
			w &= cmap;
			if (w.Count()==2) {
				if(we.isExocet(w, 1, i2rc, icross_rc)) return;
			}
			w= p36cbf & units3xBM[icross_rc];// 2 cols 1 row
			w &= cmap;
			if (w.Count() == 2) {
				if (we.isExocet(w, 0, i2rc, icross_rc)) return;
			}
		}
	}
}
/*

int PM_GO::Is_VLoop() { // solver mode
	if(bits_tasks_done.On(bit_skl_done) )return 0;
	bits_tasks_done.Set(bit_skl_done);
	if(!Locate_VLoop()) return 0;
	// sk loop found, print it and clean it
	EE.Enl("SK Loop");
	PM_DATA & myd=zpmd[0];
	for(int i=0;i<vloop.units_number;i++){
		ImagePoints(vloop.cells[i]); EE.Esp();
	}
	EE.Enl();
	USHORT * mydigits=vloop.digits[vloop.units_number-1];
	EE.E(mydigits[0]+1); EE.E(mydigits[1]+1);EE.E("   ");
	for(int i=0;i<vloop.units_number;i++){
		mydigits=vloop.digits[i];
		EE.E(mydigits[0]+1); EE.E(mydigits[1]+1);EE.E("    ");
	}
	EE.Enl();

	int ir=0;
	for(int i=0;i<vloop.units_number;i++){
		int unit=vloop.units[i];
		if(unit<9) {EE.E("R");EE.E(unit+1);}
		else if(unit<18) {EE.E("C");EE.E(unit-8);}
		else {EE.E("B");EE.E(unit-17);}
		EE.Esp();
		USHORT * digits=vloop.digits[i];
		BF81 w(cellsInHouseBM[unit]);
		w-=vloop.cells[i];
		w-=vloop.cells[i+1];
		for(int j=0;j<2;j++){
			int dig=digits[j];
			BF81 wd=w & c[dig];
			if(wd.IsEmpty()) continue;
			ir=1;
			myd.Clean(dig,wd);
			EE.E(dig+1);ImagePoints(wd);EE.Esp();
		}
		EE.Enl();
	}
	return ir;
}

}

int PM_GO::FindMultiFish2(){
// in that process, we look for bi floors active
//   having a rank 0 or nearly rank 0 logic)
  
if (active_floors.f) return 0; // on ne fait pas si encore du 1
PM_DATA & myd = zpmd[0];
int ir = 0;
for (int i1 = 0; i1 < 8; i1++) {
	if (c[i1].IsEmpty())continue;
	for (int i2 = i1 + 1; i2 < 9; i2++) {
		if (c[i2].IsEmpty())continue;
		if ((c[i1] & c[i2]).IsEmpty())continue; // not multi fish
		mfloors.f = 0;
		mfloors.Set(i1);
		mfloors.Set(i2);
		if (0) {
			EE.E("look for "); EE.E(i1 + 1); EE.Enl(i2 + 1);
			myd.ImageFloors_myd(mfloors);
		}
		int welims = game[1].Go_Multi(mfloors.f);
		if (welims) {
			EE.E("found active bi floor "); EE.E(i1 + 1); EE.Enl(i2 + 1);
			myd.ImageFloors_myd(mfloors);
			EE.E(" welims="); EE.Enl(welims);
			for (int i = 0; i < 9; i++) {
				BF81 & ww = GAME::glb.elims.bfc[i];
				if (ww.IsNotEmpty()) {
					EE.E(i + 1); EE.E(": ");
					ImagePoints(ww);
					EE.Enl();
				}
			}
			ir = 1;
		}
	}
}

return ir;
}
*/
//__________________________________________________________Solve

void PM_GO::Status(const char * lib, int option){
	if ((!option )||(opprint2 & option)){
		cout <<"status "<< lib << endl;
		zhou_solve.ImageCandidats();
	}

}
int PM_GO::SolveGetLow44(int pack,int diag) {
	//===========================================================
	zh_g.diag = opprint = opprint2 = stop_rating = cycle = assigned = rat_er = rat_ep = rat_ed = 0;
	zh_g.Init(1);	ur_serate_mode = 1;
	if (diag) {
		cout << "\n<<<<<<<<entry   unsolved=" << zhou_solve.cells_unsolved.Count() << " assigned=" << assigned << endl;
		zhou_solve.ImageCandidats();
	}
	while (cycle++ < 150) {
		if (cycle > 148 || stop_rating) return 0;
		if (zhou_solve.cells_unsolved.isEmpty())break; // solved below 4.5
		if (diag) {
			cout << "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<next cycle=" << cycle
				<< " rating=" << rat_er
				<< " unsolved=" << zhou_solve.cells_unsolved.Count() << " assigned=" << assigned << endl;
			zhou_solve.ImageCandidats();
		}
		zh_g2.Init_Assign();
		if (rat_er < 28){ if (Next10_28()) continue; }
		else if (Next28()) continue;
		if (Next30_44()) continue;
		if (!rat_ed)rat_er = rat_ep = rat_ed = 200; else rat_er = 200;
		return 0;
	}
	// the puzzle is solved 
	if (!pack) return 1;
	int r_list[17] = { 10, 12, 15, 17, 20, 23, 25, 26,
		28, 30, 32, 34, 36, 38, 40, 42, 44 };
	// ignore it if already the same ER
	if (rat_er < 45){// safety always true here
		int i;
		for (i = 0; i < 17; i++) if (rat_er == r_list[i]) break;
		if (i > 16) return 0; // should never be
		UINT rr = ((rat_er * 100) + rat_ep) * 100 + rat_ed;
		if (rr > ratfound[i]){
			ratfound[i] = rr;
			return 1;
		}
		return -1;// ask to ignore it
	}
	return 0;
}
int PM_GO::Solved_xx(int lim) {
	//===========================================================
	zh_g.diag = opprint = opprint2 = stop_rating = cycle = assigned = rat_er = rat_ep = rat_ed = 0;
	zh_g.nsol = 0; zh_g.lim = 1;	ur_serate_mode = 1;
	if (sgo.vx[9])opprint = opprint2 = sgo.vx[9];
	while (cycle++ < 150) {
		if (cycle > 148 || stop_rating) return 0;
		if (zhou_solve.cells_unsolved.isEmpty())return 1; // solved 
		zh_g2.Init_Assign();
		if (rat_er < 28){ if (Next10_28()) continue; }
		else if (Next28()) continue;
		if (Next30_44()) continue;
		//Status("after no WXYZwing", 2);
		//to test Rate45_52_Fast () smal additional risk with multi URs ULs
		if (lim < 45) return 0;
		if (Rate45_52()) continue;
		if (Rate52())continue;		
		if (Rate54())continue;
		if (Rate56())continue;
		if (lim < 62) return 0;
		SetupActiveDigits();
		XStatusPrepare();
		if (Rate65Xcycle(1)) continue;
		if (Rate66Xchain(1)) continue;
		if (opprint) {
			cout << "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<next cycle=" << cycle
				<< " rating=" << rat_er
				<< " unsolved=" << zhou_solve.cells_unsolved.Count() 
				<< " assigned=" << assigned << endl;
			zhou_solve.ImageCandidats();
		}
		if (rat_er < 75)// skip Y loop if XY chain can be applied
			if (ylsearch.Search(1)
				|| ylsearch.SearchOut(1)) {
				Quickrate(66); continue;
			}
		if (Rate70_75(1)) continue;
		if (Rate75())continue;
		if (lim < 75) return 0;
		if (opprint) cout << "call 76" << endl;
		if (Rate76Nishio(1)) continue;
		if (opprint) cout << "call 83" << endl;
		if (xysearch.SearchMulti(1)) { Quickrate(83); continue; }
		if (opprint) cout << "call 85" << endl;
		if (xysearch.SearchDyn(1)) { Quickrate(85); continue; }
		if (lim < 90) return 0;
		if (opprint) cout << "call 96" << endl;
		if (xysearchplus.SearchDynPlus(1)) { Quickrate(96); continue; }
		break;
	}
	return 0;
}


int PM_GO::SolveDet(int lim,int printopt,int mode) {
	//===========================================================
	zh_g.diag = 0;	opprint = printopt; opprint2 = 0;
	stop_rating = cycle = assigned = 0;
	rat_er = rat_ep = rat_ed = ratecycle = 0;
	zh_g.nsol = 0; zh_g.lim = 1;
	ur_serate_mode = 1;
	if (!lim)lim = 200;
	while (cycle++ < 150) {
		if (cycle > 148) { stop_rating = 7;	break; }
		if (stop_rating) 	break;
		if (zhou_solve.cells_unsolved.isEmpty()) {
			if (opprint)cout << "solved ER=" << rat_er << "/"
				<< rat_ep << "/" << rat_ed << endl;
			return 1;
		}
		if (pm_go.opprint & 2) {
			cout << "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<next cycle=" << cycle
				<< " rating=" << rat_er
				<< " unsolved=" << zhou_solve.cells_unsolved.Count() << " assigned=" << assigned << endl;
			zhou_solve.ImageCandidats();
			if (is_valid_puzzle && zhou_solve.CheckStatus()) {
				return 2;
			}
		}
		zh_g2.Init_Assign();
		ratecycle = 0;
		if (Next_below_45()) continue;
		if (lim <= 44) return 0;
		//to test Rate45_52_Fast () smal additional risk with multi URs ULs
		if (pm_go.opprint & 2) 	if ((cycle - 1) & 15) 		zhou_solve.ImageCandidats();
		if (Rate45_52()) continue;
		if (Rate52())continue;
		if (Rate54())continue;
		if (Rate56())continue;
		if (lim <= 61) return 0;
		if (Rate62())continue;
		SetupActiveDigits();
		if ((pm_go.opprint & 2) && zh_g2.active_floor)
			cout << Char9out(zh_g2.active_floor) << " active digits" << endl;
		XStatusPrepare();
		if (Rate65Xcycle(0)) continue;
		if (lim <= 65) return 0;
		if (Rate6xXcycle(66)) continue;
		if (Rate66Xchain(0)) continue;
		if (rat_er < 75)// skip Y loop if XY chain can be applied
			if (ylsearch.Search()) { Quickrate(66); continue; }
		if (Rate67_70()) continue;
		if (Rate70_75(0)) continue;// no fast mode here
		if (Rate75())continue;
		if (Rate76Nishio(0)) continue;//no fast mode  
		if (lim <= 81) return 0;
		if (Rate80Multi(0))continue;//no fast mode 
		if (lim <= 85) return 0;
		if (Rate85Dynamic(0))continue;//no fast mode 
		if (lim <= 90) return 0;
		if (1) return 0;
		if (Rate90DynamicPlus(1))continue;
		break;
		//next_cycle:;
	}
	return 0;
}


void PM_GO::SolveSerate110() {
	//===========================================================
	zh_g.diag = sgo.vx[9];	opprint = sgo.vx[8];	opprint2 = sgo.bfx[8];
	if (opprint)cout << zh_g2.zsol << "valid puzzle printoption=" << opprint << "  print2=" << opprint2 << endl;
	stop_rating = cycle = assigned = 0;
	rat_er = rat_ep = rat_ed = ratecycle = 0;
	zh_g.nsol = 0; zh_g.lim = 1;
	ur_serate_mode = 1;
	if (!sgo.vx[2])sgo.vx[2] = 200;// limit for searched rating
	while (cycle++ < 150) {//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  enough in test up to 150 later
		if (cycle > 148) { stop_rating = 7;	break; }
		if (stop_rating) 	break;
		if (zhou_solve.cells_unsolved.isEmpty()){
			if (opprint)cout << "solved ER=" << rat_er << "/"<< rat_ep << "/" << rat_ed << endl<<"___________________________" << endl;
			break;
		}
		if (pm_go.opprint & 2) {
			char zi[82];  zhou_solve.SetKnown(zi);
			cout << zi << "	cycle=" << cycle << " unsolved=" << zhou_solve.cells_unsolved.Count() << endl;
			if (!((cycle - 1) & 15)) 		zhou_solve.ImageCandidats();
			if (zhou_solve.CheckStatus()) {
				cerr << "fatal error" << endl;
				cout << "fatal error" << endl;
				return;
			}
		}
		zh_g2.Init_Assign();
		ratecycle = 0;
		if (rat_er < 28) { if (Next10_28()) continue; }
		else if (Next28()) continue;
		if (Next30_44()) continue;
		//to test Rate45_52_Fast () smal additional risk with multi URs ULs
		if (Rate45_52()) continue;
		if (Rate52())continue;		
		if (Rate54())continue;
		if (Rate56())continue;
		if (sgo.vx[2] <= 61) goto exit_limit;
		if (Rate62())continue;
		SetupActiveDigits();
		if (pm_go.opprint & 2){
			cout << Char9out(zh_g2.active_floor) << " active digits" << endl;
			if ((cycle - 1) & 15)	zhou_solve.ImageCandidats();
		}

		XStatusPrepare();
		if (Rate65Xcycle(0)) continue;
		if (sgo.vx[2] <= 65) goto exit_limit;
		if (Rate6xXcycle(66)) continue;
		if (Rate66Xchain(0)) continue;
		if (rat_er < 75)// skip Y loop if XY chain can be applied
		if (ylsearch.Search()){ Quickrate(66); continue; }
		if (Rate67_70()) continue;
		if (Rate70_75(rat_er > 75)) continue;
		if (Rate75())continue;
		if (Rate76Nishio(rat_er >82)) continue;// fast mode above
		if (sgo.vx[2] <= 81) goto exit_limit;
		if (Rate80Multi(rat_er >= 90))continue;
		if (Rate85Dynamic(rat_er >= 96))continue;
		if (sgo.vx[2] <= 90) goto exit_limit;
		if (Rate90DynamicPlus(1))continue;

		if (1) { stop_rating = 1; break; }
		stop_rating = 1;
		break;
	//next_cycle:;
	}

	if (stop_rating){
		cout << zh_g2.puz << "; puz n=" << zh_g2.npuz << " unsolved stop_rating = " << stop_rating << endl;
		fout1 << zh_g2.puz << ";0;0;0;" << stop_rating <<" stop======"<< endl;
	}
	else fout1 << zh_g2.puz << ";" << rat_er / 10 << "." << rat_er % 10 << ";" << rat_ep / 10 << "." << rat_ep % 10
		<< ";" << rat_ed / 10 << "." << rat_ed % 10 << endl;
	return;
exit_limit:
	if (sgo.bfx[7] & 1)fout2 << zh_g2.puz << endl;
}
void PM_GO::SolveSerate111(){// quick rate ans split serate mode

	zh_g.diag = opprint = opprint2 = 0;
	stop_rating = cycle = assigned = rat_er = rat_ep = rat_ed = 0;
	zh_g.nsol = 0; zh_g.lim = 1;
	ur_serate_mode = 1;
	sgo.vx[2] = 200;
	//phase 1 is it below 45
	while (cycle++ < 150) {
		if (stop_rating) 	return;// should never be skip it
		if (zhou_solve.cells_unsolved.isEmpty())break;
		zh_g2.Init_Assign();
		if (rat_er < 28){ if (Next10_28()) continue; }
		else if (Next28()) continue;
		if (Next30_44()) continue;
		//cout << "goto phase2" << endl;
		goto phase2;// not solved 
	}
	//cout << "end low " << rat_er / 10 << "." << rat_er % 10 << ";" << rat_ep / 10 << "." << rat_ep % 10
	//	<< ";" << rat_ed / 10 << "." << rat_ed % 10 << endl;
	// the puzzle is solved pack the results
	{
	int r_list[17] = { 10, 12, 15, 17, 20, 23, 25, 26,
		28, 30, 32, 34, 36, 38, 40, 42, 44 };
	// ignore it if already the same ER
	int i;
	for (i = 0; i < 17; i++) if (rat_er == r_list[i]) break;
	if (i > 16) return; // should never be
	UINT rr = ((rat_er * 100) + rat_ep) * 100 + rat_ed;
	if (rr > ratfound[i]){
		ratfound[i] = rr;
		fout1 << zh_g2.puz << ";" << rat_er << ";" << rat_ep	<< ";" << rat_ed << endl;
		return;
	}
	return;// ask to ignore it
	}//end of int r_list scope

	//=============================== rating over 44

	while (cycle++ < 150) {
		//cout << "next cycle=" << cycle << endl;
		if (cycle > 148) { stop_rating = 7;	break; }
		if (stop_rating) 	break;
		if (zhou_solve.cells_unsolved.isEmpty())break;
		zh_g2.Init_Assign();
		if (Next28()) continue;
		if (Next30_44()) continue;
	phase2:// entry phase 2 not solved below 45
		//to test Rate45_52_Fast () smal additional risk with multi URs ULs
		if (0) {
			cout << "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<next cycle=" << cycle
				<< " rating=" << rat_er
				<< " unsolved=" << zhou_solve.cells_unsolved.Count() << " assigned=" << assigned << endl;
			zhou_solve.ImageCandidats();
			//if (cycle > 10)opprint = opprint2 = 0xfe;
		}
		if (Rate45_52()) continue;
		if (Rate52())continue;		
		if (Rate54())continue;
		if (Rate56())continue;
		if (Rate62())continue;
		SetupActiveDigits();
		XStatusPrepare();
		if (Rate65Xcycle(1)) continue;
		if (Rate66Xchain(1)) continue;
		if (rat_er < 75)// skip Y loop if XY chain can be applied
			if (ylsearch.Search(1)
				|| ylsearch.SearchOut(1)){		Quickrate(66); continue;	}
		if (Rate70_75(1)) continue;
		if (Rate75())continue;
		if (Rate76Nishio(1)) continue; 
		if (xysearch.SearchMulti(1))	{ Quickrate(83); continue; }
		if (xysearch.SearchDyn(1))	{ Quickrate(85); continue; }
		if (xysearchplus.SearchDynPlus(1)) { Quickrate(96); continue; }
		stop_rating = 1;
		break;
		//next_cycle:;
	}
	//cout << "end std " << rat_er / 10 << "." << rat_er % 10 << ";" << rat_ep / 10 << "." << rat_ep % 10
	//	<< ";" << rat_ed / 10 << "." << rat_ed % 10 << " stop=" << stop_rating << endl;
	if (stop_rating){
		fout3 << zh_g2.puz << ";" << rat_er << ";" << rat_ep << ";" << rat_ed  << "; unsolved" << stop_rating << endl;
	}
	else if (rat_ed==rat_er)		
		fout_diam << zh_g2.puz << ";" << rat_er << ";" << rat_ep << ";" << rat_ed << endl;
	else if (rat_ep == rat_er)
		fout_pearl << zh_g2.puz << ";" << rat_er << ";" << rat_ep << ";" << rat_ed << endl;
	else if (rat_er<65)
		fout_l65  << zh_g2.puz << ";" << rat_er << ";" << rat_ep << ";" << rat_ed << endl;
	else fout2 << zh_g2.puz << ";" << rat_er << ";" << rat_ep << ";" << rat_ed << endl;

}
void PM_GO::SolveSerate112() {// full explain mode
	//===========================================================
	opprint = sgo.vx[8];	opprint2 = sgo.bfx[8];
	if (opprint)cout << zh_g2.zsol << "valid puzzle printoption=" << opprint << "  print2=" << opprint2 << endl;
	stop_rating = cycle = assigned = 0;
	rat_er = rat_ep = rat_ed = ratecycle = 0;
	zh_g.nsol = 0; zh_g.lim = 1;
	ur_serate_mode = 0;
	if (!sgo.vx[2])sgo.vx[2] = 200;// limit for searched rating
	while (cycle++ < 150) {//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  enough in test up to 150 later
		if (cycle > 148) { stop_rating = 7;	break; }
		if (stop_rating) 	break;
		if (zhou_solve.cells_unsolved.isEmpty()) {
			if (opprint)cout << "solved ER=" << rat_er << "/" << rat_ep << "/" << rat_ed << endl << "___________________________" << endl;
			break;
		}
		if (pm_go.opprint & 2) {
			char zi[82];  zhou_solve.SetKnown(zi);
			cout << zi << "	cycle=" << cycle << " rating=" << rat_er << " unsolved=" << zhou_solve.cells_unsolved.Count() << endl;
			if (!((cycle - 1) & 15)) 		zhou_solve.ImageCandidats();
			if (zhou_solve.CheckStatus()) {
				zhou_solve.ImageCandidats();
				cerr << "fatal error" << endl;
				cout << "fatal error" << endl;
				return;
			}
		}
		zh_g2.Init_Assign();
		ratecycle = 0;
		if( Next_below_45()) continue;
		//to test Rate45_52_Fast () smal additional risk with multi URs ULs
		if (pm_go.opprint & 2) 	if ((cycle - 1) & 15) 		zhou_solve.ImageCandidats();
		if (Rate45_52()) continue; // 50 naked quad
		if (Rate52())continue;// jellyfish
		if (Rate54())continue;// hidden quad
		if (Rate56())continue;// bug
		if (sgo.vx[2] <= 61) goto exit_limit;
		if (Rate62())continue;// aligned pair
		SetupActiveDigits();
		if ((opprint & 2) && zh_g2.active_floor) {
			cout << Char9out(zh_g2.active_floor) << " active digits" << endl;
			if (opprint & 4) {
				for (int i = 0, bit = 1; i < 9; i++, bit <<= 1) if (zh_g2.active_floor & bit)
					zhou_solve.DebugDigit(i);
			}
		}
		XStatusPrepare();
		if (Rate65Xcycle(0)) continue;
		if (sgo.vx[2] <= 65) goto exit_limit;
		if (Rate6xXcycle(66)) continue;
		if (Rate66Xchain(0)) continue;
		if (rat_er < 75)// skip Y loop if XY chain can be applied
			if (ylsearch.Search()) { Quickrate(66); continue; }
		if (Rate67_70()) continue;
		if (Rate70_75(0)) continue;
		if (Rate75())continue;// aligned triplet
		if (Rate76Nishio(0)) continue;
		if (sgo.vx[2] <= 81) goto exit_limit;
		if (Rate80Multi(0))continue;
		if (Rate85Dynamic(rat_er >= 96))continue;
		if (sgo.vx[2] <= 90) goto exit_limit;
		if (Rate90DynamicPlus(1))continue;

		if (1) { stop_rating = 1; break; }
		stop_rating = 1;
		break;
		//next_cycle:;
	}

	if (stop_rating) {
		cout << zh_g2.puz << "; puz n=" << zh_g2.npuz << " unsolved stop_rating = " << stop_rating << endl;
		fout1 << zh_g2.puz << ";0;0;0;" << stop_rating << " stop======" << endl;
	}
	else fout1 << zh_g2.puz << ";" << rat_er / 10 << "." << rat_er % 10 << ";" << rat_ep / 10 << "." << rat_ep % 10
		<< ";" << rat_ed / 10 << "." << rat_ed % 10 << endl;
	return;
exit_limit:
	if (sgo.bfx[7] & 1)fout2 << zh_g2.puz << endl;
}


void PM_GO::Solve118_subgrid() {

}

void PM_GO::Solve199test() {
	//===========================================================
	if (!Solved_xx(96))
		fout2 << zh_g2.puz << endl;
	else fout1 << zh_g2.puz << ";" << rat_er << ";" << rat_ep << ";" << rat_ed << endl;
}


//_________________________________________________________ Solve serate steps
int PM_GO::Next10_28(){
	if (Rate10())return 1;		
	if (Rate12())return 1;
	if (Rate15())return 1;		
	if (Rate17())return 1;
	zh_g2.Pm_Status(&zhou_solve);
	zhou_solve.FindNakedPairsTriplets_NoSingle();
	zhou_solve.Naked_Pairs_Seen();
	if (Rate20())return 1;		
	if (Rate23())return 1;
	if (Rate25())return 1;		
	if (Rate26())return 1;
	if (Rate28())return 1;		
	return 0;
}
int PM_GO::Next28(){
	BF128 ru = zhou_solve.cells_unsolved;
	if (zhou_solve.FullUpdate() == 2) return 1;// solved in full update
	if (zhou_solve.Rate15_SingleColumn()){
		assigned = 1;
		zhou_solve.AssignSolver(0);
		return 1;
	}
	if (ru != zhou_solve.cells_unsolved)assigned = 1;
	if (zhou_solve.cells_unsolved.isEmpty()) return 1;
	zh_g2.Pm_Status(&zhou_solve);
	zhou_solve.FindNakedPairsTriplets_NoSingle();
	zhou_solve.Naked_Pairs_Seen();
	if (Rate20())return 1;		
	if (Rate25())return 1;
	if (Rate26())return 1;
	if (Rate28())return 1;
	return 0;// can continue in the same cycle 
}
int PM_GO::Next30_44(){
	if (Rate30())return 1;
	if (Rate32())return 1;		
	if (Rate34())return 1;
	zh_g2.Pm_Status_End();// cell digits and box digit/cells
	if (Rate36())return 1;
	if (Rate38())return 1;
	if (Rate40())return 1;		
	if (Rate42())return 1;
	if (Rate44())return 1;		
	return 0;
}
int PM_GO::Next_below_45() {
	zh_g2.Pm_Status(&zhou_solve);
	zhou_solve.FindNakedPairsTriplets_NoSingle();
	zhou_solve.Naked_Pairs_Seen();
	if (Rate10()+Rate12()+Rate15()+Rate23())return 1;// single somewhere
	if (Rate26())return 1;// locked in box
	if (Rate28())return 1;// locked row col
	if (Rate30())return 1;// naked pair
	if (Rate32())return 1;// xwing
	if (Rate34())return 1;// hidden pair
	zh_g2.Pm_Status_End();// cell digits and box digit/cells
	if (Rate36())return 1;// naked triplet
	if (Rate38())return 1;// swordfish
	if (Rate40())return 1;// hidden triplet
	if (Rate42())return 1;// xywing
	if (Rate44())return 1;// xyzwing
	return 0;
}


int PM_GO::Rate10(){
	if (zhou_solve.Rate10_LastInUnit()){
		//if(opprint2 & 1)cout << "seen rating 10 last in unit" << endl;
		zhou_solve.AssignSolver(opprint & 2);
		assigned = 1;
		Quickrate(10);
		return 1;
	}
	return 0;
}
int PM_GO::Rate12(){
	if (zhou_solve.Rate12_SingleBox()){
		zhou_solve.AssignSolver(opprint & 2);
		assigned = 1;
		Quickrate(12);
		return 1;
	}
	return 0;
}
int PM_GO::Rate15(){
	if (zhou_solve.Rate15_SingleRow()){
		zhou_solve.AssignSolver(opprint & 2);
		assigned = 1;
		Quickrate(15);
		return 1;
	}
	if (zhou_solve.Rate15_SingleColumn()){
		zhou_solve.AssignSolver(opprint & 2);
		assigned = 1;
		Quickrate(15);
		return 1;
	}
	return 0;
}
int PM_GO::Rate17(){
	if (zhou_solve.Rate17_lockedBox_Assign()){
		if (opprint2 & 1)cout << "seen rating 17" << endl;
		zhou_solve.AssignSolver(opprint & 1);
		assigned = 1;
		Quickrate(17);
		return 1;
	}
	return 0;
}
int PM_GO::Rate20(){
	if (zhou_solve.Rate20_HiddenPair_Assign()){
		if (opprint2 & 1)cout << "seen rating 20 br" << endl;
		zhou_solve.AssignSolver(opprint & 1);
		assigned = 1;
		Quickrate(20);
		return 1;
	}
	return 0;
}
int PM_GO::Rate23(){
	if (zhou_solve.Rate23_SingleInCell_Assign()){
		if (opprint & 1)cout << "seen rating 23 single in cell" << endl;
		zhou_solve.AssignSolver(opprint & 1);
		assigned = 1;
		Quickrate(23);
		return 1;
	}
	return 0;
}
int PM_GO::Rate25(){
	if (zhou_solve.Rate25_HiddenTriplet_Assign()){
		if (opprint & 1)cout << "seen rating 25_HiddenTripletBox_Assign" << endl;
		zhou_solve.AssignSolver(opprint & 1);
		assigned = 1;
		Quickrate(25);
		return 1;
	}
	return 0;
}
int PM_GO::Rate26(){
	if (zhou_solve.Rate26_lockedBox()){
		if (opprint & 1)cout << "seen rating 26_lockedBox" << endl;
		Quickrate(26);
		return 1;
	}
	return 0;
}
int PM_GO::Rate28(){
	if (zhou_solve.Rate28_lockedRowCol()){
		if (opprint & 1)cout << "seen rating 28_lockedRowCol" << endl;
		Quickrate(28);
		//if (1)zhou_solve.ImageCandidats();
		return 1;
	}
	return 0;
}
int PM_GO::Rate30(){
	if (zhou_solve.Rate30_NakedPair()){
		if (opprint & 2)cout << "seen rating 30_NakedPair" << endl;
		Quickrate(30);
		//if (1)zhou_solve.ImageCandidats();
		return 1;
	}
	return 0;
}
int PM_GO::Rate32(){
	if (zhou_solve.Rate32_XWing()){
		if (opprint & 2)cout << "seen rating 32_XWing" << endl;
		Quickrate(32);
		//if (1)zhou_solve.ImageCandidats();
		return 1;
	}
	return 0;
}
int PM_GO::Rate34(){
	if (zhou_solve.Rate34_HiddenPair()){
		if (opprint & 2)cout << "seen Rate34_HiddenPair" << endl;
		Quickrate(34);
		//if (1)zhou_solve.ImageCandidats();
		return 1;
	}
	return 0;
}
int PM_GO::Rate36(){
	if (zhou_solve.Rate36_NakedTriplet()){
		if (opprint & 2)cout << "seen Rate36_NakedTriplet" << endl;
		Quickrate(36);
		//if (1)zhou_solve.ImageCandidats();
		return 1;
	}
	return 0;
}
int PM_GO::Rate38(){
	if (zhou_solve.Rate38_SwordFish()){
		if (opprint & 2)cout << "seen Rate38_SwordFish" << endl;
		Quickrate(38);
		return 1;
	}
	return 0;
}
int PM_GO::Rate40(){
	if (zhou_solve.Rate40_HiddenTriplet()){
		if (opprint & 2)cout << "Rate40_HiddenTriplet" << endl;
		Quickrate(40);
		//if (1)zhou_solve.ImageCandidats();
		return 1;
	}
	return 0;
}
int PM_GO::Rate42(){
	if (zhou_solve.Rate42_XYWing()){
		if (opprint & 2)cout << "Rate42_XYWing" << endl;
		Quickrate(42);
		return 1;
	}
	return 0;
}
int PM_GO::Rate44(){
	if (zhou_solve.Rate44_XYZWing()){
		if (opprint & 2)cout << "Rate44_XYZWing" << endl;
		Quickrate(44);
		//if (1)zhou_solve.ImageCandidats();
		return 1;
	}
	return 0;
}
int  PM_GO::Rate45_52(){//
	if (Rate45_URs(tur, ntur)){
		if (opprint & 2)cout << "Rate45_URs" << endl;
		Quickrate(45);
		return 1;
	}
	if (opprint & 1)cout << "exit UR pending URs " << ntur << endl;
	if (Rate45_2cells(tur, ntur)){// clean twin digit and clear if no UR digit in the plus cells
		if (opprint & 2)cout << "Rate45_URs twin digit" << endl;
		Quickrate(45);
		return 1;
	}
	ntul = 0;
	for (int target = 45; target <= 53; target++){// try step by step more
		if (opprint & 1)cout << "URs for target " << target << endl;
		int iret = 0;
		if (ntur && target<=48){
			for (int iur = 0; iur < ntur; iur++)
				iret += wwur2.Go_serate(tur[iur], target);
		}
		if (target == 46)Rate46_Find_ULs();// collect ULs
		if (ntul){
			for (int iul = 0; iul < ntul; iul++){
				STORE_UL & s = tul[iul];
				int rbase = 45 + s.ur2.u8[4];
				if (rbase > target) continue; // not yet time to work on it
				if (rbase == target){
					iret += RateUL_base(s);
				}
				//cout << "try UL go serate itul=" << iul<< endl;
				if (s.ur2.u16[1]){// not killed after rbase 
					iret += wwur2.Go_serate(s.ur2, target);
				}
			}
		}
		if (target == 50) 			iret += zhou_solve.Rate50_NakedQuad();
		if (iret){
			Quickrate(target);
			return 1;
		}
	}
	return 0;
}
int  PM_GO::Rate45_52_Fast(){//
	if (Rate45_URs(tur, ntur))return 1;
	if (Rate45_2cells(tur, ntur))return 1;
	ntul = 0;
	int iret = 0;
	Rate46_Find_ULs();// collect ULs
	if (ntul){
		for (int iul = 0; iul < ntul; iul++){
			STORE_UL & s = tul[iul];
			GINT64 t = s.ur2;
			iret += RateUL_base(s);
			if (RateUL_base(s))iret++;
			else if (t.u16[1])tur[ntur++] = t;
		}
	}
	if (iret)return 1;
	for (int iur = 0; iur < ntur; iur++){// now UR UL 2 cells in unit

	}
	iret += zhou_solve.Rate50_NakedQuad();

	return iret;
}
int PM_GO::Rate52(){
	if (zhou_solve.Rate52_JellyFish()){
		if (opprint & 2)cout << "seen Rate52_JellyFish" << endl;
		Quickrate(52);
		return 1;
	}
	return 0;
}
int PM_GO::Rate54(){
	if (zhou_solve.Rate54_HiddenQuad()){
		if (opprint & 2)cout << "seen Rate54_HiddenQuad" << endl;
		Quickrate(54);
		return 1;
	}
	return 0;
}
int PM_GO::Rate56() {
	if (Rate56BUG()){
		if (opprint & 2)cout << "Rate56 ... done rating " << hint.rating_done << endl;
		Quickrate(hint.rating_done);
		return 1;
	}
	return 0;
}
int PM_GO::Rate62() {
	if (Rate62_APE()){
		if (opprint2 & 16)cout << "Aligned Pair Exclusion " << endl;
		Quickrate(62);
		return 1;
	}
	return 0;
}
void PM_GO::SetupActiveDigits(){
	zh_g2.active_floor = 0;// no elim with a single digit
	for (int i = 0; i < 9; i++){
		if (!zhou_solve.FD[i][0].bf.u32[3]) continue; // solved digit
		zhou_solve.StartFloor(i);
	}
}
int PM_GO::Rate65Xcycle(int fast){
	nstore_xlc = 0;
	int iret = 0;
	for (int i = 0; i < 9; i++){
		iret+=xstatus[i].XCycle(fast);
	}
	if (iret) 		Quickrate(65);
	return iret;
}
int PM_GO::Rate6xXcycle(int rating){
	int iret = 0;
	for (int i = 0; i <nstore_xlc; i++){
		STORE_XLC & s = store_xlc[i];
		if (!s.loop) break; //no more  xcycle
		if (s.rating != rating) continue;
		if (pm_go.opprint & 1)cout << "try r6xcycle rating" << rating << " istore=" << i << endl;
		int ir= xstatus[s.dig].CleanLoop(s.t, s.nt);
		if (ir) { s.Print();		iret = 1; }
	}
	if (iret)Quickrate(rating);
	return iret;
}
int PM_GO::Rate66Xchain(int fast){
	nstore_xlc = 0;
	int iret = 0;
	for (int i = 0; i < 9; i++){
		iret += xstatus[i].XChain(fast);
	}
	if (iret) 		Quickrate(66);
	return iret;
}
int PM_GO::Rate67_70(){
	for (int irating = 67; irating <= 70; irating++){
		if (R67_70(irating)){
			Quickrate(irating);
			return 1;
		}
	}
	return 0;
}
int PM_GO::R67_70(int rating){
	//if (pm_go.opprint2 & 2)cout << "try rating=" << rating
	//	<< " nxlc=" << nstore_xlc << " nyl=" << nstore_yl << endl;
	int iret = 0;
	for (int i = 0; i <nstore_xlc; i++){
		STORE_XLC & s = store_xlc[i];
		if (s.rating != rating) continue;
		if (s.loop) {
			int ir = xstatus[s.dig].CleanLoop(s.t, s.nt);
			if (ir) { s.Print();		iret = 1; }
		}
		else{// this is an xchain
			BF128 clean;
			int ir =xstatus[s.dig].CleanChain(s.t, s.nt,clean);
			if (ir) { s.Print();		iret = 1; }
		}
	}
	for (int i = 0; i <nstore_yl; i++){
		YLSEARCH & s = store_yl[i];
		if (s.Is_To_Clean(rating)) {
			int ir = s.CleanLoop();
			if (ir) { s.PrintTback(" yloop active ");		iret = 1; }
		}
	}
	if (iret) return 1;
	if (rating == 68)		return ylsearch.SearchOut(0);//try also yloop clear out of region
	return 0;
}
int PM_GO::Rate70_75(int fast){
	if (xysearch.Search(fast))	{ 
		if (!fast )	Quickrate(hint.rating_done); 
		else Quickrate(70);
		return 1; 
	}
	return 0;
}
int PM_GO::Rate75(){
	if (Rate75_ATE())	{
		Quickrate(75); 
		return 1; 
	}
	return 0;
}
int PM_GO::Rate76Nishio(int fast){
	int diagloc = 0;
	if(diagloc)cout << "rate 76 nishio" << endl;
	int iret = 0;
	xcom.Init(fast);
	for (int i = 0; i < 9; i++){
		if (xstatus[i].Nishio1()){
			iret++;
			if (opprint & 1)cout << "nishio1 clear digit" << i + 1 << endl;
		}
	}
	if (!iret){
		for (int i = 0; i < 9; i++){
			if (xstatus[i].Nishio2()){
				iret++;
				if (opprint & 2)cout << "nishio2 clear digit" << i + 1 << endl;
			}
		}
	}
	if (xcom.clear_done) {
		Quickrate(76);
		return 1;
	}
	if (xcom.nelims){
		for (int i = 0; i < xcom.nelims; i++){
			GINT16 x = xcom.telims[i];
			zhou_solve.ClearCandidate_c(x.u8[1], x.u8[0]);
			if (opprint & 2) {
				cout << "deferred clearing" << x.u8[1] + 1 << cellsFixedData[x.u8[0]].pt << endl;
				xcom.txc[i].Print();
			}
		}
		Quickrate(xcom.hintrating);
		return 1;
	}

	return 0;
}
int PM_GO::Rate80Multi(int fast){
	if (opprint2 & 8)cout << "rate 80 multi chains" << endl;
	if (xysearch.SearchMulti(fast))	{
		if (fast)Quickrate(82);
		else Quickrate(xysearch.maxrating);
		return 1;
	}
	return 0;
}
int PM_GO::Rate85Dynamic(int fast){
	if (opprint2 & 8)cout << "rate 85 dynamic chains" << endl;
	if (xysearch.SearchDyn(fast))	{
		if (fast)Quickrate(85);
		else Quickrate(xysearch.maxrating);
		return 1;
	}
	return 0;
}
int PM_GO::Rate90DynamicPlus(int fast) {
	if (opprint2 & 8)cout << "rate 90 dynamic Plus chains" << endl;
	if (xysearchplus.SearchDynPlus(fast)) {
		if (fast)Quickrate(96);
		else Quickrate(xysearchplus.maxrating);
		return 1;
	}
	return 0;
}

/* full processing for 2 cells with adds in one object UR or UL 
can be bivalue, "hidden locket set" or "locked set"
always the lowest rating found
the rule has been copied from SE code analysis adjusted to lksudokuffixed8 veersion.

for hidden and naked sets, the lowest rating is taken depending on "n" values
summary of rating having "equalled" hidden and naked as in lksudoku 1.2.5.0

URUL hidden naked sets
cells -->  4    6    8   >=10
pair     4.6  4.7  4.8  5.1     
triplet  4.7  4.8  4.9  5.2     
quad     4.8  4.9  5.0  5.3     

cell1 cell2 digits(2) ul_plus other_digits_count other_digits(2) = 8 bytes
Rate45_URs finds URs and solve easy cases
Rate45_el finds UL
Rate2cellsGo solves URs with locked digit
Rate_ULs  solves ULs depending on the rating reached
Rate45Plus solves remaining URs ULs 2 cells depending on the rating reached
calls
Rate45_el UR UL common  process with 2 cells in a unit 
<<<<<<<<<<<<<<<<<<serate rule explaining the different ratings for the same see UniqueLoops file
// Look for naked sets
if (degree * 2 <= nbEmptyCells) {
// Look on combinations of cells that include c1 but not c2


*/
int PM_GO::Rate45_URs(GINT64 * t, int & nt){
	nt = 0;
	BF128 & uns = zh_g2.cells_unsolved_e;
	int iret = 0,tcells[4];
	int cellbox0[9] = { 0, 3, 6, 27, 30, 33, 54, 57, 60 };
	//PM_DATA & wmyd= (dynamic)? zpmd[1]:zpmd[0];
	for (int bande = 0; bande < 6; bande++){ // band/stack 1 to 3
		for (int ib1 = 0; ib1 < 2; ib1++) {  // box1
			int box1 = (bande < 3) ? 3 * bande + ib1 : (bande - 3 + 3 * ib1), startbox = cellbox0[box1];
			for (int rc1 = 0; rc1 < 2; rc1++){
				for (int cr1 = 0; cr1 < 3; cr1++){// first cell
					int cell1 = startbox + ((bande < 3) ? (9 * rc1 + cr1) : (9 * cr1 + rc1));
					if (uns.Off_c(cell1)) continue;
					int digs1 = zh_g2.dig_cells[cell1];
					for (int rc2 = rc1 + 1; rc2 < 3; rc2++){
						int cell2 = startbox + ((bande<3) ? (9 * rc2 + cr1) : (9 * cr1 + rc2));
						if (uns.Off_c(cell2)) continue;
						int digs2 = zh_g2.dig_cells[cell2]& digs1,
							digsor2= zh_g2.dig_cells[cell2] | digs1;
						if (_popcnt32(digs2) < 2) continue;
						for (int ib2 = ib1 + 1; ib2 < 3; ib2++){// box2
							int box2 = (bande < 3) ? (3 * bande + ib2) : (bande - 3 + 3 * ib2),
								startbox2 = cellbox0[box2];
							for (int cr2 = 0; cr2 < 3; cr2++){// last cells
								// set to turn  0 -> 1 -> 2 -> 3 -> 0
								int cell4 = startbox2 + ((bande<3) ? (9 * rc1 + cr2) : (9 * cr2 + rc1));
								int cell3 = startbox2 + ((bande<3) ? (9 * rc2 + cr2) : (9 * cr2 + rc2));
								int digs = zh_g2.dig_cells[cell3] & digs2 & zh_g2.dig_cells[cell4],
									digsor= zh_g2.dig_cells[cell3] | digsor2 | zh_g2.dig_cells[cell4];
								if (_popcnt32(digs) < 2) continue;
								// we have a potential UR in serate mode max 2 consecutive cells more than 2
								if (zh_g.pairs.Off_c(cell1) && zh_g.pairs.On_c(cell4)){
									tcells[0] = cell1; tcells[1] = cell2; tcells[2] = cell3; tcells[3] = cell4;
								}
								else if(zh_g.pairs.Off_c(cell2)){
									tcells[0] = cell2; tcells[1] = cell3; tcells[2] = cell4; tcells[3] = cell1;
								}
								else if (zh_g.pairs.Off_c(cell3)){
									tcells[0] = cell3; tcells[1] = cell4; tcells[2] = cell1; tcells[3] = cell2;
								}
								else if (zh_g.pairs.Off_c(cell4)){
									tcells[0] = cell4; tcells[1] = cell1; tcells[2] = cell2; tcells[3] = cell3;
								}
								else return 0; //should never be
								if (zh_g.pairs.Off_c(tcells[3])) continue; // not serate mode
								if (_popcnt32(digsor) == 3) {// one extra digit
									int extra_dig = digsor ^ digs;
									uint32_t exd; bitscanforward(exd, extra_dig);
									BF128 clean = cell_z3x[tcells[0]]; clean &= cell_z3x[tcells[1]];
									if (zh_g.pairs.Off_c(tcells[2]))clean &= cell_z3x[tcells[2]];
									clean &= zh_g2.pm.pmdig[exd];
									if (clean.isNotEmpty()) {
										iret = 1;
										if (opprint & 2)cout << "UR one extra digit "
											<< cellsFixedData[tcells[0]].pt << " " << cellsFixedData[tcells[1]].pt << " "
											<< cellsFixedData[tcells[2]].pt << " " << cellsFixedData[tcells[3]].pt << endl;
										zhou_solve.FD[exd][0] -= clean;
										continue;
									}
								}
								
								if (zh_g.pairs.Off_c(tcells[2]) ) continue; // not serate mode
								int c1 = tcells[0],c2 = tcells[1];
								int digc1 = zh_g2.dig_cells[c1], digc2 = zh_g2.dig_cells[c2];
								if (zh_g.pairs.On_c(c2)){// type 1 UR 
									iret = 1;
									if (opprint & 2)cout << "UR type1 " 
										<< cellsFixedData[tcells[0]].pt << " " << cellsFixedData[tcells[1]].pt << " "
										<< cellsFixedData[tcells[2]].pt << " " << cellsFixedData[tcells[3]].pt << endl;
									if (zh_g.triplets.On_c(c1))	zhou_solve.Setcell(c1);
									else zhou_solve.CleanCellForDigits(c1, digs);
									continue;
								}
								// not processed here, store it
								GINT64 & ws = t[nt++];
								ws.u64 = 0;
								ws.u8[0] = (uint8_t)c1;
								ws.u8[1] = (uint8_t)c2;
								ws.u16[1] = (uint16_t)digs;
								ws.u16[3] = (uint16_t)((digc1|digc2)^digs);
								ws.u8[5] = (uint8_t)_popcnt32(ws.u16[3]);
							}
						}
					}
				}
			}
		}
	}
	return iret;
}
int PM_GO::Rate2cellsGo(GINT64 & w){
	if (!w.u16[1]) return 0;;// closed digits sets to 0
	int cell1 = w.u8[0], cell2 = w.u8[1], digs = w.u16[1];
	// solve the bi-value case
	uint32_t d1, d2;
	bitscanforward(d1, digs); bitscanreverse(d2, digs);
	BF128 zcell = cell_z3x[cell1];		zcell &= cell_z3x[cell2];
	if ((zhou_solve.FD[d1][0] & zcell).isEmpty()){// d1 is a bi value kill d2
		zhou_solve.ClearCandidate_c(d2, cell1);
		zhou_solve.ClearCandidate_c(d2, cell2);
		w.u16[1] = 0;
		return 1;
	}
	else if ((zhou_solve.FD[d2][0] & zcell).isEmpty()){// d2 is a bi value kill d1
		zhou_solve.ClearCandidate_c(d1, cell1);
		zhou_solve.ClearCandidate_c(d1, cell2);
		w.u16[1] = 0;
		return 1;
	}
	else{// nothing more if the 2 cells are filled by other digits
		int digstrue = (1 << zh_g2.zerobased_sol[cell1]) | (1 << zh_g2.zerobased_sol[cell2]);
		if (!(digstrue & digs)){
			w.u16[1] = 0;
		}
	}
	return 0;
}
int PM_GO::Rate45_2cells(GINT64 * t, int & nt){
	int iret = 0;
	if (!nt) return 0;
	for (int i = 0; i < nt; i++){
		if (!t[i].u16[1]) continue;// closed digits sets to 0
		if (Rate2cellsGo(t[i]))iret = 1;
	}
	// clear the list of URs to process
	int n = nt; nt = 0;
	for (int i = 0; i < n; i++) if (t[i].u16[1]) t[nt++] = t[i];
	return iret;
}
int PM_GO::Rate45Plus(GINT64 * t, int  nt, int plus){
	int iret = 0; 
	for (int iur = 0; iur < nt; iur++){
		det_mess = "empty mess";
		GINT64 & w = t[iur];
		if (!w.u16[1]) continue;// closed (digits sets to 0)
		//if ((int)w.u8[4]>plus)continue; // not yet this UL but not the right test
		int cell1 = w.u8[0], cell2 = w.u8[1], nothers = w.u8[5];
		int degree = nothers;
		if (degree< plus) degree = plus; //crazy but  to copy serate mode
		CELL_FIX cf1 = cellsFixedData[cell1], cf2 = cellsFixedData[cell2];
		if (opprint & 2)cout << "UR/UL  cells " << cf1.pt << "  " << cf2.pt
			<< " rating=" << hint.rating_done << endl;
		if (cf1.eb == cf2.eb) if (Rate45_el(w, cf1.ebu, degree)) goto ok;
		if (cf1.el == cf2.el) if (Rate45_el(w, cf1.el,degree)) goto ok;
		if (cf1.pl == cf2.pl) if (Rate45_el(w, cf1.plu,degree)) goto ok;
		continue;
		ok:
		iret++;
		if (opprint & 2)cout << det_mess << " cells " << cf1.pt << "  " << cf2.pt
			<<" rating="<< hint.rating_done<< endl;
		w.u16[1] = 0;// kill it 
	}
	return iret;
}
int PM_GO::Rate45_el(GINT64 & t, int unit,int degree){
	int locdiag = 1;
	// voir ici tentative de rallier le rating serate
	// le rating demarre  sur le nombre de "digitsothers"
	// et on serait limit� � nb cases >= 2*digitsothers pour naked > sec pour hidden

	BF128 wcells = zhou_solve.cells_unsolved;
	wcells &= units3xBM[unit];
	int wcells_nserate = wcells.Count();
	int go_naked = wcells_nserate - 2 * degree;


	int iret = 0, cell1 = t.u8[0], cell2 = t.u8[1], digs = t.u16[1]
		, ul_plus = t.u8[4]	,	rbase = 45 + ul_plus;

	if (locdiag)	cout << " Rate45_el cells=" << cellsFixedData[cell1].pt << " " << cellsFixedData[cell2].pt << " unit=" << unit
		<< " ul_plus=" << ul_plus << " degree=" << degree << " rbase=" << rbase 
			<<" go_naked="<<go_naked<< endl;


	if (go_naked < 0) return 0;// serate condition to search here

	BF128  cells_ur, cells_others_ur, cells_3;
	cells_ur.clear(); cells_others_ur.clear(); cells_3.clear();
	wcells.Clear_c(cell1); wcells.Clear_c(cell2);// now cells to consider

	int digits_ur = (zh_g2.dig_cells[cell1] | zh_g2.dig_cells[cell2]),
		digitsothers = digits_ur & ~digs;// all digits of the UR not base digits
	int nothers = _popcnt32(digitsothers);
	if (rbase  > hint.rating) return 0;


	for (int idig = 0; idig < 9; idig++){// find cells
		register int bit = 1 << idig;
		BF128 w = zh_g2.pm.pmdig[idig] & wcells;
		if (w.isEmpty())continue;
		if (digs & bit)cells_ur |= w;
		if (digitsothers & bit)cells_others_ur |= w;
		if (!(digits_ur & bit))cells_3 |= w;
	}
	BF128 w_c = wcells - cells_ur - cells_others_ur;

	if (0 &&locdiag){
		char ws[82];
		cout << cells_ur.String3X(ws) << " cells_ur" << endl;
		cout << cells_others_ur.String3X(ws) << " cells_others_ur" << endl;
		cout << cells_3.String3X(ws) << " cells_3" << endl;
		cout << w_c.String3X(ws) << " w_c" << endl;
	}

	//============================ plus 1 hidden naked pair
	if (cells_ur.Count() == 1 && go_naked){// hidden pair see if active
		int cell = cells_ur.getFirstCell(), dcell = zh_g2.dig_cells[cell];
		if (dcell != digs){// some elimination
			zhou_solve.CleanCellForDigits(cell, dcell^digs);
			hint.Done(rbase );	det_mess = " hidden pair ";
			return 1;
		}
	}
	rbase++; //======= naked pair mini 4.6
	if (rbase  > hint.rating) return iret;

	BF128 wnaked = cells_others_ur - cells_3 - cells_ur,
		wclean = wcells - wnaked;
	if (wclean.isNotEmpty()){// possible active naked set
		if (nothers == 2 && wnaked.Count() == 1 && cells_others_ur.Count()>1){// active naked pair ?
			for (int i = 0; i < 9; i++) if (digitsothers & (1 << i))
				zhou_solve.FD[i][0] -= wclean;
			hint.Done(rbase); det_mess = " naked pair ";
			return 1;
		}
	}
	BF128 w_b = cells_others_ur - cells_ur;// only extra digits in w_c

	int dfree = 0x1ff ^ digits_ur, tfree[10], nfree = 0; // digits that can contribute ot the 2+2
	for (int i = 0; i < 9; i++) if ((dfree & (1 << i)) && (wcells & zh_g2.pm.pmdig[i]).isNotEmpty()) tfree[nfree++] = i;

	//=== serate degree is now 2 mini 5 cells for a hidden, 4 for a naked
	//============== same rating 4.6 can be hidden triplet 
	BF128 serate_filter = cells_ur &(zh_g2.pm.pmdig[cell1] | zh_g2.pm.pmdig[cell2]);
	if (serate_filter.Count() != 2) goto skip_hiddentriplet;
	if (nfree&& go_naked){// minimum to have a hidden triplet and serate filter
		for (int i1 = 0; i1 < nfree; i1++){
			BF128 wd1 = cells_ur | (wcells & zh_g2.pm.pmdig[tfree[i1]]);
			if (wd1.Count() != 2) continue;
			int aig = 0, dw = digs | (1 << tfree[i1]);
			for (int i = 0; i < 9; i++) if (!(dw & (1 << i))){
				BF128 clean = zh_g2.pm.pmdig[i] & wd1;
				if (clean.isNotEmpty()){
					aig = 1;
					zhou_solve.FD[i][0] -= clean;
				}
			}
			if (aig){ hint.Done(rbase); det_mess = " hidden triplet "; return 1; }
			return 0;// hidden quad not active, nake quad granted not active
		}
	}
	skip_hiddentriplet:
	if (wcells_nserate < 6) return iret; // mini now 2 triplet 1+2 1+2
	rbase++; //=====================================plus2 naked triplet hidden quad
	if (rbase  > hint.rating) return iret;
	if (locdiag)cout << "w_b.Count()=" << w_b.Count() << " nothers=" << nothers << " wnaked compte="
		<< wnaked.Count() << " nfree=" << nfree << endl;

	//================================ plus2 naked triplet go


	if (w_b.Count() < 2) goto go_plus3; // could not be active triplet

	if (nothers == 3 && wnaked.Count() == 2 && cells_others_ur.Count()>2){// active naked triplet
		for (int i = 0; i < 9; i++) if (digitsothers & (1 << i))
			zhou_solve.FD[i][0] -= wclean;
		hint.Done(rbase); det_mess = " naked triplet "; return 1;
	}

	if (nothers - 2 || !nfree) goto go_plus3;
	if (locdiag)cout << "try  naked triplet 2+1" << endl;
	for (int i1 = 0; i1 < nfree; i1++){
		BF128 wd1 = w_b  & zh_g2.pm.pmdig[tfree[i1]];
		//cout << "try  naked triplet 2+1 dig plus=" << tfree[i1] + 1 << endl;
		//char ws[82];
		//cout << wd1.String3X(ws) << " possible cells" << endl;
		if (wd1.Count() >= 2) {		// check that no other digit is there
			for (int i2 = 0; i2 < nfree; i2++) if (i2 - i1){
				wd1 -= zh_g2.pm.pmdig[tfree[i2]];
			}
			if (wd1.Count() != 2) continue;
			//cout << wd1.String3X(ws) << " final cells" << endl;
			int  dw = digitsothers | (1 << tfree[i1]);
			BF128 clean = cells_others_ur | (zh_g2.pm.pmdig[tfree[i1]] & wcells);
			clean -= wd1;
			if (zhou_solve.CleanCellsForDigits(clean, dw)){
				hint.Done(rbase); det_mess = " naked triplet 2+1 "; return 1;
			}
		}
	}

go_plus3:
	if (locdiag)cout << "try  quads" << endl;
	if (wcells_nserate < 8) return iret; // mini now 2 quad 1+3 1+3
	rbase++; //================================ plus3 hidden/naked quad
	if (rbase  > hint.rating) return iret;
	//		__________ hidden	quad
	if (nfree >= 2){// minimum to have a hidden quad
		if (locdiag)cout << "try hidden  quad 2+2" << endl;
		for (int i1 = 0; i1 < nfree - 1; i1++){
			BF128 wd1 = cells_ur | (wcells & zh_g2.pm.pmdig[tfree[i1]]);
			if (wd1.Count()>3) continue;
			for (int i2 = i1 + 1; i2 < nfree; i2++){// test all pairs of digits
				BF128 wd2 = wd1 | (wcells & zh_g2.pm.pmdig[tfree[i2]]);
				if (wd2.Count() != 3) continue;
				int aig = 0, dw = digs | (1 << tfree[i1]) | (1 << tfree[i2]);
				for (int i = 0; i < 9; i++) if (!(dw & (1 << i))){
					BF128 clean = zh_g2.pm.pmdig[i] & wd2;
					if (clean.isNotEmpty()){
						aig = 1;
						zhou_solve.FD[i][0] -= clean;
					}
				}
				if (aig){ hint.Done(rbase-1); det_mess = " hidden quad 2+2 "; return 1; }
				return 0;// hidden quad not active, nake quad granted not active
			}
		}
	}
	//         ______________naked quad
	if (w_b.Count() < 2) return 0; // could not be active quad

	if (nothers == 4 && wnaked.Count() == 3 && cells_others_ur.Count()>3){// active naked quad
		for (int i = 0; i < 9; i++) if (digitsothers & (1 << i))
			zhou_solve.FD[i][0] -= wclean;
		hint.Done(rbase); det_mess = " naked quad "; return 1;
	}

	if (nothers == 3 && nfree) {//___________ naked quad 3+1
		if (locdiag)cout << "try  naked quad 3+1" << endl;
		for (int i1 = 0; i1 < nfree; i1++){
			BF128 wd1 = (w_b  & zh_g2.pm.pmdig[tfree[i1]]) | wnaked;
			//char ws[82];
			//cout << wd1.String3X(ws) << " possible cells dig " << tfree[i1]+1 << endl;
			if (wd1.Count() >= 3) {		// check that no other digit is there
				for (int i2 = 0; i2 < nfree; i2++) if (i2 - i1){
					wd1 -= zh_g2.pm.pmdig[tfree[i2]];
				}
				//cout << wd1.String3X(ws) << " final cells" << endl;
				if (wd1.Count() != 3) continue;
				int  dw = digitsothers | (1 << tfree[i1]);
				BF128 clean = cells_others_ur | (zh_g2.pm.pmdig[tfree[i1]] & wcells);
				clean -= wd1;
				if (zhou_solve.CleanCellsForDigits(clean, dw)){
					hint.Done(rbase); det_mess = " naked quad 3+1 "; return 1;
				}
			}
		}
	}


	//if (1) return iret; // next not ready


	if (nothers == 2 && nfree>1){// try 2 plus 2 extra digit
		if (locdiag)cout << "try  naked quad 2+2 " << endl;
		for (int i1 = 0; i1 < nfree - 1; i1++){
			BF128 wd1 = (w_b  & zh_g2.pm.pmdig[tfree[i1]]) | wnaked;
			for (int i2 = i1 + 1; i2 < nfree; i2++){
				BF128 pci1i2 = wcells &zh_g2.pm.pmdig[tfree[i1]] & zh_g2.pm.pmdig[tfree[i2]] & zh_g.pairs;
				BF128 wd2 = wd1 | (w_b  & zh_g2.pm.pmdig[tfree[i2]]);
				wd2 |= pci1i2;
				//char ws[82];
				//cout << wd1.String3X(ws) << " possible cells" << endl;
				if (wd2.Count() >= 3) {		// check that no other digit is there
					for (int i3 = 0; i3 < nfree; i3++) if (i3 - i1 && i3 - i2){
						wd2 -= zh_g2.pm.pmdig[tfree[i3]];
					}
					if (wd2.Count() != 3) continue;
					//cout << wd1.String3X(ws) << " final cells" << endl;
					int  dw = digitsothers | (1 << tfree[i1]) | (1 << tfree[i2]);
					BF128 clean = cells_others_ur | ((zh_g2.pm.pmdig[tfree[i1]] | zh_g2.pm.pmdig[tfree[i2]]) & wcells);
					clean -= wd2;
					if (zhou_solve.CleanCellsForDigits(clean, dw)){
						hint.Done(rbase); det_mess = " naked quad 2+2 "; return 1;
					}
				}
			}

		}
	}
	return iret;
}
int PM_GO::RateUL_base(STORE_UL & wul){// try to rate the UL table
	int locdiag = 0;
	int cell1 = (int)wul.ur2.u8[0], cell2 = (int)wul.ur2.u8[1],
		digs = wul.ur2.u16[1] , digc1 = zh_g2.dig_cells[cell1];

	if (locdiag)cout << "rate_uls base  plus=" << " wul.type=" << wul.type << " my_plus=" << (int)wul.ur2.u8[4]
		<< " c1=" << cellsFixedData[cell1].pt << " c2=" << cellsFixedData[cell2].pt << endl;
	switch (wul.type){
	case 0:{// one cell with digits in excess
		if (_popcnt32(digc1) == 3)zhou_solve.Setcell(cell1);
		else zhou_solve.CleanCellForDigits(cell1, digs);
		if (opprint & 2)wul.Print("type 1 UL ");
		wul.ur2.u16[1] = 0;
		return 1;
	}
	case 1:{// one digit in excess elim  is not empty 
		zhou_solve.FD[wul.digit_one][0] -= wul.one_digit_elims;
		if (opprint & 2)wul.Print("one active extra digit ");
		wul.ur2.u16[1] = 0;
		return 1;
	}
	case 2:{// 2 cells same unit same as for a UR
		if (opprint2 & 2)wul.Print("call common process  ");
		if (Rate2cellsGo(wul.ur2)){
			if (opprint2 & 2)wul.Print("2 cells one bi-value  ");
			wul.ur2.u16[1] = 0;
			return 1;
		}
		if (!wul.ur2.u16[1])  {
			if (opprint & 1)cout << "killed no digit of the URr in the solution  " << endl;
		}
		return 0;;

	}//end case2
	}//end switch

	return 0;
}
int PM_GO::Rate46_Find_ULs(){
	int locdiag =0;
	struct SPOT{
		BF128 loop, pairs, others, wgo, bf_one_digit,
			parity[2];// lock loop with no solution generation ignored by serate
		int  digst,parity_rcb;
		int more_one, nplus, cellfirstplus, cellsecondplus;
		inline void Init(BF128 & wpu,BF128 & wp,int cell1,int cell2){ 
			memset(this, 0, sizeof (*this));
			//more_one = nplus = 0; 
			parity[0] = cell_z3x[cell2];// can not reenter with even value of ispot
			parity[1] = cell_z3x[cell1];// can not reenter with odd value of ispot
			pairs = wp; 
			loop = wpu; 
			loop.Set_c(cell1); 
			parity_rcb = C_rbc27[cell1] ^ C_rbc27[cell2];
		}
	}spots[20], *s, *sp;
	ntul = 0;
	if(locdiag>1)cout << "start uls search "  << endl;
	for (int idig1 = 0; idig1 < 8; idig1++){
		BF128 & wp1 = zh_g2.digits_cells_pair_bf[idig1];
		if (wp1.Count() < 2)continue;
		for (int idig2 = idig1+1; idig2 < 9; idig2++){
			BF128  wp = wp1 &zh_g2.digits_cells_pair_bf[idig2];
			if (wp.Count() < 2)continue;
			if (locdiag)cout << "search " << idig1 + 1 << idig2 + 1 << endl;
			for (int iu = 0; iu < 18; iu++){// look for a start
				BF128 wpu = units3xBM[iu]; wpu &= wp;
				if (wpu.Count() != 2) continue;
				if (locdiag)cout << "search unit" << iu  << endl;
				s = spots;
				int cell1 = wpu.getFirstCell(), cell2;
				uint32_t digit_one=0;
				wpu.Clear_c(cell1);
				cell2 = wpu.getFirstCell();
				s->Init(wpu, wp, cell1, cell2);
				s->digst = zh_g2.dig_cells[cell1];
				BF128 allcells = zh_g2.pm.pmdig[idig1] & zh_g2.pm.pmdig[idig2];
				allcells.Clear_c(cell1); // never in wgo
				int ispot = 0, scell, ndigst;
				s->wgo = allcells - s->loop;
				s->wgo &= cell_z3x[cell2];// chain cell2
				if (locdiag>1){
					char ws[82];
					cout << "search " << idig1 + 1 << idig2 + 1 << " unit=" << iu
						<< " " << cellsFixedData[cell1].pt << " " << cellsFixedData[cell2].pt << endl
						<< Char27out(s->parity_rcb) << " start parity" << endl;
					cout << s->wgo.String3X(ws) << " wgo at start"  << endl;
					cout << s->loop.String3X(ws) << " loop at start" << endl << endl;
				}
				goto next;
				{//======================== loop expansion
				nextspot:
					if (ispot > 18) return 0; // safety control
					sp = s++;		ispot++;		*s = *sp;
					{
					int digs = zh_g2.dig_cells[scell], ndigs = _popcnt32(digs);
					s->loop.Set_c(scell);
					if(locdiag>1){
						char ws[82];
						cout << s->loop.String3X(ws) << " ispot=" << ispot << " " << cellsFixedData[scell].pt << endl;
					}
					s->digst |= digs;
					s->parity[ispot & 1] |= cell_z3x[scell];
					s->parity_rcb ^= C_rbc27[scell];
					ndigst = _popcnt32(s->digst);
					// prepare next step
					if (ndigs > 2){//============== extra digits 
						s->nplus++;
						if (ndigst > 3){// in serate, only 2 cells same units
							if (s->nplus > 2) goto back;
							if (s->nplus == 1) s->cellfirstplus = scell;
							else{// the 2 cells must be in the same unit.
								BF128 w = cell_z3x[scell];
								if (w.Off_c(s->cellfirstplus)) goto back;
								s->cellsecondplus = scell;
							}
						}
						else{// only one digit in excess, see if possible elimination
							if (!s->more_one){// first cell init the digit pattern 
								s->cellfirstplus = scell;
								s->more_one = s->digst^spots[0].digst;
								bitscanforward(digit_one, s->more_one);
								s->bf_one_digit = zh_g2.pm.pmdig[digit_one];
							}
							s->cellsecondplus = scell;
							s->bf_one_digit &= cell_z3x[scell];
							if (s->nplus > 1 && s->bf_one_digit.isEmpty()) goto back;
						}
					}
					} //end of int digs scope

					s->wgo = allcells - s->loop;
					s->wgo &= cell_z3x[scell];// chain scell
					s->wgo -= s->parity[(ispot - 1) & 1];// would be a no solution loop 
					if (locdiag>1){
						char ws[82];
						cout << s->wgo.String3X(ws) << " wgo ispot=" << ispot << endl;
						cout << s->loop.String3X(ws) << " loop" << endl;
						cout << Char27out(s->parity_rcb) << " parity rcb" << endl;
					}
					if (ispot < 4) goto next;// minimum 6 cells
					if (!s->parity_rcb){// this is a loop
						//if (ispot < 4) goto nextidig2;// minimum 6 cells
						if (locdiag){
							cout << "loop seen ispot=" << ispot << endl;
							char ws[82];
							cout << s->loop.String3X(ws) << " pattern loop"<<endl;
						}
						if (ntul){
							for (int i = 0; i < ntul;i++)
								if (s->loop == tul[i].cells)goto back;
						}
						int ncount = s->loop.Count(),
							ser_ul = (ncount - 4) >> 1;  // one cell appears twice
						//URUL cells -->  4    6    8   >=10
						if (ser_ul>2) ser_ul = 5;    // =5 if >= 10
						STORE_UL &wsul = tul[ntul++];
						if (locdiag) cout << "loop added nplus= "<< s->nplus<< " ndigst="<<ndigst << endl;
						wsul.cells = s->loop;
						wsul.one_digit_elims = s->bf_one_digit;
						wsul.ur2.u8[0] = (uint8_t)s->cellfirstplus;
						wsul.ur2.u8[1] = (uint8_t)s->cellsecondplus;
						wsul.ur2.u16[1] = (uint16_t)spots[0].digst;
						wsul.ur2.u8[4] = (uint8_t)ser_ul;
						wsul.type = 0;//0 1 cell 1 one digit 2 2 cells
						wsul.digit_one = digit_one;
						if (s->nplus == 1)goto nextidig2;
						wsul.type++; if (ndigst == 3)goto nextidig2;
						// now 2 cells same unit with extra digits
						BF128 z = cell_z3x[wsul.ur2.u8[0]];
						z &= cell_z3x[wsul.ur2.u8[1]];
						if (z.isEmpty())ntul--;// skipped if not same unit
						if (locdiag) cout << "loop added 2 cells same unit" << endl;
						wsul.ur2.u16[3] = (zh_g2.dig_cells[wsul.ur2.u8[0]]| zh_g2.dig_cells[wsul.ur2.u8[1]])
							^ wsul.ur2.u16[1];
						wsul.ur2.u8[5] = (uint8_t)_popcnt32(wsul.ur2.u16[3]);
						wsul.type++;// goto nextidig2;// only on valid UL per pair is enough
					}
				next:
					while ((scell = s->wgo.getFirstCell()) >= 0){
						s->wgo.Clear_c(scell);
						goto nextspot;
					}
					goto back;
				back:
					while (--ispot >= 0){
						s--; goto next;
					}
				}

			}
		nextidig2:;
		}//end idig2

	}//end idig1
	if(locdiag)cout << "exit uls ntul=" << ntul << endl;
	return ntul;
}
int PM_GO::Rate_ULs(int plus45){// try to rate the UL table
	int locdiag = 0;
	if(locdiag)cout << "entry rate_uls ntul=" << ntul <<" plus45="<<plus45<< endl;
	int iret = 0,ntuln=0;
	for (int iul = 0; iul < ntul; iul++){// all pending ul
		STORE_UL & wul = tul[iul];
		if ((int)wul.ur2.u8[4]>plus45)goto skip;// not yet the time to process it
		{
		int cell1 = (int)wul.ur2.u8[0], cell2 = (int)wul.ur2.u8[1],
			digs = wul.ur2.u16[1], ul_plus = wul.ur2.u8[4], digc1 = zh_g2.dig_cells[cell1];
		if(locdiag)cout <<"iul="<<iul<< " rate_uls plus=" << plus45 
			<< " wul.type=" << wul.type 	<< " my_plus=" << (int)wul.ur2.u8[4] 
			<< " c1=" << cellsFixedData[cell1].pt << " c2=" << cellsFixedData[cell2].pt << endl;
		switch (wul.type){
		case 0:{// one cell with digits in excess
			if (_popcnt32(digc1) == 3)zhou_solve.Setcell(cell1);
			else zhou_solve.CleanCellForDigits(cell1, digs);
			iret = 1;
			hint.Done(45 + ul_plus);
			if (opprint2 & 2)wul.Print("type 1 UL ");
			goto dead;
		}
		case 1:{// one digit in excess elim must is not empty 
			zhou_solve.FD[wul.digit_one][0] -= wul.one_digit_elims;
			iret = 1;
			hint.Done(45 + ul_plus);
			if (opprint2 & 2)wul.Print("one active extra digit ");
			goto dead;
		}
		case 2:{// 2 cells same unit same as for a UR
			if (ul_plus == plus45)goto skip;
			if (opprint2 & 2)wul.Print("call common process  ");
			if (Rate2cellsGo(wul.ur2)){
				if (opprint2 & 2)wul.Print("2 cells one bi-value  ");
				hint.Done(45 + ul_plus);
				iret = 1;
				goto dead; 
			}
			if (!wul.ur2.u16[1])  {
				if (opprint2 & 2)cout << "killed no digit of the URr in the solution  " << endl;
				goto dead;
			}
			if (Rate45Plus(&wul.ur2, 1, plus45)){
				if (opprint2 & 2)wul.Print("2 cells active hidden/naked set  ");
				iret = 1;
				goto dead;
			}
			if (wul.ur2.u16[1]) goto skip; else goto dead;

		}//end case2
		}//end switch
		}//end cell1 scope
	skip://just copy for later use
		if (iul > ntuln)tul[ntuln++] = tul[iul];
		else ntuln++;
	dead:;// closed for this ul
	}
	ntul = ntuln; // still active uls
	return iret;
}
int PM_GO::Rate56BUG() {
	if (opprint & 1){	cout << "start bug analysis" << endl; }
	if (bug.Init())	return 0;   // not a BUG pattern
	if (opprint & 1)cout << "bug pattern" << endl;
	int iret = 0, cell1 = bug.tplus[0];
	if (bug.ntplus == 1){		//======================= bug type 1
		if (opprint & 2)cout << "bug type 1" << endl;
		zhou_solve.Setcell(cell1);
		hint.Done(56);
		return 1;
	}
	if (_popcnt32(bug.or_change) == 1){	//======================= bug type 2 same digit
		uint32_t dig1; bitscanforward(dig1, bug.change_plus[0]);
		if (opprint & 2)cout << "bug type 2 same digit=" << dig1 + 1 << endl;
		if (0){
			char ws[82];
			cout << bug.zz.String3X(ws) << " to clean" << endl;
			cout << zhou_solve.FD[dig1][0].String3X(ws) << " dig pm" << endl;
			zhou_solve.ImageCandidats();
		}
		if ((zhou_solve.FD[dig1][0] & bug.zz).isNotEmpty()){// zz is not empty
			zhou_solve.FD[dig1][0] -= bug.zz;
			hint.Done(57);
			return 1;
		}
	}
	//=========== now must take place in a unit
	for (int iu = 26; iu >= 0; iu--){ // serate seems to give the priority to boxes
		BF128 wu = units3xBM[iu]; wu &= zhou_solve.cells_unsolved;
		if (!bug.wplus.isSubsetOf(wu))continue;
		if (opprint & 2)cout << "bug type 3/4 see unit=" << iu << endl;
		//must have self eliminations with bug.or_change
		BF128 wu_pairs = wu&zh_g.pairs, wupw = wu_pairs;// cells pair of the unit
		int or_pairs = 0, tpairs[10],tpairs_digits[10], ntpairs = 0,cell;
		int nmix = 0, nchange = 0, 
			changet=bug.or_change,
			changen=0x1ff^changet,
			count_change = _popcnt32(changet);
		while ((cell = wupw.getFirstCell()) >= 0){// analyse and store pairs
			wupw.Clear_c(cell);
			register int d = zh_g2.dig_cells[cell];
			or_pairs |= d;
			//if (!(d&changet)) continue; // forget cells with only digits not change in plus cells
			tpairs[ntpairs] = cell;
			tpairs_digits[ntpairs++] = d;
			if ((d&changet) && (d&changen)) nmix++;			
			else if(d&changet) nchange++;
		}

		//	.....1.23....2.456...6..78...8..3..5.4.....3.6..2..1...27..4...893.7....46.8.....
		//================================== locked digit
		int locked_d = bug.or_plus_tot & ~or_pairs;
		if (bug.ntplus == 2 && locked_d){// locked digits 2 cells
			if (opprint & 2)cout << "bug 3/4 locked digit(s) in plus cell(s) locked 0"
				<< oct << locked_d<<dec << endl;
			for (int i = 0; i < bug.ntplus; i++){// keep  locked plus change 
				int digbf = bug.tplus_digits[i] ^ locked_d;
				uint32_t digit; bitscanforward(digit, digbf);
				zhou_solve.ClearCandidate_c(digit, bug.tplus[i]);
			}
			hint.Done(57);
			return 1;
		}
		if (opprint & 1)cout << oct << "0" << changet << " change count=" << dec << count_change << endl;
		if (opprint & 1)cout << "nchange=" << nchange << " nmix=" << nmix << " ntpairs=" << ntpairs << endl;
		// note if 2 cells and one digit in both, ( here not a single digit) could be erased
		// likely same for more than 2 cells similar to APE rated 6.2
		//======================== now looking for naked pair triplet quad 5
		if (count_change > ntpairs)continue;// nothing to do in this unit
		if (nmix + nchange < 2) continue; // would not be active
		if (count_change > 2) goto triplet;
		for (int i1 = 0; i1 < ntpairs; i1++){
			if (tpairs_digits[i1] == changet){
				hint.Done(58);
				if (opprint & 2)cout << "bug 3/4 naked pair cell " << cellsFixedData[tpairs[i1]].pt << endl;
				BF128 cells_clean = wu_pairs; cells_clean.Clear_c(tpairs[i1]);
				if (zhou_solve.CleanCellsForDigits(cells_clean, changet))return 1;// should always be yes
				return 0;// safety code
			}
		}
	triplet:
		if (ntpairs < 3) continue; // would not be active
		if (count_change > 3) goto quad;
		for (int i1 = 0; i1 < ntpairs - 1; i1++){
			for (int i2 = i1 + 1; i2 < ntpairs; i2++){
				int digt = tpairs_digits[i1] | tpairs_digits[i2] | changet;
				if (_popcnt32(digt) ==3){
					hint.Done(59);
					if (opprint & 2)cout << "bug 3/4 naked triplet" << endl;
					BF128 cells_clean = wu_pairs; cells_clean.Clear_c(tpairs[i1]); cells_clean.Clear_c(tpairs[i2]);
					if (zhou_solve.CleanCellsForDigits(cells_clean, digt))return 1;// should always be yes
					goto quad; 
				}

			}
		}
	quad:
		if (ntpairs < 4) continue; // would not be active
		if (count_change > 4) goto r61;
		for (int i1 = 0; i1 < ntpairs - 2; i1++){			
			int dig1 = tpairs_digits[i1]|changet;
			for (int i2 = i1 + 1; i2 < ntpairs - 1; i2++){
				int dig2 = dig1 | tpairs_digits[i2];
				for (int i3 = i2 + 1; i3 < ntpairs; i3++){
					int digt = dig2 | tpairs_digits[i3];
					if (_popcnt32(digt) == 4){
						hint.Done(60);
						if (opprint & 2)cout << "bug 3/4 naked quad" << endl;
						BF128 cells_clean = wu_pairs; cells_clean.Clear_c(tpairs[i1]); 
						cells_clean.Clear_c(tpairs[i2]); cells_clean.Clear_c(tpairs[i3]);
						if (zhou_solve.CleanCellsForDigits(cells_clean, digt))return 1;// should always be yes
						goto r61; 
					}
				}
			}
		}

	r61:
		if (ntpairs < 5) continue; // would not be active
		for (int i1 = 0; i1 < ntpairs - 3; i1++){
			int dig1 = tpairs_digits[i1] | changet;
			for (int i2 = i1 + 1; i2 < ntpairs -2; i2++){
				int dig2 = dig1 | tpairs_digits[i2];
				for (int i3 = i2 + 1; i3 < ntpairs-1; i3++){
					int dig3 = dig2 | tpairs_digits[i3];
					for (int i4 = i3 + 1; i4 < ntpairs; i4++){
						int digt = dig3 | tpairs_digits[i4];
						if (_popcnt32(digt) == 5){
							hint.Done(61);
							if (opprint & 2)cout << "bug 3/4 naked (5)" << endl;
							BF128 cells_clean = wu_pairs; cells_clean.Clear_c(tpairs[i1]);
							cells_clean.Clear_c(tpairs[i2]); cells_clean.Clear_c(tpairs[i3]);
							cells_clean.Clear_c(tpairs[i4]);
							if (zhou_solve.CleanCellsForDigits(cells_clean, digt))return 1;// should always be yes
							return 0;// safety code
						}
					}
				}
			}
		}
	}
	return iret;
}
int PM_GO::Rate62_APE(){
	if (opprint & 1){ cout << "start AlignedPairExclusion" << endl; }
	int iret = 0;
	for (int iband = 0; iband < 6; iband++){
		BF128 bpairs = band3xBM[iband]; bpairs &= zh_g.pairs;
		if (bpairs.Count() < 3) continue;// Ywing is already 3 (solved if any)
		BF128 base = band3xBM[iband]; base &= zhou_solve.cells_unsolved;
		BF128 digs_pairs[9];
		int npdigs[9],active_digs=0;
		base -= bpairs; // minimum triplet as base 
		if (base.isEmpty()) continue;
		for (int idig = 0; idig < 9; idig++){//Find possible active digits
			digs_pairs[idig] = zh_g2.pm.pmdig[idig]&bpairs;
			npdigs[idig] = digs_pairs[idig].Count();
			if (npdigs[idig] >= 3)active_digs |= 1<<idig;
		}
		if (!active_digs) continue; //no APE to come
		if (opprint & 1){
			cout << "study band  " << iband
				<< " active digs 0" << oct << active_digs << dec << endl;
		}
		int cell_base;// source where on digit can be cleaned (any number of digits
		while ((cell_base = base.getFirstCell()) >= 0){
			base.Clear_c(cell_base);
			int cbase_digs = zh_g2.dig_cells[cell_base]; // must have links to pairs

			BF128 wp = cell_z3x[cell_base]; wp &= bpairs;// pairs seen by cell_base
			// try to find a digit to check
			for (int idig = 0; idig < 9; idig++){
				int bit = 1 << idig;
				if (!(active_digs & bit)) continue;
				BF128 wd = wp & digs_pairs[idig];
				BF128 seen = band3xBM[iband]; seen &= zh_g2.pm.pmdig[idig];
				int wdigs = cbase_digs;
				if (wdigs & bit){// the base contains the clearing digit
					seen &= cell_z3x[cell_base];
					wdigs ^= bit;
				}
				if (wd.Count() < (int) _popcnt32(wdigs))continue; // = is the minimum
				if (0 &&opprint2 & 2)
				cout << "more for digit " << idig + 1 << " cell base " << cellsFixedData[cell_base].pt << endl;
				// digit of the base cell must see a pair of the digit
				for (int idig2 = 0; idig2 < 9; idig2++) if (wdigs & (1 << idig2)){
					BF128 wd_dig2 = wd&zh_g2.pm.pmdig[idig2];
					if (wd_dig2.isEmpty()) goto nextidig;
					BF128 wseen; wseen.SetAll_0();//  must or if several pairs
					int cellp;
					while ((cellp = wd_dig2.getFirstCell()) >= 0){
						wd_dig2.Clear_c(cellp);
						wseen |= cell_z3x[cellp];
					}
					seen &= wseen;
					if (seen.isEmpty()) goto nextidig;
				}
				// this is a valid ape 
				if (opprint & 2) {
					char ws[82];
					cout <<seen.String3X(ws)<< " Align pair exclusion PE for digit"<< idig+1 << endl;
				}

				zhou_solve.FD[idig][0] -= seen;
				iret = 1;
			nextidig:;
			}
		}
	}
	return iret;
}
int PM_GO::Rate75_ATE() {
	int locdiag = 0;
	if (locdiag){
		cout << "start search aligned triplet" << endl;
	}
	int iret = 0;
	BF128 z23t,	// set of cells that have 2 or 3 candidates (could be excluding cells)
		zbaset;	// set of cells that are visible by one potential excluding cells
	z23t.SetAll_0(); zbaset.SetAll_0();
	for (int i = 0; i < 81; i++) {
		int ncd = _popcnt32(zh_g2.dig_cells[i]);
		if (ncd < 2 || ncd > 3)			continue;
		z23t.Set_c(i);
		zbaset |= cell_z3x[i];
	}
	zbaset &= zh_g2.cells_unsolved_e; // possible base cells reduced to cells that have no value
	for (int iband = 0; iband < 6; iband++){
		BF128 z23 = z23t&band3xBM[iband], zbase = zbaset&band3xBM[iband],usedi1;
		int i1, i2, i3,see_1_2,see_1_3,see_2_3;
		while ((i1 = zbase.getFirstCell()) >= 0){
			zbase.Clear_c(i1);
			BF128 zbase2 = zbase;
			while ((i2 = zbase2.getFirstCell()) >= 0){
				zbase2.Clear_c(i2);
				BF128 z1 = cell_z3x[i1], z2 = cell_z3x[i2],
					zand = z1&z2, zor = z1 | z2, z23or = z23 & zor;
				if ((zand&z23).Count() < 2)	continue; // must add 2 z23 cells
				see_1_2 = z1.On_c(i2);
				if (0){
					cout << cellsFixedData[i1].pt << " " << cellsFixedData[i2].pt << endl;
				}
				while ((i3 = z23or.getFirstCell()) >= 0){// third base in z23 sees one of i1,i2
					z23or.Clear_c(i3);
					BF128 z23f = cell_z3x[i3]; // exclusion sees the 3 base cells
					z23f &= (zand&z23);// where to take exclusion cells
					int texclude[81], nexclude = z23f.Table3X27(texclude);
					see_1_3 = z1.On_c(i3); see_2_3 = z2.On_c(i3);
					uint32_t d1, d2, d3; // digit of triplets to check
					int cd1 = zh_g2.dig_cells[i1], find1 = 0, find2 = 0, find3 = 0;
					while ( cd1){
						bitscanforward(d1, cd1);
						int bit1 = 1 << d1;
						cd1^=bit1;// erase the bit
						int cd2 = zh_g2.dig_cells[i2];
						if (see_1_2) cd2 &= ~bit1;// bit1 dead if same region
						while (cd2){
							bitscanforward(d2, cd2);
							int bit2 = 1 << d2;
							cd2^=bit2;// erase the bit
							int cd3 = zh_g2.dig_cells[i3];
							if (see_1_3) cd3 &= ~bit1;// bit1 dead if same region
							if (see_2_3) cd3 &= ~bit2;// bit2 dead if same region
							while ( cd3){// a possible triplet in base
								bitscanforward(d3, cd3);
								int bit3 = 1 << d3;
								cd3^=bit3;// erase the bit, now si if an excluding cell
								int nbits = ~(bit1 | bit2 | bit3);
								for (int ie = 0; ie < nexclude; ie++){
									if (!(zh_g2.dig_cells[texclude[ie]] & nbits))goto nextd3;
								}
								find1 |= bit1; find2 |= bit2; find3 |= bit3;
							nextd3:;// exit from the loop if excluded
							}// end3
						}// end2
					}// end1
					// check if a digit is never valid for the triplet
					find1 ^= zh_g2.dig_cells[i1];
					find2 ^= zh_g2.dig_cells[i2];
					find3 ^= zh_g2.dig_cells[i3];
					if (find1 || find2 || find3){//elimination found
						iret = 1;
						if (find1) zhou_solve.CleanCellForDigits(i1, find1);
						if (find2) zhou_solve.CleanCellForDigits(i2, find2);
						if (find3) zhou_solve.CleanCellForDigits(i3, find3);
						if (opprint & 2) {
							cout << "ATE exclusion cells " << cellsFixedData[i1].pt << " "
								<< cellsFixedData[i2].pt << " " << cellsFixedData[i3].pt
								<< " elims ";
							for (int idig = 0, bit = 1; idig < 9; idig++, bit <<= 1) {
								if (find1&bit) cout << idig + 1 << cellsFixedData[i1].pt << " ";
								if (find2&bit) cout << idig + 1 << cellsFixedData[i2].pt << " ";
								if (find3&bit) cout << idig + 1 << cellsFixedData[i3].pt << " ";
							}
							cout << endl;
						}					
					}
				}
			}
		}
	}
	return iret;
}

void ZH_GLOBAL2::DebugNacked(){
	if (zh_g.pairs.isNotEmpty()){
		char ws[82];
		zh_g.pairs.String3X(ws);
		cout << ws << " biv cells seen" << endl;
	}
	if (zh_g.triplets.isNotEmpty()){
		char ws[82];
		zh_g.triplets.String3X(ws);
		cout << ws << " triplets cells seen" << endl;
	}
	if (locked_nacked_brc_seen[0].isNotEmpty()){
		char ws[82];
		locked_nacked_brc_seen[0].String3X(ws);
		cout << ws << " naked pairs in box seen" << endl;
	}
	if (locked_nacked_brc_seen[1].isNotEmpty()){
		char ws[82];
		locked_nacked_brc_seen[1].String3X(ws);
		cout << ws << " naked pairs in row seen" << endl;
	}
	if (locked_nacked_brc_seen[2].isNotEmpty()){
		char ws[82];
		locked_nacked_brc_seen[2].String3X(ws);
		cout << ws << " naked pairs in column seen" << endl;
	}

}

int ZHOU::CheckStatus(){// check that the solution is still there
	//BF128 digit_sol[9];
	for (int id = 0; id < 9; id++){
		BF128 w = zh_g2.digit_sol[id] & FD[id][0];
		if (w.Count() != 9){
			cout << " missing solution digit for digit " << id + 1 << endl;
			char ws[82];
			w ^= zh_g2.digit_sol[id];
			cout << w.String3X(ws) << " digit pattern of missings" << endl;
			return 1;
		}
	}
	return 0;
}
