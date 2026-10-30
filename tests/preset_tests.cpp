#include "PresetFormat.h"
#include "PresetMapping.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>

namespace {
using Bytes = std::vector<unsigned char>;
void expect(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}
void text(Bytes& b, const char* s) { b.insert(b.end(),s,s+std::strlen(s)); }
void big32(Bytes& b, juce::uint32 n) {
    b.push_back(static_cast<unsigned char>(n>>24)); b.push_back(static_cast<unsigned char>(n>>16));
    b.push_back(static_cast<unsigned char>(n>>8)); b.push_back(static_cast<unsigned char>(n));
}
void set32(Bytes& b, size_t at, juce::uint32 n) {
    b[at] = static_cast<unsigned char>(n>>24); b[at+1] = static_cast<unsigned char>(n>>16);
    b[at+2] = static_cast<unsigned char>(n>>8); b[at+3] = static_cast<unsigned char>(n);
}
void chunk(Bytes& b, const char* id, const Bytes& data) {
    text(b,id); big32(b,static_cast<juce::uint32>(data.size()));
    b.insert(b.end(),data.begin(),data.end());
    if (data.size()&1u) b.push_back(0);
}
Bytes bytes(const char* s) { return Bytes(s,s+std::strlen(s)); }
Bytes appl(const char* json) { Bytes b=bytes("op-1"); text(b,json); return b; }
Bytes form(const Bytes& chunks, const char* type="AIFC") {
    Bytes b=bytes("FORM"); big32(b,static_cast<juce::uint32>(chunks.size()+4));
    text(b,type); b.insert(b.end(),chunks.begin(),chunks.end()); return b;
}
Bytes preset(const char* json) { Bytes b; chunk(b,"APPL",appl(json)); return form(b); }

struct Fixture {
    juce::File directory = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("field-preset-tests", "", false);
    Fixture() { expect(directory.createDirectory().wasOk(),"create temp fixture directory"); }
    ~Fixture() { directory.deleteRecursively(); }
    juce::File write(const Bytes& data) {
        auto file=directory.getChildFile("fixture.bin");
        expect(file.replaceWithData(data.data(),data.size()),"write fixture"); return file;
    }
    juce::var read(const Bytes& data, juce::String& error, juce::MemoryBlock* raw=nullptr) {
        return field::preset::readMetadata(write(data),error,raw);
    }
    void rejects(const Bytes& data, const char* label) {
        juce::String error="old"; juce::MemoryBlock raw("old",3);
        const auto value=read(data,error,&raw);
        if (!value.isVoid() || error.isEmpty() || raw.getSize()!=0)
            throw std::runtime_error((juce::String("Expected rejection: ")+label).toStdString());
    }
};

bool equalJSON(const juce::var& a, const juce::var& b) {
    if (a.getDynamicObject()!=nullptr && b.getDynamicObject()!=nullptr) {
        auto* aa=a.getDynamicObject(); auto* bb=b.getDynamicObject();
        const auto& pa=aa->getProperties(); const auto& pb=bb->getProperties();
        if (pa.size()!=pb.size()) return false;
        for (int i=0;i<pa.size();++i) {
            const auto key=pa.getName(i);
            if (!pb.contains(key) || !equalJSON(pa.getValueAt(i),pb[key])) return false;
        }
        return true;
    }
    if (a.isArray() && b.isArray()) {
        if (a.size()!=b.size()) return false;
        for (int i=0;i<a.size();++i) if (!equalJSON(a[i],b[i])) return false;
        return true;
    }
    return a==b;
}

void runSyntheticTests() {
    Fixture fixture;
    juce::String error="old";
    juce::MemoryBlock raw;
    const char* json=R"({"type":"fm","name":"Independent fixture","knobs":[7,32767,-2,0],"unrecognized":{"deep":[true,null,"{brace}\\\"quote"]},"future":3})";
    Bytes payload=appl(json); payload.resize(payload.size()+7,0);
    Bytes chunks;
    chunk(chunks,"JUNK",Bytes{1,2,3}); // Odd-length unknown chunks must skip their pad.
    chunk(chunks,"APPL",bytes("othrignored"));
    chunk(chunks,"SSND",Bytes{0,0,0,0}); // No metadata-order assumption.
    chunk(chunks,"APPL",payload);
    const auto value=fixture.read(form(chunks),error,&raw);
    expect(error.isEmpty() && field::preset::isPreset(value),"read valid AIFC preset");
    expect(field::preset::engineIndex(value)==1,"FM engine mapping");
    expect(equalJSON(value,juce::JSON::parse(json)),"unknown fields and raw knob values preserved");
    expect(raw.getSize()==payload.size()-4,"preserve exact raw payload size");
    expect(std::memcmp(raw.getData(),payload.data()+4,raw.getSize())==0,"preserve exact raw payload bytes");
    const auto aiff=fixture.read(form(chunks,"AIFF"),error);
    expect(error.isEmpty() && field::preset::isPreset(aiff),"AIFF supported as well as AIFC");

    Bytes noMetadata;
    chunk(noMetadata,"APPL",bytes("othrdata"));
    auto plain=fixture.read(form(noMetadata),error,&raw);
    expect(plain.isVoid() && error.isEmpty() && raw.getSize()==0,"ordinary AIFF clears old result");
    plain=fixture.read(bytes("RIFFordinary WAV input"),error);
    expect(plain.isVoid() && error.isEmpty(),"ordinary non-FORM file");
    plain=fixture.read(bytes("a"),error);
    expect(plain.isVoid() && error.isEmpty(),"ordinary short file");
    plain=fixture.read(form({},"ILBM"),error);
    expect(plain.isVoid() && error.isEmpty(),"other IFF FORM type ignored");
    plain=fixture.read(preset("{\"type\":\"future-engine\",\"unknown\":42}"),error);
    expect(error.isEmpty() && field::preset::isPreset(plain),"unknown engine metadata retained");
    expect(field::preset::engineIndex(plain)==-1 && int(plain["unknown"])==42,"unknown engine not guessed");
    auto object=fixture.read(preset("{}"),error);
    expect(error.isEmpty() && object.isObject() && !field::preset::isPreset(object),"untyped object retained");
    expect(!field::preset::isPreset(juce::JSON::parse("{\"type\":1}")),"numeric engine type not a preset");
    expect(!field::preset::isPreset(juce::JSON::parse("{\"type\":\" \"}")),"empty engine type not a preset");
    expect(!field::preset::isPreset(juce::var()),"void is not a preset");
    expect(!field::preset::isPreset(juce::JSON::parse("[]")),"array is not a preset object");
    static const char* types[]={"voltage","fm","cluster","digital","drwave","dimension","string","pulse","phase","dna","dsynth","sampler","vocoder","amp","dbox"};
    for (int i=0;i<15;++i) {
        auto* data=new juce::DynamicObject(); data->setProperty("type",types[i]);
        expect(field::preset::engineIndex(juce::var(data))==i,"complete engine enumeration mapping");
    }
    expect(field::preset::engineIndex(juce::JSON::parse("{\"type\":\"dbox\"}"))==14,"DBox uses dedicated drum engine index");

    fixture.rejects(bytes("FORM"),"short FORM header");
    auto bad=preset(json); set32(bad,4,3); fixture.rejects(bad,"FORM too short");
    bad=preset(json); set32(bad,4,0xffffffffu); fixture.rejects(bad,"FORM signed length overflow");
    bad=preset(json); set32(bad,4,static_cast<juce::uint32>(bad.size())); fixture.rejects(bad,"FORM beyond file");
    bad=preset(json); bad.push_back(1); fixture.rejects(bad,"trailing bytes beyond FORM");
    fixture.rejects(form(bytes("APPL")),"short chunk header");
    bad=preset(json); set32(bad,16,0xffffffffu); fixture.rejects(bad,"chunk unsigned length overflow");
    Bytes odd=bytes("JUNK"); big32(odd,1); odd.push_back(7);
    fixture.rejects(form(odd),"odd chunk missing pad");
    odd.push_back(1); fixture.rejects(form(odd),"nonzero pad");
    Bytes shortAppl; chunk(shortAppl,"APPL",bytes("op-")); fixture.rejects(form(shortAppl),"short APPL signature");
    Bytes duplicates; chunk(duplicates,"APPL",appl(json)); chunk(duplicates,"APPL",appl(json));
    fixture.rejects(form(duplicates),"duplicate preset chunks");
    fixture.rejects(preset("{\"type\":\"fm\" \"bad\":2}"),"invalid JSON syntax");
    fixture.rejects(preset("{\"type\":\"fm\"}" "[]"),"trailing JSON value");
    fixture.rejects(preset("[]"),"array rather than object");
    fixture.rejects(preset("{\"type\":\"fm\",\"x\":'bad'}"),"single-quoted JSON value");
    fixture.rejects(preset("{\"type\":\"fm\",}"),"trailing object comma");
    fixture.rejects(preset("{\"type\":\"fm\",\"x\":[1,]}"),"trailing array comma");
    fixture.rejects(preset("{\"type\":\"fm\",\"x\":01}"),"leading-zero JSON number");
    fixture.rejects(preset("{\"type\":\"fm\",\"x\":- 1}"),"spaced negative number");
    fixture.rejects(preset("{\"type\":\"fm\",\"x\":9223372036854775808}"),"integer accumulator overflow");
    fixture.rejects(preset("{\"type\":\"fm\",\"x\":1e999}"),"numeric exponent budget");

    fixture.rejects(preset("{\"type\":\"fm\""),"truncated JSON object");
    Bytes afterNul=appl("{\"type\":\"fm\"}"); afterNul.push_back(0); afterNul.push_back('x');
    Bytes nl; chunk(nl,"APPL",afterNul); fixture.rejects(form(nl),"non-padding after NUL");
    Bytes invalid=appl("{\"type\":\""); invalid.push_back(0xc0); invalid.push_back(0xaf); text(invalid,"\"}");
    Bytes invalidChunks; chunk(invalidChunks,"APPL",invalid); fixture.rejects(form(invalidChunks),"invalid UTF8");
    fixture.rejects(preset("{\"type\":\"fm\n\"}"),"unescaped string control byte");
    juce::String deep="{\"type\":\"fm\",\"x\":";
    for (int i=0;i<70;++i) deep+="[";
    deep+="0"; for (int i=0;i<70;++i) deep+="]"; deep+="}";
    fixture.rejects(preset(deep.toRawUTF8()),"deep JSON stack");
    Bytes giant=bytes("op-1"); giant.resize(1024*1024+5,' '); Bytes gc; chunk(gc,"APPL",giant);
    fixture.rejects(form(gc),"metadata size budget");
    Bytes many;
    for (int i=0;i<100001;++i) chunk(many,"JUNK",{});
    fixture.rejects(form(many),"chunk count budget");
    error.clear();
    auto missing=field::preset::readMetadata(fixture.directory.getChildFile("missing.aif"),error);
    expect(missing.isVoid() && error.isNotEmpty(),"file access error reported");
}

