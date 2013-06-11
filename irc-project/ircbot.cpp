#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <stdio.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <sqlite3.h>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std;

void    init_db(const char *file);
void    write_sql();
string  read_sql();

void irc_connect();
void irc_disconnect();
void s2u(const char *msg);
void ping_parse(const string &buffer);
int irc_parse(string buffer);
void irc_identify();
int bot_functions(string &buffer);
void init_bot_witze();
string get_random_witz();

const unsigned int BUF_SIZE = 1024;
const int PORT = 6667;
const char *HOST = "irc.freenode.net";

#ifdef WIN32
SOCKET sockfd;
#else
int sockfd;
#endif

const string BOTNAME = "Botti";
const string PASSWD = "B1234";
string channel = "Nikolaus";
vector<string> bot_witze;
bool sleeping = false;
int sleep_msg_counter = 0;

int main() {
    init_bot_witze();
    irc_connect();
    irc_identify();
    for(;;) {
        char buffer[BUF_SIZE+1] = {0};
        if (recv(sockfd, buffer, BUF_SIZE*sizeof(char), 0) < 0) {
            perror("recv()");
            irc_disconnect();
            exit(1);
        }
        cout << buffer;
        if (irc_parse(buffer) == -1)
            break;
    }
    irc_disconnect();
    return 0;
}

void irc_connect() {
#ifdef WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,0), &wsa) != 0)
        exit(1);
#endif
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (static_cast<int>(sockfd) < 0) {
        perror("socket");
        irc_disconnect();
        exit(1);
    }
    hostent *hp = gethostbyname(HOST);
    if(!hp) {
        cerr << "gethostbyname()" << endl;
        irc_disconnect();
        exit(1);
    }

    sockaddr_in sin;
    memset((char*)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    memcpy((char*)&sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_port = htons(PORT);
    memset(&(sin.sin_zero), 0, 8*sizeof(char));

    if (connect(sockfd, (sockaddr*) &sin, sizeof(sin)) == -1) {
        perror("connect");
        irc_disconnect();
        exit(1);
    }
}

void irc_disconnect() {
#ifdef WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif
}

void s2u(const char *msg) {
    send(sockfd, msg, strlen(msg), 0);
}
void ping_parse(const string &buffer) {
    size_t pingPos = buffer.find("PING");
    if (pingPos != string::npos) {
        string pong("PONG"+buffer.substr(pingPos+4)+"\r\n");
        cout << pong;
        s2u(pong.c_str());
    }
}
int irc_parse(string buffer) {
    if (buffer.find("\r\n") == buffer.length()-2)
        buffer.erase(buffer.length()-2);
    ping_parse(buffer);
    return bot_functions(buffer);
}
void irc_identify() {
    s2u(("NICK " + BOTNAME + "\r\n").c_str());
    s2u(("USER " + BOTNAME + " 0 0  :" + BOTNAME + "\r\n").c_str());
    s2u(("PRIVMSG NickServ IDENTIFY " + PASSWD + "\r\n").c_str());
    s2u(("JOIN #" + channel + "\r\n").c_str());
    s2u(("PRIVMSG #" + channel + ":Hallo!\r\n").c_str());
}
int bot_functions(string &buffer) {

    string temp;
    transform(buffer.begin(), buffer.end(), temp.begin(), ::tolower);
    if (temp.find("hello") != temp.npos || temp.find("hallo") != temp.npos) {
        s2u(("PRIVMSG #" + channel + " :Hallo!\r\n").c_str());
    }

    size_t pos = 0;
    if ((pos = buffer.find(":"+BOTNAME+":")) != buffer.npos) {
        if (sleeping) {
            switch(sleep_msg_counter) {
            case 0:
                s2u(("PRIVMSG #" + channel + " :zZzZzZzZzZzZzZzZzZzZzZzZ...\r\n").c_str());
                break;
            case 1:
                s2u(("PRIVMSG #" + channel + " :zZzZzZz nicht das Einhorn! ZzZzZzZzZzZzZzZzZ...\r\n").c_str());
                break;
            case 2:
                s2u(("PRIVMSG #" + channel + " :*gääähn*\r\n").c_str());
                break;
            case 3:
                s2u(("PRIVMSG #" + channel + " :Na toll, jetzt bin ich wach! Zufrieden?!\r\n").c_str());
                break;
            };
            sleep_msg_counter++;
            if (sleep_msg_counter == 4) {
                sleeping = false;
                sleep_msg_counter = 0;
            }
            return 0;
        }

        transform(buffer.begin(), buffer.end(), buffer.begin(), ::tolower);

        if ((pos = buffer.find(":say ")) != string::npos) {
            s2u(("PRIVMSG #" + channel + " :" + buffer.substr(pos+5)+"\r\n").c_str());
        }
        if (buffer.find(":User!User@User.user.freenode.net") == 0 && buffer.find("exit")!=string::npos) {
            s2u(("PRIVMSG #" + channel + " :Cya\r\n").c_str());
            irc_disconnect();
            exit(0);
        }

        string cmd = buffer.substr(pos+BOTNAME.length()+2);
        if (cmd.find("witz") != cmd.npos) {
            s2u(("PRIVMSG #" + channel + " :" + get_random_witz() +"\r\n").c_str());
        } if (cmd.find("time") != cmd.npos) {
            time_t rawtime;
            struct tm * timeinfo; time(&rawtime); timeinfo = localtime(&rawtime);
            s2u(("PRIVMSG #" + channel + " :" + asctime(timeinfo) + "\r\n").c_str());
        } if (cmd.find("penner") != cmd.npos || cmd.find("arschloch") != cmd.npos || cmd.find("wichser") != cmd.npos) {
            s2u(("PRIVMSG #" + channel + " :" + BOTNAME + " ist beleidigt und verlässt den Chat..." + "\r\n").c_str());
            return -1;
        } if (cmd.find("gute nacht") != cmd.npos) {
            s2u(("PRIVMSG #" + channel + " :Gute Nacht!\r\n").c_str());
            sleeping = true;
        } if (cmd.find("selbstzerstörung") != cmd.npos || cmd.find("stirb") != cmd.npos) {
            s2u(("PRIVMSG #" + channel + " :Selbstzerstörung in...\r\n").c_str());
            int start_timer;
            int clock_counter = 0;
            int kill_counter = 3;
            while (true) {
                start_timer = clock();
                clock_counter += (clock() - start_timer);
                if (clock_counter >= (CLOCKS_PER_SEC)) {
                    kill_counter--;
                    if (kill_counter == -1) {
                        s2u(("PRIVMSG #" + channel + " :BOOOOOOOOOOOMMM!!!!!\r\n").c_str());
                        return -1;
                    }
                    stringstream sstr; sstr << (kill_counter+1);
                    s2u(("PRIVMSG #" + channel + " :..." + sstr.str() + "\r\n").c_str());
                    clock_counter = 0;
                }
            }
        }
    } return 0;
}

void init_bot_witze() {
    bot_witze.push_back("Chuck Norris weiß, wieso da überall Stroh liegt!");
    bot_witze.push_back("Chuck Norris kann Drehtüren zuschlagen!");
    bot_witze.push_back("Chuck Norris hat bis unendlich gezählt. Zweimal.");
    bot_witze.push_back("Chuck Norris isst keinen Honig, er kaut Bienen.");
    bot_witze.push_back("Chuck Norris kann Zwiebeln zum Weinen bringen.");
}
string get_random_witz() {
    int index = (int)(rand() % bot_witze.size());
    return bot_witze[index];
}
