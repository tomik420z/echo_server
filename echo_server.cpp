#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>

#include <ws2tcpip.h>
#include <mswsock.h>
#include <winsock2.h>

#include <memory>
#include <iostream>




namespace async_io {
    using namespace boost::asio;
    using acceptor = ip::tcp::acceptor;
    using socket = ip::tcp::socket;
    using io_context = io_context; 
    using endpoint = ip::tcp::endpoint;
    using address = ip::address;
    using address_v4 = ip::address_v4;
    using port_type = ip::port_type;

    using tcp = ip::tcp;
}

namespace server_consts {
    static constexpr u_short PORT = 2009;
}

namespace server_types {
    using io_context_ptr = std::shared_ptr<async_io::io_context>;
    using socket_ptr = std::shared_ptr<async_io::socket>;
}



void print_message(const char* _buff, size_t _length) {

    std::cout << "message: ";
    for(size_t i = 0; i < _length; ++i) {
        std::cout << _buff[i];
    }
    std::cout << std::endl;

}



class session : public std::enable_shared_from_this<session> {
public:
    session(async_io::socket&& _socket_client) :
        m_socket_client(std::move(_socket_client)) { }

    void start() {
        // non-blocking thread
        async_read();
    }
private:
    

    void async_read() {
        auto self = shared_from_this();
    
        m_socket_client.async_read_some(
            boost::asio::buffer(buffer, MAX_CAPACITY),

            [this, self](std::error_code _error, size_t _length){
                if (!_error) {

                    print_message(buffer, _length);
                    async_write(_length);

                }
            });
    }

    void async_write(size_t _length) {
        auto self = shared_from_this();
        m_socket_client.async_write_some(
                async_io::buffer(buffer, _length),
                [this, self](std::error_code _error, size_t) {
                    if (!_error) {
                        async_read();
                    }
                });
    }

private:
    static constexpr size_t MAX_CAPACITY = 1024;
    char buffer[MAX_CAPACITY];
    
    async_io::socket m_socket_client;
};


class echo_server {
private:
    
public:
    echo_server(server_types::io_context_ptr _context, u_short _port) :
        m_context(std::move(_context)),
        m_acceptor(*m_context, async_io::endpoint(async_io::address::from_string("127.0.0.1"), _port)),
        m_client_socket(*m_context) {
        std::cout << "server port = " << m_acceptor.local_endpoint().port() << std::endl;
        std::cout << "server addr = " << m_acceptor.local_endpoint().address().to_string() << std::endl;
        std::cout << "server protocol family = " << m_acceptor.local_endpoint().protocol().family() << std::endl;
        do_accept();
    }
private:
    void do_accept() {

        std::cout << "wait client..." << std::endl;

        m_acceptor.async_accept(m_client_socket, [this](std::error_code _error) {
                                                    
                                                    if (!_error) {
                                                        std::cout << "new connection!!!" << std::endl;
                                                        std::make_shared<session>(std::move(m_client_socket))->start();
                                                    } else {
                                                        std::cout << "error: " << _error.message() << std::endl;
                                                    }
                                                    do_accept();
                                                });

    }


private:
    server_types::io_context_ptr m_context;
    async_io::acceptor m_acceptor;
    async_io::socket m_client_socket;
    
};

int main() {
    auto context = std::make_shared<async_io::io_context>();
    try {
        echo_server serv(context, server_consts::PORT);
        context->run();
    } catch(std::exception& _ex) {
        std::cerr << _ex.what() << std::endl;
    }

    return 0;
}