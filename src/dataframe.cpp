#include "dataframe.h"
#include <vector>
#include <algorithm>
#include <string>
#include <cmath>
#include "NA.h"


std::vector<double> SpatDataFrame::getD(unsigned i) {
	unsigned j = iplace[i];
	return dv[j];
}

std::vector<long> SpatDataFrame::getI(unsigned i) {
	unsigned j = iplace[i];
	return iv[j];
}

std::vector<std::string> SpatDataFrame::getS(unsigned i) {
	unsigned j = iplace[i];
	return sv[j];
}


SpatDataFrame SpatDataFrame::subset_rows(unsigned i) {
	std::vector<unsigned> r = { i }; 
	return subset_rows(r);
}


SpatDataFrame SpatDataFrame::subset_rows(std::vector<unsigned> range) { 
	SpatDataFrame out;
	out.names = names;
	out.itype = itype;
	out.iplace = iplace;
	out.dv.resize(dv.size());
	out.iv.resize(iv.size());
	out.sv.resize(sv.size());
	for (size_t i=0; i < range.size(); i++) {
		for (size_t j=0; j < dv.size(); j++) {
			out.dv[j].push_back(dv[j][i]);
		}
		for (size_t j=0; j < iv.size(); j++) {
			out.iv[j].push_back(iv[j][i]);
		}
		for (size_t j=0; j < sv.size(); j++) {
			out.sv[j].push_back(sv[j][i]);
		}
	}
	return out;	
}	


SpatDataFrame SpatDataFrame::subset_cols(unsigned i) {
	std::vector<unsigned> c = { i }; 
	return subset_cols(c);
}

SpatDataFrame SpatDataFrame::subset_cols(std::vector<unsigned> range) { 
	SpatDataFrame out;
	unsigned dcnt=0;
	unsigned icnt=0;
	unsigned scnt=0;
	for (size_t i=0; i < range.size(); i++) {
		unsigned j = range[i];
		unsigned p = iplace[j];
		out.names.push_back(names[j]);
		if (itype[j] == 0) {
			out.dv.push_back(dv[p]);
			out.iplace.push_back(dcnt);
			out.itype.push_back(0);
			dcnt++;
		} else if (itype[j] == 1) {
			out.iv.push_back(iv[p]);
			out.iplace.push_back(icnt);
			out.itype.push_back(1);
			icnt++;				
		} else {
			out.sv.push_back(sv[p]);
			out.iplace.push_back(scnt);
			out.itype.push_back(2);
			scnt++;				
		}
	}
	return out;	
}


unsigned SpatDataFrame::ncol() {
	return itype.size();
}


unsigned SpatDataFrame::nrow() {
	unsigned n;
	if (itype.size() == 0) {
		n = 0;
	} else {
		if (itype[0] == 0) {
			n = dv[0].size();
		} else if (itype[0] == 1) {
			n = iv[0].size();
		} else {
			n = sv[0].size();			
		}
	}
	return n;
}




void SpatDataFrame::add_row() {
	for (size_t i=0; i < dv.size(); i++) {
		dv[i].push_back(NAN);	
	}
	long longNA = NA<long>::value;
	for (size_t i=0; i < iv.size(); i++) {
		iv[i].push_back(longNA);	
	}
	for (size_t i=0; i < sv.size(); i++) {
		sv[i].push_back(NAS);	
	}
}


void SpatDataFrame::reserve(unsigned n) {
	for (size_t i=0; i<dv.size(); i++) {
		dv[i].reserve(n);
	}
	for (size_t i=0; i<iv.size(); i++) {
		iv[i].reserve(n);
	}
	for (size_t i=0; i<sv.size(); i++) {
		sv[i].reserve(n);
	}
}

void SpatDataFrame::resize(unsigned n) {
	for (size_t i=0; i<dv.size(); i++) {
		dv[i].resize(n, NAN);
	}
	long longNA = NA<long>::value;
	for (size_t i=0; i<iv.size(); i++) {
		iv[i].resize(n, longNA);
	}
	for (size_t i=0; i<sv.size(); i++) {
		sv[i].resize(n, NAS);
	}
}
	
void SpatDataFrame::add_column(unsigned dtype, std::string name) {
	unsigned nr = nrow();
	if (dtype == 0) {
		std::vector<double> dins(nr, NAN);
		iplace.push_back(dv.size());
		dv.push_back(dins);
	} else if (dtype == 1) {
		long longNA = NA<long>::value;
		std::vector<long> iins(nr, longNA);
		iplace.push_back(iv.size());
		iv.push_back(iins);
	} else {
		std::vector<std::string> sins(nr, NAS);
		iplace.push_back(sv.size());
		sv.push_back(sins);
	}
	itype.push_back(dtype);	
	names.push_back(name);
}

bool SpatDataFrame::add_column(std::vector<double> x, std::string name) {
	unsigned nr = nrow();
	if ((nr != 0) & (nr != x.size())) return false; 
	dv.push_back(x);
	iplace.push_back(dv.size());
	itype.push_back(0);	
	names.push_back(name);
	return true;
}


bool SpatDataFrame::add_column(std::vector<long> x, std::string name) {
	unsigned nr = nrow();
	if ((nr != 0) & (nr != x.size())) return false; 
	iv.push_back(x);
	iplace.push_back(iv.size());
	itype.push_back(1);	
	names.push_back(name);
	return true;
}


bool SpatDataFrame::add_column(std::vector<std::string> x, std::string name) {
	unsigned nr = nrow();
	if ((nr != 0) & (nr != x.size())) return false; 
	sv.push_back(x);
	iplace.push_back(sv.size());
	itype.push_back(2);	
	names.push_back(name);
	return true;
}

bool SpatDataFrame::cbind(SpatDataFrame &x) {
	unsigned nc = x.ncol();
	std::vector<std::string> nms = x.names;
	for (size_t i=0; i<nc; i++) {
		if (x.itype[i] == 0) {
			std::vector<double> d = getD(i);
			add_column(d, nms[i]);
		} else if (x.itype[i] == 1) {
			std::vector<long> d = getI(i);
			add_column(d, nms[i]);
		} else {
			std::vector<std::string> d = getS(i);
			add_column(d, nms[i]);			
		}
	}
	return true;
}

