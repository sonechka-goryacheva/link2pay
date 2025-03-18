#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tgbot/tgbot.h>
#include <vector>

using namespace TgBot;
using namespace std;

class UserData {
private:
    string phoneNumber;
    string userTag;
    string userName;

public:
    UserData()
        : phoneNumber("no number"), userTag("no tag"), userName("no username") {}

    string getPhoneNumber() const { return phoneNumber; }
    string getUserTag() const { return userTag; }
    string getUserName() const { return userName; }

    void setPhoneNumber(const string &newPhoneNumber) {
        phoneNumber = newPhoneNumber;
    }

    void setUserTag(const string &newUserTag) {
        userTag = newUserTag;
    }

    void setUserName(const string &newUserName) {
        userName = newUserName;
    }

    void printUSData() const {
        cout << "номер телефона " << phoneNumber << " тэг " << userTag << " ник "
             << userName << endl;
    }
};

bool isValidPhoneNumber(const string &phoneNumber) {
    // Проверяем, начинается ли номер с +7 (12 символов) или 8 (11 символов)
    if ((phoneNumber.find("+7") == 0 && phoneNumber.length() == 12) ||
        (phoneNumber.find("8") == 0 && phoneNumber.length() == 11)) {
        return true;
    }
    return false;
}

bool isValidPhoneNumberTGSend(const string &phoneNumber) {
    if (phoneNumber.find("7") == 0 && phoneNumber.length() == 11) {
        return true;
    }
    return false;
}

int main() {
    // Создаем объект бота с вашим токеном
    Bot bot("7980750125:AAHZgBqELthMbEC6OxqH4z-3arcc7hvkRhk");

    // Хранилище данных пользователей
    map<int64_t, UserData> users;

    // Обработчик команды /start
    bot.getEvents().onCommand("start", [&bot](TgBot::Message::Ptr message) {
        // Создаем клавиатуру для запроса номера телефона
        auto keyboard = std::make_shared<TgBot::ReplyKeyboardMarkup>();
        TgBot::KeyboardButton::Ptr button(new TgBot::KeyboardButton);
        button->text = "Отправить данные";
        button->requestContact = true;
        std::vector<TgBot::KeyboardButton::Ptr> row = {button};
        keyboard->keyboard.push_back(row);
        bot.getApi().sendMessage(
            message->chat->id,
            "Добро пожаловать в Link2Pay!\n\nЧтобы продолжить, необходимо дать "
            "согласие на обработку персональных данных.",
            nullptr, 0, keyboard);
    });

    bot.getEvents().onAnyMessage([&bot, &users](TgBot::Message::Ptr message) {
        // Игнорируем команду /start, так как она уже обрабатывается в onCommand
        if (message->text == "/start") {
            return;
        }

        // Получаем или создаем данные пользователя
        UserData &userData = users[message->chat->id];

        if (message->contact != nullptr) {
            string phoneNumber = message->contact->phoneNumber;
            if (isValidPhoneNumberTGSend(phoneNumber)) {
                userData.setPhoneNumber(phoneNumber);

                // Создаем inline-клавиатуру с кнопками "Да" и "Нет"
                auto inlineKeyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
                TgBot::InlineKeyboardButton::Ptr yesButton(new TgBot::InlineKeyboardButton);
                yesButton->text = "Да";
                yesButton->callbackData = "confirm_phone:" + userData.getPhoneNumber();

                TgBot::InlineKeyboardButton::Ptr noButton(new TgBot::InlineKeyboardButton);
                noButton->text = "Нет";
                noButton->callbackData = "request_manual_phone";

                std::vector<TgBot::InlineKeyboardButton::Ptr> row = {yesButton, noButton};
                inlineKeyboard->inlineKeyboard.push_back(row);

                // Отправляем сообщение с inline-клавиатурой
                bot.getApi().sendMessage(
                    message->chat->id,
                    "Ваш счёт привязан к этому номеру телефона?: +" +
                        userData.getPhoneNumber(),
                    nullptr, 0, inlineKeyboard);
            } else {
                bot.getApi().sendMessage(message->chat->id,
                                         "Номер телефона недействителен. Пожалуйста, "
                                         "введите корректный номер телефона.");
            }
        } else if (message->text == "Нет") {
            // Если пользователь вручную ввел "Нет", запрашиваем номер телефона
            bot.getApi().sendMessage(message->chat->id,
                                     "Пожалуйста, введите новый номер телефона:");
        } else {
            // Проверяем любой введённый текст на соответствие формату номера телефона
            string phoneNumber = message->text;
            if (isValidPhoneNumber(phoneNumber)) {
                userData.setPhoneNumber(phoneNumber);
                auto inlineKeyboard = std::make_shared<TgBot::InlineKeyboardMarkup>();
                TgBot::InlineKeyboardButton::Ptr yesButton(new TgBot::InlineKeyboardButton);
                yesButton->text = "Да";
                yesButton->callbackData = "confirm_phone:" + userData.getPhoneNumber();

                TgBot::InlineKeyboardButton::Ptr noButton(new TgBot::InlineKeyboardButton);
                noButton->text = "Нет";
                noButton->callbackData = "request_manual_phone";

                std::vector<TgBot::InlineKeyboardButton::Ptr> row = {yesButton, noButton};
                inlineKeyboard->inlineKeyboard.push_back(row);

                // Отправляем сообщение с inline-клавиатурой
                bot.getApi().sendMessage(
                    message->chat->id,
                    "Ваш счёт привязан к этому номеру телефона?: " +
                        userData.getPhoneNumber(),
                    nullptr, 0, inlineKeyboard);
            } else {
                bot.getApi().sendMessage(message->chat->id,
                                         "Номер телефона недействителен. Пожалуйста, "
                                         "введите корректный номер телефона.");
            }
        }

        // Обновляем имя и тэг пользователя
        string name = (message->chat->firstName) + " " + (message->chat->lastName);
        userData.setUserName(name);
        User::Ptr user = message->from;
        string teg = user->username;
        userData.setUserTag(teg);

        // Выводим данные пользователя в консоль
        userData.printUSData();
    });

    // Обработчик callback-запросов
    bot.getEvents().onCallbackQuery([&bot, &users](TgBot::CallbackQuery::Ptr query) {
        if (query->data.find("confirm_phone:") == 0) {
            // Извлекаем номер телефона из callbackData
            string phoneNumber = query->data.substr(std::string("confirm_phone:").length());

            // Уведомляем пользователя о успешном подтверждении
            bot.getApi().answerCallbackQuery(query->id, "Номер телефона сохранён!");
        } else if (query->data == "request_manual_phone") {
            // Если нажата кнопка "Нет", запрашиваем ручной ввод номера
            bot.getApi().sendMessage(
                query->message->chat->id,
                "Пожалуйста, введите ваш номер телефона вручную:");

            // Уведомляем пользователя о запросе
            bot.getApi().answerCallbackQuery(query->id,
                                             "Введите номер телефона вручную.");
        }
    });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    } catch (TgException &e) {
        printf("error: %s\n", e.what());
    }
    return 0;
}
