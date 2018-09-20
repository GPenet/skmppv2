/* that file contains classes managing bit fields in the program and a specific class,
BF_CONVERT doing specific operations on bit fields to improve the overall performance.

The specific classes are
================================= basic bit fields
BF8 a bit field of size 8 to handle exotic patterns
BF16 a bit field of size 16 bits mainly used as a 9 bits field.
BF32 a bit field of size 32 bits used  as a27 bit field for regions
BF64 a bit field of size 64 bits  
BF128 a bit field of size 128 bits used  to work with 128 bits registers
==========================================================================
BF81 a bit field to describe some "cell" properties in the 81 cells context

BFCAND a bit field sized to a maximum of 320 bits but worked at the used dimension
that bit field is used to describe candidates properties.
the bit entry corresponds to the index in the table of candidates

BFSETS a bit field of 324 bits one bit per set
a set can be a cell (bits 0 80) of a (digit,region) (9 * 27 )
mainly used to build rank 0 logic, storing sets and link sets

BFTAG  a bit field sized to a maximum equal to (BITCAND size * 2)
that bitfield describe the tags properties
*/

struct BF8 {
	UCHAR f;//bitfield
	inline void operator =(BF8 & e) {f = e.f;}
	inline void operator =(UCHAR fe) {f = fe;}
	
 	inline int On(int ch) const {return ((f & (1 << ch)));	}		
	inline int Off(int ch) const {	return (!(f & (1 << ch)));	}	
	inline void Set(USHORT ch) {f |= (1 << ch);	}	
	inline void Clear(USHORT ch) {	f &= ~(1 << ch);}
	inline BF8 operator &(BF8 & e) const {BF8 w;w.f = f & e.f;	return w;	}
	inline BF8 operator |(BF8 & e) {BF8 w;	w.f = f | e.f;	return w;}
	inline BF8 operator ^(BF8 & e) {BF8 w;	w.f = f ^ e.f;	return w;}
	inline BF8 operator -(BF8 & e) {BF8 w;	w.f = f ^ (e.f & f);return w;	}
	inline int  operator ==(BF8 & e) {return(e.f == f);	}
	inline void operator &=(BF8 & e) {f &= e.f;	}
	inline void operator |=(BF8 & e) {f |= e.f;	}
	inline void operator ^=(BF8 & e) {f ^= e.f;	}
	inline void operator -=(BF8 & e) {f ^= (f & e.f);	}
	inline void operator &=(UCHAR fe) {f &= fe;	}
	inline void operator |=(UCHAR fe) {f |= fe;	}
	inline void operator ^=(UCHAR fe) {f ^= fe;	}
	inline void operator -=(UCHAR fe) {f ^= (f & fe);	}
	inline unsigned int Count() {return __popcnt((unsigned int)f);	}

}; // BF8

// a 9 bitfield to give position of candidate in a house and similar functions
struct BF16 {
	USHORT f;
	// constructors
	BF16() { f = 0; }
	BF16(int i1) { f = 1 << i1; }
	BF16(int i1, int i2) { f = (1 << i1) | (1 << i2); }
	BF16(int i1, int i2, int i3) { f = (1 << i1) | (1 << i2) | (1 << i3); }
	BF16(int i1, int i2, int i3, int i4) { f = (1 << i1) | (1 << i2) | (1 << i3) | (1 << i4); }
	BF16(int i1, int i2, int i3, int i4, int i5) { f = (1 << i1) | (1 << i2) | (1 << i3) | (1 << i4) | (1 << i5); }