void checkExternalFixtures(const juce::File& root, const juce::File& manifest) {
    const auto inventory=juce::JSON::parse(manifest);
    const auto fixtures=inventory.getProperty("fixtures",juce::var());
    expect(fixtures.isArray() && fixtures.size()>0,"fixture inventory array");
    int count=0;
    for (int i=0;i<fixtures.size();++i) {
        const auto record=fixtures[i];
        const auto file=root.getChildFile(record["file"].toString());
        juce::String error;
        const auto metadata=field::preset::readMetadata(file,error);
        if (error.isNotEmpty() || !field::preset::isPreset(metadata)
            || !equalJSON(metadata,record["metadata"]))
            throw std::runtime_error("External fixture mismatch: "+file.getFullPathName().toStdString()+" "+error.toStdString());
        ++count;
    }
    std::cout << "External APPL metadata fixtures passed: " << count << "\n";
}
}

void runSampleMappingTests(){
    using field::preset::SampleMapping;using field::preset::sampleMapping;
    juce::String error;SampleMapping mapping;
    auto sampler=juce::JSON::parse(R"({"type":"sampler","knobs":[0,8192,16384,24576],"base_freq":440})");
    expect(sampleMapping(sampler,6,mapping,error),"sampler mapping accepted");expect(mapping.hasRoot&&mapping.rootNote==69,"sampler root from frequency");expect(mapping.hasMarkers&&std::abs(mapping.markers[1]-.25f)<1e-6&&std::abs(mapping.markers[3]-.75f)<1e-6,"6-second Q15 trim markers");
    auto* obj=new juce::DynamicObject;juce::var drum(obj);obj->setProperty("type","drum");obj->setProperty("drum_version",2);
    juce::Array<juce::var> starts,ends,pitch,vol,pan,rev,modes;
    const double units=4058.*44100.;for(int i=0;i<24;++i){starts.add(i<2?i*.25*units:0.);ends.add(i<2?(i*.25+.1)*units:0.);pitch.add(i==0?512:0);vol.add(8192);pan.add(i==0?32767:16384);rev.add(i==0?32767:8192);modes.add(8192);}
    obj->setProperty("start",starts);obj->setProperty("end",ends);obj->setProperty("pitch",pitch);obj->setProperty("volume",vol);obj->setProperty("pan",pan);obj->setProperty("reverse",rev);obj->setProperty("playmode",modes);
    expect(sampleMapping(drum,1,mapping,error),"classic drum kit mapping accepted");expect(mapping.drums&&mapping.pads[0].reverse&&mapping.pads[0].tune==1&&mapping.pads[0].pan>.99f,"pad tuning/direction/pan mapping");expect(std::abs(mapping.pads[1].start-.25)<1e-6&&mapping.pads[3].level==0,"uneven drum regions and unused silent pads");
    obj->setProperty("stereo",true);starts.set(1,2147483646.*.25);ends.set(1,2147483646.*.3);obj->setProperty("start",starts);obj->setProperty("end",ends);expect(sampleMapping(drum,20,mapping,error)&&std::abs(mapping.pads[1].start-.25)<1e-6,"stereo20-second Q31 marker profile");
    ends.set(1,-1);obj->setProperty("end",ends);expect(!sampleMapping(drum,20,mapping,error)&&error.isNotEmpty(),"invalid drum regions rejected");
    auto unknown=juce::JSON::parse(R"({"type":"unknown"})");expect(sampleMapping(unknown,1,mapping,error)&&!mapping.sampled,"modeled metadata does not masquerade as sample mapping");
}
int main(int argc, char** argv) {
    try {
        runSyntheticTests();runSampleMappingTests();
        std::cout << "Preset container/JSON/unknown-field/mapping tests passed\n";
        if (argc==3) checkExternalFixtures(juce::File(argv[1]),juce::File(argv[2]));
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << "\n"; return 1;
    }
}
