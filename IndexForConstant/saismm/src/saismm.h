#ifndef SAISM_H
#define SAISM_H

#include "mycommon.h"
#include "namespace.h"
#include "sachecker.h"
#include <algorithm>
#include <fstream>
#include <vector>

WTL_BEG_NAMESPACE

#define _verify_sa
//#define _DEBUG_SAISM


template<typename charT>
class SAIS;

template<typename charT>
class SAISComputation {
public:
	SAISComputation(std::string & _sName, uint32 _K, std::string & _saName);
};

template<typename charT>
SAISComputation<charT>::SAISComputation(std::string & _sName, uint32 _K, std::string & _saName) {
	std::ifstream fin(_sName, std::ios_base::in | std::ios_base::binary);
	fin.seekg(0, std::ios_base::end);

	uint32 n = fin.tellg() / sizeof(charT);
	charT *s = new charT[n + 1];
	uint32 *sa = new uint32[n + 1];

	fin.seekg(0, std::ios_base::beg);
	fin.read((char*)s, n * sizeof(charT) / sizeof(char));
	s[n] = 0; //append the sentinel


	SAIS<charT>(s, sa, n + 1, _K, 0, D_LOW);

#ifdef _verify_sa  
	std::cerr << "\nstart checking\n";
	std::vector<uint8> s_vector; s_vector.resize(n);
	std::vector<uint32> sa_vector; sa_vector.resize(n);

	for (uint32 i = 0; i < n; ++i) {
		s_vector[i] = s[i];
		sa_vector[i] = sa[i + 1];
	}
	if (sachecker::sa_checker<uint8, uint32>(s_vector, sa_vector)) {
		std::cerr << "sa-ok!";
	}
	else {
		std::cerr << "sa-failed!";
	}

#endif

	std::ofstream fout(_saName, std::ios_base::out | std::ios_base::binary);
	fout.seekp(0, std::ios_base::beg);
	fout.write((char*)sa, n * sizeof(uint32) / sizeof(char));

	delete[] s;
	delete[] sa;
}


//compare two small-length (<=D) lms-substrings (except for the sentinel and the non-size-one substring ending with the sentinel).
template<typename Iterator>
inline int lexcompare_type_3way(Iterator _beginA, Iterator _endA, Iterator _beginB, Iterator _endB) {
	Iterator strA = _beginA, strB = _beginB;
	for (; strA != _endA && strB != _endB; ++strA, ++strB) {
		if (*strA < *strB) return -1;
		if (*strB < *strA) return +1;
	}

	if (strA == _endA && strB == _endB) { // A == B
		return 0;
	}
	else if (strA == _endA) { // A > B
		return +1;
	}
	else { // A < B
		return -1; 
	}
}


struct SubstringPtr {
	uint32 pos;
	uint8 len; // D <= 256.

	SubstringPtr(const uint32 _pos, const uint8 _len) : pos(_pos), len(_len) {}
};

template<typename charT>
struct SubstringPtrSort {
	const charT * sBuf;
	bool operator() (const SubstringPtr & _a, const SubstringPtr & _b) {
		return lexcompare_type_3way(sBuf + _a.pos, sBuf + _a.pos + _a.len, sBuf + _b.pos, sBuf + _b.pos + _b.len) < 0;
	}
};


template<typename charT>
class SAIS {
private:
	//fuction member
	charT *mS; //s
	uint32 *mSA; //sa
	uint8 *mT;  //type
	uint32 *mBkt; //bucket
	uint8 D;
	
	//data member
	uint32 mNum;
	uint32 mK;
	uint32 mLevel;
public:
	SAIS(charT *_s, uint32 *_sa, uint32 _num, uint32 _K, uint32 _level, uint8 _D) :mS(_s), mSA(_sa), mNum(_num), mK(_K), mLevel(_level), D(_D), mT(nullptr), mBkt(nullptr) {
		run();
	}
	void getBuckets(bool _end);
	void induceSAL(bool _sortStr);
	void induceSAS(bool _sortStr);
	bool isLMS(uint32 _pos); //determine whether mS[_pos] is an lms-char.
	void computeType(uint32 _pos); //compute the type of mS[_pos]
	void run();
	int compareSubstring(uint32 substringA_spos, uint32 substringB_spos);
};

