#include <tgbot/tgbot.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <map>
#include <random>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>

using namespace std;
using namespace TgBot;

// Новые классы для обработки запроса перевода

// Вложенный класс для запроса ссылки
class LinkRequest {
private:
    double transferAmount;      // Сумма перевода
    string message;             // Сообщение
    string recipientRequisites; // Реквизиты счета получателя

public:
    LinkRequest() : transferAmount(0.0), message("noMessage"), recipientRequisites("default_requisites") {}

    // Геттеры
    double getTransferAmount() const {
        return transferAmount;
    }

    string getMessage() const {
        return message;
    }

    string getRecipientRequisites() const {
        return recipientRequisites;
    }

    // Сеттеры
    void setTransferAmount(double newTransferAmount) {
        transferAmount = newTransferAmount;
    }

    void setMessage(const string& newMessage) {
        message = newMessage;
    }

    void setRecipientRequisites(const string& newRequisites) {
        recipientRequisites = newRequisites;
    }
};

class UserData {
private:
    string phoneNumber;
    string userTag;
    string userName;
public:
    UserData():
    phoneNumber("no number"), userTag("no tag"), userName("no username"){}
    string getPhoneNumber() const {
        return phoneNumber;
    }
    string getUserTag() const {
        return userTag;
    }
    string getUserName() const {
        return userName;
    }
    void setPhoneNumber(const string& newPhoneNumber) {
        phoneNumber = newPhoneNumber;
    }
    void setUserTag(const string& newUserTag) {
        userTag = newUserTag;
    }
    void setUserName(const string& newUserName) {
        userName = newUserName;
    }
    void printUSData() const {
        cout << "номер телефона " << phoneNumber << " тэг " << userTag << " ник " << userName;
    }
};

// Основной класс для запроса на перевод
class TransferRequest {
private:
    int id;                     // ID запроса
    string payerTag;            // Тэг плательщика
    string recipientTag;        // Тэг получателя
    LinkRequest linkRequest;    // Вложенный объект для запроса ссылки

public:
    // Конструктор
    TransferRequest():
     id(1), payerTag("default_payer"), recipientTag("default_recipient") {}

    // Геттеры
    int getId() const {
        return id;
    }

    string getPayerTag() const {
        return payerTag;
    }

    string getRecipientTag() const {
        return recipientTag;
    }

    LinkRequest getLinkRequest() const {
        return linkRequest;
    }

    // Сеттеры
    void setPayerTag(const string& newPayerTag) {
        payerTag = newPayerTag;
    }

    void setRecipientTag(const string& newRecipientTag) {
        recipientTag = newRecipientTag;
    }

    void setLinkRequest(const LinkRequest& newLinkRequest) {
        linkRequest = newLinkRequest;
    }

    // Метод для вывода данных
    void printData() const {
        cout << "ID: " << id << endl;
        cout << "Тэг плательщика: " << payerTag << endl;
        cout << "Тэг получателя: " << recipientTag << endl;
        cout << "Сумма перевода: " << linkRequest.getTransferAmount() << endl;
        cout << "Сообщение: " << linkRequest.getMessage() << endl;
        cout << "Реквизиты счета получателя: " << linkRequest.getRecipientRequisites() << endl;
    }
};

// Структура для хранения данных перевода (старый функционал)
struct SendMoneyData {
    string recipient;
    string amount;
    string message;
    string paymentDetails;
    enum State { None, WaitingForRecipient, WaitingForAmount, WaitingForMessage, WaitingForDetails, WaitingForConfirmation } state = None;
};

map<string, string> paymentDetailsMap;
map<int64_t, SendMoneyData> sendMoneyDataMap;


struct metadata {
    int64_t chat_id;
    string phone_number;
    bool operator<(const metadata& data) const {
        if (chat_id != data.chat_id) {
            return chat_id < data.chat_id;
        } else {
            return phone_number < data.phone_number;
        }
    }
};

map<string, metadata> tg_id;
map<int64_t, string> id_tg;
map<int64_t, string> pending_phone_changes;
map<int64_t, string> confirmation_codes;

bool isRegistered2(int64_t chatId) {
    return paymentDetailsMap.find(to_string(chatId)) != paymentDetailsMap.end();
}

void requestPaymentDetails(Bot& bot, int64_t chatId) {
    bot.getApi().sendMessage(chatId, "Введите реквизиты для перевода:");
    sendMoneyDataMap[chatId].state = SendMoneyData::WaitingForDetails;
}

