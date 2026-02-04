#include "crow_all.h"

int main(){
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        auto page = crow::mustache::load("test_page.html");
        crow::mustache::context ctx ({{"person", "World"}});
        return page.render(ctx);
    });

    CROW_ROUTE(app, "/<string>")([](std::string name){
        auto page = crow::mustache::load("test_page.html");
        crow::mustache::context ctx ({{"person", name}});
        return page.render(ctx);
    });

    app.port(18080).multithreaded().run();
}