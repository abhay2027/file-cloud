#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <filesystem>

using namespace std;
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

class recvfile
{
    char filename[80];
    int n;
    long filesize;

public:
    void filerecv(int clientsock, string foldername)
    {
        cout << "recieve\n";
        bzero(filename, 80);
        n = read(clientsock, filename, 80);
        if (n < 0)
        {
            cerr << "Error reading filename\n";
            return;
        }
        filename[n] = '\0';
        filename[strcspn(filename, "\r\n")] = '\0';
        cout << filename << endl;
        n = recv(clientsock, &filesize, sizeof(filesize), 0);
        if (n < 0)
        {
            cerr << "failed to recieve file size";
            exit(1);
        }
        if (strncmp((char *)&filesize, "ERROR", 5) == 0)
        {
            cerr << "file not found";
        }
        std::string filepath = "users/" + foldername + "/" + filename;
        cout << filepath << endl;
        cout << "received file:  " << filename << endl;
        ofstream outfile(filepath, ios::binary);
        if (!outfile)
        {
            cerr << "failed to open output file";
        }
        char buffer[1024];      //-----
        long totalrecieved = 0; //-------
        while (totalrecieved < filesize)
        {
            n = recv(clientsock, buffer, sizeof(buffer), 0);
            if (n < 0)
            {
                cerr << "file not recived";
                exit(1);
            }
            outfile.write(buffer, n);
            totalrecieved += n;
        }
        outfile.close();
        cout << "file recieved: " << totalrecieved << " bytes" << endl;
    }
};

class sendfile
{
protected:
    int n;
    char filename[200];
    string menu;

public:
    void filestored(int clientsock, string foldername)
    {
        string path = "/home/abhaythakur/project/cpp/server/users/" + foldername;
        for (const auto &entry : filesystem::directory_iterator(path))
        {
            menu += entry.path().filename().string() + "\n";
        }
        cout << menu;
        write(clientsock, menu.c_str(), menu.size());
        bzero(filename, 200);
        n = read(clientsock, filename, 200);
        if (n <= 0)
        {
            cerr << "Error reading file name\n";
            return;
        }
        filename[strcspn(filename, "\r\n")] = '\0';
        cout << "Uploaded file:" << filename << endl;
    }
    void sendfilefun(int clientsock, string foldername)
    {
        string path = "/home/abhaythakur/project/cpp/server/users/" + foldername + "/" + string(filename);
        ifstream file(path, ios::binary | ios::ate);
        cout << path << endl;
        if (!file)
        {
            cerr << "file not found";
            return;
        }
        long filesize = file.tellg();
        file.seekg(0, ios::beg);
        send(clientsock, &filesize, sizeof(filesize), 0);

        char buffer[1024];
        while (!file.eof())
        {
            file.read(buffer, sizeof(buffer));
            send(clientsock, buffer, file.gcount(), 0);
        }
        file.close();
        cout << "file sent: " << filesize << " bytes" << endl;
    }
};

class details
{
protected:
    string buffer;
    char username[20];
    int password;
    int n;
    int count;
    int byteschoice;
    string foldername;
    char storedusr[20];
    int storedpwd;

private:
public:
    void createaccountdetails(int clientsock)
    {
        buffer = "enter a unique username :";
        n = read(clientsock, username, 20);
        username[strcspn(username, "\n\r")] = '\0';

        cout << username;
        if (n <= 0)
        {
            cerr << "failed to read username";
            exit(1);
        }

        ifstream infile("password.txt");
        string storedusr;
        int storedpwd;
        bool exists = false;
        while (infile >> storedusr >> storedpwd)
        {
            if (storedusr == username)
            {
                exists = true;
                break;
            }
        }
        infile.close();

        if (exists)
        {
            buffer = "ERROR\n";
            send(clientsock, buffer.c_str(), buffer.size(),0);
            return;
        }

        buffer = "enter a password";
        n = read(clientsock, (char *)&byteschoice, sizeof(byteschoice));
        password = ntohl(byteschoice);
        cout << password << endl;

        ofstream outfile("password.txt", ios::app);
        outfile << username << " " << password << endl;
        cout << "pwd stored" << endl;
        outfile.close();
    }
    void enterdetails(int clientsock)
    {
        buffer = "enter a unique username :";
        n = read(clientsock, username, 20);
        username[strcspn(username, "\n\r")] = '\0';

        cout << username;
        if (n <= 0)
        {
            cerr << "failed to read username";
            exit(1);
        }

        buffer = "enter a password";
        n = read(clientsock, (char *)&byteschoice, sizeof(byteschoice));
        if (n <= 0)
        {
            cerr << "failed to read password";
            exit(1);
        }
        password = ntohl(byteschoice);
        cout << password << endl;
    }
    string returnuser()
    {
        return string(username);
    }
    void checkdetails(int clientsock)
    {
        ifstream infile("password.txt");

        bool match = false;
        string storedusr;
        int storedpwd;
        cout << "enterd " << username << " " << password << endl;

        while (infile >> storedusr >> storedpwd)
        {
            cout << "stored " << storedusr << " " << storedpwd << endl;
            if (storedusr == username && storedpwd == password)
            {
                match = true;
                break;
            }
        }
        if (match)
        {
            string msg = "Login successful\n";
            write(clientsock, msg.c_str(), msg.size());
        }
        else
        {
            string msg = "Invalid username or password\n";
            write(clientsock, msg.c_str(), msg.size());
        }
    }

