#include "main.h"
#include <algorithm>
#include <condition_variable>

using namespace std;

Tags g_tags;
PLUGIN_EXPOSE(Tags, g_tags);
IShopApi* g_pShopCore;
IUtilsApi* g_pUtils;
IPlayersApi* g_pPlayers;
IMySQLConnection* g_pConnection = nullptr;

IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
map<string, string> g_items;

map<string, int> g_itemDurations;
vector<string> items;
string ClanTag[64];
string CurrentItem[64];
bool isCustom[64];
vector<tagInfo> tagInform;

string deleteRestrictedSymbols(std::string str) {
    const string restricted = "\'\";-=*/";
    str.erase(std::remove_if(str.begin(), str.end(), [&restricted](char c) {
            return restricted.find(c) != std::string::npos;
    }),
    str.end());
    return str;
}

void insertToDatabase(int iSlot, const char* tag, long long expires);
void loadCustomTagFromDB(int iSlot, uint64 steamID64);
void deleteFromDB(uint64 steamID64);
CGameEntitySystem* GameEntitySystem()
{
    return g_pUtils->GetCGameEntitySystem();
}
void StartupServer()
{
    g_pGameEntitySystem = GameEntitySystem();
    g_pEntitySystem = g_pUtils->GetCEntitySystem();
}

bool Tags::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();
    GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, g_pSource2GameClients, IServerGameClients, SOURCE2GAMECLIENTS_INTERFACE_VERSION);
    ConVar_Register(FCVAR_RELEASE | FCVAR_SERVER_CAN_EXECUTE | FCVAR_GAMEDLL);
    g_SMAPI->AddListener( this, this );
    return true;
}

void checkCustomTag(uint64 sid, function<void(bool, const tagInfo&)> callback) {
    char buf[256];
    g_SMAPI->Format(buf, sizeof(buf), "SELECT `tag`, `expires` FROM `%scustomTags` WHERE `steamid` = %llu", g_pShopCore->GetTablePrefix(), sid);

    g_pConnection->Query(buf, [sid, callback](ISQLQuery *pquery) {
        bool exists = false;
        tagInfo info = {"", 0, sid};

        if (pquery) {
            ISQLResult* presult = pquery->GetResultSet();
            if (presult && presult->FetchRow()) {
                info.tag = presult->GetString(0);
                info.expires = presult->GetInt(1);
                exists = true;
            }
        }
        else META_CONPRINTF("[TAGS] Query failed for steamid %llu\n", sid);

        callback(exists, info);
    });
}

bool EnableClanTag(int iSlot, const char* szCategory, const char* szItem, ItemState oldStatus, ItemState &newState, bool customTag = false)
{
    CCSPlayerController *player = CCSPlayerController::FromSlot(iSlot);
    uint64 sid = player->m_steamID;

    if (newState == ItemState_Enabled)
    {
        for (auto &item : items) {
            if (strcmp(item.c_str(), szItem) != 0) g_pShopCore->ChangeToggleItemState(iSlot, item.c_str(), szCategory, ItemState_Disabled);
        }
        CurrentItem[iSlot] = szItem;

        if (customTag) {
            isCustom[iSlot] = true;

            checkCustomTag(sid, [iSlot, player, szItem](bool exists, const tagInfo& info) {
                if (exists) {
                    time_t now = time(nullptr);
                    if (info.expires == 0 || info.expires > now) ClanTag[iSlot] = info.tag;
                    else {
                        deleteFromDB(info.sid);
                        ClanTag[iSlot] = "";
                    }
                }
                else {
                    g_pShopCore->PrintToChat(iSlot, g_pShopCore->GetTranslation("Tags_customTagGuide"));
                    ClanTag[iSlot] = "";
                }
            });
            return true;
        }
        isCustom[iSlot] = false;
        ClanTag[iSlot] = g_items[szItem];
    }
    else
    {
        isCustom[iSlot] = false;
        ClanTag[iSlot].clear();
        CurrentItem[iSlot].clear();
    }
    return true;
}

void deleteFromDB(uint64 steamID64) {
    char buf[256];
    g_SMAPI->Format(buf, sizeof(buf), "DELETE FROM `%scustomTags` WHERE `steamid` = %llu", g_pShopCore->GetTablePrefix(), steamID64);
    g_pConnection->Query(buf, [](ISQLQuery *q) {});
}

