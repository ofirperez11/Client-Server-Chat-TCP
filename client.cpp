//Authors: Ofir Perez, Aviv Kenan 
//IDs: 207781469, 207782863  

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <condition_variable>

// Mutex to ensure thread-safe access to console output.
std::mutex cout_mutex;

/**
 * This function runs in a separate thread to handle incoming messages from the server.
 * It continuously listens for messages from the server and prints them to the console.
 * If the connection is closed or an error occurs, the function terminates.
 *
 * @param client_socket The socket descriptor for the client connection.
 */
void receive_messages(int client_socket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            // Ensures synchronized output to the console when multiple threads are running.
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\n" << buffer << std::endl;
            std::cout << "Enter message: ";
            std::cout.flush(); // Ensure the prompt is visible immediately
        } else {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <IP> <NAME>" << std::endl;
        return 1;
    }

    std::string name = argv[2];
    const char *ip = argv[1];
    int port = 4444;

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(client_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to connect to server." << std::endl;
        return 1;
    }

    // Send the client's name to the server
    send(client_socket, name.c_str(), name.size(), 0);

    std::cout << "Connected to the server as: " << name << std::endl;

    // Start a thread to receive messages
    // This thread is started to handle incoming messages concurrently with user input.
    std::thread receive_thread(receive_messages, client_socket);
    receive_thread.detach();

    char buffer[1024];
    while (true) {
        std::string message;
        {
            // Ensures synchronized output to the console when multiple threads are running.
            // This ensures the user prompt remains visible and doesn't conflict with incoming message output.
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Enter message: ";
        }
        std::getline(std::cin, message);

        if (message == "exit") {
            break;
        }

        // Send message to the server
        send(client_socket, message.c_str(), message.size(), 0);
    }

    close(client_socket);
    return 0;
}

