#include "Neutrino/Neutrino.hpp"

namespace neutrino {

static bool starts(const std::string& text, const char* prefix)
{
    return text.rfind(prefix, 0) == 0;
}

Neu_TypedValue Neu_TypeInterpreter::interpret(const std::string& text)
{
    Neu_TypedValue value;
    value.original = text;
    value.value = text;

    try {
        if (text == "true" || text == "1:true" || text == "checkbox:1") {
            value.type = Neu_CellType::Boolean;
            value.value = true;
            return value;
        }

        if (text == "false" || text == "0:false" || text == "checkbox:0") {
            value.type = Neu_CellType::Boolean;
            value.value = false;
            return value;
        }

        if (starts(text, "hex:") || starts(text, "0x")) {
            value.type = Neu_CellType::Hex;
            value.value = static_cast<uint64_t>(std::stoull(starts(text, "hex:") ? text.substr(4) : text, nullptr, 16));
            return value;
        }

        if (starts(text, "bin8:")) {
            value.type = Neu_CellType::Binary8;
            value.value = static_cast<uint64_t>(std::stoull(text.substr(5), nullptr, 2));
            return value;
        }

        if (starts(text, "bin16:")) {
            value.type = Neu_CellType::Binary16;
            value.value = static_cast<uint64_t>(std::stoull(text.substr(6), nullptr, 2));
            return value;
        }

        if (starts(text, "bin32:")) {
            value.type = Neu_CellType::Binary32;
            value.value = static_cast<uint64_t>(std::stoull(text.substr(6), nullptr, 2));
            return value;
        }

        if (starts(text, "bin64:")) {
            value.type = Neu_CellType::Binary64;
            value.value = static_cast<uint64_t>(std::stoull(text.substr(6), nullptr, 2));
            return value;
        }

        if (starts(text, "bin128:")) {
            value.type = Neu_CellType::Binary128;
            value.value = text.substr(7);
            return value;
        }

        if (starts(text, "image:")) {
            value.type = Neu_CellType::ImageBmp;
            value.value = text.substr(6);
            return value;
        }

        if (starts(text, "enum:")) {
            value.type = Neu_CellType::Enum;
            value.value = text.substr(5);
            return value;
        }

        if (starts(text, "tri:")) {
            value.type = Neu_CellType::TriState;
            value.value = text.substr(4);
            return value;
        }

        if (starts(text, "utf:")) {
            value.type = Neu_CellType::UtfString;
            value.value = text.substr(4);
            return value;
        }

        if (text.find('.') != std::string::npos) {
            value.type = Neu_CellType::Double;
            value.value = std::stod(text);
            return value;
        }

        size_t parsed = 0;
        const long long integerValue = std::stoll(text, &parsed, 10);
        if (parsed == text.size()) {
            value.type = Neu_CellType::Int64;
            value.value = static_cast<int64_t>(integerValue);
            return value;
        }
    } catch (...) {
    }

    value.type = Neu_CellType::String;
    value.value = text;
    return value;
}

} // namespace neutrino
