#include "../libs/Crow/include/crow.h"

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        return "Medley++ Backend Running";
    });

    app.port(8080).multithreaded().run();
}