    void setusername(string user)
    {
        foldername = user;
    }
    void createfolder()
    {
        mkdir("users", 0777);
        std::string path = "users/" + foldername;
        mkdir(path.c_str(), 0777);
        cout << "folder created";
    }

    void openfolder()
    {
        cout << "folder opened";
    }
};

class choice
{
protected:
    int byteschoice = 0;
    int selectedchoice = 0;
    int loginchoice;
    int n;

public:
    string buffer;

    void login(int clientsock)
    {
        buffer = "1. create account\n"
                 "2. login\n";
        n = read(clientsock, (char *)&byteschoice, sizeof(byteschoice));
        if (n <= 0)
        {
            cerr << "Failed to read input on login menu\n";
            close(clientsock);
        }
        loginchoice = ntohl(byteschoice);
    }

    void operation(int clientsock)
    {
        buffer = "select the operation you want to perform\n"
                 "1. download file \n"
                 "2. upload file\n"
                 "Enter a choice :";
        n = read(clientsock, (char *)&byteschoice, sizeof(byteschoice));
        if (n <= 0)
        {
            cerr << "Failed to read choice\n";
            close(clientsock);
        }
        selectedchoice = ntohl(byteschoice);
        cout << "choice sent " << selectedchoice << endl;
    }
};

class server : public choice
{
    int argc;
    char **argv;

public:
    server(int c, char *v[])
    {
        argc = c;
        argv = v;
    }
    void ser()
    {
        if (argc < 2)
        {
            cout << "port no. not provided, program terminated";
            exit(1);
        }
        int sockfd, newsockfd, portno;
        struct sockaddr_in serv_addr, cli_addr;
        socklen_t clilen;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            error("Error opening socket.");
        }

        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        bzero((char *)&serv_addr, sizeof(serv_addr));

        portno = atoi(argv[1]);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);

        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            error("Binding failed");

        listen(sockfd, 20);
        clilen = sizeof(cli_addr);
        cout << "Server started on port " << portno << endl;

        sendfile filesend;
        recvfile filerecive;
        while (true)
        {
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0)
            {
                perror("Error on Accept");
                continue;
            }
            cout << "Client connected" << endl;

            login(newsockfd);
            details logindetails;

            if (loginchoice == 1)
            {
                logindetails.createaccountdetails(newsockfd);
                logindetails.returnuser();
                string username = logindetails.returnuser();
                logindetails.setusername(username);
                logindetails.createfolder();
                close(newsockfd);
                continue;
            }
            else if (loginchoice == 2)
            {
                logindetails.enterdetails(newsockfd);
                logindetails.checkdetails(newsockfd);
            }
            operation(newsockfd);
            string username = logindetails.returnuser();
            string username_recive_send = logindetails.returnuser();

            if (selectedchoice == 1)
            {
                filesend.filestored(newsockfd, username_recive_send);
                filesend.sendfilefun(newsockfd, username_recive_send);
                cout << "Client disconnected" << endl;
                close(newsockfd);
            }
            else if (selectedchoice == 2)
            {
                filerecive.filerecv(newsockfd, username_recive_send);
                cout << "Client disconnected" << endl;
                close(newsockfd);
            }
        }

        close(sockfd);
    }
};

int main(int argc, char *argv[])
{
    server s(argc, argv);
    s.ser();
    return 0;
}