//
// Created by ZY on 24/9/2019.
//

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <iterator>
#include <fstream>

using namespace std;

/**
 * Self explanatory
 */
void handleError(const std::string& errorMessage) {
    perror(errorMessage.c_str());
    exit(1);
}

/**
 * Handler for when a message is received from client
 */
void onMessageReceived(int clientSocket, const char* msg) {
    // Parse client request
    istringstream iss(msg);
    vector<string> parsed((istream_iterator<string>(iss)), istream_iterator<string>());

    // Defaults
    string content = "<h1>404 Not Found</h1>";
    string htmlFile = "/index.html";
    int errorCode = 404;

    // Some simple error check for valid GET request
    if (parsed.size() >= 3 && parsed[0] == "GET") {
        htmlFile = parsed[1];
        if (htmlFile == "/") {
            htmlFile = "/index.html";
        }
    } else {
        cout << "Invalid GET: parsed[0] = " + parsed[0] << endl;
        cout << "Invalid GET: parsed[1] = " + parsed[1] << endl;
    }

    // Open document attempt and copy
    ifstream f("./wwwroot" + htmlFile);
    if (f.good()) {
        string str((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        content = str;
        errorCode = 200;
    } else {
        cout << "File NOT GOOD: fileUrl = " + string("./wwwroot") + htmlFile << endl;
    }
    f.close();

    // Write document back to client
    ostringstream oss;
    oss << "HTTP/1.1 " << errorCode << " OK\r\n";
    oss << "Cache-Control: no-cache, private\r\n";
    oss << "Content-Type: text/html\r\n";
    oss << "Content-Length: " << content.size() << "\r\n";
    oss << "\r\n";
    oss << content;

    string output = oss.str();
    int size = output.size() + 1;
    cout << "SENDING DATA = " + output << endl;
    send(clientSocket, output.c_str(), size, 0);
}

int main() {
    int one = 1, bytes_received, client_fd;
    char recv_data[1024];
    struct sockaddr_in svr_addr, cli_addr;
    int processID; // for concurrent server
    socklen_t sin_len = sizeof(cli_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) handleError("Socket");

    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

    int port = 80;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
        close(sock);
        handleError("Unable to bind");
    }

    //Listening for connections from client.
    //Maximum 5 connection requests are accepted.
    if (listen(sock, 5) == -1) handleError("Listen");

    cout << "Server started on port " + port << endl;

    while (true) {
        // Accept connection request from client and create new socket for client
        client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);

        processID = fork();
        if (processID == 0) {
            cout << "Got a connection from (" + string(inet_ntoa(cli_addr.sin_addr)) + ", " + to_string(ntohs(cli_addr.sin_port)) + ")" << endl;
            close(sock); // close server socket or listening socket

            bytes_received = recv(client_fd, recv_data, 1024, 0);
            recv_data[bytes_received] = '\0';
            cout << "RECEIVED DATA = " + string(recv_data) << endl;
            onMessageReceived(client_fd, recv_data);

            close(client_fd);  // close connect socket used for the client
            cout << "Connection is closed with (" + string(inet_ntoa(cli_addr.sin_addr)) + ", " + to_string(ntohs(cli_addr.sin_port)) + ")" << endl;

            exit(0);
        }

        close(client_fd);   // Parent process. close connect socket (used for the clinet) in parent process
    }
    return 0;
}
