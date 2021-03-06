
//#include "main.h"

struct GINTBUF{//Storing variable lentgh chains .. start one 64 bit limit
	GINT * buf;
	int n32, ncur,nback;
	inline void Set(GINT * b, int en32){ buf = b; n32=en32; }
	inline void Init(){ ncur = 0; }
	int Get(int n);
	int Store(GINT * t);
	inline int InitGetback(){ nback = 0; return ncur; }
	int GetBack(GINT * d);
	void EraseOlds(int nkeep);
	inline void ResetTo(int x){ ncur = x; }
};
struct EXPLAIN_BUFFER{// kind of stack to store data for back path in a chain
	USHORT t[20000];
	int n, nr;
	inline USHORT * GetBuf(){ return &t[n]; }
	inline void Addn(int ne){ n += ne; }
	void StoreChain(SCAND * tc, int ntc)	{
		t[n++] =(USHORT) ntc;
		for (int i = 0; i<ntc; i++)
			t[n++] = tc[i];
	}
	inline USHORT * StoreStart(){ nr = n; return &t[n]; }
	inline void StoreCancel(){ n = nr; }

};//expbuf;

struct STORE_UL{
	BF128 cells, one_digit_elims;// pattern and elims if one digit
	GINT64 ur2;// ur 2 cells equivalent (see UR)
	int type,digit_one;// keep it nx128 bits

	void Print(const char * lib);
};
struct BUG{// in wpaires, data for bug processing
	int cell,  el_par_ch[27];    // parity of digits for pairs in units
	int or_change, or_plus_tot, wchange;
	BF128 zz, wplus;
	BF16 r_c, c_c, b_c;		// row,col,box having plus
	int tplus[10], tplus_digits[10], change_plus[10],ntplus;  // same in table limited to 8
	USHORT r_cn[9], c_cn[9], b_cn[9], // count plus per unit
		np, aigpmax, aigun;
	int  Init();
	int  DoChange(int iplus,  int el);

};

struct WWUR2{// solving UR/UL 2 cells in a unit
	int cell1, cell2, digs, ul_plus, digitsothers, nothers, digits_ur,
    target, rbase, go_naked, wcell_nserate, degree,unit,
    wcells_nserate,
    dfree,tfree[10],nfree;
	int locdiag;
	const char * det_mess;
	BF128 wcells, cells_ur, cells_others_ur, cells_3
    ,w_b,// cells with extra digits no common digit
    w_c,// cells with only free digits 
    wnaked,wclean ;
	inline void Set(GINT64 & t,const char * lib){
		cell1 = t.u8[0], cell2 = t.u8[1], digs = t.u16[1], ul_plus = t.u8[4], nothers= t.u8[5],
			digitsothers=t.u16[3],rbase = 45 + ul_plus;
		det_mess = lib;
		locdiag=0;
	}
	inline void SetTarget(int etarget, int edegree){
		degree = edegree, target = etarget;
	}
	void Init( int eunit);
	void InitFreeDigits();
	int Hidden_pair();
	int Naked_pair();
	int Hidden_triplet();
	int Naked_triplet();
	int Naked_triplet21();
	int Hidden_quad();
	int Naked_quad();
	int Naked_quad31();
	int Naked_quad22();
	int Go_serate(GINT64 & t,int etarget);
	int Go_serate_unit(int eunit,int hdegree, int ndegree);
	int Go_serate_Fast(GINT64 & t);
	int Go_serate_unit_Fast(int eunit);


};
struct XSTATUS{// fish data for fish processing
	int active, digit,maxpas,npas;// maxpas=20 at start
	int dig_sets[27];
	BF32 bivsets;// unit with bivalue 27 bits field
	BF128 pm, // all candidates of the digit
		pmbiv, // cells belonging to one or more bi values
		elims; // potential eliminations (through brute force
	BF128 expon, expoff;
	void Init(int dig);
	void AddStart(int xcell, int unit, GINT64 * t, int & nt,BF128 * tx,BF128 & x);
	int R65Xexpand(int xcell1, int xcell2, int loop, int * back,BF128 & loopend);
	int XCycle(int fast);
	int XChain(int fast);
	int CleanLoop(int * i,int nt);
	int CleanChain(int * i, int nt,BF128 & clean);
	int Nishio1();
	int Nishio2();
	int XexpandNishio(int cell1);
};

