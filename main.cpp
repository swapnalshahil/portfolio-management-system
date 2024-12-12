
#include <bits/stdc++.h>
#include <curl/curl.h>
#include "dependency/json.hpp"
#include "crow.h"

using namespace std;
using json = nlohmann::json;

int const PORT = 8085;
const string clientId = "YaeHq3kl";
const string clientSecret = "M7VEgoPqZxN_Hh2t6xnlCC9ih7_VPh5VdgJLJR35KNI";

unordered_map<std::string, std::string> API = {
    {"AUTH", "https://test.deribit.com/api/v2/public/auth"},
    {"GET_POSITIONS", "https://test.deribit.com/api/v2/private/get_positions"},
    {"PLACE_ORDER", "https://test.deribit.com/api/v2/private/place_order"},
    {"BUY", "https://test.deribit.com/api/v2/private/buy"},
    {"SELL", "https://test.deribit.com/api/v2/private/sell"},
    {"CANCEL", "https://test.deribit.com/api/v2/private/cancel"},
    {"GET_ORDER_BOOK", "https://test.deribit.com/api/v2/public/get_order_book"},
    {"MODIFY_ORDER", "https://test.deribit.com/api/v2/private/edit"}};

// WebSocket connections storage
std::unordered_set<crow::websocket::connection *> active_connections;

// Helper function for handling CURL responses
size_t WriteCallback(void *ptr, size_t size, size_t nmemb, std::string *data)
{
    data->append((char *)ptr, size * nmemb);
    return size * nmemb;
}

// GET request
string sendGetRequest(string url)
{
    CURL *curl;
    CURLcode res;
    std::string response;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::cerr << "cURL Error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return response;
}

// authentication
string checkAuth()
{
    CURL *curl;
    CURLcode res;
    string responseData;
    string authUrl = API["AUTH"] + "?client_id=" + clientId + "&client_secret=" + clientSecret + "&grant_type=client_credentials";

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, authUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            cerr << "CURL Error: " << curl_easy_strerror(res) << std::endl;
            return "";
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    try
    {
        auto resJsonData = json::parse(responseData);
        cout << resJsonData.dump(4) << endl;
        return resJsonData["result"]["access_token"];
    }
    catch (const json::exception &e)
    {
        cerr << "JSON parsing error: " << e.what() << endl;
        return "";
    }
}

// send POST requests with authentication
string sendPostRequest(string url, string payload, string accessToken)
{
    CURL *curl;
    CURLcode res;
    string response;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("Authorization: Bearer " + accessToken).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            cerr << "CURL Error: " << curl_easy_strerror(res) << endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return response;
}

