#ifndef __INC_LZO_MANAGER_H
#define __INC_LZO_MANAGER_H

#include "minilzo.h"

class LZOManager : public singleton<LZOManager>
{
	public:
		LZOManager();
		virtual ~LZOManager();

		bool	Compress(const BYTE* src, size_t srcsize, BYTE* dest, lzo_uint * puiDestSize);
		bool	Decompress(const BYTE* src, size_t srcsize, BYTE* dest, lzo_uint * puiDestSize);
		size_t	GetMaxCompressedSize(size_t original);

		BYTE *	GetWorkMemory() { return m_workmem; }

	private:
		BYTE *	m_workmem;
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