struct STORE_XLC{
	int dig, rating,t[20],nt,loop;
	BF128 pattern;
	void Print();
};
struct YLSEARCH{
	int idig, xcell1, xcell2, maxpas, ncells, 
		ncells1, mode, c0, c1, c2, locdiag, diag;
	unsigned long d2;
	BF128 loop;
	GINT64 tback[30];
	int Search(int fast=0);
	int SearchOut(int fast );
	int Expand();
	int ExpandOut();
	int CleanLoop();
	int CleanLoopOut();
	int Is_To_Clean(int rating);
	void PrintTback(const char * lib);
};
struct XYSEARCH{
	struct PATH{
		GINT64  t[400];// cell,digit,source,step
		int nt, dig,cell;
	}paths[9];
	
	int idig, digit,  cell, ddig,dcell,dind,
		nt,ntd,ntp, maxpas,maxrating, npaths,elim_done,
		nsteps, c1, c2, nback,
		locdiag, diag, mode,fastmode,opprint;
	int dig_sets[9][27];
	uint32_t d2;
	BF128 pairs, cells_biv_all, cells_all,cells_biv_true,  	dig_b_true[9];
	BF128 loop, wb;
	BF32 dig_bivsets[9],dig_sets3[9];
	GINT64 tback[300],t[400];// cell,digit,digit2,source
	PM3X used_off_digits, used_on_digits, cleang, cleanstart,
		active_all,active_unit,dbiv;
	GINT telims[50];	int ntelims ;
	//====== for dynamic and more
	int ind_pm[9][81],nind,is_contradiction,ntcands; // direct index for storing tables
	PM3X off_status[400],contradiction,elim_stored;
	GINT64 off_path[100][400];
	GINT tcands[400]; // candidates tables and index if belong to binary
	int tex[9][81]; // index to first "on" index in the path
	inline void SearchInit(int fast){
		fastmode = fast;
		elim_done = ntelims = 0;
		maxrating = 200;
		elim_stored.SetAll_0();
	}
	inline void Addt(int cell, int di, int source){
		GINT64 & tx = t[nt++];
		tx.u16[0] = (uint16_t)cell; tx.u16[1] = (uint16_t)di;
		tx.u16[2] = (uint16_t)source; tx.u16[3] = (uint16_t)nsteps;
	}
	inline void AddLastInCell(int cell, int di){
		GINT64 & tx = t[nt++];
		tx.u16[0] = (uint16_t)cell; tx.u16[1] = (uint16_t)di;
		tx.u16[2] = (uint16_t)(0x1000|cell); tx.u16[3] = (uint16_t)nsteps;
	}	
	inline void AddLastInUnit(int cell, int di,int unit){
		GINT64 & tx = t[nt++];
		tx.u16[0] = (uint16_t)cell; tx.u16[1] = (uint16_t)di;
		tx.u16[2] = (uint16_t)(0x2000|unit); tx.u16[3] = (uint16_t)nsteps;
	}
	void AddUnit(int unit, int source);
	void Init();
	void Init2();
	void InitCandidatesTable();
	int Search(int fast = 1);
	int SearchMulti(int fast);
	int MultiCell(int c0);
	int MultiCellExpand(int cell, int digit,PM3X & clean);
	int MultiUnit(int udigit, int unit);
	void OffToOn(int i);
	void OffToOn_Dyn(int i);
	void OnToOff(int i);
	void OnToOff_Dyn(int i);
	void StartMulti(int dig, int cell);
	void Expand_Multi(PM3X & cleanstart);
	int Expand_true_false();
	int CleanLoopOut();
	int Is_To_Clean(int rating);
	int CleanXYChain();
	int CleanXYLoop(GINT64 * t, int nt);
	int Do_Clean();
	int AddElim(int d, int c, int rat);
	int SearchDyn(int fast);
	void SearchDynPass(int nmax);
	void SearchDynPassMulti(int nmax);
	void ExpandDynamic(GINT cand);
	int ExpandDynamicToElim(GINT cand,GINT targ);
	int BackDynamic(GINT64 target, GINT64 * tb, int ntb);
	int BackDynamicOff(GINT target);
	void DynamicSolveContradiction(GINT cand,PM3X cont);
	void DebugDynamicSolveContradiction(const char * lib,GINT cand, GINT targ);
	void DynamicSolveContradiction(int dig1, int cell1, int dig2, int cell2, PM3X cont);
	void PrintTback();
	void PrintBackMulti(int elim_dig, int elim_cell);
	void PrintBackCom(const char * lib,GINT64 * ptback, int nback,int mode );
	void DebugT();
};

