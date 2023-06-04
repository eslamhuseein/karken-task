#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <iostream>
#include <queue>
#include <atomic>
#include <thread>
#include <fstream>
#include <functional>
#include <vector>
#include <ctime>
#include <mutex>
#include <algorithm>
#include <condition_variable>

namespace Dafa {

namespace net = boost::asio;
namespace ssl = net::ssl;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

using tcp = net::ip::tcp;
using Request = http::request<http::string_body>;
using Stream = beast::ssl_stream<beast::tcp_stream>;
using Response = http::response<http::dynamic_body>;

class Observer {
public:
    virtual ~Observer() {}
    virtual void update(const std::string& data) = 0;
};

class Observable {
public:
    void addObserver(Observer* observer);
    void removeObserver(Observer* observer);
    void notifyObservers(const std::string& data);

private:
    std::vector<Observer*> observers;
    std::mutex mutex_;
};

class Connection : public Observable {
public:
    Connection(std::string name, const std::string& http_host);

    void init_http(const std::string& host);
    void init_webSocket(const std::string& host, const std::string& port, const std::string& p = "");

    void read_Socket();
    bool is_socket_open();
    void write_Socket(const std::string& text);
    std::string get_socket_data();
    void buffer_clear();
    void webSocket_close();

    void process_socket_data();
    void pause();
    void resume();

private:
    std::string m_name;
    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};
    tcp::resolver resolver{ioc};
    Stream stream{ioc, ctx};

    std::string m_web_socket_host;
    std::string m_web_socket_port;
    beast::flat_buffer buffer;
    net::io_context ioc_webSocket;
    ssl::context ctx_webSocket{ssl::context::tlsv12_client};
    tcp::resolver resolver_webSocket{ioc_webSocket};
    websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc_webSocket, ctx_webSocket};

    std::mutex mutex_;
    std::condition_variable conditionVariable;
    std::atomic<bool> isPaused;
};

class FileObserver : public Observer {
public:
    FileObserver();

    void update(const std::string& data) override;

private:
    std::string m_filename;
    std::mutex mutex_;

    void store_data_in_file(const std::string& data);
    std::streampos get_file_size(const std::string& filename);
};


static void runKraken()
{
    try {
        Dafa::FileObserver fileObserver;
        Dafa::Connection kraken("kraken", "ws.kraken.com");
        kraken.addObserver(&fileObserver);

        kraken.init_webSocket("ws.kraken.com", "443", "/");
        if (kraken.is_socket_open())
            kraken.write_Socket(R"({"event": "subscribe","pair": ["XBT/USD"],"subscription": {"name": "ticker"}})");

        std::thread dataProcessingThread(&Dafa::Connection::process_socket_data, &kraken);

        bool isRunning = true;
        bool isPaused = false;

        while (isRunning) {
            std::string input;
            bool stopProcess = false;

            while (!stopProcess) {
                std::cout << (isPaused ? "Enter to resume the process: " : "Enter 'pause' to pause the process: ");
                std::getline(std::cin, input);
                if (input == "pause") {
                    if (!isPaused) {
                        kraken.pause();  // Pause the process'stop' to stop the process
                        isPaused = true;
                        std::cout << "Process paused." << std::endl;
                    } else {
                        std::cout << "Process is already paused." << std::endl;
                    }
                } else if (input == "resume") {
                    if (isPaused) {
                        kraken.resume();  // Resume the process
                        isPaused = false;
                        std::cout << "Process resumed." << std::endl;
                    } else {
                        std::cout << "Process is already running." << std::endl;
                    }
                }
                // Other conditions or logic to handle different inputs
            }

            if (kraken.is_socket_open()) {
                kraken.webSocket_close();  // Close the WebSocket connection
            }
            dataProcessingThread.join();  // Wait for the data processing thread to finish
        }
    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << std::endl;
    }
}

}  // namespace Dafa
