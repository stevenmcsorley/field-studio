#include "PresetFormat.h"
#include <array>
#include <cstring>
#include <limits>

namespace field::preset {
namespace {
constexpr juce::int64 maxMetadataBytes = 1024 * 1024;
constexpr size_t maxChunks = 100000;
constexpr int maxJsonDepth = 64;

juce::uint32 big32(const unsigned char* p) {
    return (juce::uint32(p[0]) << 24) | (juce::uint32(p[1]) << 16)
         | (juce::uint32(p[2]) << 8) | juce::uint32(p[3]);
}
bool tag(const unsigned char* p, const char* s) { return std::memcmp(p,s,4) == 0; }
bool space(unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

// JUCE's JSON parser intentionally accepts some non-JSON syntax and does not
// require EOF after the first value. Validate the complete grammar first, with
// bounded depth/numbers, so malformed input cannot reach its integer accumulator.
struct JsonSyntax {
    const unsigned char* data;
    size_t length, at = 0;
    void ws() { while (at < length && space(data[at])) ++at; }
    bool consume(unsigned char c) {
        ws();
        if (at == length || data[at] != c) return false;
        ++at; return true;
    }
    bool string() {
        if (at == length || data[at++] != '"') return false;
        while (at < length) {
            const auto c = data[at++];
            if (c == '"') return true;
            if (c < 0x20) return false;
            if (c != '\\') continue;
            if (at == length) return false;
            const auto escape = data[at++];
            if (escape == 'u') {
                if (length-at < 4) return false;
                for (int i=0;i<4;++i) {
                    const auto h=data[at++];
                    if (!((h >= '0' && h <= '9') || (h >= 'a' && h <= 'f')
                          || (h >= 'A' && h <= 'F'))) return false;
                }
            } else if (escape != '"' && escape != '\\' && escape != '/'
                    && escape != 'b' && escape != 'f' && escape != 'n'
                    && escape != 'r' && escape != 't') return false;
        }
        return false;
    }
    bool number() {
        const auto begin=at;
        if (at < length && data[at]=='-') ++at;
        if (at == length || data[at] < '0' || data[at] > '9') return false;
        const auto integral=at;
        if (data[at]=='0') ++at;
        else while (at < length && data[at]>='0' && data[at]<='9') ++at;
        // JUCE accumulates the integral prefix as int64, even for decimal
        // values. Eighteen digits keep that accumulation within range.
        if (at-integral > 18) return false;
        if (at < length && data[at]=='.') {
            ++at; const auto fraction=at;
            while (at < length && data[at]>='0' && data[at]<='9') ++at;
            if (at==fraction) return false;
        }
        if (at < length && (data[at]=='e' || data[at]=='E')) {
            ++at;
            if (at < length && (data[at]=='+' || data[at]=='-')) ++at;
            const auto exponentStart=at;
            int exponent=0;
            while (at < length && data[at]>='0' && data[at]<='9') {
                if (at-exponentStart >= 3) return false;
                exponent=exponent*10+data[at++]-'0';
            }
            if (at==exponentStart || exponent>280) return false;
        }
        return at-begin <= 128;
    }
    bool literal(const char* value) {
        const auto n=std::strlen(value);
        if (length-at < n || std::memcmp(data+at,value,n)!=0) return false;
        at+=n; return true;
    }
    bool value(int depth) {
        ws();
        if (at==length) return false;
        const auto c=data[at];
        if (c=='"') return string();
        if (c=='t') return literal("true");
        if (c=='f') return literal("false");
        if (c=='n') return literal("null");
        if (c=='-' || (c>='0' && c<='9')) return number();
        if (c!='{' && c!='[') return false;
        if (depth >= maxJsonDepth) return false;
        ++at;
        const auto closing=static_cast<unsigned char>(c=='{' ? '}' : ']');
        ws();
        if (at < length && data[at]==closing) { ++at; return true; }
        for (;;) {
            ws();
            if (c=='{' && (!string() || !consume(':'))) return false;
            if (!value(depth+1)) return false;
            ws();
            if (at < length && data[at]==closing) { ++at; return true; }
            if (!consume(',')) return false;
            // Next value/key must exist: trailing commas are not JSON.
        }
    }
};

bool jsonExtent(const unsigned char* p, size_t n, size_t& textLength,
                juce::String& error) {
    textLength = 0;
    while (textLength < n && p[textLength] != 0) ++textLength;
    for (size_t j = textLength; j < n; ++j)
        if (p[j] != 0 && !space(p[j])) {
            error = "Non-padding bytes after the preset JSON terminator.";
            return false;
        }
    if (textLength == 0 || !juce::CharPointer_UTF8::isValidString(
            reinterpret_cast<const char*>(p), static_cast<int>(textLength))) {
        error = "Preset JSON is empty or is not valid UTF-8.";
        return false;
    }
    JsonSyntax syntax{p,textLength};
    syntax.ws();
    if (syntax.at==textLength || p[syntax.at]!='{') {
        error = "Preset JSON must be an object.";
        return false;
    }
    if (!syntax.value(0)) {
        error = "Invalid preset JSON syntax or supported nesting/number limit exceeded.";
        return false;
    }
    syntax.ws();
    if (syntax.at != textLength) {
        error = "Trailing bytes after the preset JSON object.";
        return false;
    }
    return true;
}
}

juce::var readMetadata(const juce::File& file, juce::String& error,
                       juce::MemoryBlock* rawPayload) {
    error.clear();
    if (rawPayload != nullptr) rawPayload->reset();
    const auto fail = [&error](const juce::String& message) -> juce::var {
        error = message;
        return {};
    };
    auto stream = file.createInputStream();
    if (!stream) return fail("Cannot open the selected file.");
    const auto length = stream->getTotalLength();
    std::array<unsigned char,12> header{};
    const int bytes = stream->read(header.data(), static_cast<int>(header.size()));
    if (bytes < 4 || !tag(header.data(), "FORM")) return {};
    if (bytes != static_cast<int>(header.size())) return fail("Truncated FORM header.");
    if (!tag(header.data()+8,"AIFF") && !tag(header.data()+8,"AIFC")) return {};
    const auto formBytes = big32(header.data()+4);
    if (formBytes < 4 || formBytes > static_cast<juce::uint32>(std::numeric_limits<int>::max()))
        return fail("Invalid AIFF FORM length.");
    const juce::int64 end = 8 + static_cast<juce::int64>(formBytes);
    if (end > length) return fail("AIFF FORM extends beyond the file.");
    if (end != length) return fail("Trailing bytes outside the AIFF FORM.");

    juce::MemoryBlock candidate;
    bool found = false;
    juce::int64 at = 12;
    size_t chunks = 0;
    while (at < end) {
        if (++chunks > maxChunks) return fail("AIFF contains too many chunks.");
        if (end-at < 8) return fail("Truncated AIFF chunk header.");
        if (!stream->setPosition(at)) return fail("Cannot seek to AIFF chunk.");
        std::array<unsigned char,8> h{};
        if (stream->read(h.data(),8) != 8) return fail("Cannot read AIFF chunk header.");
        const auto chunkBytes = big32(h.data()+4);
        const juce::int64 dataAt = at+8;
        // Difference comparison avoids integer overflow, and all arithmetic
        // after this point is signed64 with a signed32 FORM upper bound.
        if (static_cast<juce::int64>(chunkBytes) > end-dataAt)
            return fail("AIFF chunk extends beyond FORM.");
        const juce::int64 dataEnd = dataAt+static_cast<juce::int64>(chunkBytes);
        const juce::int64 pad = chunkBytes & 1u;
        if (pad > end-dataEnd) return fail("Missing AIFF chunk padding byte.");
        if (pad != 0) {
            if (!stream->setPosition(dataEnd)) return fail("Cannot seek to AIFF padding.");
            unsigned char padding = 0;
            if (stream->read(&padding,1) != 1 || padding != 0)
                return fail("Invalid AIFF chunk padding byte.");
        }
        if (tag(h.data(),"APPL")) {
            if (chunkBytes < 4) return fail("Truncated APPL application signature.");
            if (!stream->setPosition(dataAt)) return fail("Cannot seek to APPL signature.");
            std::array<unsigned char,4> signature{};
            if (stream->read(signature.data(),4) != 4) return fail("Cannot read APPL signature.");
            if (tag(signature.data(),"op-1")) {
                if (found) return fail("Multiple op-1 metadata chunks are ambiguous.");
                found = true;
                const juce::int64 size = static_cast<juce::int64>(chunkBytes)-4;
                if (size <= 0 || size > maxMetadataBytes)
                    return fail("Preset metadata exceeds the supported size or is empty.");
                candidate.setSize(static_cast<size_t>(size));
                if (stream->read(candidate.getData(),static_cast<int>(size)) != size)
                    return fail("Truncated preset metadata.");
            }
        }
        at = dataEnd+pad;
    }
    if (!found) return {};
    size_t textLength = 0;
    if (!jsonExtent(static_cast<const unsigned char*>(candidate.getData()),
                    candidate.getSize(),textLength,error)) return {};
    const auto text = juce::String::fromUTF8(static_cast<const char*>(candidate.getData()),
                                            static_cast<int>(textLength));
    juce::var metadata;
    const auto parsed = juce::JSON::parse(text,metadata);
    if (parsed.failed()) return fail("Invalid preset JSON: " + parsed.getErrorMessage());
    if (metadata.getDynamicObject()==nullptr) return fail("Preset JSON must be an object.");
    if (rawPayload != nullptr) *rawPayload = std::move(candidate);
    return metadata;
}

bool isPreset(const juce::var& value) {
    if (value.getDynamicObject()==nullptr) return false;
    const auto type = value.getProperty("type",juce::var());
    return type.isString() && type.toString().trim().isNotEmpty();
}

int engineIndex(const juce::var& value) {
    if (!isPreset(value)) return -1;
    const auto type = value.getProperty("type",juce::var()).toString().trim().toLowerCase();
    static const std::array<const char*,15> names{{
        "voltage","fm","cluster","digital","drwave","dimension","string",
        "pulse","phase","dna","dsynth","sampler","vocoder","amp","dbox"
    }};
    for (size_t i=0; i<names.size(); ++i)
        if (type == names[i]) return static_cast<int>(i);
    return -1;
}
}