	inline void SetAll_0() { f = 0; }
	inline void SetAll_1() { f = 0x1ff; }
	inline int isEmpty() const { return (!f); }
	inline int isNotEmpty() const { return f; }
	inline int On(int ch) const { return ((f & (1 << ch))); }
	inline int Off(int ch) const { return (!(f & (1 << ch))); }
	inline void Set(USHORT ch) { f |= (1 << ch); }
	inline void Clear(USHORT ch) { f &= ~(1 << ch); }
	inline BF16 operator &(BF16 & e) const { BF16 w;	w.f = f & e.f;	return w; }
	inline BF16 operator |(BF16 & e) const { BF16 w;	w.f = f | e.f;	return w; }
	inline BF16 operator ^(BF16 & e) const { BF16 w;	w.f = f ^ e.f;	return w; }
	inline BF16 operator -(BF16 & e) { BF16 w;	w.f = f ^ (e.f & f);	return w; }
	inline int  operator ==(BF16 & e) { return(e.f == f); }
	inline void operator &=(BF16 & e) { f &= e.f; }
	inline void operator |=(BF16 & e) { f |= e.f; }
	inline void operator ^=(BF16 & e) { f ^= e.f; }
	inline void operator -=(BF16 & e) { f ^= (f & e.f); }
	inline int First0_8(){
		register unsigned long res;
		if (_BitScanForward(&res, f))	return  res;
		return 0;
	} // must be one bit
	inline USHORT bitCount() { return __popcnt(f); }
	inline int paire() { return (__popcnt(f) == 2); }

	USHORT CountEtString(char *s);
	char * String(char * ws, int lettre = 0);
	USHORT String(USHORT * rr);

}; // BF16
/* BIT32 is used mainly in that program as a 27 region bit field
describing some binary properties fo these regions
*/
struct BF32 {
	UINT f;   // bitfield
	BF32() { f = 0; }
	inline int On(int unit) { return ((f & (1 << unit))); }
	inline int Off(int unit) { return (!(f & (1 << unit))); }
	inline void Set(USHORT unit) { f |= (1 << unit); }
	inline void Clear(USHORT unit) { f &= ~(1 << unit); }
	inline void Inv(USHORT unit) { f ^= (1 << unit); }
	inline BF32 operator -(BF32 & e) {
		BF32 w;	w.f = f ^ (e.f & f); return w;
	}
	inline void operator -=(BF32 & e) { f ^= (f & e.f); }

	USHORT String(USHORT * r);
	USHORT String(USHORT * r, int i1, int i2);
};

//#include "sk_bitfieldsw.h"

struct BF64 {
	GINT64 bf;
	inline void clear() { bf.u64 = 0; }
	inline void SetAll_0() { bf.u64 = 0; }
	inline void SetAll_1() { bf.u64 = BIT_SET_64; };

	inline BF64 operator| (const uint64_t &r) const { BF64 w; w.bf.u64 = bf.u64 | r; return w; }
	inline BF64 operator| (const BF64 &r) const { BF64 w; w.bf.u64 = bf.u64 | r.bf.u64; return w; }
	inline void operator|= (const uint64_t &r) { bf.u64 |= r; }
	inline void operator|= (const BF64 &r) { bf.u64 |= r.bf.u64; }

	inline BF64 operator& (const uint64_t &r) const { BF64 w; w.bf.u64 = bf.u64 & r; return w; }
	inline BF64 operator& (const BF64& r) const { BF64 w; w.bf.u64 = bf.u64 & r.bf.u64; return w; }
	inline void operator&= (const __int64 &r) { bf.u64 &= r; }
	inline void operator&= (const BF64& r) { bf.u64 &= r.bf.u64; }

	inline BF64 operator^ (const uint64_t &r) const { BF64 w; w.bf.u64 = bf.u64 ^ r; return w; }
	inline BF64 operator^ (const BF64& r) const { BF64 w; w.bf.u64 = bf.u64 ^ r.bf.u64; return w; }
	inline void operator^= (const uint64_t &r) { bf.u64 ^= r; }
	inline void operator^= (const BF64& r) { bf.u64 ^= r.bf.u64; };

	inline BF64 operator- (const uint64_t &r) const { BF64 w; w.bf.u64 = bf.u64 & ~r; return w; }
	inline BF64 operator- (const BF64 &r) const { BF64 w; w.bf.u64 = bf.u64 &~r.bf.u64; return w; }
	inline void operator-= (const uint64_t &r) { bf.u64 &= ~r; }
	inline void operator-= (const BF64& r) { bf.u64 &= ~r.bf.u64; };

	inline void operator= (const uint64_t &rhs) { bf.u64 = rhs; }

