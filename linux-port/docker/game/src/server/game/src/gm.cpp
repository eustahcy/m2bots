#include "stdafx.h"
#include "constants.h"
#include "gm.h"
#include "locale_service.h"
#include "config.h"

//ADMIN_MANAGER
std::set<std::string> g_set_Host;
std::map<std::string, tGM> g_map_GM;
std::vector<std::string> g_vector_ImplementorLoginList;

void gm_new_clear()
{
	g_set_Host.clear();
	g_map_GM.clear();
	g_vector_ImplementorLoginList.clear();
}

void gm_new_insert( const tAdminInfo &rAdminInfo )
{
	sys_log( 0, "InsertGMList(account:%s, player:%s, contact_ip:%s, server_ip:%s, auth:%d)",
			rAdminInfo.m_szAccount,
			rAdminInfo.m_szName,
			rAdminInfo.m_szContactIP,
			rAdminInfo.m_szServerIP,
			rAdminInfo.m_Authority);

	tGM t;

	if ( strlen( rAdminInfo.m_szContactIP ) == 0 )
	{
		t.pset_Host = &g_set_Host;
		sys_log(0, "GM Use ContactIP" );
	}
	else
	{
		t.pset_Host = NULL;
		sys_log(0, "GM Use Default Host List" );
	}

	memcpy ( &t.Info, &rAdminInfo, sizeof ( rAdminInfo ) );

	g_map_GM[rAdminInfo.m_szName] = t;
	if (rAdminInfo.m_Authority >= GM_IMPLEMENTOR)
	{
		sys_log(0, "IMPLEMENTOR %s", rAdminInfo.m_szAccount);
		g_vector_ImplementorLoginList.push_back(rAdminInfo.m_szAccount);
	}
}

void gm_new_host_inert( const char * host )
{
	g_set_Host.emplace(host);
	sys_log( 0, "InsertGMHost(ip:%s)", host );
}

BYTE gm_new_get_level( const char * name, const char * host, const char* account)
{
	if ( test_server ) return GM_IMPLEMENTOR;

	std::map<std::string, tGM >::iterator it = g_map_GM.find(name);

	if (g_map_GM.end() == it)
		return GM_PLAYER;

	// GERMAN_GM_NOT_CHECK_HOST

#ifdef ENABLE_NEWSTUFF
	if (!g_bGMHostCheck)
#else
	if (true)
#endif
	{
	    if (account)
	    {
		if ( strcmp ( it->second.Info.m_szAccount, account  ) != 0 )
		{
		    sys_log(0, "GM_NEW_GET_LEVEL : BAD ACCOUNT [ACCOUNT:%s/%s", it->second.Info.m_szAccount, account);
		    return GM_PLAYER;
		}
	    }
	    sys_log(0, "GM_NEW_GET_LEVEL : FIND ACCOUNT");
	    return it->second.Info.m_Authority;
	}
	// END_OF_GERMAN_GM_NOT_CHECK_HOST
	else
	{
	    if ( host )
	    {
		if ( it->second.pset_Host )
		{
		    if ( it->second.pset_Host->end() == it->second.pset_Host->find( host ) )
		    {
			sys_log(0, "GM_NEW_GET_LEVEL : BAD HOST IN HOST_LIST");
			return GM_PLAYER;
		    }
		}
		else
		{
		    if ( strcmp ( it->second.Info.m_szContactIP, host  ) != 0 )
		    {
			sys_log(0, "GM_NEW_GET_LEVEL : BAD HOST IN GMLIST");
			return GM_PLAYER;
		    }
		}
	    }
	    sys_log(0, "GM_NEW_GET_LEVEL : FIND HOST");

	    return it->second.Info.m_Authority;
	}
	return GM_PLAYER;
}

//END_ADMIN_MANAGER
BYTE gm_get_level(const char * name, const char * host, const char* account)
{
	return gm_new_get_level( name, host, account );
}

bool gm_is_login_implementor(const char* account)
{
	if (test_server)
		return true;

	return std::find(g_vector_ImplementorLoginList.begin(), g_vector_ImplementorLoginList.end(), account) != g_vector_ImplementorLoginList.end();
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
