#define BOOST_ASIO_DISABLE_LIB
#define BOOST_ERROR_CODE_HEADER_ONLY

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <memory>
#include <algorithm>

using namespace std;
namespace beast = boost::beast;
namespace ws_ns = beast::websocket;
namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;

using WsStream = ws_ns::stream<tcp::socket>;

mutex clients_mutex;
vector<WsStream*> g_clients;

void register_client(WsStream* ws)
{
    lock_guard<mutex> lock(clients_mutex);
    g_clients.push_back(ws);
    cout << "[registry] Client added. Total: " << g_clients.size() << "\n";
}

void unregister_client(WsStream* ws)
{
    lock_guard<mutex> lock(clients_mutex);
    g_clients.erase(remove(g_clients.begin(), g_clients.end(), ws), g_clients.end());
    cout << "[registry] Client removed. Total: " << g_clients.size() << "\n";
}

void broadcast(const string& msg)
{
    lock_guard<mutex> lock(clients_mutex);
    auto buf = boost::asio::buffer(msg);
    for (WsStream* client : g_clients)
    {
        try {
            client->write(buf);
        }
        catch (exception& e) {
            cerr << "[broadcast] Write failed for a client: " << e.what() << "\n";
        }
    }
}

void handle_client(tcp::socket socket)
{
    auto ws_ptr = make_unique<WsStream>(std::move(socket));
    WsStream& ws = *ws_ptr;

    try {
        beast::flat_buffer buffer;

        http::request<http::string_body> req;
        http::read(ws.next_layer(), buffer, req);
        ws.accept(req);
        cout << "[server] WebSocket handshake OK\n";

        register_client(&ws);

        while (true)
        {
            buffer.clear();
            ws.read(buffer);

            string msg = beast::buffers_to_string(buffer.data());
            cout << "[recv] " << msg << "\n";

            broadcast(msg);
        }
    }
    catch (beast::system_error const& se)
    {
        if (se.code() == ws_ns::error::closed)
            cout << "[server] Client disconnected cleanly.\n";
        else
            cerr << "[server] Client error: " << se.what() << "\n";
    }
    catch (exception& e)
    {
        cerr << "[server] Exception: " << e.what() << "\n";
    }

    unregister_client(&ws);
}

int main()
{
    try {
        cout << "STARTING BROADCAST SERVER...\n";

        boost::asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));
        cout << "Listening on port 12345...\n";

        while (true)
        {
            cout << "Waiting for connection...\n";
            tcp::socket socket(io);
            acceptor.accept(socket);
            cout << "Client connected!\n";
            thread(handle_client, std::move(socket)).detach();
        }
    }
    catch (exception& e)
    {
        cerr << "FATAL: " << e.what() << "\n";
    }
}