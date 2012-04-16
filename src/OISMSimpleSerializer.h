// Licensed under the zlib License
// Copyright (C) 2012 Sebastien Raymond

#include "OISMHandler.h"

#include <fstream>
#include <map>
#include <vector>


namespace oism
{


typedef std::unordered_map<std::string, unsigned> NamedEnum;


struct File
{
    File(const std::string& filename);
    bool isOpen();

    // Return length
    int nextWord(std::string& str);
    int nextNumber(int& num);
    int nextLine();

    bool readKeyValuePair(const std::string& key, int& value);
    bool readKeyValuePair(const std::string& key, float& value);
    bool readKeyValuePair(const std::string& key, std::string& value);

    enum KeyValueOperation
    {
        KVO_Write,
        KVO_Read
    };

    template <class T>
    void keyValuePair(const std::string& key, T& value, KeyValueOperation oper)
    {
        if (oper == KVO_Write) writeKeyValuePair(key, value);
        else readKeyValuePair(key, value);
    }

    template <class T>
    void writeKeyValuePair(const std::string& key, const T& value)
    {
        fs << key << ' ' << value << std::endl;
    }

    std::string _findKey(const std::string& key);

    std::fstream fs;
    std::string line;
    std::string filename;
    size_t wend;
    size_t lineNum;
};


template <class T>
inline File& operator<<(File& file, const T& t)
{
    file.fs << t;
    return file;
}


class SimpleSerializer : public Serializer
{
public:
    SimpleSerializer(const std::string& path);
    
    virtual void loadBinding(NamedBindingMap&);
    virtual void loadConfig(Handler::Configuration*);
    virtual void saveBinding(const NamedBindingMap&);
    virtual void saveConfig(Handler::Configuration*);

protected:
    void addKey(Bind* b, File& fr) const;
    void addMouse(Bind* b, File& fr) const;
    void addJoyStick(Bind* b, File& fr) const;

    std::string keyEventToString(const InputEvent::Type& evt);
    std::string mouseEventToString(const InputEvent::Type& evt);
    std::string joyStickEventToString(const InputEvent::Type& evt);

    void doConfig(Handler::Configuration* c, File::KeyValueOperation oper);

    NamedEnum mKeyNames;
    NamedEnum mKeyModifierNames;
    NamedEnum mMouseComponentNames;
    NamedEnum mJoyStickComponentNames;

    std::vector<std::string> mKeyboardDeviceNames;
    std::vector<std::string> mMouseDeviceNames;
    std::vector<std::string> mJoyStickDeviceNames;
};


} // namespace oism
