#ifndef JV_MSGPACK_HPP
#define JV_MSGPACK_HPP
#pragma once

#include "json_view.hpp"
#include <type_traits>
#include <limits>
#include <string.h>
#include <stdlib.h>
#include <endian.h>

namespace mjv::msgpack
{

template<typename Fn>
concept writer = requires(Fn f, std::string_view msg)
{
    {f(msg)} -> std::convertible_to<bool>;
};

template<typename Fn>
concept alloc = requires(Fn f, size_t sz)
{
    {f(sz)} -> std::convertible_to<void*>;
};

enum Flags {
    Default = 0,
    NativeEndian = 1,
};

using CannotFail = std::false_type;

template<int flags = Default, writer Writer>
auto Dump(JsonView j, Writer&& out, unsigned depthLimit = 30) noexcept;

template<int flags = Default, alloc Alloc>
JsonView Parse(std::string_view buffer, Alloc&& out, unsigned depthLimit = 30) noexcept;

namespace detail {

template<typename T> concept bswappable = std::integral<T> || std::floating_point<T>;

[[gnu::always_inline]]
static inline auto bswap(bswappable auto val) noexcept
    requires (sizeof(val) <= 8)
{
    if constexpr (sizeof(val) == 1) {
        return val;
    } else if constexpr (sizeof(val) == 2) {
        return __builtin_bswap16(val);
    } else if constexpr (sizeof(val) == 4) {
        return __builtin_bswap32(val);
    } else {
        return __builtin_bswap64(val);
    }
}

template<int flags>
[[gnu::always_inline]]
static inline auto toBig(bswappable auto raw) noexcept
{
    if constexpr (flags & NativeEndian || BYTE_ORDER == BIG_ENDIAN) {
        return raw;
    } else  {
        return bswap(raw);
    }
}

template<int flags, bswappable T>
[[gnu::always_inline]]
static inline T fromBig(const char* data) noexcept
{
    T res;
    memcpy(&res, data, sizeof(res));
    if constexpr (flags & NativeEndian || BYTE_ORDER == BIG_ENDIAN) {
        return res;
    } else  {
        return bswap(res);
    }
}

template<int flags, typename Writer>
[[gnu::always_inline]]
inline auto writeType(uint8_t what, Writer& out) {
    auto conv = char(what);
    return out(std::string_view{&conv, 1});
}

template<int flags, typename T, typename Writer>
[[gnu::always_inline]]
inline auto write(T what, Writer& out){
    T temp = toBig<flags>(what);
    return out(std::string_view{(const char*)&temp, sizeof(temp)});
};

template<int flags, typename Writer>
auto writeString(std::string_view sv, Writer& out)
{
    if (sv.size() <= 0b11111) {
        if (auto err = writeType<flags>(uint8_t(0b10100000 | sv.size()), out)) [[unlikely]] return err;
    } else if (sv.size() <= std::numeric_limits<uint8_t>::max()) {
        if (auto err = writeType<flags>(0xd9, out)) [[unlikely]] return err;
        if (auto err = write<flags>(uint8_t(sv.size()), out)) [[unlikely]] return err;
    }  else if (sv.size() <= std::numeric_limits<uint16_t>::max()) {
        if (auto err = writeType<flags>(0xda, out)) [[unlikely]] return err;
        if (auto err = write<flags>(uint16_t(sv.size()), out)) [[unlikely]] return err;
    } else {
        if (auto err = writeType<flags>(0xdb, out)) [[unlikely]] return err;
        if (auto err = write<flags>(uint32_t(sv.size()), out)) [[unlikely]] return err;
    }
    return out(sv);
}

template<int flags, typename Writer>
auto writeBin(std::string_view sv, Writer& out)
{
    if (sv.size() <= std::numeric_limits<uint8_t>::max()) {
        if (auto err = writeType<flags>(0xc4, out)) [[unlikely]] return err;
        if (auto err = write<flags>(uint8_t(sv.size()), out)) [[unlikely]] return err;
    } else if (sv.size() <= std::numeric_limits<uint16_t>::max()) {
        if (auto err = writeType<flags>(0xc5, out)) [[unlikely]] return err;
        if (auto err = write<flags>(uint16_t(sv.size()), out)) [[unlikely]] return err;
    } else {
        if (auto err = writeType<flags>(0xc6, out)) [[unlikely]] return err;
        if (auto err = write<flags>(uint32_t(sv.size()), out)) [[unlikely]] return err;
    }
    return out(sv);
}

template<int flags, typename Writer>
auto writeNegInt(int64_t i, Writer& out) {
    if (i >= -32) {
        return writeType<flags>(int8_t(i), out);
    } else if (i >= std::numeric_limits<int8_t>::min()) {
        if (auto err = writeType<flags>(0xD0, out)) [[unlikely]] return err;
        return write<flags>(int8_t(i), out);
    } else if (i >= std::numeric_limits<int16_t>::min()) {
        if (auto err = writeType<flags>(0xD1, out)) [[unlikely]] return err;
        return write<flags>(int16_t(i), out);
    } else if (i >= std::numeric_limits<int32_t>::min()) {
        if (auto err = writeType<flags>(0xD2, out)) [[unlikely]] return err;
        return write<flags>(int32_t(i), out);
    } else {
        if (auto err = writeType<flags>(0xD3, out)) [[unlikely]] return err;
        return write<flags>(int64_t(i), out);
    }
}

template<int flags, typename Writer>
auto writePosInt(uint64_t i, Writer& out) {
    if (i < 128) {
        return writeType<flags>(uint8_t(i), out);
    } else if (i <= std::numeric_limits<uint8_t>::max()) {
        if (auto err = writeType<flags>(0xCC, out)) [[unlikely]] return err;
        return write<flags>(uint8_t(i), out);
    } else if (i <= std::numeric_limits<uint16_t>::max()) {
        if (auto err = writeType<flags>(0xCD, out)) [[unlikely]] return err;
        return write<flags>(uint16_t(i), out);
    } else if (i <= std::numeric_limits<uint32_t>::max()) {
        if (auto err = writeType<flags>(0xCE, out)) [[unlikely]] return err;
        return write<flags>(uint32_t(i), out);
    } else {
        if (auto err = writeType<flags>(0xCF, out)) [[unlikely]] return err;
        return write<flags>(uint64_t(i), out);
    }
}

#define _JV_CHECK(x) if ((x).type() == t_discarded) [[unlikely]] return (x)

inline constexpr auto ErrEOF = JsonView::Discarded("unexpected eof");
inline constexpr auto ErrOOM = JsonView::Discarded("unexpected oom");
inline constexpr auto ErrTooDeep = JsonView::Discarded("recursion is too deep");

inline constexpr std::string_view consume(std::string_view& buff, size_t amount) noexcept {
    auto res = buff.substr(0, amount);
    buff = buff.substr(amount);
    return res;
}

template<int flags, typename Alloc>
JsonView parseOne(std::string_view& data, Alloc& ctx, unsigned depthLimit) noexcept;
template<int flags, typename Alloc>
JsonView parseObject(unsigned count, std::string_view& data, Alloc& ctx, unsigned depthLimit) noexcept;
template<int flags, typename Alloc>
JsonView parseArray(unsigned int count, std::string_view& data, Alloc& ctx, unsigned depthLimit) noexcept;

template<int flags, typename T>
[[gnu::always_inline]]
inline JsonView unpackTrivial(std::string_view& data) noexcept
{
    if (data.size() < sizeof(T)) [[unlikely]] {
        return ErrEOF;
    } else {
        auto res = fromBig<flags, T>(data.data());
        data = data.substr(sizeof(T));
        return JsonView{res};
    }
}

template<int flags, typename SzT>
[[gnu::always_inline]]
inline JsonView unpackStr(std::string_view& data) noexcept
{
    auto len = unpackTrivial<flags, SzT>(data);
    _JV_CHECK(len);
    auto act = len.GetData().uinteger;
    if (data.size() < act) [[unlikely]] return ErrEOF;
    return JsonView(consume(data, act));
}

template<int flags, typename SzT, SzT add = 0>
[[gnu::always_inline]]
inline JsonView unpackBin(std::string_view& data) noexcept
{
    auto len = unpackTrivial<flags, SzT>(data);
    _JV_CHECK(len);
    auto act = len.GetData().uinteger;
    if (data.size() < act + add) [[unlikely]] return ErrEOF;
    return JsonView::Binary(consume(data, act + add));
}

template<int flags, typename SzT, typename Alloc>
JsonView unpackArr(std::string_view& data, Alloc& ctx, unsigned depthLimit) noexcept
{
    auto len = unpackTrivial<flags, SzT>(data);
    _JV_CHECK(len);
    return parseArray<flags>(unsigned(len.GetData().uinteger), data, ctx, depthLimit);
}

template<int flags, typename SzT, typename Alloc>
JsonView unpackObj(std::string_view& data, Alloc& ctx, unsigned depthLimit) noexcept
{
    auto len = unpackTrivial<flags, SzT>(data);
    _JV_CHECK(len);
    return parseObject<flags>(unsigned(len.GetData().uinteger), data, ctx, depthLimit);
}

template<int flags, size_t size>
[[gnu::always_inline]]
inline JsonView unpackExt(std::string_view& data) noexcept {
    if (data.size() < 1 + size) [[unlikely]] return ErrEOF;
    return JsonView::Binary(consume(data, 1 + size));
}

template<int flags, typename Alloc>
[[gnu::always_inline]]
inline JsonView parseObject(unsigned count, std::string_view& data, Alloc& ctx, unsigned depthLimit) noexcept try
{
    auto obj = (JsonPair*)ctx(sizeof(JsonPair) * count);
    if (!obj) [[unlikely]] return ErrOOM;
    for (size_t i = 0u; i < count; ++i) {
        _JV_CHECK(obj[i].key = parseOne<flags>(data, ctx, depthLimit));
        _JV_CHECK(obj[i].value = parseOne<flags>(data, ctx, depthLimit));
    }
    return JsonView(obj, count);
} catch(...) {
    return ErrOOM;
}

template<int flags, typename Alloc>
[[gnu::always_inline]]
inline JsonView parseArray(unsigned int count, std::string_view& data, Alloc& ctx, unsigned depthLimit) noexcept try
{
    auto arr = (JsonView*)ctx(sizeof(JsonView) * count);
    if (!arr) [[unlikely]] return ErrOOM;
    for (size_t i = 0u; i < count; ++i) {
        _JV_CHECK(arr[i] = parseOne<flags>(data, ctx, depthLimit));
    }
    return JsonView(arr, count);
} catch (...) {
    return ErrOOM;
}

template<int flags, typename Alloc>
[[gnu::flatten]]
JsonView parseOne(std::string_view& data, Alloc& ctx, unsigned depthLimit) noexcept
{
    if (!depthLimit) [[unlikely]] {
        return ErrTooDeep;
    }
    static_assert(std::numeric_limits<float>::is_iec559, "non IEEE 754 float");
    static_assert(std::numeric_limits<double>::is_iec559, "non IEEE 754 double");
    if ((!data.size())) [[unlikely]] {
        return ErrEOF;
    }
    auto head = uint8_t(data.front());
    data = data.substr(1);
    switch (head) {
    case 0xc0: return JsonView{nullptr};
    case 0xc2: return JsonView{false};
    case 0xc3: return JsonView{true};
    case 0xcc: return unpackTrivial<flags, uint8_t>(data);
    case 0xcd: return unpackTrivial<flags, uint16_t>(data);
    case 0xce: return unpackTrivial<flags, uint32_t>(data);
    case 0xcf: return unpackTrivial<flags, uint64_t>(data);
    case 0xd0: return unpackTrivial<flags, int8_t>(data);
    case 0xd1: return unpackTrivial<flags, int16_t>(data);
    case 0xd2: return unpackTrivial<flags, int32_t>(data);
    case 0xd3: return unpackTrivial<flags, int64_t>(data);
    case 0xca: return unpackTrivial<flags, float>(data);
    case 0xcb: return unpackTrivial<flags, double>(data);
    case 0xd9: return unpackStr<flags, uint8_t>(data);
    case 0xda: return unpackStr<flags, uint16_t>(data);
    case 0xdb: return unpackStr<flags, uint32_t>(data);
    case 0xc4: return unpackBin<flags, uint8_t>(data);
    case 0xc5: return unpackBin<flags, uint16_t>(data);
    case 0xc6: return unpackBin<flags, uint32_t>(data);
    case 0xdc: return unpackArr<flags, uint16_t>(data, ctx, depthLimit - 1);
    case 0xdd: return unpackArr<flags, uint32_t>(data, ctx, depthLimit - 1);
    case 0xde: return unpackObj<flags, uint16_t>(data, ctx, depthLimit - 1);
    case 0xdf: return unpackObj<flags, uint32_t>(data, ctx, depthLimit - 1);
    case 0xd4: return unpackExt<flags, 1>(data);
    case 0xd5: return unpackExt<flags, 2>(data);
    case 0xd6: return unpackExt<flags, 4>(data);
    case 0xd7: return unpackExt<flags, 8>(data);
    case 0xd8: return unpackExt<flags, 16>(data);
    case 0xc7: return unpackBin<flags, uint8_t, 1>(data);
    case 0xc8: return unpackBin<flags, uint16_t, 1>(data);
    case 0xc9: return unpackBin<flags, uint32_t, 1>(data);
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10: case 11: case 12:
    case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23:
    case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31: case 32: case 33: case 34:
    case 35: case 36: case 37: case 38: case 39: case 40: case 41: case 42: case 43: case 44: case 45:
    case 46: case 47: case 48: case 49: case 50: case 51: case 52: case 53: case 54: case 55: case 56:
    case 57: case 58: case 59: case 60: case 61: case 62: case 63: case 64: case 65: case 66: case 67:
    case 68: case 69: case 70: case 71: case 72: case 73: case 74: case 75: case 76: case 77: case 78:
    case 79: case 80: case 81: case 82: case 83: case 84: case 85: case 86: case 87: case 88: case 89:
    case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97: case 98: case 99: case 100:
    case 101: case 102:  case 103: case 104: case 105: case 106: case 107: case 108: case 109: case 110:
    case 111: case 112: case 113: case 114: case 115: case 116: case 117: case 118: case 119: case 120:
    case 121: case 122: case 123: case 124: case 125: case 126:
    case 127: { //pos fixint
        return JsonView(head);
    }
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88:
    case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e:
    case 0x8f: { //fixmap
        return parseObject<flags>(head & 0b1111, data, ctx, depthLimit - 1);
    }
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e:
    case 0x9f: { //fixarr
        return parseArray<flags>(head & 0b1111, data, ctx, depthLimit - 1);
    }
    case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
    case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
    case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
    case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe:
    case 0xbf: { //fixstr
        size_t len = head & 0b11111;
        if ((len > data.size())) [[unlikely]] {
            return ErrEOF;
        }
        return JsonView(consume(data, len));
    }
    case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
    case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
    case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7: case 0xf8:
    case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe:
    case 0xff: { //neg fixint
        return JsonView{static_cast<int8_t>(head)};
    }
    [[unlikely]] default: {
        return JsonView::Discarded("unknown type");
    }
    }
}

} //<anon>

template<int flags, writer Writer>
[[gnu::flatten]]
auto Dump(JsonView j, Writer&& out, unsigned depthLimit) noexcept {
    if (depthLimit == 0) [[unlikely]] {
        return out(std::string_view{});
    }
    using namespace detail;
    switch (j.type())
    {
    case t_null: {
        return writeType<flags>(uint8_t(0xc0), out);
    }
    case t_bool: {
        return writeType<flags>(j.GetData().boolean ? uint8_t(0xc3) : uint8_t(0xc2), out);
    }
    case t_int: {
        if (j.GetData().integer < 0) {
            return writeNegInt<flags>(j.GetData().integer, out);
        } else {
            return writePosInt<flags>(j.GetData().integer, out);
        }
    }
    case t_uint: {
        return writePosInt<flags>(j.GetData().integer, out);
    }
    case t_num: {
        if (auto err = writeType<flags>(uint8_t(0xcb), out)) [[unlikely]] return err;
        return write<flags>(j.GetData().number, out);
    }
    case t_string: {
        return writeString<flags>(j.String(), out);
    }
    case t_binary: {
        return writeBin<flags>(j.String(), out);
    }
    case t_array: {
        auto sz = j.GetData().size;
        if (sz <= 0b1111) {
            if (auto err = writeType<flags>(uint8_t(0b10010000 | sz), out)) [[unlikely]] return err;
        } else if (sz <= std::numeric_limits<uint16_t>::max()) {
            if (auto err = writeType<flags>(0xdc, out)) [[unlikely]] return err;
            if (auto err = write<flags>(uint16_t(sz), out)) [[unlikely]] return err;
        } else {
            if (auto err = writeType<flags>(0xdd, out)) [[unlikely]] return err;
            if (auto err = write<flags>(uint32_t(sz), out)) [[unlikely]] return err;
        }
        for (auto v: j.Array()) {
            if (auto err = Dump<flags>(v, out, depthLimit - 1)) [[unlikely]] return err;
        }
        return out(std::string_view{});
    }
    case t_object: {
        auto sz = j.GetData().size;
        if (sz <= 0b1111)  {
            if (auto err = writeType<flags>(uint8_t(0b10000000 | sz), out)) [[unlikely]] return err;
        } else if (sz <= std::numeric_limits<uint16_t>::max()) {
            if (auto err = writeType<flags>(0xde, out)) [[unlikely]] return err;
            if (auto err = write<flags>(uint16_t(sz), out)) [[unlikely]] return err;
        } else {
            if (auto err = writeType<flags>(0xdf, out)) [[unlikely]] return err;
            if (auto err = write<flags>(uint32_t(sz), out)) [[unlikely]] return err;
        }
        for (auto [k, v]: j.Object()) {
            if (auto err = Dump<flags>(k, out, depthLimit - 1)) [[unlikely]] return err;
            if (auto err = Dump<flags>(v, out, depthLimit - 1)) [[unlikely]] return err;
        }
        return out(std::string_view{});
    }
    [[unlikely]] case t_discarded: {
        return out(std::string_view{});
    }
    [[unlikely]] default: {
        assert(false && "Invalid json type");
        std::abort();
    }
    }
}

template<int flags, alloc Alloc>
[[gnu::flatten]]
JsonView Parse(std::string_view buffer, Alloc&& alloc, unsigned depthLimit) noexcept {
    JsonView res = detail::parseOne<flags>(buffer, alloc, depthLimit);
    if (res.Valid() && buffer.size()) {
        return JsonView::Discarded("msgpack was not fully consumed");
    } else {
        return res;
    }
}

}

#undef _JV_CHECK
#endif //JV_MSGPACK_HPP
