#ifndef __INC_SINGLETON_H__
#define __INC_SINGLETON_H__

#include <assert.h>

template <typename T> class singleton
{
	public:
		static T * ms_singleton;

		singleton()
		{
			assert(!ms_singleton);
			long offset = (long) (T*) 1 - (long) (singleton <T>*) (T*) 1;
			ms_singleton = (T*) ((long) this + offset);
		}

		virtual ~singleton()
		{
			assert(ms_singleton);
			ms_singleton = 0;
		}

		static T & instance()
		{
			assert(ms_singleton);
			return (*ms_singleton);
		}

		static T & Instance()
		{
			assert(ms_singleton);
			return (*ms_singleton);
		}

		static T * instance_ptr()
		{
			return (ms_singleton);
		}
};

template <typename T> T * singleton <T>::ms_singleton = NULL;

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by Metin2hub.com
