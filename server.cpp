#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <unordered_map>
#include <mutex>

// Global map of client names to sockets. Used to track connected clients.
std::unordered_map<std::string, int> clients;
std::mutex clients_mutex;

/**
 * Sends a message to a specific client by their name.
 * Ensures thread-safe access to the global client map using a mutex.
 *
 * @param recipient The name of the recipient client.
 * @param message The message to send to the recipient.
 */
void send_message_to_client(const std::string &recipient, const std::string &message) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    if (clients.find(recipient) != clients.end()) {
        int recipient_socket = clients[recipient];
        send(recipient_socket, message.c_str(), message.size(), 0);
    }
}

/**
 * Handles communication with a single client.
 * Receives messages from the client and processes them. If the client sends a "send" command,
 * the server forwards the message to the intended recipient.
 *
 * @param client_socket The socket descriptor for the connected client.
 */
void handle_client(int client_socket) {
    char buffer[1024];
    std::string name;

    // Receive the client's name
    memset(buffer, 0, sizeof(buffer));
    if (recv(client_socket, buffer, sizeof(buffer), 0) > 0) {
        name = buffer;
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients[name] = client_socket; // Add client to the map
        }
    } else {
        close(client_socket);
        return;
    }

    std::cout << "Client connected: " << name << std::endl;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0 || std::string(buffer).find("exit") == 0) {
            std::cout << "Client disconnected: " << name << std::endl;
            close(client_socket);
            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                clients.erase(name);
            }
            break;
        }

        // Parse the message
        std::string message = buffer;
        if (message.find("send") == 0) {
            size_t first_space = message.find(' ');
            size_t second_space = message.find(' ', first_space + 1);

            if (first_space != std::string::npos && second_space != std::string::npos) {
                std::string recipient = message.substr(first_space + 1, second_space - first_space - 1);
                std::string content = message.substr(second_space + 1);

                std::cout << "Message from " << name << " to " << recipient << ": " << content << std::endl;

                send_message_to_client(recipient, "Incoming message: " + content);
            }
        }
    }
}

/**
 * Entry point for the server application.
 * Initializes the server, binds to the specified IP and port, and listens for incoming client connections.
 * For each client, a new thread is created to handle their communication.
 *
 * @param argc The number of command-line arguments.
 * @param argv The command-line arguments (not used).
 * @return int Exit status of the application.
 */
int main(int argc, char *argv[]) {
    const char *ip = "127.0.0.1";
    int port = 4444;

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Bind failed." << std::endl;
        return 1;
    }

    if (listen(server_socket, 10) == -1) {
        std::cerr << "Listen failed." << std::endl;
        return 1;
    }

    std::cout << "Server listening on " << ip << ":" << port << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr *)&client_addr, &client_size);

        if (client_socket == -1) {
            std::cerr << "Failed to accept connection." << std::endl;
            continue;
        }

        // Handle the client in a new thread
        std::thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}
