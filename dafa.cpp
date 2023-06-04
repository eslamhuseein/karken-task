#include "dafa.h"

namespace Dafa {

void Observable::addObserver(Observer* observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    observers.push_back(observer);
}

void Observable::removeObserver(Observer* observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
}

void Observable::notifyObservers(const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (Observer* observer : observers) {
        observer->update(data);
    }
}

Connection::Connection(std::string name, const std::string& http_host)
    : m_name(std::move(name)), isPaused(false) {
    init_http(http_host);
}

void Connection::init_http(const std::string& host) {
    const auto results = resolver.resolve(host, "443");
    get_lowest_layer(stream).connect(results);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        boost::system::error_code ec{
            static_cast<int>(::ERR_get_error()),
            boost::asio::error::get_ssl_category()
        };
        throw boost::system::system_error{ec};
    }
    stream.handshake(ssl::stream_base::client);
}

void Connection::init_webSocket(const std::string& host, const std::string& port, const std::string& p) {
    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()),
                              net::error::get_ssl_category()),
            "Failed to set SNI Hostname");

    const auto results = resolver_webSocket.resolve(host, port);
    net::connect(ws.next_layer().next_layer(), results.begin(), results.end());
    ws.next_layer().handshake(ssl::stream_base::client);

    ws.handshake(host, p);
}

void Connection::read_Socket() {
    ws.read(buffer);
}

bool Connection::is_socket_open() {
    return ws.is_open();
}

void Connection::write_Socket(const std::string& text) {
    ws.write(net::buffer(text));
}

std::string Connection::get_socket_data() {
    return beast::buffers_to_string(buffer.data());
}

void Connection::buffer_clear() {
    buffer.clear();
}

void Connection::webSocket_close() {
    if (is_socket_open()) {
        ws.close(websocket::close_code::none);
    }
}

void Connection::process_socket_data() {
    try {
        while (is_socket_open()) {
            beast::flat_buffer buffer;
            boost::system::error_code ec;
            ws.read(buffer, ec);
            if (ec) {
                if (ec == websocket::error::closed || ec == boost::asio::error::eof)
                    break;
                else
                    throw boost::system::system_error(ec);
            }

            std::string data = beast::buffers_to_string(buffer.data());
            notifyObservers(data);  // Notify observers of the new data
            buffer.clear();

            // Pause the process if isPaused flag is set
            std::unique_lock<std::mutex> lock(mutex_);
            if (isPaused) {
                conditionVariable.wait(lock, [this] { return !isPaused; });
            }
        }
    } catch (const std::exception& ex) {
        // Handle the exception
        std::cerr << "Exception occurred in process_socket_data(): " << ex.what() << std::endl;
    }
}

void Connection::pause() {
        isPaused.store(true, std::memory_order_relaxed);
}

void Connection::resume() {
    isPaused.store(false, std::memory_order_relaxed);
    conditionVariable.notify_all();
}

FileObserver::FileObserver() {
    // Get the current date and time
    time_t now = time(nullptr);
    struct tm* currentTime = localtime(&now);

    // Generate the file name using the current date and time
    char filename[100];
    strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S.txt", currentTime);
    m_filename = filename;
}

void FileObserver::update(const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    store_data_in_file(data);
}

void FileObserver::store_data_in_file(const std::string& data) {
    std::ofstream file(m_filename, std::ios::app);
    if (file.is_open()) {
        std::string line = data;

        if (line.find(R"({"event":"heartbeat"})") != std::string::npos) {
            return;
        }

        file << line << std::endl;
        file <<std::endl;

        file.close();

        if (get_file_size(m_filename) > 1024 * 1024) {

            time_t now = time(nullptr);
            struct tm* currentTime = localtime(&now);

            char newFilename[100];
            strftime(newFilename, sizeof(newFilename), "%Y-%m-%d_%H-%M-%S.txt", currentTime);
            m_filename = newFilename;
        }
    } else {
        std::cerr << "Error opening file..." << std::endl;
    }

}

std::streampos FileObserver::get_file_size(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        return file.tellg();
    } else {
        return 0;
    }
}
}  // namespace dafa.h
