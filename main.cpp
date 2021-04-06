#include <bot.h>
#include "Item.h"
#include <memory>
#include <libs/data/Item.h>
#include "interface.h"

#define TELEGRAM_TOKEN "1586156130:AAGi4NWZEbMVmsgOLuWmGD2jWJUFkqdGxd0"

//1586156130:AAGi4NWZEbMVmsgOLuWmGD2jWJUFkqdGxd0

#define CHAT_ID -1001197907014

void send_keyboard(Table& messages, Bot& bot, long long receiver_telegram_id, const vector<vector<Telegram::InlineKeyboardButton>>& keyboard, const string& text, const string& = "");

int extract_int_parameter(const string& str){
    const char* pchar = str.c_str();
    while(*pchar){
        if(*pchar++ == '_')
            return atoi(pchar);
    }
    return -1;
}

enum BUTTONS{
    NEW_CYCLE,
    MAIN_MENU,
    NEW_ENTRY,
    DEL_ENTRY,
    CYCLE,
    PUBLIC_CYCLES,

    //CYCLE OPTIONS
    CYCLE_NAME,
    WATER_PPM,
    FERT_TYPE,
    SUBSTRAT,
    CYCLE_SAVE,

    //ENTRY OPTIONS
    START_PPM,
    END_PPM,
    START_VOL,
    END_VOL,
    DAY_COUNT,
    NOTE,
    FLOWER,
    ENTRY_SAVE,

    //SUBSTRATES
    GHE_COCO,
    UGRO,
    UGRO_RHIZA,
    MIN_VATA,
    PLAGRON_PREMIUM,

    //FERTILIZERS
    PNS_ORIGINAL
};