template<typename charT>
void SAIS<charT>::getBuckets(bool _end) {
	uint32 i, sum = 0;
	for (i = 0; i <= mK; ++i) mBkt[i] = 0; // clear all buckets
	for (i = 0; i < mNum; ++i) ++mBkt[mS[i]]; // compute the size of each bucket
	for (i = 0; i <= mK; ++i) { sum += mBkt[i]; mBkt[i] = _end ? sum - 1 : sum - mBkt[i]; }
}

template<typename charT>
void SAIS<charT>::induceSAL(bool _sortStr) {
	uint32 i, j;
	getBuckets(false); // find heads of buckets
	if (mLevel == 0) ++mBkt[0];
	for (i = 0; i < mNum; ++i) {
		if (mSA[i] != UINT32_MAX && mSA[i] != 0) { //induce non-empty element
			j = mSA[i] - 1;
			if (j >= 0 && !mT[j]) {
				mSA[mBkt[mS[j]]++] = j; //induced L-type
				if (_sortStr) {
					mSA[i] = UINT32_MAX; //clear current element if it induces an L_TYPE.
				}
			}
		}
	}
}


template<typename charT>
void SAIS<charT>::induceSAS(bool _sortStr) {
	uint32 i, j;
	getBuckets(true); // find heads of buckets
	for (i = mNum - 1; i >= 0; --i) {
		if (mSA[i] != UINT32_MAX && mSA[i] != 0) { //induce non-empty element
			j = mSA[i] - 1;
			if (j >= 0 && mT[j]) {
				mSA[mBkt[mS[j]]--] = j; //induced S-type
				if (_sortStr) {
					mSA[i] = UINT32_MAX; //clear current element if it induces an S_TYPE.
				}
			}
		}
		if (i == 0) break;
	}
}

template<typename charT>
bool SAIS<charT>::isLMS(uint32 _pos) {
	return (_pos > 0 && mT[_pos] && !mT[_pos - 1]);
}


//compute T[0, n - 3]. Note that, T[n - 2] = L_TYPE, T[n - 1] = B_TYPE.
template<typename charT>
void SAIS<charT>::computeType(uint32 _pos) {
	mT[_pos] = (mS[_pos] < mS[_pos + 1] || mS[_pos] == mS[_pos + 1] && mT[_pos + 1]) ? S_TYPE : L_TYPE;
}

