#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/yield.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/coroutine2/coroutine.hpp>

#include <ws2tcpip.h>
#include <mswsock.h>
#include <winsock2.h>

#include <memory>
#include <iostream>
#include <coroutine>

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

template <typename... Args>
struct std::coroutine_traits<void, Args...> {
    struct promise_type {
        void get_return_object() noexcept {}
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
    };
};

struct awaiter_read {
    awaiter_read(async_io::socket& _socket, char* _buffer, size_t _length) : 
        m_socket(_socket), 
        m_buffer(_buffer), 
        m_length_buffer(_length) {}

    bool await_ready() { return false; }
    std::pair<boost::system::error_code, size_t> await_resume() { return std::make_pair(m_ec, m_size); }
    void await_suspend(std::coroutine_handle<> coro) {
        m_socket.async_read_some(async_io::buffer(m_buffer, m_length_buffer), 
                            [this, coro](std::error_code ec, size_t size) {
                                
                                m_ec = ec;
                                m_size = size;
                                coro.resume();
                            });
    }


private:
    async_io::socket& m_socket;
    char* m_buffer;
    size_t m_length_buffer;

    boost::system::error_code m_ec;
    size_t m_size;
};


struct awaiter_write {
    awaiter_write(async_io::socket& _socket, char* _buffer, size_t _length) : 
        m_socket(_socket), 
        m_buffer(_buffer), 
        m_length_buffer(_length) {}

    bool await_ready() { return false; }
    auto await_resume() { return std::make_pair(m_ec, m_size); }
    void await_suspend(std::coroutine_handle<> coro) {
        m_socket.async_write_some(async_io::buffer(m_buffer, m_length_buffer), 
                            [this, coro](std::error_code ec, size_t size){
                                m_ec = ec;
                                m_size = size;
                                coro.resume();
                            });
    }


private:
    async_io::socket& m_socket;
    char* m_buffer;
    size_t m_length_buffer;

    boost::system::error_code m_ec;
    size_t m_size;
};



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
        auto self = shared_from_this();

        for(;;){
            auto[ec, size_buffer] = co_await async_read(m_socket_client, buffer, MAX_CAPACITY);
            if (ec) {
                std::cout << ec.message() << " " << ec.value() << std::endl;
                break;
            }

            print_message(buffer, size_buffer);
            // callback
            auto [err_code_write, _] = co_await async_write(m_socket_client, buffer, size_buffer);

            if (err_code_write) {
                std::cout << err_code_write.message() << " " << ec.value() << std::endl;
            }
        }
    }
private:

    static awaiter_read async_read(async_io::socket& _socket, char* _buffer, size_t _length) {
        return awaiter_read{_socket, _buffer, _length};
    }

    static awaiter_write async_write(async_io::socket& _socket, char* _buffer, size_t _length_buffer) {
        return awaiter_write{_socket, _buffer, _length_buffer};
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