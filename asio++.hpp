#pragma once

#define _WIN32_WINNT 0x0601

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <functional>
#include <memory>

namespace asio {
        void async_write_full(
        std::shared_ptr<asio::ip::tcp::socket> socket,
        std::shared_ptr<std::vector<unsigned char>> buffer,
        std::function<void(std::error_code)> callback
    ) {
        std::shared_ptr<std::function<void(std::error_code, std::size_t)>> write_callback;

        auto offset = std::make_shared<std::size_t>(0U);

        write_callback = std::make_shared<std::function<void(std::error_code, std::size_t)>>(
            [socket, buffer, offset, write_callback, callback](std::error_code ec, std::size_t n) {
                if (ec) {
                    callback(ec);
                } else {
                    *offset += n;
                    if (*offset < buffer->size()) {
                        asio::async_write(*socket, asio::buffer(buffer->data() + *offset, buffer->size() - *offset), *write_callback);
                    } else {
                        callback(ec);
                    }
                }
            }
        );

        asio::async_write(*socket, asio::buffer(buffer->data(), buffer->size()), *write_callback);
    }

    template <typename T>
    void async_write_header(
        std::shared_ptr<asio::ip::tcp::socket> socket,
        const T& header,
        std::function<void(std::error_code)> callback
    ) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        
        auto buffer = std::make_shared<std::vector<unsigned char>>(sizeof(T));

        std::memcpy(buffer->data(), &header, sizeof(T));