// get current positions
string getCurrentPosition(string currency = "USDC", string kind = "future")
{
    string accessToken = checkAuth();
    if (accessToken.empty())
    {
        cerr << "Authentication failed, no access token received." << endl;
        return "Authentication failed.";
    }

    string payload = R"({
        "jsonrpc": "2.0",
        "method": "private/get_positions",
        "params": {
            "currency": ")" +
                     currency + R"(",
            "kind": ")" +
                     kind + R"("
        },
        "id": 1
    })";

    try
    {
        string response = sendPostRequest(API["GET_POSITIONS"], payload, accessToken);
        auto resJson = json::parse(response);
        cout << "Response: " << resJson.dump(4) << endl;
        return resJson.dump(4); // Return formatted JSON response
    }
    catch (const json::exception &e)
    {
        cerr << "JSON parsing error: " << e.what() << endl;
        return "Error parsing server response.";
    }
}
string placeOrder(string type = "BUY", double amount = 0, string instrument_name = "", string orderType = "market", string label = "", double price = 0, double trigger_price = 0, string triggerType = "last_price")
{
    string access_token = checkAuth();
    if (access_token.empty())
    {
        cerr << "Authentication failed, no access token received." << endl;
        return "Authentication failed.";
    }
    string url = API["BUY"];
    string payload = R"({
        "jsonrpc": "2.0",
        "id": )" + to_string(1) +
                     R"(,
        "method": "private/buy",
        "params": {
            "instrument_name": ")" +
                     instrument_name + R"(",
            "amount": )" +
                     to_string(amount) + R"(,
            "type": ")" +
                     orderType + R"(",
            "price": )" +
                  to_string(price) + R"(,
            "label": ")" +
                     label + R"("
        }
    })";
    if (type == "SELL")
    {
        url = API["SELL"];
        payload = R"({
            "jsonrpc": "2.0",
            "id": )" +
                  to_string(1) + R"(,
            "method": "private/sell",
            "params": {
                "instrument_name": ")" +
                  instrument_name + R"(",
                "amount": )" +
                  to_string(amount) + R"(,
                "type": ")" +
                  orderType + R"(",
                "price": )" +
                  to_string(price) + R"(
            }
        })";
    }

    try
    {
        string response = sendPostRequest(url, payload, access_token);
        auto resJson = json::parse(response);
        cout << "Response: " << resJson.dump(4) << endl;
        return resJson.dump(4);
    }
    catch (const exception &e)
    {
        cerr << "JSON parsing error: " << e.what() << endl;
        return "Error parsing server response.";
    }
}
string cancelOrder(string order_id)
{
    string access_token = checkAuth();
    if (access_token.empty())
    {
        cerr << "Authentication failed, no access token received." << endl;
        return "Authentication failed.";
    }
    string url = API["CANCEL"];
    string payload = R"({
        "jsonrpc": "2.0",
        "id": )" + to_string(1) +
                     R"(,
        "method": "private/cancel",
        "params": {
            "order_id": ")" +
                     order_id + R"("
        }
    })";

    try
    {
        string response = sendPostRequest(url, payload, access_token);
        auto resJson = json::parse(response);
        cout << "Response: " << resJson.dump(4) << endl;
        return resJson.dump(4);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return "Error parsing server response.";
    }
}
string getOrderBook(string instrument_name, int depth = 10)
{
    string url = API["GET_ORDER_BOOK"];
    string payload = R"({
        "jsonrpc": "2.0",
        "id": )" + to_string(1) +
                     R"(,
        "method": "public/get_order_book",
        "params": {
            "instrument_name": ")" +
                     instrument_name + R"(",
            "depth": )" +
                     to_string(depth) + R"(
        }
    })";
    try
    {
        string response = sendPostRequest(url, payload, "");
        auto resJson = json::parse(response);
        cout << "Response: " << resJson.dump(4) << endl;
        return resJson.dump(4);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return "Error parsing server response.";
    }
}
string modifyOrder(string order_id, double newPrice, double newAmount)
{
    string access_token = checkAuth();
    if (access_token.empty())
    {
        cerr << "Authentication failed, no access token received." << endl;
        return "Authentication failed.";
    }
    string url = API["MODIFY_ORDER"];
    string payload = R"({
        "jsonrpc": "2.0",
        "id": )" + to_string(1) +
                   R"(,
        "method": "private/edit",
        "params": {
            "order_id": ")" + order_id + R"(",
            "amount": )" + to_string(newAmount) + R"(,
            "price": )" + to_string(newPrice) + R"(
        }
    })";

    try
    {
        string response = sendPostRequest(url, payload, access_token);
        auto resJson = json::parse(response);
        cout << "Response: " << resJson.dump(4) << endl;
        return resJson.dump(4);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return "Error parsing server response.";
    }
}

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")
    ([]()
     { return "Hello, Crow World!"; });


    CROW_ROUTE(app, "/ws").websocket(&app).onopen([](crow::websocket::connection &conn)
                                                  {
        std::cout << "WebSocket client connected.\n";
        active_connections.insert(&conn); })
        .onclose([](crow::websocket::connection &conn, const std::string &reason, uint16_t code)
                 {
        std::cout << "WebSocket client disconnected. Reason: " << reason << ", Code: " << code << '\n';
        active_connections.erase(&conn); })
        .onmessage([](crow::websocket::connection &conn, const std::string &message, bool is_binary)
                   {
        std::cout << "Received WebSocket message: " << message << '\n';
        conn.send_text("Echo: " + message); });

    //function to broadcast updates
    auto broadcast_message = [](const string &message)
    {
        for (auto *conn : active_connections)
        {
            conn->send_text(message);
        }
    };

    CROW_ROUTE(app, "/health")
    ([]()
     { return "Server running on port: " + to_string(PORT); });


    CROW_ROUTE(app, "/auth")
    ([]()
     { return checkAuth(); });


    CROW_ROUTE(app, "/get-position")
    ([&broadcast_message](const crow::request &req)
     {
        auto query_params = crow::query_string(req.url_params);
        string currency = "USDC";
        string kind = "future";
        if (query_params.get("currency")){
            currency = query_params.get("currency");
        }
        if (query_params.get("kind")){
            kind = query_params.get("kind");
        }
        auto res = getCurrentPosition(currency, kind); ;
        broadcast_message(res);
         return res;
    });


    CROW_ROUTE(app, "/place-order/buy")
    ([&broadcast_message](const crow::request &req)
     { 
        auto query_params = crow::query_string(req.url_params);
        double amount = 0;
        string instrument = "BNB_USDC";
        string type = "market";
        string label = "";
        double price = 0;
        if(query_params.get("amount")){
            try
            {
                amount = stod(query_params.get("amount"));;
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            
        }
         if (query_params.get("instrument"))
        {
            instrument = query_params.get("instrument");
        }
        if (query_params.get("type"))
        {
            type = query_params.get("type");
        }
        if (query_params.get("label"))
        {
            label = query_params.get("label");
        }
        if(query_params.get("price")){
             try
            {
                price = stod(query_params.get("price"));;
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        }
        auto res = placeOrder("BUY", amount, instrument, type, label, price);
        broadcast_message(res);
        return res; 
    });
    CROW_ROUTE(app, "/place-order/sell")
    ([&broadcast_message](const crow::request &req)
     {
        auto query_params = crow::query_string(req.url_params);
        double amount = 0;
        string instrument = "BNB_USDC";
        string type = "market";
        string label = "";
        double price = 0;
        double triggerprice =0;
        if(query_params.get("amount")){
            try
            {
                amount = stod(query_params.get("amount"));;
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            
        }
         if (query_params.get("instrument"))
        {
            instrument = query_params.get("instrument");
        }
        if (query_params.get("type"))
        {
            type = query_params.get("type");
        }
        if (query_params.get("label"))
        {
            label = query_params.get("label");
        }
        if(query_params.get("price")){
            try
            {
                price = stod(query_params.get("price"));
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            
        }
        if(query_params.get("trigger_price")){
            try
            {
                triggerprice = stod(query_params.get("trigger_price"));
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            
        }
        auto res = placeOrder("SELL", amount, instrument, type, label, price, triggerprice);
        broadcast_message(res);
        return res; 
    });

    CROW_ROUTE(app, "/cancel-order")
    ([&broadcast_message](const crow::request &req)
     { 
        auto query_params = crow::query_string(req.url_params);
        string order_id = "";
        if(query_params.get("order_id")){
            order_id = query_params.get("order_id");
        }
        auto res = cancelOrder(order_id);
        broadcast_message(res);
        return res; 
    });
    
    CROW_ROUTE(app, "/get-order-book")
    ([&broadcast_message](const crow::request &req)
     {
        auto query_params = crow::query_string(req.url_params);
        string instrument="BNB_USDC";
        if(query_params.get("instrument")){
            instrument = query_params.get("instrument");
        }
        auto res = getOrderBook(instrument);
        broadcast_message(instrument +" order book fetched");
        broadcast_message(res);
        return res; });
    CROW_ROUTE(app, "/modify-order")
    ([&broadcast_message](const crow::request &req)
     { 
        auto query_params = crow::query_string(req.url_params);
        string order_id = "";
        double price = 0;
        double amount = 0;
        if(query_params.get("order_id")){
            order_id = query_params.get("order_id");
        }
        if(query_params.get("amount")){
            try
            {
                amount = stod(query_params.get("amount"));
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            
        }
        if(query_params.get("price")){
            try
            {
                price = stod(query_params.get("price"));
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
            
        }
        cout<<order_id << '\n';
        cout<<amount << '\n';
        cout<<price << '\n';
        auto res = modifyOrder(order_id, price, amount);
        broadcast_message(res);
        return res; 
        });

    cout << "Starting server on port " << PORT << "..." << endl;
    app.port(PORT).multithreaded().run();

    return 0;
}