void confirmPayment(Bot& bot, int64_t chatId) {
    SendMoneyData& data = sendMoneyDataMap[chatId];
    string confirmationText = "Подтвердите перевод:\n";
    confirmationText += "Получатель: " + data.recipient + "\n";
    confirmationText += "Сумма: " + data.amount + "\n";
    confirmationText += "Реквизиты: " + data.paymentDetails + "\n";

    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);
    InlineKeyboardButton::Ptr confirmBtn(new InlineKeyboardButton);
    confirmBtn->text = "Подтвердить";
    confirmBtn->callbackData = "confirm_send";
    keyboard->inlineKeyboard.push_back({ confirmBtn });

    bot.getApi().sendMessage(chatId, confirmationText, nullptr, nullptr, keyboard);
}


void print_functions(const Bot& bot, Message::Ptr message) {
    bot.getApi().sendMessage(message->chat->id, "Введите желаемую операцию:\n1. Отправить деньги /send <@username> <money> <msg>\n2. Запросить деньги");
}

void print_functions_with_buttons(const Bot& bot, Message::Ptr message) {
    InlineKeyboardMarkup::Ptr keyboard(new InlineKeyboardMarkup);

    // Создаем кнопку для отправки денег
    InlineKeyboardButton::Ptr sendMoneyBtn(new InlineKeyboardButton);
    sendMoneyBtn->text = "Отправить деньги";
    sendMoneyBtn->callbackData = "send_money";

    // Можно добавить другие кнопки, например, для запроса денег
    InlineKeyboardButton::Ptr requestMoneyBtn(new InlineKeyboardButton);
    requestMoneyBtn->text = "Запросить деньги";
    requestMoneyBtn->callbackData = "request_money";

    // Располагаем кнопки в одном ряду
    keyboard->inlineKeyboard.push_back({ sendMoneyBtn, requestMoneyBtn });


    bot.getApi().sendMessage(message->chat->id, "Выберите операцию:", nullptr, nullptr, keyboard);
}

string urlEncode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;
    for (char c : value) {
        // Не кодируем буквы, цифры и некоторые спецсимволы
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << setw(2) << int((unsigned char)c);
        }
    }
    return escaped.str();
}

// Формирование ссылки на оплату
string generatePaymentLink(const string &recipient, const string &amount, const string &paymentDetails) {
    // Базовый URL вашего платежного шлюза
    string baseUrl = "https://your-payment-gateway.com/create_payment";
    string link = baseUrl +
                  "?recipient=" + urlEncode(recipient) +
                  "&amount=" + urlEncode(amount) +
                  "&details=" + urlEncode(paymentDetails);
    return link;
}

