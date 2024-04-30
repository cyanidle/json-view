#ifndef JSON_VIEW_HPP
#define JSON_VIEW_HPP
#pragma once

#include "stdint.h"
#include "stddef.h"
#include "assert.h"
#include <string_view>

namespace jv
{

struct JsonPair;
struct AsArray;
struct AsObject;

struct JsonView
{   
    enum Types {
        t_null,
        t_bool,
        t_num,
        t_int,
        t_uint,
        t_str,
        t_bin,
        t_arr,
        t_obj,
        t_discarded,
    };
    Types Type = t_null; 
    unsigned Size = 0;
    union {
        bool Boolean;
        intmax_t Integer;
        uintmax_t UInteger;
        double Number;
        const JsonView* Array;
        const JsonPair* Object;
        const char* String = nullptr;
    };
    AsObject Obj() const noexcept;
    AsArray Arr() const noexcept;
    std::string_view Str() const noexcept {
        return {String, Size};
    }
    static constexpr JsonView Discarded(std::string_view reason = {}) noexcept {
        return JsonView {
            .Type = t_discarded,
            .Size = unsigned(reason.size()),
            .String = reason.data()
        };
    }
};

struct JsonPair {
    JsonView key;
    JsonView value;
};

struct AsArray {
    AsArray(JsonView targ) noexcept : target(targ) {
        assert(targ.Type == JsonView::t_arr);
    }
    const JsonView* begin() const noexcept {
        return target.Array;
    }
    const JsonView* end() const noexcept {
        return target.Array + target.Size;
    }
    JsonView target;
};

struct AsObject {
    AsObject(JsonView targ) noexcept : target(targ) {
        assert(targ.Type == JsonView::t_obj);
    }
    const JsonPair* begin() const noexcept {
        return target.Object;
    }
    const JsonPair* end() const noexcept {
        return target.Object + target.Size;
    }
    JsonView target;
};

AsArray JsonView::Arr() const noexcept {
    return {*this};
}

AsObject JsonView::Obj() const noexcept {
    return {*this};
}

}


#endif //JSON_VIEW_HPP