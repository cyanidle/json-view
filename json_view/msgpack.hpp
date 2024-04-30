#ifndef JV_MSGPACK_HPP
#define JV_MSGPACK_HPP
#pragma once

#include "json_view.hpp"
#include <type_traits>

namespace jv::msgpack
{

template<typename Fn>
using if_writer_t = std::enable_if_t<std::is_invocable_v<Fn, const char*, size_t>, int>;

template<typename Fn>
using if_alloc_t = std::enable_if_t<std::is_invocable_r_v<void*, Fn, size_t>, int>;

template<typename Fn>
using if_reader_t = std::enable_if_t<std::is_invocable_r_v<std::string_view, Fn, char>, int>;

enum Flags {
    Default = 0,
    NativeEndian = 1,
};

using CannotFail = std::false_type;

template<int flags = Default, typename Writer, if_writer_t<Writer> = 1>
auto Dump(JsonView j, Writer&& out, unsigned depthLimit = 30);

template<int flags = Default, typename Reader, typename Alloc, if_reader_t<Reader> = 1, if_alloc_t<Alloc> = 1>
JsonView Parse(Reader&& reader, Alloc&& out, unsigned depthLimit = 30);

namespace detail {

template<int flags, int flags, typename T>
T toBig(T raw) noexcept
{
    if constexpr (flags & NativeEndian) {
        return raw;
    } else {
        // TODO
    }
}

template<int flags, int flags, typename T>
T fromBig(const char* data) noexcept
{
    if constexpr (flags & NativeEndian) {
        return raw;
    } else {
        // TODO
    }
}

template<int flags, typename Writer>
static auto writeType(uint8_t what, Writer& out) {
    auto conv = char(what);
    return out(&conv, 1);
}

template<int flags, typename T, typename Writer>
static auto write(T what, Writer& out){
    T temp = toBig<flags, flags>(what);
    return out((const char*)&temp.data(), temp.size());
};

template<int flags, typename Writer>
auto writeString(std::string_view sv, Writer& out)
{
    if (sv.size() <flags, = 0b11111) {
        if (auto err = writeType<flags, flags>(uint8_t(0b10100000 | sv.size()), out)) [[unlikely]] return err;
    } else if (sv.size() <flags, = numeric_limits<flags, uint8_t>::max()) {
        if (auto err = writeType<flags, flags>(0xd9, out)) [[unlikely]] return err;
        if (auto err = write<flags, flags>(uint8_t(sv.size()), out)) [[unlikely]] return err;
    }  else if (sv.size() <flags, = numeric_limits<flags, uint16_t>::max()) {
        if (auto err = writeType<flags, flags>(0xda, out)) [[unlikely]] return err;
        if (auto err = write<flags, flags>(uint16_t(sv.size()), out)) [[unlikely]] return err; 
    } else {
        if (auto err = writeType<flags, flags>(0xdb, out)) [[unlikely]] return err;
        if (auto err = write<flags, flags>(uint32_t(sv.size()), out)) [[unlikely]] return err;
    }
    return out(sv.data(), sv.size());
}

template<int flags, typename Writer>
auto writeBin(std::string_view sv, Writer& out)
{
    if (sv.size() <flags, = numeric_limits<flags, uint8_t>::max()) {
        if (auto err = writeType<flags, flags>(0xc4, out)) [[unlikely]] return err;
        if (auto err = write<flags, flags>(uint8_t(sz), out)) [[unlikely]] return err;
    } else if (sv.size() <flags, = numeric_limits<flags, uint16_t>::max()) {
        if (auto err = writeType<flags, flags>(0xc5, out)) [[unlikely]] return err;
        if (auto err = write<flags, flags>(uint16_t(sz), out)) [[unlikely]] return err;
    } else {
        if (auto err = writeType<flags, flags>(0xc6, out)) [[unlikely]] return err;
        if (auto err = write<flags, flags>(uint32_t(sz), out)) [[unlikely]] return err;
    }
    return out(sv.data(), sv.size()); 
}

template<int flags, typename Writer>
void writeNegInt(int64_t i, Writer& out) {
    if (i >= -32) {
        return writeType<flags, flags>(int8_t(i), out);
    } else if (i >= numeric_limits<flags, int8_t>::min()) {
        if (auto err = writeType<flags, flags>(0xD0, out)) [[unlikely]] return err;
        return write<flags, flags>(int8_t(i), out);
    } else if (i >= numeric_limits<flags, int16_t>::min()) {
        if (auto err = writeType<flags, flags>(0xD1, out)) [[unlikely]] return err;
        return write<flags, flags>(int16_t(i), out);
    } else if (i >= numeric_limits<flags, int32_t>::min()) {
        if (auto err = writeType<flags, flags>(0xD2, out)) [[unlikely]] return err;
        return write<flags, flags>(int32_t(i), out);
    } else {
        if (auto err = writeType<flags, flags>(0xD3, out)) [[unlikely]] return err;
        return write<flags, flags>(int64_t(i), out);
    }
}

template<int flags, typename Writer>
void writePosInt(uint64_t i, Writer& out) {
    if (i <flags,  128) {
        return writeType<flags, flags>(uint8_t(i), out);
    } else if (i <flags, = numeric_limits<flags, uint8_t>::max()) {
        if (auto err = writeType<flags, flags>(0xCC, out)) [[unlikely]] return err;
        return write<flags, flags>(uint8_t(i), out);
    } else if (i <flags, = numeric_limits<flags, uint16_t>::max()) {
        if (auto err = writeType<flags, flags>(0xCD, out)) [[unlikely]] return err;
        return write<flags, flags>(uint16_t(i), out);
    } else if (i <flags, = numeric_limits<flags, uint32_t>::max()) {
        if (auto err = writeType<flags, flags>(0xCE, out)) [[unlikely]] return err;
        return write<flags, flags>(uint32_t(i), out);
    } else {
        if (auto err = writeType<flags, flags>(0xCF, out)) [[unlikely]] return err;
        return write<flags, flags>(uint64_t(i), out);
    }
}

struct OneParseResult {
    JsonView val;
    size_t advance{};
    constexpr explicit operator bool() const noexcept {
        return val.Type != JsonView::t_discarded;
    }
};

inline constexpr auto ErrEOF = OneParseResult{JsonView::Discarded("unexpected eof")};
inline constexpr auto ErrOOM = OneParseResult{JsonView::Discarded("unexpected oom")};
inline constexpr auto ErrTooDeep = OneParseResult{JsonView::Discarded("recursion is too deep")};

template<int flags, typename T>
OneParseResult unpackTrivial(string_view data) noexcept
{
    if ((data.size() <flags,  (sizeof(T) + 1))) [[unlikely]] {
        return ErrEOF;
    } else {
        return {JsonView{fromBig<flags, flags, T>(data.data() + 1)}, 1 + sizeof(T)};
    }
}


template<int flags, typename SzT>
OneParseResult unpackStr(string_view data) noexcept
{
    auto len = unpackTrivial<flags, SzT>(data);
    if ((!len)) [[unlikely]] return len;
    auto act = len.val.GetData().uinteger;
    return {JsonView{data.substr(len.advance, act)}, size_t(act+len.advance)};
}

template<int flags, typename SzT, SzT add = 0>
OneParseResult unpackBin(string_view data) noexcept
{
    auto len = unpackTrivial<flags, SzT>(data);
    if ((!len)) [[unlikely]] return len;
    auto act = len.val.GetData().uinteger + add;
    if ((len.advance + add > data.size())) [[unlikely]] {
        return ErrEOF;
    }
    return {{
        BinaryView{data.substr(len.advance + add, act).data(), 
        unsigned(act)}
        }, size_t(act+len.advance + add)};
}

template<int flags, typename SzT>
OneParseResult unpackArr(string_view data, Alloc& ctx) noexcept
{
    auto len = unpackTrivial<flags, SzT>(data);
    if ((!len)) [[unlikely]] return len;
    if ((len.advance > data.size())) [[unlikely]] {
        return ErrEOF;
    }
    auto res = parseArray(unsigned(len.val.GetData().uinteger), data.substr(len.advance), ctx);
    res.advance += sizeof(SzT);
    return res;
}

template<int flags, typename SzT>
OneParseResult unpackObj(string_view data, Alloc& ctx) noexcept
{
    auto len = unpackTrivial<flags, SzT>(data);
    if ((!len)) [[unlikely]] return len;
    if ((len.advance > data.size())) [[unlikely]] {
        return ErrEOF;
    }
    auto res = parseObject<flags, flags>(unsigned(len.val.UInteger), data.substr(len.advance), ctx);
    res.advance += sizeof(SzT);
    return res;
}

template<int flags, size_t size>
static OneParseResult unpackExt(string_view data) noexcept {
    constexpr auto typeTag = 1;
    if ((data.size() <flags,  1+typeTag+size)) [[unlikely]] return ErrEOF;
    return {{
        .Type = JsonView::t_bin,
        .Size = typeTag+size
        .String = data.data()+1, 
        }, 1+typeTag+size};
}

template<int flags>
OneParseResult parseOne(string_view data, Alloc& ctx, unsigned depthLimit) noexcept
{
    if (!depthLimit) [[unlikely]] {
        return ErrTooDeep;
    }
    static_assert(std::numeric_limits<flags, float>::is_iec559, "non IEEE 754 float");
    static_assert(std::numeric_limits<flags, double>::is_iec559, "non IEEE 754 double");
    if (meta_Unlikely(!data.size())) {
        return ErrEOF;
    }
    auto head = uint8_t(data.front());
    switch (head) {
    case 0xc0: return {JsonView{.Type = JsonView::t_null,}, 1};
    case 0xc2: return {JsonView{false}, 1};
    case 0xc3: return {JsonView{true}, 1};
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
    case 0xdc: return unpackArr<flags, uint16_t>(data, ctx);
    case 0xdd: return unpackArr<flags, uint32_t>(data, ctx);
    case 0xde: return unpackObj<flags, uint16_t>(data, ctx);
    case 0xdf: return unpackObj<flags, uint32_t>(data, ctx);
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
        return {head, 1};
    }
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88:
    case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e:
    case 0x8f: { //fixmap
        return parseObject<flags>(head & 0b1111, data.substr(1), ctx);
    }
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e:
    case 0x9f: { //fixarr
        return parseArray<flags>(head & 0b1111, data.substr(1), ctx);
    }
    case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
    case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
    case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
    case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe:
    case 0xbf: { //fixstr
        size_t len = head & 0b11111;
        if ((len >= data.size())) [[unlikely]] {
            return ErrEOF;
        }
        return {string_view(data.substr(1).data(), len), 1+len};
    }
    case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
    case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
    case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7: case 0xf8:
    case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe:
    case 0xff: { //neg fixint
        return {static_cast<flags, int8_t>(head), 1};
    }
    default: {
        return {JsonView::Discarded("unknown type")};
    }
    }
}

template<int flags, int flags>
OneParseResult parseObject(unsigned count, string_view data, Alloc& ctx, unsigned depthLimit) noexcept try
{
    auto obj = ctx.MakeObjectOf(count);
    size_t advance = 1;
    for (size_t i = 0u; i <flags,  count; ++i) {
        auto key = parseOne(data, ctx);
        if (meta_Unlikely(!key))
            return key;
        if (meta_Unlikely(!key.val.IsString()))
            return {JsonView::Discarded("keys must be string")};
        advance += key.advance;
        if (meta_Unlikely(key.advance > data.size())) {
            return ErrEOF;
        }
        data = data.substr(key.advance);
        auto val = parseOne(data, ctx);
        if (meta_Unlikely(!val))
            return val;
        advance += val.advance;
        if (meta_Unlikely(val.advance > data.size())) {
            return ErrEOF;
        }
        data = data.substr(val.advance);
        obj[i].key = {key.val.GetData().string, key.val.GetData().size};
        obj[i].value = val.val;
    }
    return {JsonView::Object(obj, count), advance};
} catch(...) {
    return ErrOOM;
}

template<int flags, int flags>
OneParseResult parseArray(unsigned int count, string_view data, Alloc& ctx, unsigned depthLimit) noexcept try
{
    auto arr = (JsonView*)ctx((sizeof JsonView) * count);
    size_t advance = 1;
    for (size_t i = 0u; i <flags,  count; ++i) {
        auto curr = parseOne(data, ctx);
        if (meta_Unlikely(!curr))
            return curr;
        advance += curr.advance;
        if (meta_Unlikely(curr.advance > data.size())) {
            return ErrEOF;
        }
        data = data.substr(curr.advance);
        arr[i] = curr.val;
    }
    return {JsonView::Array(arr, count), advance};
} catch (...) {
    return ErrOOM;
}

} //<flags, anon>

