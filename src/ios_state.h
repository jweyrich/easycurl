#pragma once

#include <ios>

class ios_state_saver {
//
// Based on boost:io::ios_base_all_saver
// http://www.boost.org/doc/libs/1_48_0/boost/io/ios_state.hpp
//
public:
	ios_state_saver(std::ios_base& s)
		: _s(s)
		, _fmtflags(s.flags())
		, _precision(s.precision())
		, _width(s.width())
		, _restored(false)
	{}
	~ios_state_saver() {
		if (_restored)
			return;
		restore();
	}
	void restore() {
		_s.flags(_fmtflags);
		_s.precision(_precision);
		_s.width(_width);
		_restored = true;
	}
private:
	std::ios_base&					_s;
	const std::ios_base::fmtflags	_fmtflags;
	const std::streamsize			_precision;
	const std::streamsize			_width;
	bool							_restored;
private:
	ios_state_saver(const ios_state_saver&);
	ios_state_saver& operator=(const ios_state_saver&);
};
