//
// Created by MyPC on 10/26/2025.
//

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iomanip>
#include <chrono>
#include <memory>
#include "feed_handler.hpp"
#include "simdjson.h"
#include "./Binance_feed_parser.hpp"
#include "../common/ring_buffer.hpp"

// Use TLS config for wss:// connections
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;

class BinanceWSClient {
private:
    client m_client;
    websocketpp::connection_hdl m_hdl;
    size_t m_message_count;
    simdjson::ondemand::parser parser;
    RingBuffer<Binance_Feed> r_buf;

    // TLS initialization
    context_ptr on_tls_init(websocketpp::connection_hdl) {
        context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
        try {
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                           boost::asio::ssl::context::no_sslv2 |
                           boost::asio::ssl::context::no_sslv3 |
                           boost::asio::ssl::context::single_dh_use);

            // Don't verify server certificate for simplicity
            ctx->set_verify_mode(boost::asio::ssl::verify_none);
        } catch (std::exception& e) {
            std::cerr << "TLS init error: " << e.what() << std::endl;
        }
        return ctx;
    }

public:
    BinanceWSClient(RingBuffer<Binance_Feed> &r_buf) :
      m_message_count(0),
      r_buf(r_buf)
      {
        // Minimal logging - only show important events
        m_client.clear_access_channels(websocketpp::log::alevel::all);
        m_client.clear_error_channels(websocketpp::log::elevel::all);

        // Initialize ASIO
        m_client.init_asio();

        // Set TLS init handler
        m_client.set_tls_init_handler([this](websocketpp::connection_hdl hdl) {
            return on_tls_init(hdl);
        });

        // Connection opened handler
        m_client.set_open_handler([this](websocketpp::connection_hdl hdl) {
            m_hdl = hdl;
            std::cout << "\nConnected to Binance WebSocket Stream" << std::endl;
            std::cout << "Listening for market data..." << std::endl;
        });

        // Message handler - prints received data
        m_client.set_message_handler([this](websocketpp::connection_hdl hdl, client::message_ptr msg) {
            m_message_count++;
            std::string payload = msg->get_payload();

            parser.iterate(payload);
            r_buf.write([parser](Binance_Feed &param){
              param = Binance_Feed(parser["E"], parser["s"], parser["t"], parser["p"], parser["q"], parser["m"]);
            };);
        });

        // Pong handler - respond to ping frames
        m_client.set_pong_handler([](websocketpp::connection_hdl, std::string) {
            // Silent pong response
            return true;
        });

        // Connection closed handler
        m_client.set_close_handler([this](websocketpp::connection_hdl hdl) {
            std::cout << "Total messages received: " << m_message_count << std::endl;
        });

        // Connection failed handler
        m_client.set_fail_handler([](websocketpp::connection_hdl hdl) {
            std::cerr << "Unable to establish connection to Binance WebSocket" << std::endl;
        });
    }

    void connect(const std::string& uri) {
        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_client.get_connection(uri, ec);

        if (ec) {
            throw std::runtime_error("Connection creation error: " + ec.message());
        }

        m_client.connect(con);

        std::cout << "Connecting to: " << uri << std::endl;

        // Run the ASIO io_service
        m_client.run();
    }

    // Subscribe to additional streams (for combined stream endpoint)
    void subscribe(const std::vector<std::string>& streams) {
        if (streams.empty()) return;

        std::string sub_msg = "{\"method\":\"SUBSCRIBE\",\"params\":[";
        for (size_t i = 0; i < streams.size(); ++i) {
            sub_msg += "\"" + streams[i] + "\"";
            if (i < streams.size() - 1) sub_msg += ",";
        }
        sub_msg += "],\"id\":1}";

        websocketpp::lib::error_code ec;
        m_client.send(m_hdl, sub_msg, websocketpp::frame::opcode::text, ec);

        if (ec) {
            std::cerr << "Subscribe error: " << ec.message() << std::endl;
        } else {
            std::cout << "Subscribed to streams: ";
            for (const auto& s : streams) std::cout << s << " ";
            std::cout << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        RingBuffer<Bianance_Feed> r_buf();

        BinanceWSClient ws_client(&r_buf);

        // Default to BTC/USDT trade stream
        std::string stream = "btcusdt@trade";
        std::string endpoint = "data-stream.binance.vision";  // Use market data only endpoint

        // Allow user to specify stream via command line
        if (argc > 1) {
            stream = argv[1];
        }
        if (argc > 2) {
            endpoint = argv[2];
        }

        // Construct URI - raw stream endpoint (port 443)
        std::string binance_uri = "wss://" + endpoint + ":443/ws/" + stream;

        std::cout << "\n=== Binance WebSocket Stream Reader ===" << std::endl;
        std::cout << "Endpoint: " << endpoint << std::endl;
        std::cout << "Stream: " << stream << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Connect to Binance WebSocket
        ws_client.connect(binance_uri);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