	inline bool operator== (const BF64& r) const { return (bf.u64 == r.bf.u64); }
	inline bool operator!= (const BF64 &r) const { return (bf.u64 != r.bf.u64); };
	inline void operator<<= (const int bits) { bf.u64 <<= bits; };
	inline void operator>>= (const int bits) { bf.u64 >>= bits; };

	inline unsigned char On(const int theBit) const { return    _bittest64((long long*)&bf.u64, theBit); }
	inline unsigned char Off(const int theBit) const { return    (!_bittest64((long long*)&bf.u64, theBit)); }
	inline void Set(const int theBit) { _bittestandset64((long long*)&bf.u64, theBit); }
	inline void SetToBit(const int theBit) { clear(); _bittestandset64((long long*)&bf.u64, theBit); }
	inline void Clear(const int theBit) { _bittestandreset64((long long*)&bf.u64, theBit); }

	inline uint64_t isNotEmpty() const { return bf.u64; }
	inline bool isEmpty() const { return (!bf.u64); }
	inline int Count() { return (int)_popcnt64(bf.u64); }
	inline int GetFirstBit(unsigned long & res){ return  _BitScanForward64(&res, bf.u64); }
	uint64_t Convert_to_54(){ uint64_t w = bf.u32[1]; w <<= 27; w |= bf.u32[0]; return w; }// 2x27 to 54
};


class BF128 {
public:
	T128 bf;
	BF128() {}
	BF128(const BF128 &v) { bf.u128 = v.bf.u128; }
	BF128(const __m128i &v) { bf.u128 = v; }
	BF128(const T128 &v) { bf.u128 = v.u128; }

	inline void clear() { bf.u64[0] = bf.u64[1] = 0; }
	inline void SetAll_0() { bf.u64[0] = bf.u64[1] = 0; };
	inline void SetAll_1() { bf.u64[0] = bf.u64[1] = BIT_SET_64; };

	inline BF128 operator| (const BF128 &r) const { BF128 w; w.bf.u128 = _mm_or_si128(bf.u128, r.bf.u128); return w; }
	inline BF128 operator| (const __m128i r) const { BF128 w; w.bf.u128 = _mm_or_si128(bf.u128, r); return w; }
	inline void operator|= (const BF128& r) { bf.u128 = _mm_or_si128(bf.u128, r.bf.u128); }
	inline void operator|= (const __m128i r) { bf.u128 = _mm_or_si128(bf.u128, r); }

	inline BF128 operator& (const BF128 &r) const { BF128 w; w.bf.u128 = _mm_and_si128(bf.u128, r.bf.u128); return w; };
	inline BF128 operator& (const __m128i r) const { BF128 w; w.bf.u128 = _mm_and_si128(bf.u128, r); return w; }
	inline void operator&= (const BF128& r) { bf.u128 = _mm_and_si128(bf.u128, r.bf.u128); }
	inline void operator&= (const __m128i r) { bf.u128 = _mm_and_si128(bf.u128, r); }

	inline BF128 operator^ (const BF128 &r) const { BF128 w; w.bf.u128 = _mm_xor_si128(bf.u128, r.bf.u128); return w; }
	inline BF128 operator^ (const __m128i r) const { BF128 w; w.bf.u128 = _mm_xor_si128(bf.u128, r); return w; }
	inline void operator^= (const BF128& r) { bf.u128 = _mm_xor_si128(bf.u128, r.bf.u128); }
	inline void operator^= (const __m128i r) { bf.u128 = _mm_xor_si128(bf.u128, r); };

	inline BF128 operator- (const BF128 &r) const { BF128 w; w.bf.u128 = _mm_andnot_si128(r.bf.u128, bf.u128); return w; }
	inline BF128 operator- (const __m128i r) const { BF128 w; w.bf.u128 = _mm_andnot_si128(r, bf.u128); return w; }
	inline void operator-= (const BF128& r) { bf.u128 = _mm_andnot_si128(r.bf.u128, bf.u128); }
	inline void clearBits(const BF128& r) { bf.u128 = _mm_andnot_si128(r.bf.u128, bf.u128); }
	inline void operator-= (const __m128i r) { bf.u128 = _mm_andnot_si128(r, bf.u128); };

