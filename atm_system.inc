#define CUSTOM_ATM_ID 0xFA
#define E_STREAMER_ATM_AREA   E_STREAMER_CUSTOM(CUSTOM_ATM_ID)

#define SetAreaATM(%0,%1)            Streamer_SetIntData(STREAMER_TYPE_AREA, %0,  E_STREAMER_ATM_AREA, %1)
#define GetAreaATM(%0)               Streamer_GetIntData(STREAMER_TYPE_AREA, %0,  E_STREAMER_ATM_AREA)

const MAX_ATM = 50;

const MAX_ATM_TRANSACTION_LIMIT = 5_000_000;

new areaid_player[MAX_PLAYERS];

enum {
    DIALOG_ATM_MENU = 1,
    DIALOG_ATM_WITHDRAW,
    DIALOG_ATM_DEPOSIT,
    DIALOG_ATM_BALANCE
}

enum E_ATM_STRUCT
{
    E_ATM_ID,

    bool:E_ATM_USED,
    
    Text3D: E_ATM_TEXT,
    E_ATM_OBJECT_ID,
    E_ATM_OBJECT_DYNAMIC,

    E_ATM_WORLD,
    E_ATM_INT,
    E_ATM_AREA,

    Float: E_ATM_POS_X,
    Float: E_ATM_POS_Y,
    Float: E_ATM_POS_Z,

    Float: E_ATM_POS_RX,
    Float: E_ATM_POS_RY,
    Float: E_ATM_POS_RZ
}

new atm_info[MAX_ATM][E_ATM_STRUCT];

enum
{
    ATM_OPERATION_WITHDRAW = 0,
    ATM_OPERATION_DEPOSIT,
    ATM_OPERATION_CHECK_BALANCE
} 

forward LoadATM();
public LoadATM()
{
    mysql_query(mysql, "CREATE TABLE IF NOT EXISTS `atm` (\
        `id` INT AUTO_INCREMENT PRIMARY KEY,\
        `object_id` INT NOT NULL,\
        `world_id` INT NOT NULL,\
        `interior_id` INT NOT NULL,\
        `pos_x` FLOAT NOT NULL,\
        `pos_y` FLOAT NOT NULL,\
        `pos_z` FLOAT NOT NULL,\
        `pos_rx` FLOAT NOT NULL,\
        `pos_ry` FLOAT NOT NULL,\
        `pos_rz` FLOAT NOT NULL\
    ) ENGINE=InnoDB;", false);

    mysql_query(mysql, "CREATE TABLE IF NOT EXISTS `atm_operation` (\
        `id` INT AUTO_INCREMENT PRIMARY KEY,\
        `account_id` INT NOT NULL,\
        `type` VARCHAR(20) NOT NULL,\
        `amount` INT NOT NULL,\
        `time` INT NOT NULL \
    ) ENGINE=InnoDB", false);

    return mysql_tquery(mysql, "SELECT * FROM atm", "Callback_LoadATM");
}