template<typename charT>
void SAIS<charT>::run() {
#ifdef _DEBUG_SAISM
	std::cerr <<"s:\n" << std::endl;
	for(uint32 i = 0; i< mNum; ++i){
		std::cerr << (uint32)mS[i] <<" ";
	}
	std::cerr << std::endl;
#endif

	uint32 i, j;
	mT = new uint8[mNum]; //one byte per type.
	std::vector<SubstringPtr> substrings; //records the start position of each lms-substring of which the length is no more than D. 
	substrings.reserve(mNum / 2); //the number of substrings is no more than mNum / 2;

	// scan s to compute t
	mT[mNum - 1] = B_TYPE; mT[mNum - 2] = L_TYPE;
	for (i = mNum - 3; i >= 0; --i) {
		computeType(i);
		if (i == 0) break;
	}

	//sort lms-substrings
	mBkt = new uint32[mK + 1];
	getBuckets(true); // find ends of buckets
	for(i = 0; i < mNum; ++i) mSA[i] = UINT32_MAX;

	//find the rightmost lms-char except for the sentinel
	uint32 leftLmsPos, rightLmsPos, substringLen;
	for (i = mNum - 3; i >= 1; --i) {//i != 0
		if (isLMS(i)) {
			rightLmsPos = i--;
			break;
		}
	}
	//find the remaining lms-chars and insert them into substrings or sa.
	for (; i >= 1; --i) {
		if (isLMS(i)) {
			leftLmsPos = i;
			substringLen = rightLmsPos - leftLmsPos + 1;
			if (substringLen <= D) {//record the starting position of current lms-substring.
				substrings.push_back(SubstringPtr(leftLmsPos, substringLen));
			}
			else {//insert the ending position of current lms-substring into sa.
				mSA[mBkt[mS[rightLmsPos]]--] = rightLmsPos;
			}
			rightLmsPos = leftLmsPos;
		}
	}

	// insert the sentinel into sa.
	mSA[0] = mNum - 1;

#ifdef _DEBUG_SAISM
	std::cerr << "lms-chars in sa:\n";
	for(uint32 i = 0; i < mNum; ++i){
		std::cerr << mSA[i] <<" ";
	}
	std::cerr << std::endl;
	
	std::cerr << "lms-chars in substrings:\n";
	for(auto it = substrings.begin(); it != substrings.end(); ++it){
		std::cerr << it->pos <<" ";
	}
	std::cerr << std::endl;
#endif

	//sort small-length lms-substrings recorded in substrings. 
	SubstringPtrSort<charT> substringPtrSort;
	substringPtrSort.sBuf = mS;
	std::sort(substrings.begin(), substrings.end(), substringPtrSort);

#ifdef _DEBUG_SAISM
	std::cerr << "sort substrings in lms-chars:\n";
	for(auto it = substrings.begin(); it != substrings.end(); ++it){
		std::cerr << it->pos <<" ";	
	}	
	std::cerr << std::endl;
#endif

	//sort long-length lms-substrings recorded in sa. 
	induceSAL(true);

#ifdef _DEBUG_SAISM
	std::cerr << "sort l-type prefixes:\n";
	for(uint32 i = 0; i < mNum; ++i){
		std::cerr << mSA[i] << " ";
	}	
	std::cerr << std::endl;
#endif

	induceSAS(true);
	mSA[0] = mNum - 1; //reinsert the sentinel, a size-one lms-substring.

#ifdef _DEBUG_SAISM
	std::cerr << "sort s-type prefixes:\n";
	for(uint32 i = 0; i < mNum; ++i){
		std::cerr << mSA[i] << " ";
	}
	std::cerr << std::endl;
#endif

	delete[] mBkt;


	//merge the two parts to name all the lms-substrings.
	uint32 n11 = 0, n12 = 0, n1 = 0;
	for (i = 0; i < mNum; ++i) {
		if (mSA[i] != UINT32_MAX && mSA[i] != 0) {
			mSA[n11++] = mSA[i];
		}
	}
	
#ifdef _DEBUG_SAISM
	std::cerr << "substrings in sa: \n";
	for(i = 0; i < n11; ++i){
		std::cerr << mSA[i] << " ";
	}
	std::cerr << std::endl;
#endif


	n12 = substrings.size();
	n1 = n11 + n12;

#ifdef _DEBUG_SAISM
	std::cerr <<"n1: " << n1 << " n11: " << n11 << " n12: " << n12 << std::endl;
#endif

	for (i =  n1; i < mNum; ++i) mSA[i] = UINT32_MAX;
	uint32 name = 0, pre_name = UINT32_MAX, pos1, pos2;
	int result;

	i = 0, j = 0;
	mSA[n1 + mSA[i++] / 2] = name; //must be the sentinel
	pre_name = name++;
	while (i < n11 && j < n12) {
		result = compareSubstring(mSA[i], substrings[j].pos);
		if (result == -1) {
#ifdef _DEBUG_SAISM
			std:: cout << "rel< " << mSA[i] <<" " << std::endl;
#endif
			mSA[n1 + mSA[i++] / 2] = name;
			pre_name = name++;
			while(i < n11 && 0 == compareSubstring(mSA[i], mSA[i - 1])){
#ifdef _DEBUG_SAISM
			std:: cout << "rel= " << mSA[i] <<" " << std::endl;
#endif
				mSA[n1 + mSA[i++] / 2] = pre_name;
			}
		}
		else if (result == +1) {
#ifdef _DEBUG_SAISM
			std:: cout <<"rel> " << substrings[j].pos <<" " << std::endl;
#endif
			mSA[n1 + substrings[j++].pos / 2] = name;
			pre_name = name++;
			while(j < n12 && 0 == compareSubstring(substrings[j].pos, substrings[j - 1].pos)){
#ifdef _DEBUG_SAISM
			std:: cout << "rel= " << substrings[j].pos <<" " << std::endl;
#endif
				mSA[n1 + substrings[j++].pos / 2] = pre_name;
			}
		}
		else {
#ifdef _DEBUG_SAISM
			std:: cout << "rel= " << mSA[i] <<" and " << substrings[j].pos << std::endl;
#endif
			mSA[n1 + mSA[i++] / 2] = name;
			mSA[n1 + substrings[j++].pos / 2] = name;
			pre_name = name++;
			while (i < n11 && 0 == compareSubstring(mSA[i], mSA[i - 1])) {
#ifdef _DEBUG_SAISM
			std:: cout << "rel= " << mSA[i] <<" " << std::endl;
#endif
				mSA[n1 + mSA[i++] / 2] = pre_name;
			}
			while (j < n12 && 0 == compareSubstring(substrings[j].pos, substrings[j - 1].pos)) {
#ifdef _DEBUG_SAISM
			std:: cout << "rel= " << substrings[j].pos <<" " << std::endl;
#endif
				mSA[n1 + substrings[j++].pos / 2] = pre_name;
			}
		}
	}

	while (i < n11) {		
#ifdef _DEBUG_SAISM
			std:: cout << "1 remain: " << mSA[i] <<" " << std::endl;
#endif
		mSA[n1 + mSA[i++] / 2] = name;
		pre_name = name++;
		while (i < n11 && 0 == compareSubstring(mSA[i], mSA[i - 1])) {
#ifdef _DEBUG_SAISM
			std:: cout << "rel= " << mSA[i] <<" " << std::endl;
#endif
			mSA[n1 + mSA[i++] / 2] = pre_name;
		}
	}
	while (j < n12) {
#ifdef _DEBUG_SAISM
			std:: cout << "2 remain: " << substrings[j].pos <<" " << std::endl;
#endif
		mSA[n1 + substrings[j++].pos / 2] = name;
		pre_name = name++;
		while (j < n12 && 0 == compareSubstring(substrings[j].pos, substrings[j - 1].pos)) {
#ifdef _DEBUG_SAISM
			std:: cout << "rel= " << substrings[j].pos <<" " << std::endl;
#endif
			mSA[n1 + substrings[j++].pos / 2] = pre_name;
		}
	}


	for (i = mNum - 1, j = mNum - 1; i >= n1; --i) { //n1 > 0, i != 0
		if (mSA[i] != UINT32_MAX) mSA[j--] = mSA[i];
	}

	// s1 is done now
	uint32 *sa1 = mSA, *s1 = mSA + mNum - n1;

	// stage 2: solve the reduced problem
	// recurse if names are not yet unique
	if (name < n1) {
		SAIS<uint32>(s1, sa1, n1, name - 1, mLevel + 1, D_HIGH);
	}
	else { // generate the suffix array of s1 directly
		for (i = 0; i < n1; i++) sa1[s1[i]] = i;
	}

	// stage 3: induce the result for the original problem
	mBkt = new uint32[mK + 1];
	// put all left-most S characters into their buckets
	getBuckets(true); // find ends of buckets
	j = 0;
	for (i = 1; i < mNum; ++i) {
		if (isLMS(i)) s1[j++] = i; // get p1
	}
	for (i = 0; i < n1; ++i) {
		sa1[i] = s1[sa1[i]]; // get index in s1
	}
	for (i = n1; i < mNum; ++i) {
		mSA[i] = UINT32_MAX; // init SA[n1..n-1]
	}
	for (i = n1 - 1; i >= 0; --i) {
		j = mSA[i]; mSA[i] = UINT32_MAX;
		if (mLevel == 0 && i == 0) {
			mSA[0] = mNum - 1;
		}
		else {
			mSA[mBkt[mS[j]]--] = j;
		}
		if (i == 0)break;
	}

	induceSAL(false);
	induceSAS(false);

	delete[] mBkt;
	delete[] mT;
}

