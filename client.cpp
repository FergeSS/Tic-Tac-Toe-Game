#include "client.h"

//Запись в логи
void logWriter(std::string buffer){
    time_t now = time(nullptr);
    std::tm *localTime = std::localtime(&now);
    log << "[" << localTime->tm_mday << "." <<
                  localTime->tm_mon + 1 << "." <<
                  localTime->tm_year + 1900 << " ";
    if (localTime->tm_hour < 10){
        log << 0 << localTime->tm_hour;
    }
    else {
        log << localTime->tm_hour;
    }
    log << ":";
    if (localTime->tm_min < 10){
        log << 0 << localTime->tm_min;
    }
    else {
        log << localTime->tm_min;
    }
    log << ":"; 
    if (localTime->tm_sec < 10){
        log << 0 << localTime->tm_sec;
    }
    else {
        log << localTime->tm_sec;
    }
    log << "] " << buffer << std::endl;   
}

//Установка времени хода
void getTimeForMove(){
    std::ifstream config("config.txt");
    if (!config){
        const char* error = "No config file!";
        std::cout << error << std::endl;
        logWriter(error);
        log.close();
        exit(0);
    }
    std::string string;
    while (config.peek() != EOF){
        std::getline(config, string);
        if (string.find("time_for_move: ") != std::string::npos){
            std::string num;
            for (int i = 15; i < string.length(); ++i){
                num += string[i];
            }
            try {
                time_for_move = std::stoi(num);
                logWriter("Installed time for move: " + num);
            } catch(std::invalid_argument const& ex){
                logWriter(ex.what());
                std::cout << ex.what() << std::endl;
                config.close();
                log.close();
                exit(0);
            }
            break;
        }        
    }
    config.close();
}

//Установка порта
void getPort(){
    std::ifstream config("config.txt");
    if (!config){
        const char* error = "No config file!";
        logWriter(error);
        log.close();
        std::cout << error << std::endl;
        exit(0);
    }
    std::string string;
    while (config.peek() != EOF){
        std::getline(config, string);
        if (string.find("port: ") != std::string::npos){
            std::string num;
            for (int i = 6; i < string.length(); ++i){
                num += string[i];
            }
            try {
                port = std::stoi(num);
                logWriter("Installed port: " + num);
            } catch(std::invalid_argument const& ex){
                logWriter(ex.what());
                std::cout << ex.what() << std::endl;
                config.close();
                log.close();
                exit(0);
            }
            break;
        }        
    }
    config.close();
}

//Авторизация клиента
bool authorize(int socket){
    char buffer[1024];

    std::cout << "\033[1m" << "Nickname: \033[0m";
    std::cin >> buffer;
    std::string nickname = buffer;

    send(socket, buffer, strlen(buffer), 0);
    std::cout << "\033[1m" << "Password: \033[0m";
    std::cin >> buffer;
    send(socket, buffer, strlen(buffer), 0);

    std::system("clear");

    memset(buffer, 0, sizeof(buffer));
    int valread = read(socket, buffer, 1024);
    if (valread <= 0) {
        logWriter("Invalid message from server on authorize client");
        log.close();
        exit(0);
    }

    if (strcmp(buffer, "Success connect!\n") == 0){
        logWriter(nickname + " successfully connect");
        std::cout << "\033[1;32m" << buffer << "\033[0m";
        return true;
    }
    
    logWriter("Unknown user try to connect to the server!");
    std::cout << "\033[1;31m" << "Unknown user!\n" << "\033[0m";
    return false;    
}

//Заполнение игрового поля по буфферу, полученному с сервера
void fillGameMap(const char* buffer){
    for (int i = 0, j = 0; i < 3; ++i){
        for (int k = 0; k < 3; ++k, ++j){
            game_map[i][k] = buffer[j];
        }
    }
}

//Вывод игрового поля
void printGameMap(){
    std::cout << "\033[1m";
    for (int i = 0; i < 3; ++i){
        for (int k = 0; k < 3; ++k){
            if ( k == 0){ 
                std::cout << " "; 
            }
            if (game_map[i][k] == 'X'){
                if (k != 2){ 
                    std::cout << "\033[0;31m"<<  game_map[i][k] << "\033[0m" << " ┃ ";
                }
                else { 
                    std::cout << "\033[0;31m"<<  game_map[i][k] << "\033[0m" << std::endl;
                }
            }
            else {
                if (k != 2){ 
                    std::cout << "\033[0;34m"<<  game_map[i][k] << "\033[0m" << " ┃ ";
                }
                else { 
                    std::cout << "\033[0;34m"<<  game_map[i][k] << "\033[0m" << std::endl;
                }
            }
                        
        }
        if (i != 2) { std::cout << "―――――――――――\n";}
    }
    std::cout << "\033[0m";
}

//Обработка сообщений с сервера
void receiveMessages(int socket) {
    int valread;
    char buffer[1024];
    while (true) {
        valread = read(socket, buffer, 1024);
        if (valread > 0) {
            std::system("clear");
            //Игра закончилась, выводим победителя и выходим из цикла
            if (strcmp(buffer, "O winner!") == 0 || 
                strcmp(buffer, "X winner!") == 0 || 
                strcmp(buffer, "Draw!") == 0) {
                    logWriter(buffer);
                    std::cout << "\033[1m" << buffer << "\033[0m" << std::endl;
                    break;                
            }
            //Пользователь ввел некорректное значение
            else if (strcmp(buffer, "Wrong input!\n") == 0) {
                std::cout << "\033[1;31m" << buffer << "\033[0m" << std::endl;
                logWriter("Wrong input");
            }
            //Игра продолжается
            else {
                logWriter("Next move!");
                fillGameMap(buffer);
                gamer = !gamer;
            }            
            printGameMap();        
        }
        else {
            logWriter("Invalid message from server on recieveMessages!");
            break;
        }
        memset(buffer, 0, sizeof(buffer));
    }
    close(socket);
    log.close();
    exit(0); //Игра закончилась, выходим из программы
}