struct SETS_LINKS {// attempt to have a new design 
	BF128 dig_sets[50], dig_links[50], cells_sets,cells_links;
	uint32_t cells[81];// cell valid digits
	GINT assigned_cands[50];
	uint32_t ndsets, ndlinks, nassigned;
	//dig_set 3X + digit
	int Assign(int cell, int dig, SETS_LINKS &o);
	void LoopNextSet();
	int CheckCover();
};
struct TB_MULT_9P {// tables for a given 9 perm
	BF128 rows, cols, boxes; // cells of the perm
	void Init(int p9);// one out of floors tables 
};

struct TB_MULT_9P_FL  {// more with the floor seen
	BF128 base_truths_cells;
	int tcrossing_units[9], ncrossing_units, nraw_crossing_links;
	int sets_per_unit[27];

};
struct MULTISEARCH {
	BF128 tperms[5][100], mask, sol[5];
	uint32_t  nperms[5], ntsd, digssols[5],ind;
	GINT tsd[6];
	inline void Init(MULTISEARCH & mso) {
		memcpy(sol, mso.sol, sizeof sol);
		memcpy(digssols, mso.digssols, sizeof digssols);
	}
	inline void AddMask(BF128 m, int index,int d) {
		mask =m; 
		sol[index] = m;
		digssols[index] = d;
		ind = index+1;
	}
	int Shrink(int rd2, BF128 *td2, uint32_t nd2);
	inline void Add(int digit, int np) {
		GINT & w = tsd[ntsd];
		w.u8[0] = ntsd++;
		w.u8[1] = digit;
		w.u16[1] = np;
	}
	inline void OrOlds(BF128 & w) {
		w = sol[0];
		for (uint32_t i = 0; i < ind; i++) w |= sol[i];
	}
	void GetDigitPm(int digit, BF128 & pmout, BF128 & pmsolved);
};
struct R0SEARCH {
	struct CROSS {
		BF128 cross, crosstruth, cross_more;
		int iu, ncrosstruth, nlsets, digs_cross;
		int tcross[10], ntcross, tmore[10], ntmore,
			tmore2[10], ntmore2,min_links;
		int Build(R0SEARCH &r, int i);
		void BuildElims(R0SEARCH &r, PM3X & elims);
		void PrintLinks(R0SEARCH &r);
	};
	PM3X cumsols,elims;
	BF128 tperms_digits[9][300];
	BF128 infield, outfield, in_no_out,
		infield_and_sols,
		pmfloors[6],truths_basis_pm;

	// see constructor for initial values 
	TB_MULT_9P tb9p2[36], tb9p3[84], tb9p4[126], tb9p5[126];
	int fl2d_rboxes[36], fl2d_cboxes[36];

	uint32_t floors,nperms[9],ndigits,lastindex;

	GINT tin[9]; // mapping in floors
	uint32_t ntin ,tfloors[6],// number of active digits
		unit_sets_count[27]; // number of unsolved sets per unit
	uint32_t ntruth, nlinks,tu_sets[10], ntu_sets, 
		tu_links[10], 	ntu_links ;
	// using relative digits as of tfloors
	uint32_t dig_cells[81], cells_count[81]; 
	
