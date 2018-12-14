

// standard first band (or unique band)
struct STD_B416 {
	char band[28];
	int band0[27],i416,gangster[9],map[27],dband;
	uint32_t tua[100], nua;//   maximum 81  
	void Initstd();
	void GetBandTable(int i) ;
	void SetGangster();
	inline void GetUAs() {
		nua = t16_nua[i416];
		memcpy(tua, &t16_UAs[t16_indua[i416]], 4 * nua);
	}
	void InitC10(int i);
	void InitG12(int i) ;
	void InitBand2_3(int i16,char * ze, BANDMINLEX::PERM & p) ;
	void PrintStatus();
};
struct STD_B1_2 :STD_B416 {
	// 12 115 maximum see band 28
	int index1[30][3], index2[135][3],
		n5,n6,nind[2];// bitfiedl,current index 5 current index 6
	XY_EXPAND xye6[MAXN6], xye5[MAXN5];
	// row solution pattern in digit
	int mini_digs[9],mini_pairs[27],
		revised_g[9];// after false forced in 1/2 minirows
	int  valid_pairs; //9 or 27 bits 
	void FillMiniDigsMiniPairs();
	inline void InitRevisedg() {
		memcpy(revised_g, gangster, sizeof gangster);
	}
	int ReviseG_triplet(int imini, int ip, STD_B1_2 * bb);
	uint32_t GetCellsBf(int box, int imini, int icase);
	uint32_t GetMiniData(int index,  uint32_t & bcells, STD_B1_2 *bb);
	void DoExpandBand(int dband);// dband 0/27
	void DebugIndex(int ind6 = 0);
};

struct STD_B3 :STD_B416 {// data specific to bands 3
	struct GUAs {
		BF128 isguasocket2, isguasocket3, isguasocket4;// active i81
		int pairs[27] ;// gua2s i81 and bf of active
		int triplet[9] ;//same gua3s
		int ua_pair[81], ua_triplet[81]; // storing ua bitfields
	}guas;
	//int ti81_pairs[27];// index to the relevant tsgua2 (genb12)
	//int ti81_triplet[9];//index to the relevant tsgua3
	//BF128 tbands_UA4_6s, tbands_pairs, tbands_triplets;
	//int tuas46[81];
	//GINT64 tipairs[96];
	//int tindexUA4s[96];// pair id_81 (3x27) to bit 0_8 in the band
	//int tindextriplets[96];// triplet id_81 (3x27) to bit 0_8 in the band
	//_______________________
	void InitBand3(int i16, char * ze, BANDMINLEX::PERM & p);
	int IsGua(int i81);
	void PrintB3Status();
};

//================== UA collector 2 bands 

struct GENUAS_B12 {// for uas collection in bands 1+2 using brute force 
	int dig_cells[9][9],
		gangbf[9],// columns for band 3 in bit field
		revised_gangbf[9],// same revised UA2s UA3s ***
		mini_digs[9], mini_pairs[27], // UA2s UA3  ***
		valid_pairs, //  27 bits valid sockets UA2s ***
		nfloors, limstep,map[9], cptdebug;
	BF128 valid_sockets;

	//=============== uas collector 
	int limsize,floors;
	uint64_t  tuaold[1000],// previous non hit uas infinal table of uas for bands 1+2
		tua[TUA64_12SIZE],// 
		tuab1b2[200];// collecting bands uas in 2x mode
	uint32_t nuaold,nua,nuab1b2,
		tuamore[500];
	//_______________uamore control
	STD_B1_2 *ba, *bb;
	int ib,digp;
	uint64_t w0, ua;
	//______________________ small tasks to handle sockets UA2s UA3s
	// doing things similar to uamore in one band 
	int CheckSocket2(int isocket);
	//_____________________ functions collect UAs bands 1+2
	void Initgen();
	void BuildFloorsAndCollectOlds(int fl);
	int AddUA64(uint64_t * t, uint32_t & nt);
	inline void AddUA(uint64_t v) {
		ua = v; AddUA64(tua, nua);
	}
	void BuilOldUAs( uint32_t r0);
	int CheckOld();
	void CollectMore();
	void CollectMoreTry6_7();
	void EndCollectMoreStep();
	void CollectTriplets();
	void CollectMore2minirows();
	//_____________________ functions collect UA2s UAs3 socket 

	void ProcessSocket2(int i81);

	//	genb12.CollectUA2s();// collect GUA2s
	//	genb12.CollectUA3s();//collect GUA3s

};

