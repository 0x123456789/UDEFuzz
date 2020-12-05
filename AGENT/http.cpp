#include "http.h"

#include <cpprest/http_listener.h>
#include <cpprest/json.h>

#pragma comment(lib, "Httpapi.lib")
#pragma comment(lib, "..\\UDEFX2\\packages\\cpprestsdk_x64-windows-static\\lib\\cpprest_2_10")

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;


#include <iostream>
#include <map>
#include <set>
#include <string>
using namespace std;

#define TRACE(msg)            wcout << msg
#define TRACE_ACTION(a, k, v) wcout << a << L" (" << k << L", " << v << L")\n"

map<utility::string_t, utility::string_t> dictionary;


void display_json(
    json::value const& jvalue,
    utility::string_t const& prefix)
{
    wcout << prefix << jvalue.serialize() << endl;
}


void handle_get(http_request request)
{
    TRACE(L"\nhandle GET\n");
    auto answer = json::value::object();

    dictionary[L"gg"] = L"x";
    dictionary[L"QQ"] = L"aa";

    for (auto const& p : dictionary) {
        answer[p.first] = json::value::string(p.second);
    }

    display_json(json::value::null(), L"R: ");
    display_json(answer, L"S: ");

    request.reply(status_codes::OK, answer);
}


int ListenAndServe() {

    http_listener listener(L"http://*:8080/");


    listener.support(methods::GET, handle_get);
    //listener.support(methods::POST, handle_post);
    //listener.support(methods::PUT, handle_put);
    //listener.support(methods::DEL, handle_del);

    try
    {
        listener
            .open()
            .then([&listener]() {TRACE(L"\nstarting to listen\n"); })
            .wait();

        while (true);
    }
    catch (exception const& e)
    {
        wcout << e.what() << endl;
    }

    return 0;

}