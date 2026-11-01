#include "ControlBridge.h"
#include <stdexcept>
#if ! JUCE_WINDOWS
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace {
void privateDirectory(const juce::File& file){if(file.isSymbolicLink()||file.createDirectory().failed())throw std::runtime_error("Cannot create private control directory");
#if ! JUCE_WINDOWS
 struct stat info{};if(::stat(file.getFullPathName().toRawUTF8(),&info)!=0||info.st_uid!=::geteuid()||::chmod(file.getFullPathName().toRawUTF8(),0700)!=0)throw std::runtime_error("Control directory is not private to this user");
#endif
}
juce::var object(std::initializer_list<std::pair<const char*,juce::var>> entries){auto* obj=new juce::DynamicObject;for(auto& entry:entries)obj->setProperty(entry.first,entry.second);return juce::var(obj);}
void atomicWrite(const juce::File& file,const juce::var& value){juce::TemporaryFile temporary(file);if(!temporary.getFile().replaceWithText(juce::JSON::toString(value,true))||!temporary.overwriteTargetFileWithTemporary())throw std::runtime_error("Control response write failed");}
}
ControlBridge::ControlBridge(Handler callback,juce::File root):handler(std::move(callback)){
 if(root==juce::File{}){auto base=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("FieldStudio");privateDirectory(base);root=base.getChildFile("control");}
 privateDirectory(root);id=juce::Uuid().toString().removeCharacters("-");token=juce::Uuid().toString()+juce::Uuid().toString();directory=root.getChildFile(id);privateDirectory(directory);heartbeat();
}
ControlBridge::~ControlBridge(){directory.deleteRecursively();}
void ControlBridge::heartbeat(){atomicWrite(descriptor(),object({{"instance_id",id},{"token",token},{"heartbeat_ms",(double)juce::Time::currentTimeMillis()},{"protocol",1}}));}
void ControlBridge::poll(){
 try{if(++ticks%60==0)heartbeat();auto files=directory.findChildFiles(juce::File::findFiles,false,"request-*.json");if(files.isEmpty())return;auto file=files[0];const auto requestId=file.getFileNameWithoutExtension().substring(8);if(requestId.length()!=32||!requestId.containsOnly("0123456789abcdef")||file.isSymbolicLink()||file.getSize()>65536){file.deleteFile();return;}
 auto request=juce::JSON::parse(file.loadFileAsString());file.deleteFile();auto response=directory.getChildFile("response-"+requestId+".json");
 try{if(!request.isObject()||request["token"].toString()!=token)throw std::runtime_error("Unauthorized control request");const double expiry=(double)request["expires_ms"],now=(double)juce::Time::currentTimeMillis();if(!std::isfinite(expiry)||expiry<now||expiry>now+10000)throw std::runtime_error("Expired or invalid request deadline");if(std::find(seen.begin(),seen.end(),requestId)!=seen.end())throw std::runtime_error("Duplicate request ID; do not replay edits");seen.push_back(requestId);if(seen.size()>128)seen.pop_front();auto result=handler(request["operation"].toString(),request["args"]);atomicWrite(response,object({{"ok",true},{"result",result}}));}
 catch(const std::exception& error){atomicWrite(response,object({{"ok",false},{"error",juce::String(error.what())}}));}
 // Reclaim abandoned replies; clients normally consume and remove them.
 for(auto old:directory.findChildFiles(juce::File::findFiles,false,"response-*.json"))if(old.getLastModificationTime()<juce::Time::getCurrentTime()-juce::RelativeTime::seconds(60))old.deleteFile();
 }catch(const std::exception&){} // IPC failure cannot escape the GUI timer.
}
