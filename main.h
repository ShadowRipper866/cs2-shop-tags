#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <ISmmPlugin.h>
#include <sh_vector.h>
#include <stdio.h>
#include <fstream>
#include <iserver.h>
#include <entity2/entitysystem.h>
#include "igameevents.h"
#include "vector.h"
#include <deque>
#include <functional>
#include <utlstring.h>
#include <KeyValues.h>
#include "CCSPlayerController.h"
#include <ctime>
#include "time.h"
#include "include/mysql_mm.h"
#include <complex>
#include <iomanip>
#include "metamod_oslink.h"
#include "schemasystem/schemasystem.h"
#include "include/menus.h"
#include "algorithm"
#include "include/shop.h"
using namespace std;

struct tagInfo {
    string tag;
    long long expires;
    uint64 sid;
};

class Tags final : public ISmmPlugin, public IMetamodListener {
public:
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
    bool Unload(char* error, size_t maxlen);
    void AllPluginsLoaded();
private:
    const char* GetAuthor();
    const char* GetName();
    const char* GetDescription();
    const char* GetURL();
    const char* GetLicense();
    const char* GetVersion();
    const char* GetDate();
    const char* GetLogTag();
};

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