void send_cycle_edit(Table& messages, Table& cycles, Bot& bot, long long telegram_id){
    int cycle_id = -1;

    vector<int> iDS = cycles.getIDs();
    for(auto it = iDS.begin(); it != iDS.end(); ++it){
        if(cycles.readLong(*it, "owner_tid") == telegram_id && !cycles.readInt(*it, "end_edit")) {
            cycle_id = *it;
            break;
        }
    }

    if(cycle_id < 0){
        cycle_id = cycles.addRow();
        cycles.writeLong(cycle_id, "owner_tid", telegram_id);
    }

    string ferts = "Не выбрано", substrate = "Не выбрано";

    if(cycles.readInt(cycle_id, "fert")){
        switch (cycles.readInt(cycle_id, "fert")) {
            case BUTTONS::PNS_ORIGINAL:
                ferts = "ПНС Оригинал";
                break;
            default:
                break;
        }
    }

    if(cycles.readInt(cycle_id, "substrate")){
        switch (cycles.readInt(cycle_id, "substrate")) {
            case BUTTONS::PLAGRON_PREMIUM:
                substrate = "Plagron Premium";
                break;
            case BUTTONS::GHE_COCO:
                substrate = "GHE COCO";
                break;
            case BUTTONS::UGRO:
                substrate = "UGRO";
                break;
            case BUTTONS::UGRO_RHIZA:
                substrate = "UGRO Rhiza";
                break;
            case BUTTONS::MIN_VATA:
                substrate = "Мин. вата";
                break;
            default:
                break;
        }
    }

    send_keyboard(messages, bot, telegram_id,
                  {{Telegram::InlineKeyboardButton("Название цикла: " + cycles.readString(cycle_id, "name"), to_string(BUTTONS::CYCLE_NAME) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("Удобрения: " + ferts, to_string(BUTTONS::FERT_TYPE) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("PPM чистой воды: " + to_string(cycles.readInt(cycle_id, "ppm_water")), to_string(BUTTONS::WATER_PPM) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("Субстрат: " + substrate, to_string(BUTTONS::SUBSTRAT) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("Готово", to_string(BUTTONS::CYCLE_SAVE) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("◀ Назад", to_string(BUTTONS::MAIN_MENU))}},
                  "Добавить новый цикл");
}

void delete_messages(Table& table, Bot& bot, long long telegram_id){
    vector<int> IDs = table.getIDs();
    for(auto it = IDs.begin(); it != IDs.end(); ++it){
        if(table.readLong(*it, "chat_tid") == telegram_id){
            bot.deleteMessage(telegram_id, table.readLong(*it, "message_tid"));
            table.removeRow(*it);
        }
    }
}

void send_message(Table& messages, Bot& bot, long long receiver_telegram_id, const string& text){
    int row_id = messages.addRow();
    int message_tid = bot.sendMessage(receiver_telegram_id, text);
    messages.writeLong(row_id, "chat_tid", receiver_telegram_id);
    messages.writeLong(row_id, "message_tid", message_tid);
}

void send_keyboard(Table& messages, Bot& bot, long long receiver_telegram_id, const vector<vector<Telegram::InlineKeyboardButton>>& keyboard, const string& text, const string& service){
    int row_id = messages.addRow();
    int message_tid = bot.sendKeyboard(receiver_telegram_id, text, keyboard);
    messages.writeLong(row_id, "chat_tid", receiver_telegram_id);
    messages.writeLong(row_id, "message_tid", message_tid);
    messages.writeString(row_id, "service", service);
}

void send_main_form(Table& messages, Table& cycles, Bot& bot, long long telegram_id, bool pub = false){
    vector<int> iDS = cycles.getIDs();
    for(auto jt = iDS.begin(); jt != iDS.end(); ++jt) {
        if (((cycles.readLong(*jt, "owner_tid") == telegram_id && !pub) || (pub && cycles.readLong(*jt, "owner_tid") != telegram_id)) &&
            cycles.readInt(*jt, "end_edit")) {
            send_keyboard(messages, bot, telegram_id, {{Telegram::InlineKeyboardButton("▶️Открыть", to_string(BUTTONS::CYCLE) + "_" + to_string(*jt))}},cycles.readString(*jt, "name"));
        }
    }
    if(pub)
        send_keyboard(messages, bot, telegram_id, {{Telegram::InlineKeyboardButton("◀ Назад", to_string(BUTTONS::MAIN_MENU))}},"✳ Публичные циклы");
    else
        send_keyboard(messages, bot, telegram_id, {{Telegram::InlineKeyboardButton("⏏  Добавить цикл", to_string(BUTTONS::NEW_CYCLE))},
                                                   {Telegram::InlineKeyboardButton("✳ Публичные циклы", to_string(BUTTONS::PUBLIC_CYCLES))}},"Главное меню");
}

void send_entry_edit(Table& messages, Table& entries, Table& cycles, Bot& bot, long long telegram_id, int cycle_id){ //with auth-check
    int entry_id = -1;

    vector<int> iDS = entries.getIDs();
    for(auto it = iDS.begin(); it != iDS.end(); ++it){
        if(entries.readInt(*it, "cycle_id") == cycle_id &&
            !entries.readInt(*it, "end_edit") &&
            cycles.readLong(cycle_id, "owner_tid") == telegram_id) {
            entry_id = *it;
            break;
        }
    }
    if(entry_id < 0){
        entry_id = entries.addRow();
        entries.writeInt(entry_id, "cycle_id", cycle_id);
    }

    string stage = "Раствор для роста (Стадия вегетации растений)";
    if(entries.readInt(entry_id, "flower"))
        stage = "Раствор для цветения (Стадия цветения растений)";

    send_keyboard(messages, bot, telegram_id,
                  {{Telegram::InlineKeyboardButton(stage, to_string(BUTTONS::FLOWER) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("Начальный PPM бака: " + to_string(entries.readInt(entry_id, "start_ppm")), to_string(BUTTONS::START_PPM) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("PPM бака перед заменой: " + to_string(entries.readInt(entry_id, "end_ppm")), to_string(BUTTONS::END_PPM) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("~ Кол-во раствора в начале, литры: " + to_string(entries.readInt(entry_id, "start_volume")), to_string(BUTTONS::START_VOL) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("~ Остаток раствора, литры: " + to_string(entries.readInt(entry_id, "end_volume")), to_string(BUTTONS::END_VOL) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("Срок жизни бака, дни: " + to_string(entries.readInt(entry_id, "days")), to_string(BUTTONS::DAY_COUNT) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("Текстовая заметка, необязательное поле: " + entries.readString(entry_id, "note"), to_string(BUTTONS::NOTE) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("Готово", to_string(BUTTONS::ENTRY_SAVE) + "_" + to_string(cycle_id))},
                   {Telegram::InlineKeyboardButton("◀ Назад", to_string(BUTTONS::MAIN_MENU))}},
                  "Завершенный бак к циклу *" + markdown_free(cycles.readString(cycle_id, "name")) + "*");

}

int get_edit_entry(Table& entries, Table& cycles, int cycle_id, long long telegram_id){
    vector<int> IDs = entries.getIDs();
    for(auto it = IDs.begin(); it != IDs.end(); ++it){
        if(entries.readInt(*it, "cycle_id") == cycle_id &&
           !bool(entries.readInt(*it, "end_edit")) &&
           cycles.readLong(cycle_id, "owner_tid") == telegram_id) {
            return *it;
        }
    }
    return -1;
}

int main(){
    Bot bot(TELEGRAM_TOKEN);

    Table auth("auth");
    auth.addCol("code", Table::TYPE::INT);
    auth.addCol("owner_tid", Table::TYPE::LONG);

    Table tg_messages("messages");
    tg_messages.addCol("message_tid", Table::TYPE::LONG);
    tg_messages.addCol("chat_tid", Table::TYPE::LONG);
    tg_messages.addCol("service", Table::TYPE::STRING);

    Table cycles("cycles");
    cycles.addCol("owner_tid", Table::TYPE::LONG);
    cycles.addCol("name", Table::TYPE::STRING);
    cycles.addCol("ppm_water", Table::TYPE::INT);
    cycles.addCol("fert", Table::TYPE::INT);
    cycles.addCol("substrate", Table::TYPE::INT);
    cycles.addCol("end_edit", Table::TYPE::INT);

    Table entries("entries");
    entries.addCol("cycle_id", Table::TYPE::INT);
    entries.addCol("start_ppm", Table::TYPE::INT);
    entries.addCol("end_ppm", Table::TYPE::INT);
    entries.addCol("start_volume", Table::TYPE::INT);
    entries.addCol("end_volume", Table::TYPE::INT);
    entries.addCol("days", Table::TYPE::INT);
    entries.addCol("flower", Table::TYPE::INT);
    entries.addCol("note", Table::TYPE::STRING);
    entries.addCol("end_edit", Table::TYPE::INT);

    while(bot.waitMessages(120)){
        vector<Telegram::Message> messages = bot.getMessages();
        for(auto it = messages.begin(); it != messages.end(); ++it){
            Telegram::Message& message = *it;

            if(message.getMessageType() == Telegram::Message::MESSAGE_TYPE::CALLBACK_MESSAGE){
                string data = message.getCallbackData();

                auto button = static_cast<BUTTONS>(atoi(data.c_str()));

                switch(button){
                    case BUTTONS::ENTRY_SAVE:{
                        int cycle_id = extract_int_parameter(data);

                        int entry_id = -1;

                        vector<int> iDS = entries.getIDs();
                        for(auto it = iDS.begin(); it != iDS.end(); ++it){
                            if(entries.readInt(*it, "cycle_id") == cycle_id &&
                               !entries.readInt(*it, "end_edit") &&
                               cycles.readLong(cycle_id, "owner_tid") == message.getSenderTelegramId()) {
                                entry_id = *it;

                                if(entries.readInt(entry_id, "start_ppm") &&
                                        entries.readInt(entry_id, "end_ppm") &&
                                        entries.readInt(entry_id, "start_volume") &&
                                        entries.readInt(entry_id, "end_volume") &&
                                        entries.readInt(entry_id, "days")) {
                                    entries.writeInt(entry_id, "end_edit", 1);

                                    delete_messages(tg_messages, bot, message.getSenderTelegramId());
                                    send_main_form(tg_messages, cycles, bot, message.getSenderTelegramId());
                                }
                                else
                                    send_message(tg_messages, bot, message.getSenderTelegramId(), markdown_free("Нулевые поля недопустимы. Пустым разрешается оставлять лишь поле Заметки"));

                                break;
                            }
                        }

                        break;
                    }
                    case BUTTONS::NEW_ENTRY:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        send_entry_edit(tg_messages, entries, cycles, bot, message.getSenderTelegramId(), extract_int_parameter(data));
                        break;
                    }

                    case BUTTONS::CYCLE:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());

                        int cycle_id = extract_int_parameter(data);

                        int vega_counter = 0, flower_counter = 0, all_cepm = 0, all_vepm = 0;

                        send_message(tg_messages, bot, message.getSenderTelegramId(), "Баки вегетации: ");

                        auto IDs = entries.getIDs();
                        //vega entries
                        for(auto it = IDs.begin(); it != IDs.end(); ++it){
                            if(entries.readInt(*it, "cycle_id") == cycle_id &&
                            entries.readInt(*it, "end_edit") &&
                            !bool(entries.readInt(*it, "flower"))){

                                int vepm = entries.readInt(*it, "start_ppm") * (double(entries.readInt(*it, "start_ppm")) / entries.readInt(*it, "end_ppm")) / (double(entries.readInt(*it, "end_volume")) / entries.readInt(*it, "start_volume")) / entries.readInt(*it, "days");
                                all_vepm += vepm;

                                send_message(tg_messages, bot, message.getSenderTelegramId(),

                                             "*Бак № " + to_string(++vega_counter) + "*" +
                                             markdown_free("\nPPM в начале: ") + to_string(entries.readInt(*it, "start_ppm")) + ", в конце: " + to_string(entries.readInt(*it, "end_ppm")) +
                                             markdown_free("\nЛитры в начале: ") + to_string(entries.readInt(*it, "start_volume")) + ", в конце: " + to_string(entries.readInt(*it, "end_volume")) +
                                             markdown_free("\nВремя жизни в днях: ") + to_string(entries.readInt(*it, "days")) + "\n" + markdown_free(entries.readString(*it, "note")) +
                                             "\nВЭПМ: " +
                                             to_string(vepm)

                                 );
                            }
                        }

                        if(!vega_counter)
                            send_message(tg_messages, bot, message.getSenderTelegramId(), "_Пусто_");

                        send_message(tg_messages, bot, message.getSenderTelegramId(), "Баки цветения: ");

                        for(auto it = IDs.begin(); it != IDs.end(); ++it){
                            if(entries.readInt(*it, "cycle_id") == cycle_id &&
                               entries.readInt(*it, "end_edit") &&
                               bool(entries.readInt(*it, "flower"))){

                                int cepm = entries.readInt(*it, "start_ppm") * (double(entries.readInt(*it, "start_ppm")) / entries.readInt(*it, "end_ppm")) / (double(entries.readInt(*it, "end_volume")) / entries.readInt(*it, "start_volume")) / entries.readInt(*it, "days");
                                all_cepm += cepm;

                                send_message(tg_messages, bot, message.getSenderTelegramId(),

                                             "*Бак № " + to_string(++flower_counter) + "*" +
                                             markdown_free("\nPPM в начале: ") + to_string(entries.readInt(*it, "start_ppm")) + ", в конце: " + to_string(entries.readInt(*it, "end_ppm")) +
                                             markdown_free("\nЛитры в начале: ") + to_string(entries.readInt(*it, "start_volume")) + ", в конце: " + to_string(entries.readInt(*it, "end_volume")) +
                                             markdown_free("\nВремя жизни в днях: ") + to_string(entries.readInt(*it, "days")) + "\n" + markdown_free(entries.readString(*it, "note")) +
                                             "\nЦЭПМ: " +
                                             to_string(cepm)

                                );
                            }
                        }

                        if(!flower_counter)
                            send_message(tg_messages, bot, message.getSenderTelegramId(), "_Пусто_");

                        int avg_cepm = 0, avg_vepm = 0;
                        if(vega_counter)
                            avg_vepm = double(all_vepm) / vega_counter;
                        if(flower_counter)
                            avg_cepm = double(all_cepm) / flower_counter;

                        send_message(tg_messages, bot, message.getSenderTelegramId(), "Средняя эффективность прироста масс\n↕ Вегетации: *" + to_string(avg_vepm) + "*\n↔ Цветения: *" + to_string(avg_cepm) + "*");

                        if(cycles.readLong(cycle_id, "owner_tid") == message.getSenderTelegramId())
                            send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                          {{Telegram::InlineKeyboardButton("Добавить бак", to_string(BUTTONS::NEW_ENTRY) + "_" + to_string(cycle_id))},
                                           {Telegram::InlineKeyboardButton("В меню", to_string(BUTTONS::MAIN_MENU))}},
                                          "Опции");
                        else
                            send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                          {{Telegram::InlineKeyboardButton("В меню", to_string(BUTTONS::MAIN_MENU))}},
                                          "Опции");

                        break;
                    }
                    case BUTTONS::MAIN_MENU:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        send_main_form(tg_messages, cycles, bot, message.getSenderTelegramId());

                        break;
                    }
                    case BUTTONS::PUBLIC_CYCLES:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        send_main_form(tg_messages, cycles, bot, message.getSenderTelegramId(), true);
                        break;
                    }

                    case BUTTONS::NEW_CYCLE:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        send_cycle_edit(tg_messages, cycles, bot, message.getSenderTelegramId());
                        break;
                    }
                    case BUTTONS::CYCLE_NAME:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());

                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_CYCLE))}},
                                      "Введите название цикла", to_string(BUTTONS::CYCLE_NAME));
                        break;
                    }
                    case BUTTONS::WATER_PPM:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());

                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_CYCLE))}},
                                      "Введите PPM вашей исходной воды", to_string(BUTTONS::WATER_PPM));
                        break;
                    }
                    case BUTTONS::SUBSTRAT:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());

                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("Plagron Premium", to_string(BUTTONS::PLAGRON_PREMIUM))},
                                       {Telegram::InlineKeyboardButton("GHE COCO", to_string(BUTTONS::GHE_COCO))},
                                       {Telegram::InlineKeyboardButton("UGRO RHIZA", to_string(BUTTONS::UGRO_RHIZA))},
                                       {Telegram::InlineKeyboardButton("UGRO", to_string(BUTTONS::UGRO))},
                                       {Telegram::InlineKeyboardButton("Мин. вата", to_string(BUTTONS::MIN_VATA))},
                                       {Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_CYCLE))}},
                                      "Выберите ваш субстрат");
                        break;
                    }
                    case BUTTONS::FERT_TYPE:{
                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("ПНС Оригинал", to_string(BUTTONS::PNS_ORIGINAL))},
                                       {Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_CYCLE))}},
                                      "Выберите ваши удобрения");
                        break;
                    }

                    case BUTTONS::CYCLE_SAVE:{
                        vector<int> iDS = cycles.getIDs();
                        for(auto jt = iDS.begin(); jt != iDS.end(); ++jt) {
                            if (cycles.readLong(*jt, "owner_tid") == message.getSenderTelegramId() &&
                                !cycles.readInt(*jt, "end_edit")) {
                                cycles.writeInt(*jt, "end_edit", 1);
                            }
                        }
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        send_main_form(tg_messages, cycles, bot, message.getSenderTelegramId());
                        break;
                    }

                    case BUTTONS::UGRO_RHIZA:
                    case BUTTONS::UGRO:
                    case BUTTONS::MIN_VATA:
                    case BUTTONS::PLAGRON_PREMIUM:
                    case BUTTONS::GHE_COCO:
                    case BUTTONS::PNS_ORIGINAL:{
                        vector<int> iDS = cycles.getIDs();
                        for(auto jt = iDS.begin(); jt != iDS.end(); ++jt){
                            if(cycles.readLong(*jt, "owner_tid") == message.getSenderTelegramId() && !cycles.readInt(*jt, "end_edit")){
                                if(button == BUTTONS::UGRO_RHIZA)
                                    cycles.writeInt(*jt, "substrate", BUTTONS::UGRO_RHIZA);
                                else if(button == BUTTONS::UGRO)
                                    cycles.writeInt(*jt, "substrate", BUTTONS::UGRO);
                                else if(button == BUTTONS::PLAGRON_PREMIUM)
                                    cycles.writeInt(*jt, "substrate", BUTTONS::PLAGRON_PREMIUM);
                                else if(button == BUTTONS::GHE_COCO)
                                    cycles.writeInt(*jt, "substrate", BUTTONS::GHE_COCO);
                                else if(button == BUTTONS::PNS_ORIGINAL)
                                    cycles.writeInt(*jt, "fert", BUTTONS::PNS_ORIGINAL);
                                break;
                            }
                        }
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        send_cycle_edit(tg_messages, cycles, bot, message.getSenderTelegramId());
                        break;
                    }

                    case BUTTONS::START_PPM:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        //"data" is cycle id
                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_ENTRY) + "_" + to_string(extract_int_parameter(data)))}},
                                      "Введите начальный PPM: ", to_string(BUTTONS::START_PPM) + "_" + to_string(extract_int_parameter(data)));
                        break;
                    }
                    case BUTTONS::END_PPM:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        //"data" is cycle id
                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_ENTRY) + "_" + to_string(extract_int_parameter(data)))}},
                                      "Введите конечный PPM: ", to_string(BUTTONS::END_PPM) + "_" + to_string(extract_int_parameter(data)));
                        break;
                    }
                    case BUTTONS::NOTE:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        //"data" is cycle id
                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_ENTRY) + "_" + to_string(extract_int_parameter(data)))}},
                                      "Введите заметку: ", to_string(BUTTONS::NOTE) + "_" + to_string(extract_int_parameter(data)));
                        break;
                    }
                    case BUTTONS::START_VOL:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        //"data" is cycle id
                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_ENTRY) + "_" + to_string(extract_int_parameter(data)))}},
                                      "Введите начальный объем: ", to_string(BUTTONS::START_VOL) + "_" + to_string(extract_int_parameter(data)));
                        break;
                    }
                    case BUTTONS::END_VOL:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        //"data" is cycle id
                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_ENTRY) + "_" + to_string(extract_int_parameter(data)))}},
                                      "Введите конечный объем: ", to_string(BUTTONS::END_VOL) + "_" + to_string(extract_int_parameter(data)));
                        break;
                    }
                    case BUTTONS::DAY_COUNT:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        //"data" is cycle id
                        cout << "extracted cycle id: " << extract_int_parameter(data) << endl;
                        send_keyboard(tg_messages, bot, message.getSenderTelegramId(),
                                      {{Telegram::InlineKeyboardButton("◀ Отмена", to_string(BUTTONS::NEW_ENTRY) + "_" + to_string(extract_int_parameter(data)))}},
                                      "Введите количество дней: ", to_string(BUTTONS::DAY_COUNT) + "_" + to_string(extract_int_parameter(data)));
                        break;
                    }
                    case BUTTONS::FLOWER:{
                        delete_messages(tg_messages, bot, message.getSenderTelegramId());
                        //"data" is cycle id
                        int cycle_id = extract_int_parameter(data);

                        int entry_id = get_edit_entry(entries, cycles, cycle_id, message.getSenderTelegramId());
                        if(entry_id >= 0)
                            entries.writeInt(entry_id, "flower", !bool(entries.readInt(entry_id, "flower")));

                        send_entry_edit(tg_messages, entries, cycles, bot, message.getSenderTelegramId(), cycle_id);
                        break;
                    }

                    default:
                        break;
                }
            }
            
            else if(message.getMessageType() == Telegram::Message::MESSAGE_TYPE::TEXT_MESSAGE){
                if(message.getText() == "/start"){
                    bot.deleteMessage(message.getSenderTelegramId(), message.getMessageId());
                    send_main_form(tg_messages, cycles, bot, message.getSenderTelegramId());
                }else{
                    vector<int> iDS = tg_messages.getIDs();
                    for(auto it = iDS.begin(); it != iDS.end(); ++it){
                        if(tg_messages.readLong(*it, "chat_tid") == message.getSenderTelegramId() && tg_messages.readString(*it, "service").size()){
                            bot.deleteMessage(message.getSenderTelegramId(), message.getMessageId());
                            switch (atoi(tg_messages.readString(*it, "service").c_str())){
                                case BUTTONS::CYCLE_NAME:
                                case BUTTONS::WATER_PPM:{
                                    vector<int> iDS = cycles.getIDs();
                                    for(auto jt = iDS.begin(); jt != iDS.end(); ++jt){
                                        if(cycles.readLong(*jt, "owner_tid") == message.getSenderTelegramId() && !cycles.readInt(*jt, "end_edit")) {
                                            if(atoi(tg_messages.readString(*it, "service").c_str()) == BUTTONS::CYCLE_NAME)
                                                cycles.writeString(*jt, "name", message.getText());
                                            else if(atoi(tg_messages.readString(*it, "service").c_str()) == BUTTONS::WATER_PPM)
                                                cycles.writeInt(*jt, "ppm_water", atoi(message.getText().c_str()));
                                            break;
                                        }
                                    }
                                    delete_messages(tg_messages, bot, message.getSenderTelegramId());
                                    send_cycle_edit(tg_messages, cycles, bot, message.getSenderTelegramId());

                                    break;
                                }
                                case BUTTONS::DAY_COUNT:
                                case BUTTONS::NOTE:
                                case BUTTONS::START_PPM:
                                case BUTTONS::END_PPM:
                                case BUTTONS::START_VOL:
                                case BUTTONS::END_VOL:{

                                    int cycle_id = extract_int_parameter(tg_messages.readString(*it, "service"));
                                    int entry_id = get_edit_entry(entries, cycles, cycle_id, message.getSenderTelegramId());

                                    if(atoi(tg_messages.readString(*it, "service").c_str()) == BUTTONS::DAY_COUNT)
                                        entries.writeInt(entry_id, "days", atoi(message.getText().c_str()));
                                    else if(atoi(tg_messages.readString(*it, "service").c_str()) == BUTTONS::START_PPM)
                                        entries.writeInt(entry_id, "start_ppm", atoi(message.getText().c_str()));
                                    else if(atoi(tg_messages.readString(*it, "service").c_str()) == BUTTONS::END_PPM)
                                        entries.writeInt(entry_id, "end_ppm", atoi(message.getText().c_str()));
                                    else if(atoi(tg_messages.readString(*it, "service").c_str()) == BUTTONS::START_VOL)
                                        entries.writeInt(entry_id, "start_volume", atoi(message.getText().c_str()));
                                    else if(atoi(tg_messages.readString(*it, "service").c_str()) == BUTTONS::END_VOL)
                                        entries.writeInt(entry_id, "end_volume", atoi(message.getText().c_str()));
                                    else if(atoi(tg_messages.readString(*it, "service").c_str()) == BUTTONS::NOTE)
                                        entries.writeString(entry_id, "note", message.getText());
                                    else
                                        throw runtime_error("service not found");

                                    delete_messages(tg_messages, bot, message.getSenderTelegramId());
                                    send_entry_edit(tg_messages, entries, cycles, bot, message.getSenderTelegramId(), cycle_id);

                                    break;
                                }

                                default:
                                    break;
                            }

                            break; //for
                        }
                    }
                }
            }
        }
        tg_messages.save();
        cycles.save();
        entries.save();
        auth.save();
    }
}