forward Callback_LoadATM();
public Callback_LoadATM()
{
    new rows = cache_num_rows();

    if(!rows)
        return print("Банкоматов в базе данных нет!");

    if(rows > MAX_ATM) 
        return print("Лимит банкоматов превышен!");

    for(new i; i < rows; i++)
    {
        cache_get_value_name_int(i, "id", atm_info[i][E_ATM_ID]);

        cache_get_value_name_int(i, "object_id", atm_info[i][E_ATM_OBJECT_ID]);
        cache_get_value_name_int(i, "world_id", atm_info[i][E_ATM_WORLD]);
        cache_get_value_name_int(i, "interior_id", atm_info[i][E_ATM_INT]);

        cache_get_value_name_float(i, "pos_x", atm_info[i][E_ATM_POS_X]);
        cache_get_value_name_float(i, "pos_y", atm_info[i][E_ATM_POS_Y]);
        cache_get_value_name_float(i, "pos_z", atm_info[i][E_ATM_POS_Z]);

        cache_get_value_name_float(i, "pos_rx", atm_info[i][E_ATM_POS_RX]);
        cache_get_value_name_float(i, "pos_ry", atm_info[i][E_ATM_POS_RY]);
        cache_get_value_name_float(i, "pos_rz", atm_info[i][E_ATM_POS_RZ]);

        atm_info[i][E_ATM_USED] = true;

        atm_info[i][E_ATM_OBJECT_DYNAMIC] = CreateDynamicObject(
            atm_info[i][E_ATM_OBJECT_ID], 
            atm_info[i][E_ATM_POS_X], 
            atm_info[i][E_ATM_POS_Y], 
            atm_info[i][E_ATM_POS_Z], 
            atm_info[i][E_ATM_POS_RX], 
            atm_info[i][E_ATM_POS_RY], 
            atm_info[i][E_ATM_POS_RZ], 
            atm_info[i][E_ATM_WORLD],
            atm_info[i][E_ATM_INT]
        );

        atm_info[i][E_ATM_AREA] = CreateDynamicSphere(
            atm_info[i][E_ATM_POS_X], atm_info[i][E_ATM_POS_Y], atm_info[i][E_ATM_POS_Z], 2.0, 
            atm_info[i][E_ATM_WORLD], atm_info[i][E_ATM_INT]
        );
        SetAreaATM(atm_info[i][E_ATM_AREA], i + 1);

        new text[64];

        format(text, sizeof(text), "Банкомат №%d\n{00FF00}Взаимодействие через {FFFF00}ALT", atm_info[i][E_ATM_ID]);

        atm_info[i][E_ATM_TEXT] = CreateDynamic3DTextLabel(
            text, 0xFFFFFFFF, 
            atm_info[i][E_ATM_POS_X], atm_info[i][E_ATM_POS_Y], atm_info[i][E_ATM_POS_Z] + 1.5, 
            3.0, .worldid = atm_info[i][E_ATM_WORLD], .interiorid = atm_info[i][E_ATM_INT]
        );
    }

    return printf("[ATM] Банкоматов загружено %d шт.", rows);
}

forward LogATMOperation(playerid, type, amount);
public LogATMOperation(playerid, type, amount) 
{
    new query[256];
    new type_operation[20];

    switch(type)
    {
        case ATM_OPERATION_WITHDRAW: 
        {
            format(type_operation, sizeof(type_operation), "Снятие");
        }
        case ATM_OPERATION_DEPOSIT: 
        {
            format(type_operation, sizeof(type_operation), "Пополнение");
        }
    }

    mysql_format(mysql, query, sizeof(query), "INSERT INTO `atm_operation` (`account_id`, `type`, `amount`, `time`) VALUES ('%d', '%e', '%d', '%d')", 
        PlayerInfo[playerid][pAccountID], type_operation, amount, gettime());

    mysql_tquery(mysql, query);

    return 1;
}

forward ShowATMDialog(playerid);
public ShowATMDialog(playerid)
{
    return ShowPlayerDialog(playerid, DIALOG_ATM_MENU, DIALOG_STYLE_LIST, "Банкомат", "1. Снять деньги\n2. Положить деньги\n3. Мой баланс", "Далее", "Закрыть");
}

