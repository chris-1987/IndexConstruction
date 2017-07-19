//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Copyright (c) 2017, Sun Yat-sen University.
/// All rights reserved.
/// \file logger.h
/// \brief Record measurements (PDU + IOV). 
///
///
/// \author Yi Wu
/// \date 2017.7
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _LOGGER_H
#define _LOGGER_H

#include "common.h"


/// \brief a logger for recording pdu and iov
///
class Logger{

private:
	static double max_pdu; ///< maximum peak disk use

	static double cur_pdu; ///< current peak disk use

	static double cur_iv; ///< current input volume

	static double cur_ov; ///< current output volume
public:

	/// \brief increase pdu
	///
	static void addPDU(const double _delta) {

		cur_pdu += _delta;

		if (cur_pdu >= max_pdu) max_pdu = cur_pdu;
	}

	/// \brief decrease pdu
	static void delPDU(const double _delta) {

		cur_pdu -= _delta;
	}

	/// \brief increase iv
	///
	static void addIV(const double _delta) {

		cur_iv += _delta;
	}

	/// \brief increase ov
	///
	static void addOV(const double _delta) {

		cur_ov += _delta;
	}

	static void report(const uint64 _corpora_size) {

		std::cerr << "--------------------------------------------------------------\n";

		std::cerr << "MAX_MEM:" << MAX_MEM / 1024 / K_1024 << " GB" << std::endl;

		std::cerr << "Statistics collection:\n";

		std::cerr << "peak disk use: " << max_pdu / K_1024 / 1024 << " GB" << std::endl;

		std::cerr << "peak disk use (per char): " << max_pdu / _corpora_size << std::endl;

		std::cerr << "read volume: " << cur_iv / K_1024 / 1024 << " GB" << std::endl;

		std::cerr << "read volume (per char) " << cur_iv / _corpora_size <<std::endl;

		std::cerr << "write volume: " << cur_ov / K_1024 / 1024 << " GB" <<std::endl;

		std::cerr << "write volume (per char)" << cur_ov / _corpora_size << std::endl;
	}
};


double Logger::max_pdu = 0;

double Logger::cur_pdu = 0;

double Logger::cur_iv = 0;

double Logger::cur_ov = 0;

#endif
