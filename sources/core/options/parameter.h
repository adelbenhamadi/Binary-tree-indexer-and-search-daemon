#pragma once
#include <string>
#include <iostream>
#include <map>
#include <unordered_map>

// abstract base parameter type struct
struct Parameter {
    Parameter() = default;
    virtual ~Parameter() = default;

    virtual void flushValue() {}

    virtual bool requiresValue() const { return true; }
    virtual std::string name() const = 0;
    virtual std::string valueString() const = 0;
    virtual std::string set(std::string const&) = 0;
    virtual std::string description() const { return std::string(); }

    virtual std::string typeDescription() const {
        return std::string("<") + name() + std::string(">");
    }

   
};

// specialized type for boolean values
struct BooleanParameter : public Parameter {
    typedef bool ValueType;

    explicit BooleanParameter(ValueType* ptr, bool required = false)
        : ptr(ptr), required(required) {}

    bool requiresValue() const override { return required; }
    std::string name() const override { return "boolean"; }
    std::string valueString() const override { return stringifyValue(*ptr); }

    std::string set(std::string const& value) override {
        if (!required && value.empty()) {
            // the empty value "" is considered "true", e.g. "--force" will mean
            // "--force true"
            *ptr = true;
            return "";
        }
        if (value == "true" || value == "false" || value == "on" || value == "off" ||
            value == "1" || value == "0" || value == "yes" || value == "no") {
            *ptr =
                (value == "true" || value == "on" || value == "1" || value == "yes");
            return "";
        }
        return "invalid value for type " + this->name() + ". expecting 'true' or 'false'";
    }

    std::string typeDescription() const override {
        return Parameter::typeDescription();
    }

    ValueType* ptr;
    bool required;
};

// specialized type for atomic boolean values
struct AtomicBooleanParameter : public Parameter {
    typedef std::atomic<bool> ValueType;

    explicit AtomicBooleanParameter(ValueType* ptr, bool required = false)
        : ptr(ptr), required(required) {}

    bool requiresValue() const override { return required; }
    std::string name() const override { return "boolean"; }
    std::string valueString() const override {
        return stringifyValue(ptr->load());
    }

    std::string set(std::string const& value) override {
        if (!required && value.empty()) {
            // the empty value "" is considered "true", e.g. "--force" will mean
            // "--force true"
            *ptr = true;
            return "";
        }
        if (value == "true" || value == "false" || value == "on" ||
            value == "off" || value == "1" || value == "0") {
            ptr->store(value == "true" || value == "on" || value == "1");
            return "";
        }
        return "invalid value for type " + this->name() + ". expecting 'true' or 'false'";
    }

    std::string typeDescription() const override {
        return Parameter::typeDescription();
    }

    void toVPack(VPackBuilder& builder) const override {
        builder.add(VPackValue(ptr->load()));
    }

    ValueType* ptr;
    bool required;
};

// specialized type for numeric values
// this templated type needs a concrete number type
template <typename T>
struct NumericParameter : public Parameter {
    typedef T ValueType;

    explicit NumericParameter(ValueType* ptr) : ptr(ptr), base(1) {}
    NumericParameter(ValueType* ptr, ValueType base) : ptr(ptr), base(base) {}

    std::string valueString() const override { return stringifyValue(*ptr); }

    std::string set(std::string const& value) override {
        try {
            ValueType v = toNumber<ValueType>(value, base);
            *ptr = v;
            return "";
        }
        catch (...) {
            return "invalid numeric value '" + value + "' for type " + this->name();
        }
    }

    void toVPack(VPackBuilder& builder) const override {
        builder.add(VPackValue(*ptr));
    }

    ValueType* ptr;
    ValueType base;
};

// concrete int16 number value type
struct Int16Parameter : public NumericParameter<int16_t> {
    typedef int16_t ValueType;

    explicit Int16Parameter(ValueType* ptr) : NumericParameter<ValueType>(ptr) {}
    Int16Parameter(ValueType* ptr, ValueType base)
        : NumericParameter<ValueType>(ptr, base) {}

    std::string name() const override { return "int16"; }
};

// concrete uint16 number value type
struct UInt16Parameter : public NumericParameter<uint16_t> {
    typedef uint16_t ValueType;

    explicit UInt16Parameter(ValueType* ptr) : NumericParameter<ValueType>(ptr) {}
    UInt16Parameter(ValueType* ptr, ValueType base)
        : NumericParameter<ValueType>(ptr, base) {}

    std::string name() const override { return "uint16"; }
};

// concrete int32 number value type
struct Int32Parameter : public NumericParameter<int32_t> {
    typedef int32_t ValueType;

    explicit Int32Parameter(ValueType* ptr) : NumericParameter<ValueType>(ptr) {}
    Int32Parameter(ValueType* ptr, ValueType base)
        : NumericParameter<ValueType>(ptr, base) {}

    std::string name() const override { return "int32"; }
};

// concrete uint32 number value type
struct UInt32Parameter : public NumericParameter<uint32_t> {
    typedef uint32_t ValueType;

    explicit UInt32Parameter(ValueType* ptr) : NumericParameter<ValueType>(ptr) {}
    UInt32Parameter(ValueType* ptr, ValueType base)
        : NumericParameter<ValueType>(ptr, base) {}

    std::string name() const override { return "uint32"; }
};

// concrete int64 number value type
struct Int64Parameter : public NumericParameter<int64_t> {
    typedef int64_t ValueType;

    explicit Int64Parameter(ValueType* ptr) : NumericParameter<ValueType>(ptr) {}
    Int64Parameter(ValueType* ptr, ValueType base)
        : NumericParameter<ValueType>(ptr, base) {}

    std::string name() const override { return "int64"; }
};

// concrete uint64 number value type
struct UInt64Parameter : public NumericParameter<uint64_t> {
    typedef uint64_t ValueType;

    explicit UInt64Parameter(ValueType* ptr) : NumericParameter<ValueType>(ptr) {}
    UInt64Parameter(ValueType* ptr, ValueType base)
        : NumericParameter<ValueType>(ptr, base) {}

    std::string name() const override { return "uint64"; }
};

template <typename T>
struct BoundedParameter : public T {
    BoundedParameter(typename T::ValueType* ptr, typename T::ValueType minValue,
        typename T::ValueType maxValue)
        : T(ptr), minValue(minValue), maxValue(maxValue) {}

    std::string set(std::string const& value) override {
        try {
            typename T::ValueType v =
                toNumber<typename T::ValueType>(value, static_cast<typename T::ValueType>(1));
            if (v >= minValue && v <= maxValue) {
                *this->ptr = v;
                return "";
            }
        }
        catch (...) {
            return "invalid numeric value '" + value + "' for type " + this->name();
        }
        return "number '" + value + "' out of allowed range (" +
            std::to_string(minValue) + " - " + std::to_string(maxValue) + ")";
    }

    typename T::ValueType minValue;
    typename T::ValueType maxValue;
};

// concrete double number value type
struct DoubleParameter : public NumericParameter<double> {
    typedef double ValueType;

    explicit DoubleParameter(ValueType* ptr) : NumericParameter<double>(ptr) {}

    std::string name() const override { return "double"; }
};

// string value type
struct StringParameter : public Parameter {
    typedef std::string ValueType;

    explicit StringParameter(ValueType* ptr) : ptr(ptr) {}

    std::string name() const override { return "string"; }
    std::string valueString() const override { return stringifyValue(*ptr); }

    std::string set(std::string const& value) override {
        *ptr = value;
        return "";
    }

    ValueType* ptr;
};