forward ATM_OnDialogResponse(playerid, dialogid, response, listitem, inputtext[]);
public ATM_OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    switch(dialogid) 
    {
        case DIALOG_ATM_MENU:
        {
            if(!response) 
                return 1;

            switch(listitem)
            {
                case ATM_OPERATION_WITHDRAW:
                {
                    ShowPlayerDialog(playerid, DIALOG_ATM_WITHDRAW, DIALOG_STYLE_INPUT, "Снятие денег", "Укажите сумму, которую хотите снять:", "Далее", "Назад");
                }
                case ATM_OPERATION_DEPOSIT:
                {
                    ShowPlayerDialog(playerid, DIALOG_ATM_DEPOSIT, DIALOG_STYLE_INPUT, "Пополнение счёта", "Укажите сумму, на которую хотите пополнить:", "Далее", "Назад");
                }
                case ATM_OPERATION_CHECK_BALANCE:
                {
                    new query[100];

                    format(query, sizeof(query), "На вашем банковском счету находится %d рублей", PlayerInfo[playerid][pBank]);

                    ShowPlayerDialog(playerid, DIALOG_ATM_BALANCE, DIALOG_STYLE_MSGBOX, "Проверка баланса", query, "Назад", "");
                }
            }
        }
        case DIALOG_ATM_WITHDRAW:
        {
            if(!response) 
                return ShowATMDialog(playerid);

            new amount = strval(inputtext);

            if(amount < 1 || amount > MAX_ATM_TRANSACTION_LIMIT)
                return SendClientMessage(playerid, 0xFFFFFFFF, "Сумма должна быть не менее 1 рубля или не больше %d рублей", MAX_ATM_TRANSACTION_LIMIT);

            if(amount > PlayerInfo[playerid][pBank])
                return SendClientMessage(playerid, 0xFFFFFFFF, "На вашем банковском счету нет такой суммы.");

            GivePlayerMoney(playerid, amount);
            PlayerInfo[playerid][pBank] -= amount;
            LogATMOperation(playerid, ATM_OPERATION_WITHDRAW, amount);

            SendClientMessage(playerid, 0xFFFFFFFF, "Вы успешно сняли %d рублей со своего банковского счета.", amount);
        }
        case DIALOG_ATM_DEPOSIT:
        {
            if(!response) 
                return ShowATMDialog(playerid);

            new amount = strval(inputtext);

            if(amount < 1 || amount > MAX_ATM_TRANSACTION_LIMIT)
                return SendClientMessage(playerid, 0xFFFFFFFF, "Сумма должна быть не менее 1 рубля или не больше %d рублей", MAX_ATM_TRANSACTION_LIMIT);

            if(amount > GetPlayerMoney(playerid))
                return SendClientMessage(playerid, 0xFFFFFFFF, "У вас недостаточно средств.");

            GivePlayerMoney(playerid, -amount);
            PlayerInfo[playerid][pBank] += amount;
            LogATMOperation(playerid, ATM_OPERATION_DEPOSIT, amount);

            SendClientMessage(playerid, 0xFFFFFFFF, "Вы успешно пополнили %d рублей на свой банковский счет.", amount);
        }
        case DIALOG_ATM_BALANCE:
        {
            ShowATMDialog(playerid);
        }
    }

    return 1;
}

forward ATM_OnPlayerKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys);
public ATM_OnPlayerKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    if((newkeys & KEY_WALK) && !(oldkeys & KEY_WALK) && GetPlayerState(playerid) == PLAYER_STATE_ONFOOT)
    {
        new atm_id = areaid_player[playerid];

        if(atm_id != -1)
        {
            if(GetPlayerVirtualWorld(playerid) != atm_info[atm_id][E_ATM_WORLD] || GetPlayerInterior(playerid) != atm_info[atm_id][E_ATM_INT])
                return 1;
            
            if(IsPlayerInRangeOfPoint(playerid, 2.0, atm_info[atm_id][E_ATM_POS_X], atm_info[atm_id][E_ATM_POS_Y], atm_info[atm_id][E_ATM_POS_Z]))
            {
                ShowATMDialog(playerid);
                return 1;
            }
        }
    }
    return 1;
}

forward ATM_OnPlayerEnterDynamicArea(playerid, areaid);
public ATM_OnPlayerEnterDynamicArea(playerid, areaid)
{
    if(GetAreaATM(areaid) != 0)
    {
        areaid_player[playerid] = GetAreaATM(areaid) - 1;
        return 1;
    }

    return 1;
}

forward ATM_OnPlayerLeaveDynamicArea(playerid, areaid);
public ATM_OnPlayerLeaveDynamicArea(playerid, areaid)
{
    if(GetAreaATM(areaid) != 0)
    {
        areaid_player[playerid] = -1;
        return 1;
    }

    return 1;
}

forward ATM_OnPlayerConnect(playerid);
public ATM_OnPlayerConnect(playerid)
{
    areaid_player[playerid] = -1;
}

forward ATM_OnPlayerDisconnect(playerid);
public ATM_OnPlayerDisconnect(playerid)
{
    areaid_player[playerid] = -1;
}