	R0SEARCH();
	void CollectPerms(int pr=0);
	int IsElims(uint32_t floors, int mode);
	int IsNext(MULTISEARCH & mso,int index);
	int IsNextLast(MULTISEARCH & mso, int index);
	void DebugFloors();
	void DebugFloors2(MULTISEARCH & ms);
	//_____ rank0 search
	int TryRank0Logic();
	void BuildRelativetables();
	int Try_RC(TB_MULT_9P &t9p, int p9, int mode);
	int GoX(int pr, int pc);
};
/*
	struct R0SEARCH{
		struct R0S_LOT4{// a 3 rows or 3 colums base for cells
			BF81 c81;
			BF16 rcbm;
			USHORT rc4[5],nrc4;  // keep it open for 5 rows or columns
		}rclot[84+126][2];
		BF81 cellsf,cellspuref,cellstruth,cells_sets,cells_link,cells_base,
			 subgrids[235][2];
		USHORT rank,code_ret,opt,			  
			   telims[100],nelims,r0count,r1count,debug,ntr1,do_elims,
			   tcovers[50],ncovers,ntruths_cells;
		int diag;
		PMBF telim_native[30],welim_native;
		int TryRowColBand(USHORT * tu,USHORT ntu,USHORT dtruth);
		int CheckBand(USHORT * tu,USHORT ntu);
		int CheckElims();
		int CheckElims(BF16 ordigs,R0S_LOT4 & lr,R0S_LOT4 & lc);
		int GoForBox4();
		void Add(BFSETS & ss,USHORT unit);
		void GoForCellsPrepare();
		int GoForCells();
		int GoForCells63(); // option no given
		int GoForCells(R0S_LOT4 & lr,R0S_LOT4 & lc);
		void GoForCellsRC(R0S_LOT4 & lx,BF16 digpat,int moderc);
		int TryUsingElims(BF16 floors,BF81 * elims_floors, BF81 & elims_cells);
	}r0search;	*/

/* Build Strings v2
main task : store and print a multi chains of any kind
don't store if print off
each candidate is a GINT64  cell,digit,{sign,end chain,end multi}, 0 

*/
class BUILDSTRING{
public:
	GINT64 *zs, * p, *lim, *current_elim_start;
	int mysize;
	// must be called before any use
	void SetUp(GINT64 * zse, int mysizee){ mysize = mysizee; zs = zse; lim = zs + mysize - 100;
	Init();
	}
	void Init() { p = current_elim_start = zs; }
	inline int NoFree() { return p > lim; }
	inline GINT64 * GetFree() { return p; }
	inline void Lock(int n) { p += n; }
	void StoreChain(SCAND * t, int n);
	inline void SetCurrentElim(){ current_elim_start = p; }
	inline void ClearCurrent(){ p = current_elim_start; }
	void SetCurrentAsFirst(){
		int n = (int)(p - current_elim_start);
		p = zs;
		for (int i = 0; i<n; i++)
			*p++ = *current_elim_start++;
		current_elim_start = zs;
	}
};



class PM_GO;
struct TWO_DIGITS{// one of 36 possible 2 digits 
	BF128 bf2,bf2d;
	int free_units,digit[2],nhp;
	GINT64 thp[20];// band 2 cells ; i band ; 
	int Hiden_Pairs_Box();
	int Hiden_Pairs_Rows();

};
/*
*/


class PM_GO{
public:

	struct HINT {  // combine all hints having the same "lowest"rating
		PM_GO * parent;
		PM3X pmelims;

		USHORT rating,rating_done;

		inline void Set_Target(int r){ rating = r; rating_done = 0; }
		inline int IsToDo(int r){ return (r <= rating); }
		inline void Done(int r){ if (r > rating_done) rating_done = r; }


		inline void Init() {rating=999;pmelims.SetAll_0();}