	inline void operator= (const BF128 &r) { bf.u128 = r.bf.u128; }
	inline void operator= (const void *p) { bf.u128 = _mm_loadu_si128((const __m128i*)p); }

	inline bool operator== (const BF128& r) const { return(bf.u64[0] == r.bf.u64[0] && bf.u64[1] == r.bf.u64[1]); }
	inline bool operator!= (const BF128 &r) const { return(bf.u64[0] != r.bf.u64[0] || bf.u64[1] != r.bf.u64[1]); };

	inline void setBit(const int theBit) { _bittestandset64((long long*)&bf.u64[0], theBit); }
	inline void Set(const int theBit) { _bittestandset64((long long*)&bf.u64[0], theBit); }
	inline void SetToBit(const int theBit) { clear(); _bittestandset64((long long*)&bf.u64[0], theBit); }
	inline void MaskToBit(const int theBit) { 
		register int R = theBit;		if (R >= 128)SetAll_1();
		else if (R <= 0)SetAll_0();
		else if(R <64){
			bf.u64[0] = 0;
			bf.u64[0] = (1 << R)-1;
		}
		else {
			bf.u64[0] = 1;
			bf.u64[0] = (1 << (R-64)) - 1;
		}
	}
	inline void Mask(const int theBit){ BF128 w; w.MaskToBit(theBit); *this &=  w; }

	inline unsigned char isBitSet(const int theBit) const { return  _bittest64((long long*)&bf.u64[0], theBit); }
	inline unsigned char On(const int theBit) const { return  _bittest64((long long*)&bf.u64[0], theBit); }
	inline int Off(const int theBit) const { return (!_bittest64((long long*)&bf.u64[0], theBit)); }

	inline void clearBit(const int theBit) { _bittestandreset64((long long*)&bf.u64[0], theBit); }
	inline void Clear(const int theBit) { _bittestandreset64((long long*)&bf.u64[0], theBit); }

	//  code to use in a 3 bands pattern calling using a cell 0-80
	inline int On_c(const int cell) const { return On(C_To128[cell]); }
	inline int Off_c(const int cell) const { return Off(C_To128[cell]); }
	inline void Set_c(const int cell) { Set(C_To128[cell]); }
	inline void Clear_c(const int cell) { Clear(C_To128[cell]); }
	inline void SetDiagX(const int theBit){		Set_c(C_transpose_d[From_128_To_81[theBit]]);	}
	void ClearDiag(int clear, int stack);
	void ClearRow(int clear, int row);
	void ClearCol(int clear, int col);

	inline bool isZero() const { return bf.u64[0] == 0 && bf.u64[1] == 0; }
	inline bool isEmpty() const { return bf.u64[0]==0 && bf.u64[1] == 0; }
	inline bool isNotEmpty() const { return bf.u64[0] != 0 || bf.u64[1] != 0; }

	inline int Count(){ return (int)(_popcnt64(bf.u64[0]) + _popcnt64(bf.u64[1])); }
	inline int Count96(){ return (int)(_popcnt64(bf.u64[0]) + __popcnt(bf.u32[2])); }

	inline int isSubsetOf(const BF128 &s) const { return  _mm_testc_si128(s.bf.u128, bf.u128); }
	inline int isDisjoint(const BF128& r) const { return _mm_test_all_zeros(r.bf.u128, bf.u128); }
	inline int Disjoint(const BF128& r) const { return _mm_test_all_zeros(r.bf.u128, bf.u128); }
	inline int SupersetOf(const BF128 &s) const { return  _mm_testc_si128(bf.u128, s.bf.u128); }

	inline int SupersetOf81(const BF128 &s) const { BF128 w = s; w.Mask(81); return  SupersetOf(w); }
	inline int SupersetOf96(const BF128 &s) const { BF128 w = s; w.Mask(96); return  SupersetOf(w); }
	inline bool operator< (const BF128 &rhs) const {
		if (bf.u64[1] < rhs.bf.u64[1]) return true;
		if (bf.u64[1] > rhs.bf.u64[1]) return false;
		return bf.u64[0] < rhs.bf.u64[0];
	}
	inline int Compare(const BF128 &r) const {
		if (*this == r) return 0;
		if (bf.u64[1] == r.bf.u64[1]){
			if (bf.u64[0] < r.bf.u64[0]) return -1;
			else return 1;
		}
		if (bf.u64[1] < r.bf.u64[1]) return -1;
		else return 1;
	}