void OnShopLoaded()
{
    g_pConnection = g_pShopCore->GetDatabase();
    KeyValues* pKVConfig = new KeyValues("Tags");
    const char* pszPath = "addons/configs/shop/tags.ini";
    if (!pKVConfig->LoadFromFile(g_pFullFileSystem, pszPath)) {
        g_pUtils->ErrorLog("[TAGS] Failed to load config '%s'", pszPath);
        delete pKVConfig;
        return;
    }
    const char* szCategory = pKVConfig->GetString("category");
    char query[256];
    g_SMAPI->Format(query, sizeof(query), "CREATE TABLE IF NOT EXISTS `%scustomTags` ("
        "`steamid` BIGINT UNSIGNED NOT NULL PRIMARY KEY,"
        "`name` VARCHAR(64) NOT NULL,"
        "`tag` VARCHAR(64) NOT NULL,"
        "`expires` BIGINT UNSIGNED NOT NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;", g_pShopCore->GetTablePrefix());
    g_pConnection->Query(query, [](ISQLQuery* test) {
            if (!test) META_CONPRINTF("[Tags] Database Error\n");
            else META_CONPRINTF("[Tags] Successfully connected\n");
    });
    FOR_EACH_TRUE_SUBKEY(pKVConfig, pValue)
    {
        const char* szIdentity = pValue->GetName();
        items.push_back(szIdentity);
        const char* szName = pValue->GetString("name");
        int duration = pValue->GetInt("duration");
        g_itemDurations[szIdentity] = duration;
        bool hide = pValue->GetBool("hide", false);
        bool customTag = pValue->GetBool("isCustom", false);
        g_items[szIdentity] = pValue->GetString("tag", "xyi");
        vector<ItemPrice> vPrices;
        KeyValues* pKVPrices = pValue->FindKey("prices", false);
        if(pKVPrices)
        {
            FOR_EACH_SUBKEY(pKVPrices, pKey)
            {
                const char* szCurrency = pKey->GetName();
                ItemPrice price;
                price.iPrice = pKey->GetInt("price", 0);
                price.iSellPrice = pKey->GetInt("sellprice", 0);
                price.currency = g_pShopCore->GetCurrencyByIdentity(szCurrency);
                vPrices.push_back(price);
            }
        }
        g_pShopCore->RegisterItem(szCategory, szIdentity, szName, vPrices, Item_Togglable, g_itemDurations[szIdentity], hide);
        g_pShopCore->HookToggleItem(szCategory, szIdentity, [customTag](int iSlot, const char* szCategory, const char* szItem, ItemState oldSt, ItemState &newSt) {
            return EnableClanTag(iSlot, szCategory, szItem, oldSt, newSt, customTag);
        });
        g_pShopCore->HookSellItem(szCategory, szIdentity, [customTag](int iSlot, const char* szCategory, const char* szItem, const char* szCurrency, int &iSellPrice) -> bool
            {
                if (customTag) {
                    CCSPlayerController *player = CCSPlayerController::FromSlot(iSlot);
                    if (!player) return false;
                    uint64 sid = player->m_steamID;
                    deleteFromDB(sid);
                }
                return true;
            }
        );
    }
    delete pKVConfig;
}

bool OnPlayerSetCustomTag(int iSlot, const char* szContent) {
    if (!isCustom[iSlot] || !ClanTag[iSlot].empty()) return false;

    string input = deleteRestrictedSymbols(szContent);

    if (input.empty())
    {
        g_pShopCore->PrintToChat(iSlot, g_pShopCore->GetTranslation("Tags_NameError"));
        return true;
    }
    if (input.length() > 21)
    {
        g_pShopCore->PrintToChat(iSlot, g_pShopCore->GetTranslation("Tags_tooLongError"));
        return true;
    }

    if (input.length() < 2)
    {
        g_pShopCore->PrintToChat(iSlot, g_pShopCore->GetTranslation("Tags_tooShortError"));
        return true;
    }
    if (input.find("!tag") == 0) {
        input.erase(0, 4);
    }
    size_t start = input.find_first_not_of(" \t");
    if (start != string::npos) input = input.substr(start);
    else input.clear();

    ClanTag[iSlot] = input;
    CCSPlayerController* pPlayer = CCSPlayerController::FromSlot(iSlot);
    if (pPlayer) {
        pPlayer->m_szClan = CUtlSymbolLarge(ClanTag[iSlot].c_str());
    }
    g_pShopCore->PrintToChat(iSlot, g_pShopCore->GetTranslation("Tags_customTagSet"));
    auto it = g_itemDurations.find(CurrentItem[iSlot]);
    int duration = (it != g_itemDurations.end()) ? it->second : 0;
    insertToDatabase(iSlot, ClanTag[iSlot].c_str(), duration);
    return true;
}

void insertToDatabase(int iSlot, const char* tag, long long durationSeconds) {
    CCSPlayerController *player = CCSPlayerController::FromSlot(iSlot);
    if (!player) return;

    uint64 sid = player->m_steamID;
    string name = g_pPlayers->GetPlayerName(iSlot);

    long long expires = 0;
    if (durationSeconds > 0) {
        time_t now = time(nullptr);
        expires = (long long)now + durationSeconds;
    }

    char szBuffer[512];
    g_SMAPI->Format(szBuffer, sizeof(szBuffer),
        "INSERT INTO `%scustomTags` (`steamid`, `name`, `tag`, `expires`) "
        "VALUES (%llu, '%s', '%s', %lld) "
        "ON DUPLICATE KEY UPDATE `name` = '%s', `tag` = '%s', `expires` = %lld",
        g_pShopCore->GetTablePrefix(), sid, name.c_str(), tag, expires,
        name.c_str(), tag, expires);
    g_pConnection->Query(szBuffer, [](ISQLQuery *pQuery) {});
}

