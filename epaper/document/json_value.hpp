#pragma once
/**
 * Minimal JSON value + parser for device-document fixtures (no Qt).
 * @implements [SRS-EP-09] shared ops/ fixture envelopes
 */

#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace epaper {
namespace document {

struct JsonValue {
    using Object = std::vector<std::pair<std::string, JsonValue>>;
    using Array = std::vector<JsonValue>;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> v;

    static JsonValue null() { return JsonValue{}; }
    static JsonValue boolean(bool b) { JsonValue j; j.v = b; return j; }
    static JsonValue number(double n) { JsonValue j; j.v = n; return j; }
    static JsonValue string(std::string s)
    {
        JsonValue j;
        j.v = std::move(s);
        return j;
    }
    static JsonValue array(Array a)
    {
        JsonValue j;
        j.v = std::move(a);
        return j;
    }
    static JsonValue object(Object o)
    {
        JsonValue j;
        j.v = std::move(o);
        return j;
    }

    bool isNull() const { return std::holds_alternative<std::nullptr_t>(v); }
    bool isBool() const { return std::holds_alternative<bool>(v); }
    bool isNumber() const { return std::holds_alternative<double>(v); }
    bool isString() const { return std::holds_alternative<std::string>(v); }
    bool isArray() const { return std::holds_alternative<Array>(v); }
    bool isObject() const { return std::holds_alternative<Object>(v); }

    bool asBool() const { return std::get<bool>(v); }
    double asNumber() const { return std::get<double>(v); }
    const std::string &asString() const { return std::get<std::string>(v); }
    const Array &asArray() const { return std::get<Array>(v); }
    const Object &asObject() const { return std::get<Object>(v); }

    const JsonValue *get(const std::string &key) const
    {
        if (!isObject())
            return nullptr;
        for (const auto &kv : asObject()) {
            if (kv.first == key)
                return &kv.second;
        }
        return nullptr;
    }

    std::string getString(const std::string &key, const std::string &fallback = {}) const
    {
        const JsonValue *x = get(key);
        if (!x || !x->isString())
            return fallback;
        return x->asString();
    }

    double getNumber(const std::string &key, double fallback = 0) const
    {
        const JsonValue *x = get(key);
        if (!x || !x->isNumber())
            return fallback;
        return x->asNumber();
    }

    bool has(const std::string &key) const { return get(key) != nullptr; }
};

inline std::string jsonEscape(const std::string &s)
{
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += static_cast<char>(c);
            break;
        }
    }
    return out;
}

inline std::string stringify(const JsonValue &j)
{
    if (j.isNull())
        return "null";
    if (j.isBool())
        return j.asBool() ? "true" : "false";
    if (j.isNumber()) {
        std::ostringstream oss;
        oss.precision(17);
        const double n = j.asNumber();
        if (std::floor(n) == n && std::abs(n) < 1e15)
            oss << static_cast<long long>(n);
        else
            oss << n;
        return oss.str();
    }
    if (j.isString())
        return std::string("\"") + jsonEscape(j.asString()) + "\"";
    if (j.isArray()) {
        std::string out = "[";
        const auto &a = j.asArray();
        for (size_t i = 0; i < a.size(); ++i) {
            if (i)
                out += ",";
            out += stringify(a[i]);
        }
        out += "]";
        return out;
    }
    std::string out = "{";
    const auto &o = j.asObject();
    for (size_t i = 0; i < o.size(); ++i) {
        if (i)
            out += ",";
        out += std::string("\"") + jsonEscape(o[i].first) + "\":" + stringify(o[i].second);
    }
    out += "}";
    return out;
}

class JsonParser {
public:
    explicit JsonParser(std::string src) : m_s(std::move(src)) {}

    JsonValue parse()
    {
        skip();
        JsonValue v = parseValue();
        skip();
        if (m_i != m_s.size())
            throw std::runtime_error("json_trailing");
        return v;
    }

private:
    std::string m_s;
    size_t m_i = 0;

    void skip()
    {
        while (m_i < m_s.size() && std::isspace(static_cast<unsigned char>(m_s[m_i])))
            ++m_i;
    }

    char peek() const { return m_i < m_s.size() ? m_s[m_i] : '\0'; }