		int ChainLengthAdjusted(int base,int length);
		int MaxLengthForRating(int newbase);
		void Add(PM3X & elime, USHORT rate);
		int AddCand(USHORT dig,USHORT cell,USHORT rate);
	}hint;  
	class XYCOM;
	enum Ex_End{
		ex_end_nothing=0,  // process assumes 0 for nothing
		ex_end_contradiction,
		ex_end_loop,
		ex_end_maxpas
	};
	enum Explain_mode{ex_lastinregion,ex_lastincell,ex_pointing,
		ex_NP,ex_HP,ex_xwr,ex_xwc,
		ex_Ntrip,ex_Htrip,ex_swr,ex_swc,ex_chains,ex_UR,ex_exo,
		ex_symg,ex_aahs,ex_kite,ex_fish
	};


/*
	class  NESTED:public XYCOM{ // work area to search nested chains
		//				chaines	" ~x et x"	multi	dynami
		//95	level2		x			
		//105	level 3		x			x??		x	
		//110	level 4		x			x		x		x
		int nnstart,maxdynlength;
	public:
		void InitNested();
		void NewExpand(SCAND x); // start a new on/off to expand
		void All_Biv_AICs(int mode);
		void DynamicChains();
		int XYexpandDynamic(UCAND cand1);
		int XYexpandDynamicToTarget(UCAND cand1,SCAND target);
		void DynMultiChains(REG_LIST &myrl);
		void XYexpandDynamicWhile();
		int DynBackSkfr(UCAND x,USHORT * tret){return DynBackSkfrCom(x, tret,pmddyncycle);}
	}nested;
	struct TWO_CELLS{  // store 2 cells mini row mini col 3 or 4 digits
		USHORT cell1,cell2,rel_box,row_col,tdigits[4],ndigits;
		BF16 digits16;
		// canonical form
		BF8 pdigits[4], //  pattern  01 10 11 exlusively
		pairvalid,scen1valid,scen2valid;//pairs order is given in bf_fix.tp6
		void LoadInit(BF81 * c,USHORT ecell1,USHORT ecell2,BF16 digits,
			          USHORT endigits,USHORT unit,USHORT relbox);
		void Canonical3(BF81 * c);
		void Canonical4(BF81 * c);
		void ClearPair(int i);
	};
	struct TWO_CELLS_TABLE{
		PM_GO * parent;
		TWO_CELLS			c2[100];
		USHORT nc2;
		int Find_2Cells() ;
	}t2cells;
	struct WDYN_EXO{// working area in dynamic processing for an exocet
	//	BF81 
		struct WDIG{
			USHORT new_in_base,new_in_target,new_on_in_target,
				old_in_base,old_in_target,
				is_in_base,is_in_target,
				on_in_base,on_in_target,
				cell_in_base,cell_in_target,
				cell_in_target1,cell_in_target2;
			BF81 zbase,ztarget,ztarget1,ztarget2,
				base81,t_81,t1_81,t2_81; // zbase is in the box
			BF8 base,target,pdigit ;
			void Init();
		}wdig[4];
	};
	struct EXOCET{ // what is collected in the first step
		PM_GO * parent;
		USHORT cells[8],type; // base and target  (1 or 2 cells per target)
		BF16 digits,abi_digits,exo_digits;
		USHORT units[3],cross_lines[3],ndigits,  bande1,bande2;//,altbox1,altbox2;
		// canonical form
		BF8 pattern,pdigits[4], //  pattern  1111 ou 11111 ou 111111
			pairvalid,scen1valid,scen2valid;//,pairmask[4]; // pair mask is 01 10 11
		USHORT tdigits[4],ncells,npairs,twin1,twin2;

		void Loadinit(PM_GO * parent,TWO_CELLS & source);
		int IsNotCrossPossible(PM_DATA & myd,BF81 & map,USHORT unit,BF16 & w16);
		void Canonical();
		void ClearPair(USHORT d1,USHORT d2);
		int FindEliminations();
		//void DynPrepare(PM_DATA & myd,PM_GO::XYSEARCH & xys,WDYN_EXO & wdex); //MD 23.9.2018: seel next line
		void DynPrepare(PM_DATA & myd,XYSEARCH & xys,WDYN_EXO & wdex);
	};
	struct EXO8: public EXOCET{
	};


*/
	//          start of PM_GO data and functions
	//EXPLAIN_BUFFER expbuf;// locate the source in a path
	//USHORT * texplain[1000];// start of each "event"
	BF16 active_floors,mfloors; 
	ONE_FLOOR one_floor;
	//ACTIVERCB activercb;
//	EXOCET	texocet[30],	wexo; // 30 in open band mode
//	EXO8 texo8[5],wexo8;
//	PMBFONOFF pof_store[1000];  // use in vloop expansion

