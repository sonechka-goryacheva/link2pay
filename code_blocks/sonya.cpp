#include <tgbot/tgbot.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <map>


using namespace std;
using namespace TgBot;
struct metadata {
    int64_t chat_id;
    string phone_number;
    bool operator<(const metadata& data) {
        if (chat_id != data.chat_id) {
            return chat_id < data.chat_id;
        }
        else {
            return phone_number < data.phone_number;
        }
    }

};

map <string, metadata> tg_id;// = {"", metadata{1435929932, ""}};
map <int64_t, string> id_tg;


bool isRegistered(int64_t chatId) {
    return id_tg.find(chatId) != id_tg.end();
}

int main() {
    TgBot::Bot bot("");

    bot.getEvents().onCommand("start", [&bot](Message::Ptr message) {
        if (message) {
            bot.getApi().sendMessage(message->chat->id, "Привет! Я бот, который может отправлять сообщения другим пользователям. Введите свой @username и номер телефона для регистрации в системе, а потом другой @username для отправки сообщений");
        }
    });

    bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
        if (!message) return; // Проверка на nullptr

        int64_t chatId = message->chat->id;
        string text = message->text;

        if (StringTools::startsWith(text, "/start")) {
            return;
        }

        if (!isRegistered(chatId)) {
            if (text.find('@') == string::npos || text[0] != '@') {
                bot.getApi().sendMessage(chatId, "Введите свой @username для регистрации в системе");
                return;
            }


            if (count(text.begin(), text.end(), ' ') != 1)
            {
                bot.getApi().sendMessage(chatId, "Некорректный ввод");
                return;
            }

            tg_id[text.substr(0, text.find(" "))] = metadata{chatId, text.substr(text.find(" ") + 1)};
            id_tg[chatId] = text.substr(0, text.find(" "));

            bot.getApi().sendMessage(chatId, "Пользователь добавлен!");
        } else {
            if (text.find('@') == string::npos) {
                bot.getApi().sendMessage(chatId, "Использование:  @username");
                return;
            }

            try {
                if (tg_id.find(text) != tg_id.end()) {
                    bot.getApi().sendMessage(tg_id[text].chat_id, "5");
                    bot.getApi().sendMessage(chatId, "Сообщение отправлено!");
                } else {
                    bot.getApi().sendMessage(chatId, "Пользователь не найден.");
                }
            } catch (TgException& e) {
                bot.getApi().sendMessage(chatId, "Ошибка: " + string(e.what()));
            }
        }
    });

    signal(SIGINT, [](int s) {
        printf("SIGINT got\n");
        exit(0);
    });

    try {
        cout << "Бот запущен..." << endl;
        bot.getApi().deleteWebhook();
        TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }
    } catch (TgException& e) {
        cout << "Ошибка: " << e.what() << endl;
    }

    return 0;
}
