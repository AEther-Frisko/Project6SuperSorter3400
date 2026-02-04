#include "crow_all.h"

crow::response render_page(const std::string& title, const std::string& content_file){
    auto base = crow::mustache::load("base.html");
    auto content = crow::mustache::load_text(content_file);

    crow::mustache::context ctx;
    ctx["title"] = title;
    ctx["content"] = content;

    return base.render(ctx);
}

int main(){
    crow::SimpleApp app;

    // Home page
    CROW_ROUTE(app, "/")([](){
        return render_page("Home", "home.html");
    });

    // Leaderboard page
    CROW_ROUTE(app, "/leaderboard")([](){
        return render_page("Leaderboard", "leaderboard.html");
    });

    // Game page
    CROW_ROUTE(app, "/game")([](){
        return render_page("Play Game", "game.html");
    });

    app.port(18080).multithreaded().run();
}