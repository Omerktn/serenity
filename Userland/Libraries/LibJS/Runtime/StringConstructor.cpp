/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/StringBuilder.h>
#include <AK/Utf32View.h>
#include <LibJS/Runtime/AbstractOperations.h>
#include <LibJS/Runtime/Array.h>
#include <LibJS/Runtime/Error.h>
#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/StringConstructor.h>
#include <LibJS/Runtime/StringObject.h>

namespace JS {

StringConstructor::StringConstructor(GlobalObject& global_object)
    : NativeFunction(vm().names.String.as_string(), *global_object.function_prototype())
{
}

void StringConstructor::initialize(GlobalObject& global_object)
{
    auto& vm = this->vm();
    NativeFunction::initialize(global_object);

    // 22.1.2.3 String.prototype, https://tc39.es/ecma262/#sec-string.prototype
    define_property(vm.names.prototype, global_object.string_prototype(), 0);

    define_property(vm.names.length, Value(1), Attribute::Configurable);

    u8 attr = Attribute::Writable | Attribute::Configurable;
    define_native_function(vm.names.raw, raw, 1, attr);
    define_native_function(vm.names.fromCharCode, from_char_code, 1, attr);
    define_native_function(vm.names.fromCodePoint, from_code_point, 1, attr);
}

StringConstructor::~StringConstructor()
{
}

// 22.1.1.1 String ( value ), https://tc39.es/ecma262/#sec-string-constructor-string-value
Value StringConstructor::call()
{
    if (!vm().argument_count())
        return js_string(heap(), "");
    if (vm().argument(0).is_symbol())
        return js_string(heap(), vm().argument(0).as_symbol().to_string());
    auto* string = vm().argument(0).to_primitive_string(global_object());
    if (vm().exception())
        return {};
    return string;
}

// 22.1.1.1 String ( value ), https://tc39.es/ecma262/#sec-string-constructor-string-value
Value StringConstructor::construct(FunctionObject&)
{
    PrimitiveString* primitive_string = nullptr;
    if (!vm().argument_count())
        primitive_string = js_string(vm(), "");
    else
        primitive_string = vm().argument(0).to_primitive_string(global_object());
    if (!primitive_string)
        return {};
    return StringObject::create(global_object(), *primitive_string);
}

// 22.1.2.4 String.raw ( template, ...substitutions ), https://tc39.es/ecma262/#sec-string.raw
JS_DEFINE_NATIVE_FUNCTION(StringConstructor::raw)
{
    auto* cooked = vm.argument(0).to_object(global_object);
    if (vm.exception())
        return {};

    auto raw_value = cooked->get(vm.names.raw).value_or(js_undefined());
    if (vm.exception())
        return {};

    auto* raw = raw_value.to_object(global_object);
    if (vm.exception())
        return {};

    auto literal_segments = length_of_array_like(global_object, *raw);
    if (vm.exception())
        return {};

    if (literal_segments == 0)
        return js_string(vm, "");

    const auto number_of_substituions = vm.argument_count() - 1;

    StringBuilder builder;
    for (size_t i = 0; i < literal_segments; ++i) {
        auto next_key = String::number(i);
        auto next_segment_value = raw->get(next_key).value_or(js_undefined());
        if (vm.exception())
            return {};
        auto next_segment = next_segment_value.to_string(global_object);
        if (vm.exception())
            return {};

        builder.append(next_segment);

        if (i + 1 == literal_segments)
            break;

        if (i < number_of_substituions) {
            auto next = vm.argument(i + 1);
            auto next_sub = next.to_string(global_object);
            if (vm.exception())
                return {};
            builder.append(next_sub);
        }
    }
    return js_string(vm, builder.build());
}

// 22.1.2.1 String.fromCharCode ( ...codeUnits ), https://tc39.es/ecma262/#sec-string.fromcharcode
JS_DEFINE_NATIVE_FUNCTION(StringConstructor::from_char_code)
{
    StringBuilder builder;
    for (size_t i = 0; i < vm.argument_count(); ++i) {
        auto char_code = vm.argument(i).to_i32(global_object);
        if (vm.exception())
            return {};
        auto truncated = char_code & 0xffff;
        // FIXME: We need an Utf16View :^)
        builder.append(Utf32View((u32*)&truncated, 1));
    }
    return js_string(vm, builder.build());
}

// 22.1.2.2 String.fromCodePoint ( ...codePoints ), https://tc39.es/ecma262/#sec-string.fromcodepoint
JS_DEFINE_NATIVE_FUNCTION(StringConstructor::from_code_point)
{
    StringBuilder builder;
    for (size_t i = 0; i < vm.argument_count(); ++i) {
        auto next_code_point = vm.argument(i).to_number(global_object);
        if (vm.exception())
            return {};
        if (!next_code_point.is_integral_number()) {
            vm.throw_exception<RangeError>(global_object, ErrorType::InvalidCodePoint, next_code_point.to_string_without_side_effects());
            return {};
        }
        auto code_point = next_code_point.to_i32(global_object);
        if (code_point < 0 || code_point > 0x10FFFF) {
            vm.throw_exception<RangeError>(global_object, ErrorType::InvalidCodePoint, next_code_point.to_string_without_side_effects());
            return {};
        }
        builder.append_code_point(code_point);
    }
    return js_string(vm, builder.build());
}

}
