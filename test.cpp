#include "json_view/json_view.hpp"
#include "json_view/msgpack.hpp"

int main(int argc, char *argv[])
{
    using namespace jv;
    JsonView obj[3];
    std::string serial;
    jv::msgpack::Dump<jv::msgpack::NativeEndian>(JsonView(obj), [&](auto sv) -> jv::msgpack::CannotFail {
        serial += sv;
        return {};
    });
    auto back = jv::msgpack::Parse<jv::msgpack::NativeEndian>(serial, malloc);
    return 0;
}