	inline void Convert3X27to81(const BF128 & rhs){
		register uint64_t R0 = rhs.bf.u32[0],
			R1 = rhs.bf.u32[1],
			R2 = rhs.bf.u32[2];
		R1 <<= 27; R0 |= R1;
		R1 = R2; R1 <<= 54;
		bf.u64[0] = R0 | R1;
		bf.u64[1] = R2 >> 10;
	}
	inline void Convert81to3X27(const T128 & rhs){
		register uint64_t R0 = rhs.u64[0],
			R1 = rhs.u64[1];
		bf.u32[0] = R0 & BIT_SET_27;
		R0 >>= 27;
		bf.u32[1] = R0 & BIT_SET_27;
		R0 >>= 27;
		R1 <<= 10;
		R0 |= R1;
		bf.u32[2] = R0 & BIT_SET_27;
		bf.u32[3] = 0;
	}

	inline int GetBande3_out_of_81(){
		int w = (bf.u32[2] << 10) | (bf.u32[1] >> 22);
		return w;
	}
	inline int mask8() const { return _mm_movemask_epi8(bf.u128); }
	inline int getFirst96() const {
		unsigned long res;
		if (_BitScanForward64(&res, bf.u64[0]))	return res;
		if (_BitScanForward(&res, bf.u32[2]))	return 64 + res;
		return -1;
	}
	inline int getLast96() const {
		unsigned long res;
		if (_BitScanReverse(&res, bf.u32[2]))	return 64 + res;
		if (_BitScanReverse64(&res, bf.u64[0]))	return res;
		return -1;
	}
	inline int getFirst128() const {
		unsigned long res;
		if (_BitScanForward64(&res, bf.u64[0]))	return res;
		if (_BitScanForward64(&res, bf.u64[1]))	return 64 + res;
		return -1;
	}
	inline int getFirsCell() const {
		unsigned long res;
		if (_BitScanForward(&res, bf.u32[0]))	return res;
		if (_BitScanForward64(&res, bf.u32[1]))	return 27 + res;
		if (_BitScanForward64(&res, bf.u32[2]))	return 54 + res;
		return -1;
	}
	inline int getLast128() const {
		unsigned long res;
		if (_BitScanReverse64(&res, bf.u64[1]))	return 64 + res;
		if (_BitScanReverse64(&res, bf.u64[0]))	return res;
		return -1;
	}
	void GetDiagonal(int v, int ind){// Or in diagonal a 27 bits field 
		int d = 27 * ind;
		unsigned long res;
		while (_BitScanForward(&res, v)){
			v ^= 1 << res;
			register int b = C_transpose_d[res + d];
			Set_c(b);
		}
	}
	void Diag3x27(BF128 & r);
	inline void Store(USHORT * tstore){ memcpy(tstore, this, 16); }
	inline void Re_Load(USHORT * tstore){ memcpy(this, tstore, 16); }

	int String3X27_to_gint_c(GINT * t, int digit);
	int String3X27(int * r);// cell 0_80 as output
	int String81GP(int * t);
	int StringGP(int * t, int bloc = 0);
	char * String3X(char * ws);
	char * String3X_Rev(char * ws);
	char * String81(char * ws);
	char * String128(char * ws);
	inline static uint64_t FindLSBIndex64(const uint64_t Mask) {
		unsigned long res;
		_BitScanForward64(&res, Mask);
		return res;
	}
	inline static uint64_t FindLSBIndex64MS(unsigned long &res, const uint64_t Mask) {
		return _BitScanForward64(&res, Mask);
	}

};

