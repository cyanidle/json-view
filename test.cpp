#include "json_view/json_view.hpp"
#include "json_view/msgpack.hpp"
#include <string>

int main(int argc, char *argv[])
{
    using namespace mjv;
    using namespace mjv::msgpack;
    JsonPair obj[] = {{"a", 123}, {"b", "babra"}};
    JsonView arr[] = {nullptr, "123"};
    JsonView top[] = {1231231231, 111112, nullptr, arr, obj};
    Context ctx;
    std::string serial;
    Dump(JsonView(top), [&](auto sv) -> CannotFail {
        serial += sv;
        return {};
    });
    auto back = Parse(serial, ctx);
    auto str = back[3][1];
    assert(str.String() == "123");
    return 0;
}
