#include <nlohmann/json.hpp>
int main() {
    nlohmann::json data;
    data["test"] = "working";
    return 0;
}
