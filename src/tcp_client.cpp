// client.cpp

#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(&now_tm, "[%Y-%m-%d %H:%M:%S]") << '.' << std::setw(3) << std::setfill('0') << now_ms.count();
    return ss.str();
}

class Client {
public:
    Client(const std::string& name, const std::string& host, short port, int period)
        : name_(name), host_(host), port_(port), period_(period), io_context_(), socket_(io_context_) {}

    void run() {
        while (true) {
            try {
                tcp::resolver resolver(io_context_);
                auto endpoints = resolver.resolve(host_, std::to_string(port_));
                boost::asio::connect(socket_, endpoints);

                std::string message = getCurrentTime() + " " + name_;
                boost::asio::write(socket_, boost::asio::buffer(message));

                socket_.close();
            } catch (std::exception& e) {
                std::cerr << "Exception: " << e.what() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds(period_));
        }
    }

private:
    std::string name_;
    std::string host_;
    short port_;
    int period_;
    boost::asio::io_context io_context_;
    tcp::socket socket_;
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <client_name> <server_port> <period_seconds>" << std::endl;
        return 1;
    }

    std::string name = argv[1];
    short port = std::atoi(argv[2]);
    int period = std::atoi(argv[3]);

    Client client(name, "127.0.0.1", port, period);
    client.run();

    return 0;
}
