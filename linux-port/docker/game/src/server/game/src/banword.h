#ifndef BANWORD_MANAGER_H_
#define BANWORD_MANAGER_H_

#include <boost/unordered_map.hpp>

class CBanwordManager : public singleton<CBanwordManager>
{
	public:
		CBanwordManager();
		virtual ~CBanwordManager();

		bool Initialize(TBanwordTable * p, WORD wSize);
		bool Find(const char * c_pszString);
		bool CheckString(const char * c_pszString, size_t _len);
		void ConvertString(char * c_pszString, size_t _len);

	protected:
		typedef boost::unordered_map<std::string, bool> TBanwordHashmap;
		TBanwordHashmap m_hashmap_words;
};

#endif /* BANWORD_MANAGER_H_ */
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
