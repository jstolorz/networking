#include <iostream>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <string>
#include <vector>

std::vector<char> v_buffer(20 * 1024);

void grab_some_data(asio::ip::tcp::socket& socket){
    socket.async_read_some(asio::buffer(v_buffer.data(), v_buffer.size()),
                           [&](std::error_code ec, std::size_t length){

        if(!ec){
            std::cout << "\n\nRead " << length << "bytes\n\n";

            for (int i = 0; i < length; ++i) {
                std::cout << v_buffer[i];
            }

            grab_some_data(socket);
        }

    });
}

int main(){

    asio::error_code ec;

    // Create a "contex" - essentially the platform specific interface
    asio::io_context context;

    // Give some fake tasks to asio so the context doesn't finish
    asio::io_context::work worker(context);

    // Start the context
    std::thread thr_context = std::thread([&](){ context.run(); });

    // Get the address of somewhere we wish to connect to
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec), 80);

    // Create a socket, the context will deliver the implementation
    asio::ip::tcp::socket socket(context);

    // Tell socket to try and connect
    socket.connect(endpoint, ec);

    if(!ec){
        std::cout << "Connected! \n";
    }else{
        std::cout << "Failed to connect to address:\n" << ec.message() << "\n";
    }

    if(socket.is_open()){

        grab_some_data(socket);

        std::string request =
                "GET /index.html HTTP/1.1\r\n"
                "Host: example.com\r\n"
                "Connection: close\r\n\r\n";
        socket.write_some(asio::buffer(request.data(), request.size()),ec);

        // Program does something else, while asio handles data transfer in background
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(20000ms);

        context.stop();
        if(thr_context.joinable()){
            thr_context.join();
        }

    }

    return 0;
}

