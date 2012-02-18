// Licensed under the zlib License
// Copyright (C) 2012 Sebastien Raymond

#include "OISMHandler.h"

#include <fstream>
#include <map>
#include <vector>

namespace oism
{


struct FileReader
{
    FileReader(const std::string& filename);
    bool isOpen();

    static void toLower(std::string& str);
    static bool isNegative(std::string& str);

    // Return word length
    int nextWord(std::string& str);

    // Return word length
    int nextNumber(int& num);

    // Return number of splits
    static int splitWord(char token, const std::string& word, std::vector<std::string>& splits);

    // Return line length
    int nextLine();

    std::string _findKey(const std::string& key);
    bool parseKeyValuePair(const std::string& key, int& value);
    bool parseKeyValuePair(const std::string& key, float& value);
    bool parseKeyValuePair(const std::string& key, std::string& value);

    std::ifstream fs;
    std::string line;
    std::string filename;
    size_t wend;
    size_t lineNum;
};


class SimpleSerializer : public Serializer
{
public:
    SimpleSerializer(const std::string& path);
    
    virtual void loadBinding(NamedBindingMap&);
    virtual void loadConfig(Handler::Configuration&);
    virtual void saveBinding(const NamedBindingMap&);
    virtual void saveConfig(const Handler::Configuration&);

protected:
    void addKey(Bind* b, FileReader& fr) const;
    void addMouse(Bind* b, FileReader& fr) const;
    void addJoyStick(Bind* b, FileReader& fr) const;

    std::unordered_map<std::string, unsigned> mKeyNames;
    std::unordered_map<std::string, unsigned> mKeyModifierNames;
    std::unordered_map<std::string, unsigned> mMouseComponentNames;
    std::unordered_map<std::string, unsigned> mJoyStickComponentNames;

    std::vector<std::string> mKeyboardDeviceNames;
    std::vector<std::string> mMouseDeviceNames;
    std::vector<std::string> mJoyStickDeviceNames;
};

} // oism