	// settings for builstrings in xysearch and nested
    #define GINTBUFSIZE1 2000
	#define buildstringsize1 20000
	#define buildstringsize2 2000
	GINT64 builstr1[buildstringsize1],builstr2[buildstringsize2];
	GINT gintbuffer[GINTBUFSIZE1];
	GINTBUF gintbuf;
	BUILDSTRING  bdsp[2];  //pointers to builstring main and nested

	int opprint, opprint2, stop_rating,
		ntr0logic,r0logictype,logictype,
		rank0_min, rank0_max,//usually  2/5, can be adjusted
		assigned,cycle,ratecycle,
	   ur_serate_mode,quick_mode,ratlim,
	   find_common_known, // limit of known before start of the search
		is_valid_puzzle,
		rat_er,rat_ep,rat_ed,
		nexocet,npof_store;
	BF32	bits_tasks_done;
	// data for kites
	int nbiv,nempty;
	UINT ratfound[17]; // set to 0 in the constructor
	const char * det_mess;

	GINT64 tur[20];	STORE_UL tul[10];	WWUR2 wwur2; BF128 lastul; int ntur, ntul;//==== UR UL handling
	BUG bug; // bug handling
	//===================== fish handling
	int active_digits; //digits with potential eliminations (brute force effect
	XSTATUS xstatus[9];	STORE_XLC store_xlc[20]; int nstore_xlc;
	YLSEARCH ylsearch, store_yl[20];	 int nstore_yl;
	XYSEARCH xysearch;
	R0SEARCH r0search;
//============= data for symmetry of given processing
// 0 D1  1 D2  2 stick  3 central  4 R90
	USHORT sgiven_ok ,sgiven_paires[5][9],sgiven_singles[5][3];
	BF16 sgiven_singlesBM[5];


	//========================
	PM_GO();

	//int Assign(int digit,int cell,char * lib);
	//int Assign(int digit,BF81 &cells,char * lib);
	int CleanOr(int d1, int c1, int d2, int c2);
	//void Start();

	//int Solve_All_Singles(char * ze);
	//int  SolveFinalFilter(char * puz);

	//int Solve();
	int Solved_xx(int lim);// internal call valid puzzle 
	int SolveGetLow44(int pack=0,int diag=0);// internal call valid puzzle 
	int SolveDet(int liml, int printopt, int mode);
	void SolveSerate110();// serate mode
	void SolveSerate111();// quick rate
	void SolveSerate112();// solve full explain mode
	void Solve118_subgrid();
	void Solve199test();

	void Solve120_MultiAnalysis();
	void Solve121_Rank0_no_MultiAnalysis();
	void Solve125_Find_Vloop();
	void Solve130_Find_JExocet();

	void Quickrate(int x) ;
	void Status(const char * lib, int option);
	int Rate10(); int Rate12();	int Rate15(); int Rate17(); 
	int Rate20(); int Rate23(); int Rate25(); int Rate26();
	int Rate28(); int Rate30(); int Rate32(); int Rate34();
	//void SetupActiveDigits();
	int Rate36(); int Rate38(); int Rate40(); int Rate42(); int Rate44();
	int Rate45_52();  int Rate45_52_Fast();
	int Rate45Plus(GINT64 * t, int  nt, int plus);
	int Rate45_el(GINT64 & t, int unit,int plus);// serate mode min 2 cells within unit
	//int Rate_wwur2_serate(int unit);
	int Rate2cellsGo(GINT64 & t);// 2 cells bivalues UR UL
	int Rate45_2cells(GINT64 * t, int & nt);// serate mode min 2 cells bivalues, not diagonal
	int Rate45_URs(GINT64 * t,int & nt);// serate mode min 2 cells bivalues, not diagonal
	int Rate46_Find_ULs();
	int RateUL_base(STORE_UL & s);
	int Rate_ULs(int plus45);
	int Rate52(); int Rate54();
	int Rate56(); int Rate56BUG();
	int Rate62(); int Rate62_APE();
	void SetupActiveDigits();	
	inline void XStatusPrepare(){		for (int i = 0; i < 9; i++)xstatus[i].Init(i);	}
	int Rate65Xcycle(int fast);
	int Rate6xXcycle(int rating);
	int Rate66Xchain(int fast);
	int Rate67_70(); int R67_70(int rating);
	int Rate70_75(int rat_ed);
	int Rate75(); int Rate75_ATE();// aligned triplet exclusion
	int Rate76Nishio(int fast);
	int Rate80Multi(int fast);
	int Rate85Dynamic(int fast);
	int Rate90DynamicPlus(int fast);


