#ifndef JSON_VIEW_HPP
#define JSON_VIEW_HPP
#pragma once

#include "stdint.h"
#include "stddef.h"
#include "assert.h"
#include <string_view>
#include <concepts>

namespace jv
{

struct JsonPair;
struct AsArray;
struct AsObject;

enum Types : int {
    t_null,
    t_bool,
    t_num,
    t_int,
    t_uint,
    t_string,
    t_binary,
    t_array,
    t_object,
    t_discarded,
};

struct JsonView
{
    struct Data {
        Types type = t_null;
        unsigned size = 0;
        union {
            bool boolean;
            double number;
            const JsonView* array;
            const JsonPair* object;
            const char* string;
            uintmax_t uinteger;
            intmax_t integer = 0;
        };
    };
    constexpr JsonView(Data d) noexcept : data(d) {}
    explicit constexpr JsonView(std::nullptr_t) noexcept : data {
            .type = t_null,
        } {}
    explicit constexpr JsonView(bool b) noexcept : data {
            .type = t_bool,
            .size = 0,
            .boolean = b
        } {}
    explicit constexpr JsonView(std::signed_integral auto v) noexcept : data {
            .type = t_bool,
            .size = 0,
            .integer = v
        } {}
    explicit constexpr JsonView(std::unsigned_integral auto v) noexcept : data {
            .type = t_bool,
            .size = 0,
            .uinteger = v
        } {}
    explicit constexpr JsonView(std::floating_point auto v) noexcept : data {
            .type = t_bool,
            .size = 0,
            .number = v,
        } {}
    JsonView(std::string_view sv) noexcept : data {
            .type = t_string,
            .size = unsigned(sv.size()),
            .string = sv.data()
        } {}
    explicit constexpr JsonView(const JsonPair* obj, unsigned count) noexcept : data {
            .type = t_array,
            .size = count,
            .object = obj
        } {}
    explicit constexpr JsonView(const JsonView* arr, unsigned count) noexcept : data{
            .type = t_array,
            .size = count,
            .array = arr
        } {}
    template<size_t N>
    explicit constexpr JsonView(const JsonPair(&obj)[N]) noexcept : JsonView(obj, N) {}
    template<size_t N>
    explicit constexpr JsonView(const JsonView(&arr)[N]) noexcept : JsonView(arr, N) {}
    static constexpr JsonView Discarded(std::string_view reason = {}) noexcept {
        return Data {
            .type = t_discarded,
            .size = unsigned(reason.size()),
            .string = reason.data()
        };
    }
    static constexpr JsonView Binary(std::string_view data) noexcept {
        return Data {
            .type = t_binary,
            .size = unsigned(data.size()),
            .string = data.data()
        };
    }
    Data const& GetData() const noexcept {return data;}
    AsObject Items() const noexcept;
    AsArray Values() const noexcept;
    Types type() const noexcept {return data.type;}
    std::string_view Str() const noexcept {
        assert(data.type == t_string);
        return {data.string, data.size};
    }
protected:
    Data data;
};

struct JsonPair {
    JsonView key;
    JsonView value;
};

struct AsArray {
    AsArray(JsonView targ) noexcept : target(targ) {
        assert(targ.type() == t_array);
    }
    const JsonView* begin() const noexcept {
        return target.GetData().array;
    }
    const JsonView* end() const noexcept {
        return target.GetData().array + target.GetData().size;
    }
protected:
    JsonView target;
};

struct AsObject {
    AsObject(JsonView targ) noexcept : target(targ) {
        assert(targ.type() == t_object);
    }
    const JsonPair* begin() const noexcept {
        return target.GetData().object;
    }
    const JsonPair* end() const noexcept {
        return target.GetData().object + target.GetData().size;
    }
protected:
    JsonView target;
};

AsArray JsonView::Values() const noexcept {
    return {*this};
}

AsObject JsonView::Items() const noexcept {
    return {*this};
}

}


#endif //JSON_VIEW_HPP