template<typename charT>
int SAIS<charT>::compareSubstring(uint32 substringA_pos, uint32 substringB_pos) { 
	//compare the left lms char.
	if (mS[substringA_pos] < mS[substringB_pos]) return -1;
	if (mS[substringB_pos] < mS[substringA_pos]) return +1;
	++substringA_pos, ++substringB_pos;

	//compare the remaining chars
	while (!isLMS(substringA_pos) && !isLMS(substringB_pos)) {
		if (mS[substringA_pos] < mS[substringB_pos]) return -1;
		if (mS[substringB_pos] < mS[substringA_pos]) return +1;
		++substringA_pos, ++substringB_pos;
	}
	
	//compare the right lms char.
	if (mS[substringA_pos] < mS[substringB_pos]) return -1;
	if (mS[substringB_pos] < mS[substringA_pos]) return +1;
	
	if (isLMS(substringA_pos) && isLMS(substringB_pos)) {//2 cases: S_TYPE and S_TYPE or S_TYPE and B_TYPE
		return mT[substringB_pos] - mT[substringA_pos];
	}
	else if (isLMS(substringA_pos)) {
		return (mT[substringA_pos] == B_TYPE) ? -1 : +1;
	}
	else {
		return (mT[substringB_pos] == B_TYPE) ? +1 : -1;
	}
}

WTL_END_NAMESPACE

#endif
