#include <boost/asio.hpp>
#include <iostream>

using namespace boost::asio;
using namespace boost::asio::ip;

int main() {
    io_service io;
    tcp::socket socket(io);
    std::cout << "wait for connections..." << std::endl;
    try {
        socket.connect(tcp::endpoint(address::from_string("127.0.0.1"), 2009));
    } catch(std::exception& exc) {
        std::cerr << exc.what() << std::endl;
        return -1;
    }
    std::cout << "++++" << std::endl;
    std::string msg;
    while(msg != "exit") {
        std::cout << "input message:";
        std::cin >> msg;
        boost::system::error_code error;
    
        size_t len = socket.write_some(buffer(msg), error);
        
    }

    socket.close();
}