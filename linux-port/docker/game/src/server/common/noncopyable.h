#ifndef INC_METIN_II_COMMON_NONCOPYABLE_TEMPLATE
#define INC_METIN_II_COMMON_NONCOPYABLE_TEMPLATE

class noncopyable
{
	protected:
		noncopyable() {}
		~noncopyable() {}

	private:
		noncopyable(const noncopyable &);
		noncopyable& operator = (const noncopyable &);
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by Metin2hub.com
