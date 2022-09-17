#ifndef __RTTID__H__
#define __RTTID__H__

#include <cstdint>

namespace rv
{
	inline static uint32_t DataTypeSeq = 0;
	template<class TData>
	const inline static uint32_t DataType = DataTypeSeq++;
}

#endif //!__RTTID__H__