        async_write_full(socket, buffer, callback);
    }

    void async_write_message(
        std::shared_ptr<asio::ip::tcp::socket> socket,
        std::shared_ptr<std::vector<unsigned char>> data,
        std::function<void(std::error_code)> callback
    ) {
        uint32_t length = data->size();
        
        async_write_header<uint32_t>(socket, length, [socket, data, callback](std::error_code ec) {
            if (ec) {
                callback(ec);
            } else {
                async_write_full(socket, data, callback);
            }
        });
    }

    void async_read_full(
        std::shared_ptr<asio::ip::tcp::socket> socket,
        size_t length,
        std::function<void(std::shared_ptr<std::vector<unsigned char>>, std::error_code)> callback
    ) {
        std::shared_ptr<std::function<void(std::error_code, std::size_t)>> read_callback;

        auto buffer = std::make_shared<std::vector<unsigned char>>(length);
        auto offset = std::make_shared<std::size_t>(0U);

        read_callback = std::make_shared<std::function<void(std::error_code ec, std::size_t n)>>(
            [socket, buffer, offset, read_callback, callback](std::error_code ec, std::size_t n) {      
                if (ec) {
                    callback(buffer, ec);
                } else {
                    *offset += n;
                    if (*offset < buffer->size()) {
                        asio::async_read(*socket, asio::buffer(buffer->data() + *offset, buffer->size() - *offset), *read_callback);
                    } else {
                        callback(buffer, ec);
                    }
                }
            }
        );
        asio::async_read(*socket, asio::buffer(buffer->data(), buffer->size()), *read_callback);
    }

    template <typename T>
    void async_read_header(
        std::shared_ptr<asio::ip::tcp::socket> socket,
        std::function<void(T&, std::error_code)> callback
    ) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

        async_read_full(socket, sizeof(T), [callback](std::shared_ptr<std::vector<unsigned char>> buffer, std::error_code ec) {
            callback(*reinterpret_cast<T*>(buffer->data()), ec);
        });
    }

    void async_read_message(
        std::shared_ptr<asio::ip::tcp::socket> socket,
        std::function<void(std::shared_ptr<std::vector<unsigned char>>, std::error_code)> callback
    ) {
        async_read_header<uint32_t>(socket, [socket, callback](uint32_t& length, std::error_code ec) {
            if (ec) {
                callback(std::make_shared<std::vector<unsigned char>>(), ec);
            } else {
                async_read_full(socket, length, [callback](std::shared_ptr<std::vector<unsigned char>> buffer, std::error_code ec) {
                    callback(buffer, ec);
                });
            }
        });
    }

    void async_write_full_ssl(
        std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> stream,
        std::shared_ptr<std::vector<unsigned char>> buffer,
        std::function<void(std::error_code)> callback
    ) {
        std::shared_ptr<std::function<void(std::error_code, std::size_t)>> write_callback;

        auto offset = std::make_shared<std::size_t>(0U);

        write_callback = std::make_shared<std::function<void(std::error_code, std::size_t)>>(
            [stream, buffer, offset, write_callback, callback](std::error_code ec, std::size_t n) {
                if (ec) {
                    callback(ec);
                } else {
                    *offset += n;
                    if (*offset < buffer->size()) {
                        asio::async_write(*stream, asio::buffer(buffer->data() + *offset, buffer->size() - *offset), *write_callback);
                    } else {
                        callback(ec);
                    }
                }
            }
        );

        asio::async_write(*stream, asio::buffer(buffer->data(), buffer->size()), *write_callback);
    }

    template <typename T>
    void async_write_header_ssl(
        std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> stream,
        const T& header,
        std::function<void(std::error_code)> callback
    ) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        
        auto buffer = std::make_shared<std::vector<unsigned char>>(sizeof(T));

        std::memcpy(buffer->data(), &header, sizeof(T));

        async_write_full_ssl(stream, buffer, callback);
    }

    void async_write_message_ssl(
        std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> stream,
        std::shared_ptr<std::vector<unsigned char>> data,
        std::function<void(std::error_code)> callback
    ) {
        uint32_t length = data->size();
        
        async_write_header_ssl<uint32_t>(stream, length, [stream, data, callback](std::error_code ec) {
            if (ec) {
                callback(ec);
            } else {
                async_write_full_ssl(stream, data, callback);
            }
        });
    }

    void async_read_full_ssl(
        std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> stream,
        size_t length,
        std::function<void(std::shared_ptr<std::vector<unsigned char>>, std::error_code)> callback
    ) {
        std::shared_ptr<std::function<void(std::error_code, std::size_t)>> read_callback;

        auto buffer = std::make_shared<std::vector<unsigned char>>(length);
        auto offset = std::make_shared<std::size_t>(0U);

        read_callback = std::make_shared<std::function<void(std::error_code ec, std::size_t n)>>(
            [stream, buffer, offset, read_callback, callback](std::error_code ec, std::size_t n) {      
                if (ec) {
                    callback(buffer, ec);
                } else {
                    *offset += n;
                    if (*offset < buffer->size()) {
                        asio::async_read(*stream, asio::buffer(buffer->data() + *offset, buffer->size() - *offset), *read_callback);
                    } else {
                        callback(buffer, ec);
                    }
                }
            }
        );
        asio::async_read(*stream, asio::buffer(buffer->data(), buffer->size()), *read_callback);
    }

    template <typename T>
    void async_read_header_ssl(
        std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> stream,
        std::function<void(T&, std::error_code)> callback
    ) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

        async_read_full_ssl(stream, sizeof(T), [callback](std::shared_ptr<std::vector<unsigned char>> buffer, std::error_code ec) {
            callback(*reinterpret_cast<T*>(buffer->data()), ec);
        });
    }

    void async_read_message_ssl(
        std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> stream,
        std::function<void(std::shared_ptr<std::vector<unsigned char>>, std::error_code)> callback
    ) {
        async_read_header_ssl<uint32_t>(stream, [stream, callback](uint32_t& length, std::error_code ec) {
            if (ec) {
                callback(std::make_shared<std::vector<unsigned char>>(), ec);
            } else {
                async_read_full_ssl(stream, length, [callback](std::shared_ptr<std::vector<unsigned char>> buffer, std::error_code ec) {
                    callback(buffer, ec);
                });
            }
        });
    }
}
