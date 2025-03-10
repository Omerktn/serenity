/*
 * Copyright (c) 2020-2021, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2020-2021, Linus Groh <linusg@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Checked.h>
#include <AK/Function.h>
#include <AK/StringBuilder.h>
#include <LibJS/Heap/Heap.h>
#include <LibJS/Runtime/AbstractOperations.h>
#include <LibJS/Runtime/Array.h>
#include <LibJS/Runtime/Error.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/PrimitiveString.h>
#include <LibJS/Runtime/RegExpObject.h>
#include <LibJS/Runtime/StringIterator.h>
#include <LibJS/Runtime/StringObject.h>
#include <LibJS/Runtime/StringPrototype.h>
#include <LibJS/Runtime/Value.h>
#include <string.h>

namespace JS {

static Optional<String> ak_string_from(VM& vm, GlobalObject& global_object)
{
    auto this_value = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    return this_value.to_string(global_object);
}

static Optional<size_t> split_match(const String& haystack, size_t start, const String& needle)
{
    auto r = needle.length();
    auto s = haystack.length();
    if (start + r > s)
        return {};
    if (!haystack.substring_view(start).starts_with(needle))
        return {};
    return start + r;
}

StringPrototype::StringPrototype(GlobalObject& global_object)
    : StringObject(*js_string(global_object.heap(), String::empty()), *global_object.object_prototype())
{
}

void StringPrototype::initialize(GlobalObject& global_object)
{
    auto& vm = this->vm();
    StringObject::initialize(global_object);
    u8 attr = Attribute::Writable | Attribute::Configurable;

    define_native_function(vm.names.charAt, char_at, 1, attr);
    define_native_function(vm.names.charCodeAt, char_code_at, 1, attr);
    define_native_function(vm.names.codePointAt, code_point_at, 1, attr);
    define_native_function(vm.names.repeat, repeat, 1, attr);
    define_native_function(vm.names.startsWith, starts_with, 1, attr);
    define_native_function(vm.names.endsWith, ends_with, 1, attr);
    define_native_function(vm.names.indexOf, index_of, 1, attr);
    define_native_function(vm.names.toLowerCase, to_lowercase, 0, attr);
    define_native_function(vm.names.toUpperCase, to_uppercase, 0, attr);
    define_native_function(vm.names.toString, to_string, 0, attr);
    define_native_function(vm.names.valueOf, value_of, 0, attr);
    define_native_function(vm.names.padStart, pad_start, 1, attr);
    define_native_function(vm.names.padEnd, pad_end, 1, attr);
    define_native_function(vm.names.trim, trim, 0, attr);
    define_native_function(vm.names.trimStart, trim_start, 0, attr);
    define_property(vm.names.trimLeft, get_without_side_effects(vm.names.trimStart), attr);
    define_native_function(vm.names.trimEnd, trim_end, 0, attr);
    define_property(vm.names.trimRight, get_without_side_effects(vm.names.trimEnd), attr);
    define_native_function(vm.names.concat, concat, 1, attr);
    define_native_function(vm.names.substr, substr, 2, attr);
    define_native_function(vm.names.substring, substring, 2, attr);
    define_native_function(vm.names.includes, includes, 1, attr);
    define_native_function(vm.names.slice, slice, 2, attr);
    define_native_function(vm.names.split, split, 2, attr);
    define_native_function(vm.names.lastIndexOf, last_index_of, 1, attr);
    define_native_function(vm.names.at, at, 1, attr);
    define_native_function(vm.names.match, match, 1, attr);
    define_native_function(vm.names.matchAll, match_all, 1, attr);
    define_native_function(vm.names.replace, replace, 2, attr);
    define_native_function(vm.names.search, search, 1, attr);
    define_native_function(vm.names.anchor, anchor, 1, attr);
    define_native_function(vm.names.big, big, 0, attr);
    define_native_function(vm.names.blink, blink, 0, attr);
    define_native_function(vm.names.bold, bold, 0, attr);
    define_native_function(vm.names.fixed, fixed, 0, attr);
    define_native_function(vm.names.fontcolor, fontcolor, 1, attr);
    define_native_function(vm.names.fontsize, fontsize, 1, attr);
    define_native_function(vm.names.italics, italics, 0, attr);
    define_native_function(vm.names.link, link, 1, attr);
    define_native_function(vm.names.small, small, 0, attr);
    define_native_function(vm.names.strike, strike, 0, attr);
    define_native_function(vm.names.sub, sub, 0, attr);
    define_native_function(vm.names.sup, sup, 0, attr);
    define_native_function(*vm.well_known_symbol_iterator(), symbol_iterator, 0, attr);
}

StringPrototype::~StringPrototype()
{
}

// thisStringValue ( value ), https://tc39.es/ecma262/#thisstringvalue
static Value this_string_value(GlobalObject& global_object, Value value)
{
    if (value.is_string())
        return value;
    if (value.is_object() && is<StringObject>(value.as_object()))
        return static_cast<StringObject&>(value.as_object()).value_of();
    auto& vm = global_object.vm();
    vm.throw_exception<TypeError>(global_object, ErrorType::NotA, "String");
    return {};
}

// 22.1.3.1 String.prototype.charAt ( pos ), https://tc39.es/ecma262/#sec-string.prototype.charat
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::char_at)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    auto position = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    if (position < 0 || position >= string->length())
        return js_string(vm, String::empty());
    return js_string(vm, string->substring(position, 1));
}

// 22.1.3.2 String.prototype.charCodeAt ( pos ), https://tc39.es/ecma262/#sec-string.prototype.charcodeat
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::char_code_at)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    auto position = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    if (position < 0 || position >= string->length())
        return js_nan();
    return Value((*string)[position]);
}

// 22.1.3.3 String.prototype.codePointAt ( pos ), https://tc39.es/ecma262/#sec-string.prototype.codepointat
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::code_point_at)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    auto position = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    auto view = Utf8View(*string);
    if (position < 0 || position >= view.length())
        return js_undefined();
    auto it = view.begin();
    for (auto i = 0; i < position; ++i)
        ++it;
    return Value(*it);
}

// 22.1.3.16 String.prototype.repeat ( count ), https://tc39.es/ecma262/#sec-string.prototype.repeat
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::repeat)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    if (!vm.argument_count())
        return js_string(vm, String::empty());
    auto count = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    if (count < 0) {
        vm.throw_exception<RangeError>(global_object, ErrorType::StringRepeatCountMustBe, "positive");
        return {};
    }
    if (Value(count).is_infinity()) {
        vm.throw_exception<RangeError>(global_object, ErrorType::StringRepeatCountMustBe, "finite");
        return {};
    }
    StringBuilder builder;
    for (size_t i = 0; i < count; ++i)
        builder.append(*string);
    return js_string(vm, builder.to_string());
}

// 22.1.3.22 String.prototype.startsWith ( searchString [ , position ] ), https://tc39.es/ecma262/#sec-string.prototype.startswith
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::starts_with)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto search_string_value = vm.argument(0);

    bool search_is_regexp = search_string_value.is_regexp(global_object);
    if (vm.exception())
        return {};
    if (search_is_regexp) {
        vm.throw_exception<TypeError>(global_object, ErrorType::IsNotA, "searchString", "string, but a regular expression");
        return {};
    }

    auto search_string = search_string_value.to_string(global_object);
    if (vm.exception())
        return {};

    auto string_length = string->length();
    auto search_string_length = search_string.length();
    size_t start = 0;
    if (!vm.argument(1).is_undefined()) {
        auto position = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        start = clamp(position, static_cast<double>(0), static_cast<double>(string_length));
    }
    if (start + search_string_length > string_length)
        return Value(false);
    if (search_string_length == 0)
        return Value(true);
    return Value(string->substring(start, search_string_length) == search_string);
}

// 22.1.3.6 String.prototype.endsWith ( searchString [ , endPosition ] ), https://tc39.es/ecma262/#sec-string.prototype.endswith
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::ends_with)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto search_string_value = vm.argument(0);

    bool search_is_regexp = search_string_value.is_regexp(global_object);
    if (vm.exception())
        return {};
    if (search_is_regexp) {
        vm.throw_exception<TypeError>(global_object, ErrorType::IsNotA, "searchString", "string, but a regular expression");
        return {};
    }

    auto search_string = search_string_value.to_string(global_object);
    if (vm.exception())
        return {};

    auto string_length = string->length();
    auto search_string_length = search_string.length();

    size_t pos = string_length;

    auto end_position_value = vm.argument(1);
    if (!end_position_value.is_undefined()) {
        double pos_as_double = end_position_value.to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        pos = clamp(pos_as_double, static_cast<double>(0), static_cast<double>(string_length));
    }

    if (search_string_length == 0)
        return Value(true);
    if (pos < search_string_length)
        return Value(false);

    auto start = pos - search_string_length;
    return Value(string->substring(start, search_string_length) == search_string);
}

// 22.1.3.8 String.prototype.indexOf ( searchString [ , position ] ), https://tc39.es/ecma262/#sec-string.prototype.indexof
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::index_of)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    auto needle = vm.argument(0).to_string(global_object);
    if (vm.exception())
        return {};
    return Value((i32)string->find(needle).value_or(-1));
}

// 22.1.3.26 String.prototype.toLowerCase ( ), https://tc39.es/ecma262/#sec-string.prototype.tolowercase
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::to_lowercase)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return js_string(vm, string->to_lowercase());
}

// 22.1.3.28 String.prototype.toUpperCase ( ), https://tc39.es/ecma262/#sec-string.prototype.touppercase
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::to_uppercase)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return js_string(vm, string->to_uppercase());
}

// 22.1.3.27 String.prototype.toString ( ), https://tc39.es/ecma262/#sec-string.prototype.tostring
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::to_string)
{
    return this_string_value(global_object, vm.this_value(global_object));
}

// 22.1.3.32 String.prototype.valueOf ( ), https://tc39.es/ecma262/#sec-string.prototype.valueof
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::value_of)
{
    return this_string_value(global_object, vm.this_value(global_object));
}

enum class PadPlacement {
    Start,
    End,
};

// 22.1.3.15.1 StringPad ( O, maxLength, fillString, placement ), https://tc39.es/ecma262/#sec-stringpad
static Value pad_string(GlobalObject& global_object, const String& string, PadPlacement placement)
{
    auto& vm = global_object.vm();
    auto max_length = vm.argument(0).to_length(global_object);
    if (vm.exception())
        return {};
    if (max_length <= string.length())
        return js_string(vm, string);

    String fill_string = " ";
    if (!vm.argument(1).is_undefined()) {
        fill_string = vm.argument(1).to_string(global_object);
        if (vm.exception())
            return {};
        if (fill_string.is_empty())
            return js_string(vm, string);
    }

    auto fill_length = max_length - string.length();

    StringBuilder filler_builder;
    while (filler_builder.length() < fill_length)
        filler_builder.append(fill_string);
    auto filler = filler_builder.build().substring(0, fill_length);

    auto formatted = placement == PadPlacement::Start
        ? String::formatted("{}{}", filler, string)
        : String::formatted("{}{}", string, filler);
    return js_string(vm, formatted);
}

// 22.1.3.15 String.prototype.padStart ( maxLength [ , fillString ] ), https://tc39.es/ecma262/#sec-string.prototype.padstart
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::pad_start)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return pad_string(global_object, *string, PadPlacement::Start);
}

// 22.1.3.14 String.prototype.padEnd ( maxLength [ , fillString ] ), https://tc39.es/ecma262/#sec-string.prototype.padend
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::pad_end)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return pad_string(global_object, *string, PadPlacement::End);
}

static const Utf8View whitespace_characters = Utf8View("\x09\x0A\x0B\x0C\x0D\x20\xC2\xA0\xE1\x9A\x80\xE2\x80\x80\xE2\x80\x81\xE2\x80\x82\xE2\x80\x83\xE2\x80\x84\xE2\x80\x85\xE2\x80\x86\xE2\x80\x87\xE2\x80\x88\xE2\x80\x89\xE2\x80\x8A\xE2\x80\xAF\xE2\x81\x9F\xE3\x80\x80\xE2\x80\xA8\xE2\x80\xA9\xEF\xBB\xBF");

// 22.1.3.29 String.prototype.trim ( ), https://tc39.es/ecma262/#sec-string.prototype.trim
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::trim)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return js_string(vm, Utf8View(*string).trim(whitespace_characters, TrimMode::Both).as_string());
}

// 22.1.3.31 String.prototype.trimStart ( ), https://tc39.es/ecma262/#sec-string.prototype.trimstart
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::trim_start)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return js_string(vm, Utf8View(*string).trim(whitespace_characters, TrimMode::Left).as_string());
}

// 22.1.3.30 String.prototype.trimEnd ( ), https://tc39.es/ecma262/#sec-string.prototype.trimend
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::trim_end)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    return js_string(vm, Utf8View(*string).trim(whitespace_characters, TrimMode::Right).as_string());
}

// 22.1.3.23 String.prototype.substring ( start, end ), https://tc39.es/ecma262/#sec-string.prototype.substring
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::concat)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    StringBuilder builder;
    builder.append(*string);
    for (size_t i = 0; i < vm.argument_count(); ++i) {
        auto string_argument = vm.argument(i).to_string(global_object);
        if (vm.exception())
            return {};
        builder.append(string_argument);
    }
    return js_string(vm, builder.to_string());
}

// 22.1.3.23 String.prototype.substring ( start, end ), https://tc39.es/ecma262/#sec-string.prototype.substring
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::substring)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    if (vm.argument_count() == 0)
        return js_string(vm, *string);

    // FIXME: index_start and index_end should index a UTF-16 code_point view of the string.
    auto string_length = string->length();
    auto start = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    auto end = (double)string_length;
    if (!vm.argument(1).is_undefined()) {
        end = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
    }
    size_t index_start = clamp(start, static_cast<double>(0), static_cast<double>(string_length));
    size_t index_end = clamp(end, static_cast<double>(0), static_cast<double>(string_length));

    if (index_start == index_end)
        return js_string(vm, String(""));

    if (index_start > index_end) {
        if (vm.argument_count() == 1)
            return js_string(vm, String(""));
        auto temp_index_start = index_start;
        index_start = index_end;
        index_end = temp_index_start;
    }

    auto part_length = index_end - index_start;
    auto string_part = string->substring(index_start, part_length);
    return js_string(vm, string_part);
}

// B.2.3.1 String.prototype.substr ( start, length ), https://tc39.es/ecma262/#sec-string.prototype.substr
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::substr)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    if (vm.argument_count() == 0)
        return js_string(vm, *string);

    // FIXME: this should index a UTF-16 code_point view of the string.
    auto size = (i32)string->length();

    auto int_start = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    if (Value(int_start).is_negative_infinity())
        int_start = 0;
    if (int_start < 0)
        int_start = max(size + (i32)int_start, 0);

    auto length = vm.argument(1);

    auto int_length = length.is_undefined() ? size : length.to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};

    if (Value(int_start).is_positive_infinity() || (int_length <= 0) || Value(int_length).is_positive_infinity())
        return js_string(vm, String(""));

    auto int_end = min((i32)(int_start + int_length), size);

    if (int_start >= int_end)
        return js_string(vm, String(""));

    auto string_part = string->substring(int_start, int_end - int_start);
    return js_string(vm, string_part);
}

// 22.1.3.7 String.prototype.includes ( searchString [ , position ] ), https://tc39.es/ecma262/#sec-string.prototype.includes
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::includes)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    auto search_string = vm.argument(0).to_string(global_object);
    if (vm.exception())
        return {};
    auto string_length = string->length();
    // FIXME: start should index a UTF-16 code_point view of the string.
    size_t start = 0;
    if (!vm.argument(1).is_undefined()) {
        auto position = vm.argument(1).to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        start = clamp(position, static_cast<double>(0), static_cast<double>(string_length));
    }
    if (start == 0)
        return Value(string->contains(search_string));
    auto substring_length = string_length - start;
    auto substring_search = string->substring(start, substring_length);
    return Value(substring_search.contains(search_string));
}

// 22.1.3.20 String.prototype.slice ( start, end ), https://tc39.es/ecma262/#sec-string.prototype.slice
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::slice)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    if (vm.argument_count() == 0)
        return js_string(vm, *string);

    // FIXME: index_start and index_end should index a UTF-16 code_point view of the string.
    auto string_length = static_cast<i32>(string->length());
    auto index_start = vm.argument(0).to_i32(global_object);
    if (vm.exception())
        return {};
    auto index_end = string_length;

    auto negative_min_index = -(string_length - 1);
    if (index_start < negative_min_index)
        index_start = 0;
    else if (index_start < 0)
        index_start = string_length + index_start;

    if (vm.argument_count() >= 2) {
        index_end = vm.argument(1).to_i32(global_object);
        if (vm.exception())
            return {};

        if (index_end < negative_min_index)
            return js_string(vm, String::empty());

        if (index_end > string_length)
            index_end = string_length;
        else if (index_end < 0)
            index_end = string_length + index_end;
    }

    if (index_start >= index_end)
        return js_string(vm, String::empty());

    auto part_length = index_end - index_start;
    auto string_part = string->substring(index_start, part_length);
    return js_string(vm, string_part);
}

// 22.1.3.21 String.prototype.split ( separator, limit ), https://tc39.es/ecma262/#sec-string.prototype.split
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::split)
{
    // FIXME Implement the @@split part

    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};

    auto* result = Array::create(global_object);
    size_t result_len = 0;

    auto limit = NumericLimits<u32>::max();
    if (!vm.argument(1).is_undefined()) {
        limit = vm.argument(1).to_u32(global_object);
        if (vm.exception())
            return {};
    }

    auto separator = vm.argument(0).to_string(global_object);
    if (vm.exception())
        return {};

    if (limit == 0)
        return result;

    if (vm.argument(0).is_undefined()) {
        result->define_property(0, js_string(vm, *string));
        return result;
    }

    auto len = string->length();
    auto separator_len = separator.length();
    if (len == 0) {
        if (separator_len > 0)
            result->define_property(0, js_string(vm, *string));
        return result;
    }

    size_t start = 0;
    auto pos = start;
    if (separator_len == 0) {
        for (pos = 0; pos < len; pos++)
            result->define_property(pos, js_string(vm, string->substring(pos, 1)));
        return result;
    }

    while (pos != len) {
        auto e = split_match(*string, pos, separator);
        if (!e.has_value()) {
            pos += 1;
            continue;
        }

        auto segment = string->substring_view(start, pos - start);
        result->define_property(result_len, js_string(vm, segment));
        result_len++;
        if (result_len == limit)
            return result;
        start = e.value();
        pos = start;
    }

    auto rest = string->substring(start, len - start);
    result->define_property(result_len, js_string(vm, rest));

    return result;
}

// 22.1.3.9 String.prototype.lastIndexOf ( searchString [ , position ] ), https://tc39.es/ecma262/#sec-string.prototype.lastindexof
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::last_index_of)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    auto search_string = vm.argument(0).to_string(global_object);
    if (vm.exception())
        return {};
    auto position = vm.argument(1).to_number(global_object);
    if (vm.exception())
        return {};
    if (search_string.length() > string->length())
        return Value(-1);
    auto max_index = string->length() - search_string.length();
    auto from_index = max_index;
    if (!position.is_nan()) {
        // FIXME: from_index should index a UTF-16 code_point view of the string.
        auto p = position.to_integer_or_infinity(global_object);
        if (vm.exception())
            return {};
        from_index = clamp(p, static_cast<double>(0), static_cast<double>(max_index));
    }

    for (i32 i = from_index; i >= 0; --i) {
        auto part_view = string->substring_view(i, search_string.length());
        if (part_view == search_string)
            return Value(i);
    }

    return Value(-1);
}

// 3.1 String.prototype.at ( index ), https://tc39.es/proposal-relative-indexing-method/#sec-string.prototype.at
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::at)
{
    auto string = ak_string_from(vm, global_object);
    if (!string.has_value())
        return {};
    auto length = string->length();
    auto relative_index = vm.argument(0).to_integer_or_infinity(global_object);
    if (vm.exception())
        return {};
    if (Value(relative_index).is_infinity())
        return js_undefined();
    Checked<size_t> index { 0 };
    if (relative_index >= 0) {
        index += relative_index;
    } else {
        index += length;
        index -= -relative_index;
    }
    if (index.has_overflow() || index.value() >= length)
        return js_undefined();
    return js_string(vm, String::formatted("{}", (*string)[index.value()]));
}

// 22.1.3.33 String.prototype [ @@iterator ] ( ), https://tc39.es/ecma262/#sec-string.prototype-@@iterator
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::symbol_iterator)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto string = this_object.to_string(global_object);
    if (vm.exception())
        return {};
    return StringIterator::create(global_object, string);
}

// 22.1.3.11 String.prototype.match ( regexp ), https://tc39.es/ecma262/#sec-string.prototype.match
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::match)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto regexp = vm.argument(0);
    if (!regexp.is_nullish()) {
        if (auto* matcher = regexp.get_method(global_object, *vm.well_known_symbol_match()))
            return vm.call(*matcher, regexp, this_object);
    }
    auto s = this_object.to_string(global_object);
    if (vm.exception())
        return {};
    auto rx = regexp_create(global_object, regexp, js_undefined());
    if (!rx)
        return {};
    return rx->invoke(vm.well_known_symbol_match(), js_string(vm, s));
}

// 22.1.3.12 String.prototype.matchAll ( regexp ), https://tc39.es/ecma262/#sec-string.prototype.matchall
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::match_all)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto regexp = vm.argument(0);
    if (!regexp.is_nullish()) {
        auto is_regexp = regexp.is_regexp(global_object);
        if (vm.exception())
            return {};
        if (is_regexp) {
            auto flags = regexp.as_object().get("flags").value_or(js_undefined());
            if (vm.exception())
                return {};
            auto flags_object = require_object_coercible(global_object, flags);
            if (vm.exception())
                return {};
            auto flags_string = flags_object.to_string(global_object);
            if (vm.exception())
                return {};
            if (!flags_string.contains("g")) {
                vm.throw_exception<TypeError>(global_object, ErrorType::StringMatchAllNonGlobalRegExp);
                return {};
            }
        }
        if (auto* matcher = regexp.get_method(global_object, *vm.well_known_symbol_match_all()))
            return vm.call(*matcher, regexp, this_object);
        if (vm.exception())
            return {};
    }
    auto s = this_object.to_string(global_object);
    if (vm.exception())
        return {};
    auto rx = regexp_create(global_object, regexp, js_string(vm, "g"));
    if (!rx)
        return {};
    return rx->invoke(vm.well_known_symbol_match_all(), js_string(vm, s));
}

// 22.1.3.17 String.prototype.replace ( searchValue, replaceValue ), https://tc39.es/ecma262/#sec-string.prototype.replace
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::replace)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto search_value = vm.argument(0);
    auto replace_value = vm.argument(1);

    if (!search_value.is_nullish()) {
        if (auto* replacer = search_value.get_method(global_object, *vm.well_known_symbol_replace()))
            return vm.call(*replacer, search_value, this_object, replace_value);
    }

    auto string = this_object.to_string(global_object);
    if (vm.exception())
        return {};
    auto search_string = search_value.to_string(global_object);
    if (vm.exception())
        return {};
    Optional<size_t> position = string.find(search_string);
    if (!position.has_value())
        return js_string(vm, string);

    auto preserved = string.substring(0, position.value());
    String replacement;

    if (replace_value.is_function()) {
        auto result = vm.call(replace_value.as_function(), js_undefined(), search_value, Value(position.value()), js_string(vm, string));
        if (vm.exception())
            return {};

        replacement = result.to_string(global_object);
        if (vm.exception())
            return {};
    } else {
        // FIXME: Implement the GetSubstituion algorithm for substituting placeholder '$' characters - https://tc39.es/ecma262/#sec-getsubstitution
        replacement = replace_value.to_string(global_object);
        if (vm.exception())
            return {};
    }

    StringBuilder builder;
    builder.append(preserved);
    builder.append(replacement);
    builder.append(string.substring(position.value() + search_string.length()));

    return js_string(vm, builder.build());
}

// 22.1.3.19 String.prototype.search ( regexp ), https://tc39.es/ecma262/#sec-string.prototype.search
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::search)
{
    auto this_object = require_object_coercible(global_object, vm.this_value(global_object));
    if (vm.exception())
        return {};
    auto regexp = vm.argument(0);
    if (!regexp.is_nullish()) {
        if (auto* searcher = regexp.get_method(global_object, *vm.well_known_symbol_search()))
            return vm.call(*searcher, regexp, this_object);
        if (vm.exception())
            return {};
    }
    auto s = this_object.to_string(global_object);
    if (vm.exception())
        return {};
    auto rx = regexp_create(global_object, regexp, js_undefined());
    if (!rx)
        return {};
    return rx->invoke(vm.well_known_symbol_search(), js_string(vm, s));
}

// B.2.3.2.1 CreateHTML ( string, tag, attribute, value ), https://tc39.es/ecma262/#sec-createhtml
static Value create_html(GlobalObject& global_object, Value string, const String& tag, const String& attribute, Value value)
{
    auto& vm = global_object.vm();
    require_object_coercible(global_object, string);
    if (vm.exception())
        return {};
    auto str = string.to_string(global_object);
    if (vm.exception())
        return {};
    StringBuilder builder;
    builder.append('<');
    builder.append(tag);
    if (!attribute.is_empty()) {
        auto value_string = value.to_string(global_object);
        if (vm.exception())
            return {};
        value_string.replace("\"", "&quot;", true);
        builder.append(' ');
        builder.append(attribute);
        builder.append("=\"");
        builder.append(value_string);
        builder.append('"');
    }
    builder.append('>');
    builder.append(str);
    builder.append("</");
    builder.append(tag);
    builder.append('>');
    return js_string(vm, builder.build());
}

// B.2.3.2 String.prototype.anchor ( name ), https://tc39.es/ecma262/#sec-string.prototype.anchor
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::anchor)
{
    return create_html(global_object, vm.this_value(global_object), "a", "name", vm.argument(0));
}

// B.2.3.3 String.prototype.big ( ), https://tc39.es/ecma262/#sec-string.prototype.big
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::big)
{
    return create_html(global_object, vm.this_value(global_object), "big", String::empty(), Value());
}

// B.2.3.4 String.prototype.blink ( ), https://tc39.es/ecma262/#sec-string.prototype.blink
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::blink)
{
    return create_html(global_object, vm.this_value(global_object), "blink", String::empty(), Value());
}

// B.2.3.5 String.prototype.bold ( ), https://tc39.es/ecma262/#sec-string.prototype.bold
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::bold)
{
    return create_html(global_object, vm.this_value(global_object), "b", String::empty(), Value());
}

// B.2.3.6 String.prototype.fixed ( ), https://tc39.es/ecma262/#sec-string.prototype.fixed
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::fixed)
{
    return create_html(global_object, vm.this_value(global_object), "tt", String::empty(), Value());
}

// B.2.3.7 String.prototype.fontcolor ( color ), https://tc39.es/ecma262/#sec-string.prototype.fontcolor
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::fontcolor)
{
    return create_html(global_object, vm.this_value(global_object), "font", "color", vm.argument(0));
}

// B.2.3.8 String.prototype.fontsize ( size ), https://tc39.es/ecma262/#sec-string.prototype.fontsize
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::fontsize)
{
    return create_html(global_object, vm.this_value(global_object), "font", "size", vm.argument(0));
}

// B.2.3.9 String.prototype.italics ( ), https://tc39.es/ecma262/#sec-string.prototype.italics
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::italics)
{
    return create_html(global_object, vm.this_value(global_object), "i", String::empty(), Value());
}

// B.2.3.10 String.prototype.link ( url ), https://tc39.es/ecma262/#sec-string.prototype.link
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::link)
{
    return create_html(global_object, vm.this_value(global_object), "a", "href", vm.argument(0));
}

// B.2.3.11 String.prototype.small ( ), https://tc39.es/ecma262/#sec-string.prototype.small
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::small)
{
    return create_html(global_object, vm.this_value(global_object), "small", String::empty(), Value());
}

// B.2.3.12 String.prototype.strike ( ), https://tc39.es/ecma262/#sec-string.prototype.strike
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::strike)
{
    return create_html(global_object, vm.this_value(global_object), "strike", String::empty(), Value());
}

// B.2.3.13 String.prototype.sub ( ), https://tc39.es/ecma262/#sec-string.prototype.sub
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::sub)
{
    return create_html(global_object, vm.this_value(global_object), "sub", String::empty(), Value());
}

// B.2.3.14 String.prototype.sup ( ), https://tc39.es/ecma262/#sec-string.prototype.sup
JS_DEFINE_NATIVE_FUNCTION(StringPrototype::sup)
{
    return create_html(global_object, vm.this_value(global_object), "sup", String::empty(), Value());
}

}