int main() {
    Bot bot("7338207341:AAE2s1IPbp0YL6A16qHMRQIUn2nExzej6SU");

    bot.getEvents().onCommand("start", [&bot](Message::Ptr message) {
        if (message) {
            bot.getApi().sendMessage(message->chat->id, "Добро пожаловать в Link2Pay.\n\nЧтобы продолжить, необходимо дать согласие на обработку персональных данных.");
            if (!isRegistered2(message->chat->id)) {
                bot.getApi().sendMessage(message->chat->id, "Введите свой @username и номер телефона для регистрации в системе.");
            } else {
                print_functions_with_buttons(bot, message);
            }
        }
    });

    // Обработчик callback-запросов от inline-кнопок
    bot.getEvents().onCallbackQuery([&bot](CallbackQuery::Ptr query) {
        int64_t chatId = query->message->chat->id;

        if (query->data == "send_money") {
            // Инициализируем процесс отправки денег для данного пользователя
            sendMoneyDataMap[chatId] = SendMoneyData();
            sendMoneyDataMap[chatId].state = SendMoneyData::WaitingForRecipient;
            bot.getApi().sendMessage(chatId, "Введите имя получателя (например, @username):");
        } else if (query->data == "no_message") {
            if (sendMoneyDataMap.find(chatId) != sendMoneyDataMap.end() &&
                sendMoneyDataMap[chatId].state == SendMoneyData::WaitingForMessage) {
                sendMoneyDataMap[chatId].message = "";
                sendMoneyDataMap[chatId].state = SendMoneyData::WaitingForConfirmation;
                string confirmationText = "Проверьте данные:\n\n" +
                    string("Получатель: ") + sendMoneyDataMap[chatId].recipient + "\n" +
                    "Сумма: " + sendMoneyDataMap[chatId].amount;
                InlineKeyboardMarkup::Ptr confirmKeyboard(new InlineKeyboardMarkup);
                InlineKeyboardButton::Ptr confirmBtn(new InlineKeyboardButton);
                confirmBtn->text = "Подтвердить";
                confirmBtn->callbackData = "confirm_send";
                InlineKeyboardButton::Ptr changeBtn(new InlineKeyboardButton);
                changeBtn->text = "Изменить";
                changeBtn->callbackData = "change_send";
                confirmKeyboard->inlineKeyboard.push_back({ confirmBtn, changeBtn });


                bot.getApi().sendMessage(chatId, confirmationText, nullptr, nullptr, confirmKeyboard);
            }
        } else if (query->data == "confirm_send") {
            // Пользователь подтверждает данные перевода
            if (sendMoneyDataMap.find(chatId) != sendMoneyDataMap.end()) {
                SendMoneyData &data = sendMoneyDataMap[chatId];
                // Если пользователь получателя зарегистрирован в системе
                if (tg_id.find(data.recipient) != tg_id.end()) {
                    // Создаем объект запроса на перевод и заполняем его данными
                    TransferRequest transferRequest;
                    transferRequest.setPayerTag(id_tg[chatId]);
                    transferRequest.setRecipientTag(data.recipient);
                    LinkRequest lr;
                    // Преобразуем сумму в double (при необходимости, можно добавить обработку ошибок)
                    lr.setTransferAmount(stod(data.amount));
                    lr.setMessage(data.message);
                    lr.setRecipientRequisites(data.paymentDetails);
                    transferRequest.setLinkRequest(lr);


                    // Формируем ссылку на оплату
                    string paymentLink = generatePaymentLink(data.recipient, data.amount, data.paymentDetails);
                    bot.getApi().sendMessage(chatId, "Перевод подтвержден! Ссылка на оплату: " + paymentLink);

                    // Отправляем уведомление получателю
                    string sendMsg = "Вам поступил перевод денежных средств на сумму " + data.amount +
                        " от " + id_tg[chatId];
                    if (!data.message.empty()) {
                        sendMsg += "\nСообщение: " + data.message;
                    }
                    bot.getApi().sendMessage(tg_id[data.recipient].chat_id, sendMsg);
                } else {
                    bot.getApi().sendMessage(chatId, "Пользователь " + data.recipient + " не найден.");
                }
            }
            sendMoneyDataMap.erase(chatId);
        } else if (query->data == "change_send") {
            // Отмена и повторный ввод данных
            sendMoneyDataMap.erase(chatId);
            bot.getApi().sendMessage(chatId, "Отправка отменена. Введите заново имя получателя (например, @username):");
            sendMoneyDataMap[chatId] = SendMoneyData();
            sendMoneyDataMap[chatId].state = SendMoneyData::WaitingForRecipient;
        }
        bot.getApi().answerCallbackQuery(query->id);
    });

    // Обработчик обычных сообщений для пошагового ввода данных перевода
    bot.getEvents().onAnyMessage([&bot](Message::Ptr message) {
        if (!message) return;
        int64_t chatId = message->chat->id;
        string text = message->text;


        // Если активен процесс перевода, обрабатываем ввод в зависимости от состояния
        if (sendMoneyDataMap.find(chatId) != sendMoneyDataMap.end()) {
            SendMoneyData &data = sendMoneyDataMap[chatId];
            if (data.state == SendMoneyData::WaitingForRecipient) {
                if (text.empty() || text[0] != '@') {
                    bot.getApi().sendMessage(chatId, "Некорректный ввод. Введите имя получателя, начиная с '@'.");
                    return;
                }
                data.recipient = text;
                data.state = SendMoneyData::WaitingForAmount;
                bot.getApi().sendMessage(chatId, "Введите сумму перевода:");
            } else if (data.state == SendMoneyData::WaitingForAmount) {
                int count_dots = count(text.begin(), text.end(), '.');
                if (count_dots > 1) {
                    bot.getApi().sendMessage(chatId, "Некорректная сумма! Повторите ввод суммы:");
                    return;
                }
                bool is_money_correct = true;
                for (const auto& c: text) {
                    if (c != '.') {
                        if (c < '0' || c > '9') {
                            is_money_correct = false;
                            break;
                        }
                    }
                }
                if (!is_money_correct || text.empty()) {
                    bot.getApi().sendMessage(chatId, "Некорректная сумма! Повторите ввод суммы:");
                    return;
                }
                data.amount = text;
                data.state = SendMoneyData::WaitingForMessage;
                InlineKeyboardMarkup::Ptr noMsgKeyboard(new InlineKeyboardMarkup);
                InlineKeyboardButton::Ptr noMsgBtn(new InlineKeyboardButton);
                noMsgBtn->text = "Без сообщения";
                noMsgBtn->callbackData = "no_message";
                noMsgKeyboard->inlineKeyboard.push_back({ noMsgBtn });
                bot.getApi().sendMessage(chatId, "Введите сообщение к переводу или нажмите кнопку «Без сообщения»:", nullptr, nullptr, noMsgKeyboard);
            } else if (data.state == SendMoneyData::WaitingForMessage) {
                data.message = text;
                // Проверяем наличие получателя в системе
                if (tg_id.find(data.recipient) != tg_id.end()) {
                    data.state = SendMoneyData::WaitingForConfirmation;
                    string confirmationText = "Проверьте данные:\n\n" +
                        string("Получатель: ") + data.recipient + "\n" +
                        "Сумма: " + data.amount;
                    if (!data.message.empty()) {
                        confirmationText += "\nСообщение: " + data.message;
                    }
                    InlineKeyboardMarkup::Ptr confirmKeyboard(new InlineKeyboardMarkup);
                    InlineKeyboardButton::Ptr confirmBtn(new InlineKeyboardButton);
                    confirmBtn->text = "Подтвердить";
                    confirmBtn->callbackData = "confirm_send";
                    InlineKeyboardButton::Ptr changeBtn(new InlineKeyboardButton);
                    changeBtn->text = "Изменить";
                    changeBtn->callbackData = "change_send";
                    confirmKeyboard->inlineKeyboard.push_back({ confirmBtn, changeBtn });
                    bot.getApi().sendMessage(chatId, confirmationText, nullptr, nullptr, confirmKeyboard);
                } else {
                    bot.getApi().sendMessage(chatId, "Пользователь " + data.recipient + " не найден.");
                    sendMoneyDataMap.erase(chatId);
                }
            } else if (data.state == SendMoneyData::WaitingForDetails) {
                data.paymentDetails = text;
                confirmPayment(bot, chatId);
            }
            return;
        }


        // Обработка регистрации и других команд
        if (!isRegistered2(chatId)) {
            if (StringTools::startsWith(text, "/start")) {
                return;
            }
            if (text.find('@') == string::npos || text[0] != '@') {
                bot.getApi().sendMessage(chatId, "Введите свой @username для регистрации в системе");
                return;
            }
            if (count(text.begin(), text.end(), ' ') != 1) {
                bot.getApi().sendMessage(chatId, "Некорректный ввод");
                return;
            }
            tg_id[text.substr(0, text.find(" "))] = metadata{chatId, text.substr(text.find(" ") + 1)};
            id_tg[chatId] = text.substr(0, text.find(" "));
            bot.getApi().sendMessage(chatId, "Пользователь добавлен!");
            print_functions_with_buttons(bot, message);
        }

    });

    bot.getEvents().onCommand("send", [&bot](Message::Ptr message) {
        int64_t chatId = message->chat->id;
        string text = message->text;

        string username, msg, money;
        auto pos = text.find('@');
        if (pos != string::npos){
            username = text.substr(pos);
            bot.getApi().sendMessage(chatId, "find '@': username = " + username);
        }
        pos = username.find(' ');
        if (pos != string::npos) {
            money = username.substr(pos+1);
            username = username.substr(0,pos);
            bot.getApi().sendMessage(chatId, "find ' ': username = " + username + "\nmoney = " + money);
        }
        pos = money.find(' ');
        if (pos != string::npos) {
            msg = money.substr(pos + 1);
            money = money.substr(0, pos);
            bot.getApi().sendMessage(chatId, "find ' ' again: msg = " + msg + "\nmoney = " + money);
        }
        int count_dots = count(money.begin(), money.end(), '.');
        if (count_dots > 1) {
            bot.getApi().sendMessage(chatId, "Некорректная сумма!");
        } else {
            bool is_money_correct = true;
            for (const auto& c: money) {
                if (c != '.') {
                    if (c < '0' or c > '9') {
                        is_money_correct = false;
                    }
                }
            }
            if (!is_money_correct) {
                bot.getApi().sendMessage(chatId, "Некорректная сумма!");
            } else if (!money.empty()){
                try {
                    if (tg_id.find(username) != tg_id.end()) {
                        bot.getApi().sendMessage(chatId, "Проверьте данные:\n\nПолучатель: " + username + "\nСообщение: " + msg + "\nСумма: " + money);
                        bot.getApi().sendMessage(tg_id[username].chat_id, "Вам поступил перевод денежных средств на сумму " + money + " от " + id_tg[chatId] + "\nСообщение: " + msg);
                        bot.getApi().sendMessage(chatId, "Сообщение отправлено!");
                    } else {
                        bot.getApi().sendMessage(chatId, "Пользователь не найден.");
                    }
                } catch (const TgException& e) {
                    bot.getApi().sendMessage(chatId, "Ошибка: " + string(e.what()));

                } catch (const exception& e) {
                    bot.getApi().sendMessage(chatId, "Ошибка: " + string(e.what()));
                }
            } else {
                bot.getApi().sendMessage(chatId, "Некорректный формат!");
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
    } catch (const TgException& e) {
        cout << "Ошибка: " << e.what() << endl;
    } catch (const exception& e) {
        cout << "Ошибка: " << e.what() << endl;
    }

    return 0;
}
