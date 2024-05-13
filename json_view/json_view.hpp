#ifndef JSON_VIEW_HPP
#define JSON_VIEW_HPP
#pragma once

#include "stdint.h"
#include "stddef.h"
#include "assert.h"
#include <string_view>
#include <concepts>
#include <forward_list>

namespace mjv
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
    constexpr bool Valid() const noexcept {
        return data.type != t_discarded;
    }
    constexpr JsonView(Data d) noexcept : data(d) {}
    constexpr JsonView(std::nullptr_t = {}) noexcept : data {
            .type = t_null,
            .size = 0,
            .integer = 0
        } {}
    constexpr JsonView(bool b) noexcept : data {
            .type = t_bool,
            .size = 0,
            .boolean = b
        } {}
    constexpr JsonView(std::signed_integral auto v) noexcept : data {
            .type = t_int,
            .size = 0,
            .integer = v
        } {}
    constexpr JsonView(std::unsigned_integral auto v) noexcept : data {
            .type = t_uint,
            .size = 0,
            .uinteger = v
        } {}
    constexpr JsonView(std::floating_point auto v) noexcept : data {
            .type = t_num,
            .size = 0,
            .number = v,
        } {}
    constexpr JsonView(std::string_view sv) noexcept : data {
            .type = t_string,
            .size = unsigned(sv.size()),
            .string = sv.data()
        } {}
    constexpr JsonView(const char* s) noexcept : JsonView(std::string_view{s}) {}
    constexpr JsonView(const JsonPair* obj, unsigned count) noexcept : data {
            .type = t_object,
            .size = count,
            .object = obj
        } {}
    constexpr JsonView(const JsonView* arr, unsigned count) noexcept : data{
            .type = t_array,
            .size = count,
            .array = arr
        } {}
    template<size_t N>
    constexpr JsonView(const JsonPair(&obj)[N]) noexcept : JsonView(obj, N) {}
    template<size_t N>
    constexpr JsonView(const JsonView(&arr)[N]) noexcept : JsonView(arr, N) {}
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
    constexpr JsonView operator[](unsigned idx) const noexcept;
    constexpr JsonView operator[](std::string_view key) const noexcept;
    constexpr Data const& GetData() const noexcept {return data;}
    constexpr AsObject Object() const noexcept;
    constexpr AsArray Array() const noexcept;
    constexpr Types type() const noexcept {return data.type;}
    constexpr std::string_view String() const noexcept {
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
    constexpr AsArray(JsonView targ) noexcept : target(targ) {
        assert(targ.type() == t_array);
    }
    constexpr const JsonView* begin() const noexcept {
        return target.GetData().array;
    }
    constexpr const JsonView* end() const noexcept {
        return target.GetData().array + target.GetData().size;
    }
protected:
    JsonView target;
};

struct AsObject {
    constexpr AsObject(JsonView targ) noexcept : target(targ) {
        assert(targ.type() == t_object);
    }
    constexpr const JsonPair* begin() const noexcept {
        return target.GetData().object;
    }
    constexpr const JsonPair* end() const noexcept {
        return target.GetData().object + target.GetData().size;
    }
protected:
    JsonView target;
};

constexpr AsArray JsonView::Array() const noexcept {
    return {*this};
}

inline constexpr JsonView JsonView::operator[](unsigned int idx) const noexcept {
    assert(data.type == t_array);
    if (idx >= data.size) {
        return JsonView::Discarded("no such index");
    }
    return data.array[idx];
}

inline constexpr JsonView JsonView::operator[](std::string_view key) const noexcept {
    assert(data.type == t_object);
    for (auto& [k, v]: Object()) {
        if (k.data.type == t_string) [[likely]] {
            if (k.String() == key) {
                return v;
            }
        }
    }
    return JsonView::Discarded("no such key");
}

constexpr AsObject JsonView::Object() const noexcept {
    return {*this};
}

template<typename Alloc = std::allocator<char>>
struct Context {
    Context(Alloc&& _a = {}) :
        a(std::move(_a)),
        allocs(a)
    {}
    [[nodiscard, gnu::always_inline]] void* operator()(size_t sz) {
        return allocs.emplace_front(a.allocate(sz), sz).first;
    }
    ~Context() {
        for (auto p: allocs) {
            a.deallocate(p.first, p.second);
        }
    }
protected:
    Alloc a;
    using entry = std::pair<char*, size_t>;

    using list_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<entry>;
    std::forward_list<entry, list_alloc> allocs;
};

} //mjv


#endif //JSON_VIEW_HPP