class PM3X {
	// 9*BF128 to have candidates in native mode when needed
public:
	BF128 pmdig[9];
	inline void SetAll_0(){ __stosq((unsigned long long *)pmdig[0].bf.u64,0,18); }
	inline void SetAll_1() { __stosq((unsigned long long *)pmdig[0].bf.u64, BIT_SET_64, 18); }
	inline void Set(int dig, int cell){ pmdig[dig].Set(cell); }
	inline void Set_c(int dig, int cell){ pmdig[dig].Set_c(cell); }
	inline void Clear(int dig, int cell){ pmdig[dig].Clear(cell); }
	inline void Clear_c(int dig, int cell){ pmdig[dig].Clear_c(cell); }
	inline int On(int dig, int cell){ return pmdig[dig].On(cell); }
	inline int On_c(int dig, int cell){ return pmdig[dig].On_c(cell); }
	inline int Off(int dig, int cell){ return pmdig[dig].Off(cell); }
	inline int Off_c(int dig, int cell){ return pmdig[dig].Off_c(cell); }

	void operator &= (const PM3X &z2);
	void operator |= (const PM3X &z2);
	void operator -= (const PM3X &z2);
	void operator &= (const BF128 * bf128);
	void Image(BUILDSTRING & zs);
	USHORT String(UCAND * t);
	int IsEmpty();
	int Count();
	void Print(char * lib);
};


struct RBF27{
	// 9 BF 32 for region sets
	BF32 t[9];
	inline void InitNo(){ for (int i = 0; i<9; i++)	t[i].f = 0; }
	inline void InitYes(){ for (int i = 0; i<9; i++)	t[i].f = 0x7FFFFFF; }
	inline void Set(int dig, int unit)	{ t[dig].Set(unit); }
	inline void Clear(int dig, int unit)	{ t[dig].Clear(unit); }
	inline int On(int dig, int unit)	{ return t[dig].On(unit); }
	inline int Off(int dig, int unit)	{ return t[dig].Off(unit); }
};

class BF81 : public BF128 {
public:
	BF81() {  }
	BF81(int i1) { SetToBit(i1); }
	BF81(int i1, int i2); // same start 2 cells
	BF81(const T128 &r, const BF81 & r2); // & at start as default
	BF81(char * mode, int i1); // mainly to include cellsFixedData[i1].z
	BF81(char * mode, int i1, int i2);
	inline void SetAll_1() { MaskToBit(81); }

	void PackRows(BF16 * rows);
	void OrBand(int F, int iband);
	void BackZhou(int * Fx);
	void LoadZhou(int * Fx);
	USHORT String(USHORT *r);
	USHORT String(USHORT *r, USHORT digit);
	USHORT StringUnit(int unit, USHORT *r);
	USHORT StringUnit(int unit, USHORT *r, USHORT digit);
	USHORT GetRegion(int unit);  // return BF16.f
	BF16 GetBFRegion(int unit);
	int GetCount(int unit, BF16 & rr);
	void Image(BUILDSTRING & zs, int digit, char * lib = 0, int doinit = 1);
	void Store(USHORT * tstore);
	void Re_Load(USHORT * tstore);
};


class PMBF {
	// 9*BF81 to have candidates in native mode when needed
public:
	BF81 bfc[9];
	inline void SetAll_0() { __stosq((unsigned long long *)bfc[0].bf.u64, 0, 18); }
	inline void SetAll_1() { __stosq((unsigned long long *)bfc[0].bf.u64, 0xffffffffffffffff, 18); }
	inline void Set(int dig, int cell){ bfc[dig].Set(cell); }
	inline void Set_c(int dig, int cell){ bfc[dig].Set_c(cell); }
	inline void SetU(UCAND c){ bfc[c >> 7].Set(c & 127); }
	void Set(BF16 & digs, int cell);
	void SetRegion(int dig, int unit, BF16 & pdigs);
	inline void Clear(int dig, int cell){ bfc[dig].Clear(cell); }
	inline int SetIf(int dig, int cell){
		if (bfc[dig].On(cell)) return 0;
		bfc[dig].Set(cell);
		return 1;
	}
	int operator ==(const PMBF & b) const;
	void operator &= (const PMBF &z2);
	void operator |= (const PMBF &z2);
	void operator -= (const PMBF &z2);
	void operator &= (const BF81 * bf81);
	void Image(BUILDSTRING & zs);
	inline int On(int dig, int cell){ return bfc[dig].On(cell); }
	inline int On_c(int dig, int cell){ return bfc[dig].On_c(cell); }
	inline int OnU(UCAND x){ return bfc[x >> 7].On(x & 0x7f); }
	inline int OffU(UCAND x){ return bfc[x >> 7].Off(x & 0x7f); }
	USHORT String(UCAND * t);
	int IsEmpty(){
		for (int i = 0; i<9; i++)
			if (bfc[i].isNotEmpty()) return 0;
		return 1;
	}
	int Count();
};

