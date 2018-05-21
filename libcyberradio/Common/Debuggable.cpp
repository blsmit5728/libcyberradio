/***************************************************************************
 * \file Debuggable.cpp
 *
 * \brief Class that supports debug output.
 *
 * \author DA
* \copyright 2016 CyberRadio Solutions, Inc.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "LibCyberRadio/Common/Debuggable.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace LibCyberRadio
{

	Debuggable::Debuggable(
	        bool debug,
            const std::string& debug_name,
            FILE* debug_fp,
            const std::string& debug_timefmt
        ) :
		_debug(debug),
		_debugName(debug_name),
		_debugFp(debug_fp),
		_debugTimeFmt(debug_timefmt),
		_debugTimestamp(NULL),
		_debugTimestampSize(80)
	{
		_debugTimestamp = new char[_debugTimestampSize];
	}

	Debuggable::~Debuggable()
	{
		delete _debugTimestamp;
	}

	void Debuggable::setDebugName(
	        const std::string& debug_name
        )
	{
		_debugName = debug_name;
	}

	void Debuggable::setDebugFile(
	        FILE* debug_fp
        )
	{
		_debugFp = debug_fp;
	}

	void Debuggable::setDebugTimeFormat(
	        const std::string& debug_timefmt
        )
	{
		_debugTimeFmt = debug_timefmt;
	}

	int Debuggable::debug(
	        const char *format,
	        ...
        )
	{
		int ret = 0;
		if ( _debug && (_debugFp != NULL) )
		{
			if (_debugTimeFmt.length() > 0)
			{
				time_t now = time(NULL);
				memset(_debugTimestamp, 0, _debugTimestampSize);
				strftime(_debugTimestamp,
						 _debugTimestampSize,
						 _debugTimeFmt.c_str(),
						 localtime(&now));
				ret += fprintf(_debugFp, "[%s]", _debugTimestamp);
			}
			if (_debugName.length() > 0)
				ret += fprintf(_debugFp, "[%s] ", _debugName.c_str());
			if (ret >= 0)
			{
				va_list ap;
				va_start(ap, format);
				ret += vfprintf(_debugFp, format, ap);
				va_end(ap);
			}
		}
		return ret;
	}

    const char* Debuggable::debugBool(
            bool x
        )
    {
    	return ( x ? "true" : "false" );
    }

    bool Debuggable::isDebug() const
    {
        return _debug;
    }

    std::string Debuggable::getDebugName() const
    {
        return _debugName;
    }

} /* namespace LibCyberRadio */

