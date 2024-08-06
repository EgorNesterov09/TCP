// server.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

std::mutex log_mutex;

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(&now_tm, "[%Y-%m-%d %H:%M:%S]") << '.' << std::setw(3) << std::setfill('0') << now_ms.count();
    return ss.str();
}

class Server {
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    void start_accept() {
        auto socket = std::make_shared<tcp::socket>(acceptor_.get_executor());
        acceptor_.async_accept(*socket,
                               [this, socket](boost::system::error_code ec) {
                                   if (!ec) {
                                       std::thread(&Server::handle_client, this, socket).detach();
                                   }
                                   start_accept();
                               });
    }

    void handle_client(std::shared_ptr<tcp::socket> socket) {
        try {
            char data[1024];
            boost::system::error_code error;
            size_t length = socket->read_some(boost::asio::buffer(data), error);
            if (error == boost::asio::error::eof) return; // Connection closed cleanly by peer.
            if (error) throw boost::system::system_error(error); // Some other error.
            data[length] = '\0';

            std::lock_guard<std::mutex> guard(log_mutex);
            std::ofstream log_file("log.txt", std::ios::app);
            if (log_file.is_open()) {
                log_file << getCurrentTime() << " " << data << std::endl;
            }
        } catch (std::exception& e) {
            std::cerr << "Exception in thread: " << e.what() << std::endl;
        }
    }

    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    short port = std::atoi(argv[1]);

    try {
        boost::asio::io_context io_context;
        Server server(io_context, port);
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