    char take() { return m_i < m_s.size() ? m_s[m_i++] : '\0'; }

    JsonValue parseValue()
    {
        skip();
        const char c = peek();
        if (c == '{')
            return parseObject();
        if (c == '[')
            return parseArray();
        if (c == '"')
            return JsonValue::string(parseString());
        if (c == 't' || c == 'f')
            return parseBool();
        if (c == 'n')
            return parseNull();
        return parseNumber();
    }

    JsonValue parseObject()
    {
        take(); // {
        JsonValue::Object o;
        skip();
        if (peek() == '}') {
            take();
            return JsonValue::object(std::move(o));
        }
        while (true) {
            skip();
            if (peek() != '"')
                throw std::runtime_error("json_object_key");
            std::string key = parseString();
            skip();
            if (take() != ':')
                throw std::runtime_error("json_colon");
            o.emplace_back(std::move(key), parseValue());
            skip();
            const char c = take();
            if (c == '}')
                break;
            if (c != ',')
                throw std::runtime_error("json_object_sep");
        }
        return JsonValue::object(std::move(o));
    }

    JsonValue parseArray()
    {
        take(); // [
        JsonValue::Array a;
        skip();
        if (peek() == ']') {
            take();
            return JsonValue::array(std::move(a));
        }
        while (true) {
            a.push_back(parseValue());
            skip();
            const char c = take();
            if (c == ']')
                break;
            if (c != ',')
                throw std::runtime_error("json_array_sep");
        }
        return JsonValue::array(std::move(a));
    }

    std::string parseString()
    {
        if (take() != '"')
            throw std::runtime_error("json_string");
        std::string out;
        while (m_i < m_s.size()) {
            const char c = take();
            if (c == '"')
                return out;
            if (c == '\\') {
                const char e = take();
                switch (e) {
                case '"':
                case '\\':
                case '/':
                    out += e;
                    break;
                case 'n':
                    out += '\n';
                    break;
                case 'r':
                    out += '\r';
                    break;
                case 't':
                    out += '\t';
                    break;
                default:
                    out += e;
                    break;
                }
            } else {
                out += c;
            }
        }
        throw std::runtime_error("json_unterminated");
    }

    JsonValue parseBool()
    {
        if (m_s.compare(m_i, 4, "true") == 0) {
            m_i += 4;
            return JsonValue::boolean(true);
        }
        if (m_s.compare(m_i, 5, "false") == 0) {
            m_i += 5;
            return JsonValue::boolean(false);
        }
        throw std::runtime_error("json_bool");
    }

    JsonValue parseNull()
    {
        if (m_s.compare(m_i, 4, "null") == 0) {
            m_i += 4;
            return JsonValue::null();
        }
        throw std::runtime_error("json_null");
    }

    JsonValue parseNumber()
    {
        const size_t start = m_i;
        if (peek() == '-')
            take();
        if (!std::isdigit(static_cast<unsigned char>(peek())))
            throw std::runtime_error("json_number");
        while (std::isdigit(static_cast<unsigned char>(peek())))
            take();
        if (peek() == '.') {
            take();
            while (std::isdigit(static_cast<unsigned char>(peek())))
                take();
        }
        if (peek() == 'e' || peek() == 'E') {
            take();
            if (peek() == '+' || peek() == '-')
                take();
            while (std::isdigit(static_cast<unsigned char>(peek())))
                take();
        }
        return JsonValue::number(std::stod(m_s.substr(start, m_i - start)));
    }
};

inline JsonValue parseJson(const std::string &src)
{
    return JsonParser(src).parse();
}

inline JsonValue loadJsonFile(const std::string &path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error(std::string("json_open:") + path);
    std::ostringstream oss;
    oss << in.rdbuf();
    return parseJson(oss.str());
}

/** extras map: number | bool | string (SRS-EP-09). */
inline std::map<std::string, JsonValue> extrasFromJson(const JsonValue *obj)
{
    std::map<std::string, JsonValue> out;
    if (!obj || !obj->isObject())
        return out;
    for (const auto &kv : obj->asObject()) {
        if (kv.second.isNumber() || kv.second.isBool() || kv.second.isString())
            out.emplace(kv.first, kv.second);
    }
    return out;
}

} // namespace document
} // namespace epaper
