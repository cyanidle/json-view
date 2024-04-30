#ifndef JV_MSGPACK_HPP
#define JV_MSGPACK_HPP
#pragma once

#include "json_view.hpp"
#include <type_traits>

namespace jv::msgpack
{

template<typename Fn>
using if_writer_t = std::enable_if_t<std::is_invocable_v<Fn, const char*, size_t>, int>;

enum Flags {
    Default = 0,
    NativeEndian = 1,
};

namespace detail {

template<int flags, typename T>
static T toBig(T raw) noexcept
{
    if constexpr (flags & NativeEndian) {
        return raw;
    } else {
        // TODO
    }
}

template<int flags, typename T>
static T fromBig(T raw) noexcept
{
    if constexpr (flags & NativeEndian) {
        return raw;
    } else {
        // TODO
    }
}

template<int flags, typename Fn>
static auto writeType(uint8_t what, Fn& out) {
    auto conv = char(what);
    return out(&conv, 1);
}

template<int flags, typename T, typename Fn>
static auto write(T what, Fn& out){
    T temp = toBig<flags>(what);
    return out((const char*)&temp.data(), temp.size());
};

template<int flags, typename Fn>
auto writeString(std::string_view sv, Fn& out)
{
    if (sv.size() <= 0b11111) {
        if (auto err = writeType<flags>(uint8_t(0b10100000 | sv.size()), out)) return err;
    } else if (sv.size() <= numeric_limits<uint8_t>::max()) {
        if (auto err = writeType<flags>(0xd9, out)) return err;
        if (auto err = write<flags>(uint8_t(sv.size()), out)) return err;
    }  else if (sv.size() <= numeric_limits<uint16_t>::max()) {
        if (auto err = writeType<flags>(0xda, out)) return err;
        if (auto err = write<flags>(uint16_t(sv.size()), out)) return err; 
    } else {
        if (auto err = writeType<flags>(0xdb, out)) return err;
        if (auto err = write<flags>(uint32_t(sv.size()), out)) return err;
    }
    return out(sv.data(), sv.size());
}

template<int flags, typename Fn>
auto writeBin(std::string_view sv, Fn& out)
{
    if (sv.size() <= numeric_limits<uint8_t>::max()) {
        if (auto err = writeType<flags>(0xc4, out)) return err;
        if (auto err = write<flags>(uint8_t(sz), out)) return err;
    } else if (sv.size() <= numeric_limits<uint16_t>::max()) {
        if (auto err = writeType<flags>(0xc5, out)) return err;
        if (auto err = write<flags>(uint16_t(sz), out)) return err;
    } else {
        if (auto err = writeType<flags>(0xc6, out)) return err;
        if (auto err = write<flags>(uint32_t(sz), out)) return err;
    }
    return out(sv.data(), sv.size()); 
}

template<int flags, typename Fn>
void writeNegInt(int64_t i, Fn& out) {
    if (i >= -32) {
        return writeType<flags>(int8_t(i), out);
    } else if (i >= numeric_limits<int8_t>::min()) {
        if (auto err = writeType<flags>(0xD0, out)) return err;
        return write<flags>(int8_t(i), out);
    } else if (i >= numeric_limits<int16_t>::min()) {
        if (auto err = writeType<flags>(0xD1, out)) return err;
        return write<flags>(int16_t(i), out);
    } else if (i >= numeric_limits<int32_t>::min()) {
        if (auto err = writeType<flags>(0xD2, out)) return err;
        return write<flags>(int32_t(i), out);
    } else {
        if (auto err = writeType<flags>(0xD3, out)) return err;
        return write<flags>(int64_t(i), out);
    }
}

template<int flags, typename Fn>
void writePosInt(uint64_t i, Fn& out) {
    if (i < 128) {
        return writeType<flags>(uint8_t(i), out);
    } else if (i <= numeric_limits<uint8_t>::max()) {
        if (auto err = writeType<flags>(0xCC, out)) return err;
        return write<flags>(uint8_t(i), out);
    } else if (i <= numeric_limits<uint16_t>::max()) {
        if (auto err = writeType<flags>(0xCD, out)) return err;
        return write<flags>(uint16_t(i), out);
    } else if (i <= numeric_limits<uint32_t>::max()) {
        if (auto err = writeType<flags>(0xCE, out)) return err;
        return write<flags>(uint32_t(i), out);
    } else {
        if (auto err = writeType<flags>(0xCF, out)) return err;
        return write<flags>(uint64_t(i), out);
    }
}
}

template<int flags = Default, typename Fn, if_writer_t<Fn> = 1>
auto Dump(JsonView j, Fn&& out) {
    using namespace detail;
    switch (j.Type)
    {
    case JsonView::t_null: {
        return writeType<flags>(uint8_t(0xc0), out);
    }
    case JsonView::t_int: {
        if (j.Integer < 0) {
            return writeNegInt<flags>(j.Integer, out);
        } else {
            return writePosInt<flags>(j.Integer, out);
        }
    }
    case JsonView::t_uint: {
        return writePosInt<flags>(j.Integer, out);
    }
    case JsonView::t_num: {
        if (auto err = writeType<flags>(uint8_t(0xcb), out)) return err;
        return write<flags>(json.GetData().number, out);
    }
    case JsonView::t_str: {
        return writeString<flags>(j.Str(), out);
    }
    case JsonView::t_bin: {
        return writeBin<flags>(j.Str(), out);
    }
    case JsonView::t_arr: {
        auto sz = j.Size;
        if (sz <= 0b1111) {
            if (auto err = writeType<flags>(uint8_t(0b10010000 | sz), out)) return err;
        } else if (sz <= numeric_limits<uint16_t>::max()) {
            if (auto err = writeType<flags>(0xdc, out)) return err;
            if (auto err = write<flags>(uint16_t(sz), out)) return err;
        } else {
            if (auto err = writeType<flags>(0xdd, out)) return err;
            if (auto err = write<flags>(uint32_t(sz), out)) return err;
        }
        for (auto v: j.Arr()) {
            if (auto err = Dump<flags>(v, out)) return err;
        }
        return out(nullptr, 0);
    }
    case JsonView::t_obj: {
        auto sz = j.Size;
        if (sz <= 0b1111)  {
            if (auto err = writeType<flags>(uint8_t(0b10000000 | sz), out)) return err;
        } else if (sz <= numeric_limits<uint16_t>::max()) {
            if (auto err = writeType<flags>(0xde, out)) return err;
            if (auto err = write<flags>(uint16_t(sz), out)) return err;
        } else {
            if (auto err = writeType<flags>(0xdf, out)) return err;
            if (auto err = write<flags>(uint32_t(sz), out)) return err;
        }
        for (auto [k, v]: json.Obj()) {
            if (auto err = Dump<flags>(k, out)) return err;
            if (auto err = Dump<flags>(v, out)) return err;
        }
        return out(nullptr, 0);
    }
    default: {
        assert(false && "Invalid json type");
    }
    }
}

}

#endif //JV_MSGPACK_HPP