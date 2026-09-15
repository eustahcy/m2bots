#ifndef __INC_METIN_II_GAME_BUFFER_MANAGER_H__
#define __INC_METIN_II_GAME_BUFFER_MANAGER_H__
#include "../../common/stl.h"

class TEMP_BUFFER
{
	public:
		TEMP_BUFFER(int Size = 8192, bool ForceDelete = false );
		~TEMP_BUFFER();

		const void * 	read_peek();

		template<typename T, std::enable_if_t<utils::IsRawV<T>>* = nullptr>
		void write(const T& c_pvData) {
			write(&c_pvData, sizeof(T));
		}
		template<typename C, std::enable_if_t<utils::IsContiguousV<C>>* = nullptr>
		void write(const C& v) {
			write(v.data(), v.size() * sizeof(typename C::value_type));
		}

		void		write(const void * data, int size);
		int		size();
		void	reset();

		LPBUFFER	getptr() { return buf; }

	protected:
		LPBUFFER	buf;
		bool		forceDelete;
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
