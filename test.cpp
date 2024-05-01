#include "json_view/json_view.hpp"
#include "json_view/msgpack.hpp"

int main(int argc, char *argv[])
{
    using namespace mjv;
    using namespace mjv::msgpack;
    JsonView obj[3] = {JsonView{1}, JsonView{2}, JsonView{nullptr}};
    std::string serial;
    Dump<NativeEndian>(JsonView(obj), [&](auto sv) -> CannotFail {
        serial += sv;
        return {};
    });
    auto back = Parse<NativeEndian>(serial, malloc);
    return 0;
}