void loadCustomTagFromDB(int iSlot, uint64 steamID64) {
    char szBuffer[256];
    g_SMAPI->Format(szBuffer, sizeof(szBuffer), "SELECT `tag`, `expires` FROM `%scustomTags` WHERE `steamid` = %llu LIMIT 1", g_pShopCore->GetTablePrefix(), steamID64);

    g_pConnection->Query(szBuffer, [iSlot, steamID64](ISQLQuery *pQuery) {
        ISQLResult *pResult = pQuery->GetResultSet();
        if (pResult && pResult->FetchRow()) {
            const char* tag = pResult->GetString(0);
            long long expires = pResult->GetInt(1);
            time_t now = time(nullptr);

            if (tag && tag[0] != '\0' && (expires == 0 || expires > (long long)now)) {
                ClanTag[iSlot] = tag;
                isCustom[iSlot] = true;
            } else {
                deleteFromDB(steamID64);
                ClanTag[iSlot].clear();
                isCustom[iSlot] = false;
            }
        } else {
            ClanTag[iSlot].clear();
            isCustom[iSlot] = false;
        }
    });
}

void OnPlayerSpawn(const char* szName, IGameEvent* event, bool bDontBroadcast) {
    CBasePlayerController* pPlayerController = static_cast<CBasePlayerController*>(event->GetPlayerController("userid"));
    if (!pPlayerController) return;
    g_pUtils->NextFrame([hPlayerController = CHandle<CBasePlayerController>(pPlayerController), pPlayerSlot = event->GetPlayerSlot("userid")]()
    {
        CCSPlayerController* pPlayerController = static_cast<CCSPlayerController*>(hPlayerController.Get());
        if (!pPlayerController)
            return;

        CCSPlayerPawnBase* pPlayerPawn = pPlayerController->m_hPlayerPawn();
        if (!pPlayerPawn || pPlayerPawn->m_lifeState() != LIFE_ALIVE)
            return;
        pPlayerController->m_szClan = CUtlSymbolLarge(ClanTag[pPlayerSlot.Get()].c_str());
    });
}

void OnPlayerConnect(const char* szName, IGameEvent* pEvent, bool bDontBroadcast) {
    int iSlot = pEvent->GetInt("userid");
    CCSPlayerController *player = CCSPlayerController::FromSlot(iSlot);
    ClanTag[iSlot] = "";
    isCustom[iSlot] = false;
    CurrentItem[iSlot] = "";
    loadCustomTagFromDB(iSlot, player->m_steamID);
}

void Tags::AllPluginsLoaded()
{
    int ret;
    char error[64] = { 0 };
    g_pUtils = (IUtilsApi *)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);
    if (ret == META_IFACE_FAILED)
    {
        V_strncpy(error, "Missing Utils system plugin", 64);
        ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", g_tags.GetLogTag(), error);
        std::string sBuffer = "meta unload "+std::to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    g_pPlayers = (IPlayersApi *)g_SMAPI->MetaFactory(PLAYERS_INTERFACE, &ret, NULL);
    if (ret == META_IFACE_FAILED)
    {
        g_pUtils->ErrorLog("[%s] Missing Players system plugin", g_tags.GetLogTag());
        std::string sBuffer = "meta unload "+std::to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    g_pShopCore = (IShopApi*)g_SMAPI->MetaFactory(SHOP_INTERFACE, &ret, NULL);
    if (ret == META_IFACE_FAILED)
    {
        g_pUtils->ErrorLog("[%s] Missing Shop system plugin", g_tags.GetLogTag());
        std::string sBuffer = "meta unload "+std::to_string(g_PLID);
        engine->ServerCommand(sBuffer.c_str());
        return;
    }
    g_pUtils->StartupServer(g_PLID, StartupServer);

    g_pShopCore->HookOnCoreIsReady(g_PLID, OnShopLoaded);
    g_pUtils->RegCommand(g_PLID, {"mm_settag"}, {"!tag"}, OnPlayerSetCustomTag);
    ConVar_Register(FCVAR_CLIENT_CAN_EXECUTE);
    g_pUtils->HookEvent(g_PLID, "player_spawn", OnPlayerSpawn);
    g_pUtils->HookEvent(g_PLID, "player_connect_full", OnPlayerConnect);
}

bool Tags::Unload(char *error, size_t maxlen)
{
    g_pUtils->ClearAllHooks(g_PLID);
    return true;
}
const char *Tags::GetAuthor() {
    return "ShadowRipper"; //TY Pisex for ur github <3
}
const char *Tags::GetDate() {
    return __DATE__;
}
const char *Tags::GetDescription() {
    return "Clan Tags for pisex`s shop";
}
const char *Tags::GetLicense() {
    return "Free";
}
const char *Tags::GetLogTag() {
    return "[SHOP][Tags]";
}
const char *Tags::GetName() {
    return "[SHOP] Tags";
}
const char *Tags::GetURL() {
    return "";
}
const char *Tags::GetVersion() {
    return "1.0";
}