class PMBFONOFF{// small class managing both status
public:
	PMBF bfon, bfoff;
	void SetAll_0(){ bfon.SetAll_0(); bfoff = bfon; }
	inline void Set(SCAND x){
		PMBF & mybf = (x & (1 << 11)) ? bfoff : bfon;
		mybf.Set((x >> 7) & 15, x & 0x7f);
	}
	inline int On(SCAND x){
		PMBF & mybf = (x & (1 << 11)) ? bfoff : bfon;
		return mybf.On((x >> 7) & 15, x & 0x7f);
	}
	USHORT Stringbits(UCAND * t);
	USHORT String(UCAND * t);

};

/* BFSETS, is a bitfield used to account sets and link sets
in the rank 0 logic construction
*/

class BFSETS {
	BF128 ff[3];
public:
	void SetAll_0() {
		ff[0].clear();
		ff[1].clear();
		ff[2].clear();
	}
	inline int On(int v) const {
		return (ff[v >> 7].On(v & 127));
	}
	inline int Off(int v) const {
		return (On(v) == 0);
	}
	inline void SetCell(int v) {
		ff[0].setBit(v);
	}
	inline void SetRegion(int d, int el) {
		int v = 96 + (d << 5) + el;
		ff[v >> 7].setBit(v & 127);
	}

	UINT GetSetsDigit(USHORT d){ int dd = d + 3, ffi = dd>2, ffv = dd & 3;	return ff[ffi].bf.u32[4]; }
	inline bool IsEmpty() const
	{
		return (ff[0].isZero() && ff[1].isZero() && ff[2].isZero());
	}
	int Count() const;
	USHORT String(USHORT *r) const;
	char * XsudoPrint(int mode, char * output);
};




class ONE_FLOOR{// find all eliminations inside one floor
	class FL {
		BF81 f_cand;
		int lastrow, ctllast, exocet_perm;
		BF32 sets27;
	public:
		ONE_FLOOR *onf;
		FL(){} //empty constructor
		FL(FL & old) { (*this) = old; }
		int Init(BF81 & fc);
		int GoExocet(BF81 & wc);
		int AvanceCand(int icand);
		void FloorRow(int row);
		void FloorRowExocet(int row);

	}fl;
public:
	PMBF elims;
	BF16 active_floors;
	BF81 f_or;
	void Go_One_Floorx(PMBF & start);
};


struct ACTIVERCB{// look for active row col box in a given valid PM
	struct ARCB {
		// small class  designed to find eliminations within a set in recursive mode
		BF16  * or_perms, myor[9], tcells[9];
		BF81 myperm;
		USHORT lastcell;

		ARCB(){} //empty constructor
		ARCB(ARCB & old) { (*this) = old; }
		int DoCell(USHORT cell);
		int DoCellForPerm(USHORT cell);
		int Do(int idig, int icell);
	};
	ARCB arcb;
	BF32 active_setsx;
	BF16 or_cells[9];
	USHORT ncells;
	int Go_RCBx(PMBF & start, BF16 * dcells, BF81 & actifs, PMBF * do_it = 0);
	int URDummySet(BF16 * dcells, int n, int for_perm = 0);
};
struct COMB9{// one combination C 9-n  (9 36 64 126 126 64 36  9 ) 
	BF16 bm;
	USHORT t[5], nt;
};

struct BF_FIX{ // tables used in BF processing
	USHORT tp3[3][2];	// relative pairs in exocet 3 digits
	USHORT tp6[6][2];   // relative pairs in exocet 4 digits
	UCHAR texset[8];   // mask defining a solution in an exocet
	USHORT texset4[4][2], texset5[8][2], texset6[16][2];
};