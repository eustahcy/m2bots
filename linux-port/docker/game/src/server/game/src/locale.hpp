#ifndef __INC_METIN2_GAME_LOCALE_H__
#define __INC_METIN2_GAME_LOCALE_H__

extern "C"
{
	void locale_init(const char *filename);
	const char *locale_find(const char *string);

	extern int g_iUseLocale;

#define LC_TEXT(str) locale_find(str)
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f