//Отправка значения хода, введенное пользователем
void sendMove(int socket){   
    while (true) {
        //Если ход другого игрока, то пропускаем ввод
        if (move != gamer){
            continue; 
        }
        std::string message;
        // Создание асинхронной задачи для чтения ввода
        auto readInput = std::async(std::launch::async, []() {
            std::string input;
            std::cin >> input;
            return input;
        });

        // Установка времени ожидания ввода, которое указано в конфиге
        std::future_status status = readInput.wait_for(std::chrono::seconds(time_for_move));

        // Проверка состояния ожидания
        if (status == std::future_status::ready) {
            // Ввод был завершен до истечения времени
            message = readInput.get();
        } else if (status == std::future_status::timeout) {
            // Ввод не был завершен до истечения времени
            std::cout << "Input timed out." << std::endl;
            logWriter("Input timed out!");
            send(socket, "-1", 3, 0);            
        }
        int value;
        try {
            value = std::stoi(message);
        } catch(std::invalid_argument const& ex){
            //Пользователь ввел не число
            std::cout << "\033[0;31mIncorrect input: " << ex.what() << "\n\033[0m";
            logWriter(std::string("Incorrect input: ") + ex.what());
            continue;
        }
        if (value < 1 || value > 9){
            //Пользователь ввел некорректное число
            std::cout << "\033[0;31mIncorrect input: input number in range (1 <= value <= 9)\n\033[0m";
            logWriter("Incorrect input: input number in range (1 <= value <= 9)");
            continue;
        }    
        //Если все хорошо, то отправляем значение, введенное пользователем, на сервер
        send(socket, message.c_str(), message.length(), 0);
    }
}

//Выводим сообщение-приветствие
void printHelloMessage() {
    std::cout << "\033[1m" << "\nHey! Nice to meet you in my Tic Tac Toe Game!!!\n" << "\033[0m"
                 "――――――――――――――――――――――――――――――――――――――――――――――――――――――\n" 
                 "Our rules:\n" 
                 "1. The cross always goes first. The user's figure is determined randomly.\n"
                 "2. You have " << "\033[1;34m" << time_for_move << "\033[0m" << " seconds to make your move. Didn't have time? So You lost.\n"
                 "3. To put your figure, enter a number from 1 to 9, where the fields of the game are numbered like this:\n"
                 "1 | 2 | 3\n"
                 "―――――――――\n"
                 "4 | 5 | 6\n"
                 "―――――――――\n"
                 "7 | 8 | 9\n\n"
                 "--& Have a nice game! &-- \n"
                 "To continue, you need to" << "\033[1;32m" << " log in.\n" << "\033[0m"
                 "Write 'go' if you are ready for battle!\n"
                 "Write 'exit' if you are not ready for battle!\n";   
}

//Пользователь решает хочет ли он сыграть в Крестики-Нолики
bool startGame() {
    std::string inp;
    
    while(true){
        std::cin >> inp;
        if (inp == "exit") {
            logWriter("The client is out of the game");
            return 0;
        }
        if (inp == "go"){
            logWriter("The client has logged into the game");
            return 1;;
        }
        std::cout << "Incorrect input! Write 'exit' for exit or 'go' for continue\n";
    }    
}

//Ожидание второго игрока
void waitOtherPlayer(int socket) {
    char buffer[1024];
    int valread = read(socket, buffer, sizeof(buffer));
    if (valread > 0){
        if (strcmp(buffer, "Wait other player!\n") == 0){
            std::cout << buffer << std::endl;
            logWriter("Client wait other player!");
            memset(buffer, 0, 1024);
            valread = read(socket, buffer, sizeof(buffer));
            if (valread > 0) {
                std::system("clear");  
                logWriter(buffer);  
                std::cout << buffer << std::endl;
            }
        }
        else {
            std::system("clear");    
            logWriter(buffer);  
            std::cout << buffer << std::endl;
        }            
    }
}

//Определение очередности хода
void getMove(int socket) {
    char buffer[1024];
    int valread = read(socket, buffer, sizeof(buffer));
    if (strcmp(buffer, "1") == 0){
        move = true;
    }
    else {
        move = false;
    }
}

//Подключение к серверу
int connect2Server(){
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "\n Socket creation error \n";
        logWriter("Socket creation error");
        log.close();
        return -1;
    }

    getPort();
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "\nInvalid address/ Address not supported \n";
        logWriter("Invalid address/ Address not supported");
        log.close();
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed \n";
        logWriter("Connection Failed");
        log.close();
        return -1;
    }
    return sock;
}

int main() {
    int sock = connect2Server();
    if (sock == -1){
        return -1;
    }

    logWriter("Succecss connect to server");

    getTimeForMove();
    printHelloMessage();

    if (!startGame()) {
        close(sock);
        exit(0);
    }    

    std::system("clear");

    if (authorize(sock) == 0){
        close(sock);
        exit(0);
    }

    waitOtherPlayer(sock);
    getMove(sock);
    printGameMap();
    std::thread receiveThread(receiveMessages, sock);
    receiveThread.detach();
    sendMove(sock);    
    return 0;
}