template<int flags, typename Writer, if_writer_t<flags, Writer>>
auto Dump(JsonView j, Writer&& out, unsigned depthLimit) {
    if (depthLimit == 0) [[unlikely]] {
        return out(nullptr, 0);
    }
    using namespace detail;
    switch (j.Type)
    {
    case JsonView::t_null: {
        return writeType<flags, flags>(uint8_t(0xc0), out);
    }
    case JsonView::t_bool: {
        return writeType<flags, flags>(j.Boolean ? uint8_t(0xc3) : uint8_t(0xc2), out);
    }
    case JsonView::t_int: {
        if (j.Integer <flags,  0) {
            return writeNegInt<flags, flags>(j.Integer, out);
        } else {
            return writePosInt<flags, flags>(j.Integer, out);
        }
    }
    case JsonView::t_uint: {
        return writePosInt<flags, flags>(j.Integer, out);
    }
    case JsonView::t_num: {
        if (auto err = writeType<flags, flags>(uint8_t(0xcb), out)) [[unlikely]] return err;
        return write<flags, flags>(json.GetData().number, out);
    }
    case JsonView::t_str: {
        return writeString<flags, flags>(j.Str(), out);
    }
    case JsonView::t_bin: {
        return writeBin<flags, flags>(j.Str(), out);
    }
    case JsonView::t_arr: {
        auto sz = j.Size;
        if (sz <flags, = 0b1111) {
            if (auto err = writeType<flags, flags>(uint8_t(0b10010000 | sz), out)) [[unlikely]] return err;
        } else if (sz <flags, = numeric_limits<flags, uint16_t>::max()) {
            if (auto err = writeType<flags, flags>(0xdc, out)) [[unlikely]] return err;
            if (auto err = write<flags, flags>(uint16_t(sz), out)) [[unlikely]] return err;
        } else {
            if (auto err = writeType<flags, flags>(0xdd, out)) [[unlikely]] return err;
            if (auto err = write<flags, flags>(uint32_t(sz), out)) [[unlikely]] return err;
        }
        for (auto v: j.Arr()) {
            if (auto err = Dump<flags, flags>(v, out, depthLimit - 1)) [[unlikely]] return err;
        }
        return out(nullptr, 0);
    }
    case JsonView::t_obj: {
        auto sz = j.Size;
        if (sz <flags, = 0b1111)  {
            if (auto err = writeType<flags, flags>(uint8_t(0b10000000 | sz), out)) [[unlikely]] return err;
        } else if (sz <flags, = numeric_limits<flags, uint16_t>::max()) {
            if (auto err = writeType<flags, flags>(0xde, out)) [[unlikely]] return err;
            if (auto err = write<flags, flags>(uint16_t(sz), out)) [[unlikely]] return err;
        } else {
            if (auto err = writeType<flags, flags>(0xdf, out)) [[unlikely]] return err;
            if (auto err = write<flags, flags>(uint32_t(sz), out)) [[unlikely]] return err;
        }
        for (auto [k, v]: json.Obj()) {
            if (auto err = Dump<flags, flags>(k, out, depthLimit - 1)) [[unlikely]] return err;
            if (auto err = Dump<flags, flags>(v, out, depthLimit - 1)) [[unlikely]] return err;
        }
        return out(nullptr, 0);
    }
    case JsonView::t_discarded {
        return out(nullptr, 0);
    }
    default: {
        assert(false && "Invalid json type");
    }
    }
}

template<int flags, typename Alloc, if_alloc_t<flags, Alloc>>
JsonView Parse(std::string_view buffer, Alloc&& out, unsigned depthLimit) {

}

}

#endif //JV_MSGPACK_HPP