	int Next10_28();
	int Next28();
	int Next30_44();
	int Next_below_45();

	//int NextElim();
	//int NextSolve2();
	//int NextSolve3();
	//int NextSolve3Sym();
	//int NextSolve4();
	//int NextElimQuick90();
	//int NextElimQuick90fast();
	//int NextElim90Solve();
	//void Quick_Split(GG & gg);// called by sk_gsplit

	//void PrepareSet();
	//void PrepareSetDyn(XYCOM * xyc);   // same for dynamic cycle set and fish
	//int DoActiveRCBx(PMBF * doit); // obsolete
	//int WWings();
 

	//int Kites(int dynamic=0);
	//void KitesDynamicGo();
	//void FishDynamicOthers();

	//void TraiteCheck();
	//void LookForBackdoors();
	//void AddSingles();
	//void TraiteCallFinalFilter();
	//void SolveFilter();
	//int CheckIfAssignedR1Logic(PMBF & fn);
	//void GoForRank1Logic(int print);
	//void UseRank1Logic();

	//void Traite_Find_Common();
	//void Traite_Find_Multi_Fish();
	//int FindMultiFish2();
	//void Traite_Find_SKLoop();
	//int Locate_VLoop();
	//int Is_VLoop();
	//int ReductVLoop();
	//void AddChainVirus(SCEN_VIRUS & scv);
	//int CombineStillValidScenario(char * lib);
	//void AAHS_Dynamic(AAHS_AC2 & aahs);
	//void AAHS_Dynamic(AAHS_AC2 & aahs,int ip,int id1,int id2);
	//void Traite_Find_Exocet();
	//void  FindAllJExocets();
	//void  FindAllJExocets2Cells(int option,int ic2);	

	//void  FindBandExocets(int ic2);	
	//void  FindAllExocets(int ic2);	
	//int   BuildPairTable18(USHORT * tt,int cell);
	//void  FindExocets18(int ic2);
	//void  FindExocets183(int ic2);
	//int  Exocet18Det();
	//	void  Exocet183Det();
	//void FindExpandExocet();
	//void ExoAnalysis();
	//void Traite_Find_Conjugated();



	//void Traite_FindR0N();
	//int Traite_FindR0N_Go();
	//void R0Analysis();
	//int R0Cells();
	//int Apply_Rank0Direct();



	//char IsDoubleExocet(EXOCET & wi, EXOCET & wj);
//	int Is_JExocet();
	//int Is_FullJExocet2Cells();
	//int Is_AbiLoop ();
	//int Is_AbiLoop(USHORT * abi_pairs);
	//int Expand_Exocet();
	//void Exocet_Dynamic(); // dynamic cycle exocet effect

	//int Do_Exo_multirank1D(BF81 &x,BF81 &y,USHORT ntruths,USHORT links0);
	//int Do_Exo_multirank1();
 


	//USHORT IsConjugatedAAHS();
	//int IsConjugatedDigit(int dig);
	//int ClearFixGiven(int i);
	//int Do_First_Sym_Given();
	//void SymGiven_Dynamic(); // dynamic cycle sym given effect
	//void Given_DynamicPair(SCAND a,SCAND b);
	//void Dynam_One_Floor(PMBF & mypm);

	//============= debugging
	//void ImagePoints(BF81 & w);
	//void Image(PMBF & pm,char * lib);
	//void Image(PMBFONOFF & pof,char * lib);
	//void Image(UCAND * t,USHORT n, char * lib);
	//void ImagePaires(EXOCET & wexo);
	//void ImageExtended(EXOCET & wexo);
	//void Xdebug(UCAND x,char * lib);
	//void XYdebug(UCAND x,char * lib);
	//void ListImage(SCAND * t,int n);
	//void ListImageX(SCAND * t,int n);
	//inline void Candidats(PM_DATA & pmd){pmd.ImageCandidats();}
	//void ImagePuzzle(char * puz,char * lib);
	//void ImageSolution();
	};
