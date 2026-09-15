
class CNameReservationManager : public singleton<CNameReservationManager>
{
public:
	CNameReservationManager();

	bool Initialize(TNameReservationTable* p, WORD wSize);
	DWORD FindOwner(const char* c_pszString);
	BYTE CheckReservation(DWORD account_id, const char* c_pszString);

protected:
	std::map<std::string, DWORD> m_map_NameReservations;
};


// Files shared by GameCore.top
