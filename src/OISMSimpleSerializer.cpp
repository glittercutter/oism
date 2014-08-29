// Licensed under the zlib License
// Copyright (C) 2012 Sebastien Raymond

#include "OISMSimpleSerializer.h"

#include <algorithm>
#include <cctype>

// DEBUG
#include <iostream>


using namespace oism;


const char* g_map_filename = "inputmap";
const char* g_conf_filename = "inputconf";


template<class Vec>
bool linear_search(const Vec& v, const typename Vec::value_type& val)
{
    return std::find(v.begin(), v.end(), val) != v.end();
}


// Linear search for the value instead of the key
template<class Map>
typename Map::const_iterator reverse_map_search(const Map& m, const typename Map::mapped_type& v)
{
    for (auto it = m.begin(); it != m.end(); it++) if (it->second == v) return it;
    return m.end();
}


std::string enum_to_string(const NamedEnum& m, unsigned v)
{
    auto it = reverse_map_search(m, v);
    if (it == m.end()) 
    {
        log::log("Invalid enum: "+std::to_string(v), log::Level::Error);
        return "__INVALID__";
    }

    return it->first;
}


void to_lower(std::string& str)
{
    for (auto& c : str) c = tolower(c);
}


bool is_negative(std::string& str)
{
    if (!str.empty() && str[0] == '-')
    {
        str.erase(str.begin());
        return true;
    }
    return false;
}


// Return number of splits
int split_word(char token, const std::string& word, std::vector<std::string>& splits)
{
    size_t prev = 0; 
    size_t next = 0; 
    splits.clear();

    while (next < word.size())
    {
        next = word.find_first_of(token, prev);
        splits.push_back(word.substr(prev, next));
        prev = next + 1;
    }

    return splits.size();
}


/*
===========
File
===========
*/


File::File(const std::string& filename)
:   fs(filename), filename(filename), wend(0), lineNum(0)
{
    if (!fs.is_open())
    {
        // Attempt to create file
        {
            std::ofstream ofs(filename);
            if (!ofs.good()) log::log("Error creating file: "+filename, log::Level::Error);
        }
        
        fs.open(filename);
        if (!fs.is_open())
            log::log("Error opening file: "+filename, log::Level::Error);
    }
} 

bool File::isOpen()
{
    return fs.is_open();
}


int File::nextWord(std::string& str)
{
    size_t wstart = line.find_first_not_of(' ', wend);
    if (wstart == std::string::npos) return 0;
    wend = line.find_first_of(' ', wstart);
    str = line.substr(wstart, wend - wstart);
    to_lower(str);
    return str.size();
}


int File::nextNumber(int& num)
{
    std::string str;
    if (!nextWord(str)) return 0;

    try
    {
        num = std::stoi(str);
        return str.size();
    }
    catch (const std::invalid_argument& e)
    {
        log::log("File: "+filename+" On line:"+std::to_string(lineNum)+" -- Expected a number:"+str, log::Level::Error);
    }
    return 0;
}


int File::nextLine()
{
    wend = 0;
    std::getline(fs, line);
    ++lineNum;
    return line.size();
}


std::string File::_findKey(const std::string& key)
{
    std::string line;
    while (std::getline(fs, line))
    {
        size_t start = line.find_first_not_of(' ');
        size_t end = line.find_first_of(' ', start);

        if (end == std::string::npos)
            continue;

        if (!line.compare(start, end - start, key))
            return line.substr(end);
    }
    log::log("File: "+filename+" Key not found '"+key+"'", log::Level::Error);
    return "";
}


bool File::readKeyValuePair(const std::string& key, int& value)
{
    std::string val = _findKey(key);
    try
    {
        value = std::stod(val);
        return val.size();
    }
    catch (const std::invalid_argument& e)
    {
        log::log("File: "+filename+" Integer expected for key: '"+key+"'", log::Level::Error);
    }
    return 0;
}


bool File::readKeyValuePair(const std::string& key, float& value)
{
    std::string val = _findKey(key);
    try
    {
        value = std::stof(val);
        return val.size();
    }
    catch (const std::invalid_argument& e)
    {
        log::log("File: "+filename+" Floating point number expected for key: '"+key+"'", log::Level::Error);
    }
    return 0;
}


bool File::readKeyValuePair(const std::string& key, std::string& value)
{
    value = _findKey(key);
    return value.size();
}


/*
===========
SimpleSerializer
===========
*/


SimpleSerializer::SimpleSerializer(const std::string& path)
:   Serializer(path),
    mKeyNames(145),
    mKeyModifierNames(3),
    mMouseComponentNames(11),
    mJoyStickComponentNames(5),
    mKeyboardDeviceNames(5),
    mMouseDeviceNames(3),
    mJoyStickDeviceNames(3)

{
    mKeyNames["escape"] = OIS::KC_ESCAPE;
    mKeyNames["1"] = OIS::KC_1;
    mKeyNames["2"] = OIS::KC_2;
    mKeyNames["3"] = OIS::KC_3;
    mKeyNames["4"] = OIS::KC_4;
    mKeyNames["5"] = OIS::KC_5;
    mKeyNames["6"] = OIS::KC_6;
    mKeyNames["7"] = OIS::KC_7;
    mKeyNames["8"] = OIS::KC_8;
    mKeyNames["9"] = OIS::KC_9;
    mKeyNames["0"] = OIS::KC_0;
    mKeyNames["minus"] = OIS::KC_MINUS;
    mKeyNames["equals"] = OIS::KC_EQUALS;
    mKeyNames["back"] = OIS::KC_BACK;
    mKeyNames["tab"] = OIS::KC_TAB;
    mKeyNames["q"] = OIS::KC_Q;
    mKeyNames["w"] = OIS::KC_W;
    mKeyNames["e"] = OIS::KC_E;
    mKeyNames["r"] = OIS::KC_R;
    mKeyNames["t"] = OIS::KC_T;
    mKeyNames["y"] = OIS::KC_Y;
    mKeyNames["u"] = OIS::KC_U;
    mKeyNames["i"] = OIS::KC_I;
    mKeyNames["o"] = OIS::KC_O;
    mKeyNames["p"] = OIS::KC_P;
    mKeyNames["lbracket"] = OIS::KC_LBRACKET;
    mKeyNames["rbracket"] = OIS::KC_RBRACKET;
    mKeyNames["return"] = OIS::KC_RETURN;
    mKeyNames["lcontrol"] = OIS::KC_LCONTROL;
    mKeyNames["a"] = OIS::KC_A;
    mKeyNames["s"] = OIS::KC_S;
    mKeyNames["d"] = OIS::KC_D;
    mKeyNames["f"] = OIS::KC_F;
    mKeyNames["g"] = OIS::KC_G;
    mKeyNames["h"] = OIS::KC_H;
    mKeyNames["j"] = OIS::KC_J;
    mKeyNames["k"] = OIS::KC_K;
    mKeyNames["l"] = OIS::KC_L;
    mKeyNames["semicolon"] = OIS::KC_SEMICOLON;
    mKeyNames["apostrophe"] = OIS::KC_APOSTROPHE;
    mKeyNames["grave"] = OIS::KC_GRAVE;
    mKeyNames["lshift"] = OIS::KC_LSHIFT;
    mKeyNames["backslash"] = OIS::KC_BACKSLASH;
    mKeyNames["z"] = OIS::KC_Z;
    mKeyNames["x"] = OIS::KC_X;
    mKeyNames["c"] = OIS::KC_C;
    mKeyNames["v"] = OIS::KC_V;
    mKeyNames["b"] = OIS::KC_B;
    mKeyNames["n"] = OIS::KC_N;
    mKeyNames["m"] = OIS::KC_M;
    mKeyNames["comma"] = OIS::KC_COMMA;
    mKeyNames["period"] = OIS::KC_PERIOD;
    mKeyNames["slash"] = OIS::KC_SLASH;
    mKeyNames["rshift"] = OIS::KC_RSHIFT;
    mKeyNames["multiply"] = OIS::KC_MULTIPLY;
    mKeyNames["lalt"] = OIS::KC_LMENU;
    mKeyNames["space"] = OIS::KC_SPACE;
    mKeyNames["capital"] = OIS::KC_CAPITAL;
    mKeyNames["f1"] = OIS::KC_F1;
    mKeyNames["f2"] = OIS::KC_F2;
    mKeyNames["f3"] = OIS::KC_F3;
    mKeyNames["f4"] = OIS::KC_F4;
    mKeyNames["f5"] = OIS::KC_F5;
    mKeyNames["f6"] = OIS::KC_F6;
    mKeyNames["f7"] = OIS::KC_F7;
    mKeyNames["f8"] = OIS::KC_F8;
    mKeyNames["f9"] = OIS::KC_F9;
    mKeyNames["f10"] = OIS::KC_F10;
    mKeyNames["numlock"] = OIS::KC_NUMLOCK;
    mKeyNames["scroll"] = OIS::KC_SCROLL;
    mKeyNames["numpad7"] = OIS::KC_NUMPAD7;
    mKeyNames["numpad8"] = OIS::KC_NUMPAD8;
    mKeyNames["numpad9"] = OIS::KC_NUMPAD9;
    mKeyNames["subtract"] = OIS::KC_SUBTRACT;
    mKeyNames["numpad4"] = OIS::KC_NUMPAD4;
    mKeyNames["numpad5"] = OIS::KC_NUMPAD5;
    mKeyNames["numpad6"] = OIS::KC_NUMPAD6;
    mKeyNames["add"] = OIS::KC_ADD;
    mKeyNames["numpad1"] = OIS::KC_NUMPAD1;
    mKeyNames["numpad2"] = OIS::KC_NUMPAD2;
    mKeyNames["numpad3"] = OIS::KC_NUMPAD3;
    mKeyNames["numpad0"] = OIS::KC_NUMPAD0;
    mKeyNames["decimal"] = OIS::KC_DECIMAL;
    mKeyNames["oem_102"] = OIS::KC_OEM_102;
    mKeyNames["f11"] = OIS::KC_F11;
    mKeyNames["f12"] = OIS::KC_F12;
    mKeyNames["f13"] = OIS::KC_F13;
    mKeyNames["f14"] = OIS::KC_F14;
    mKeyNames["f15"] = OIS::KC_F15;
    mKeyNames["kana"] = OIS::KC_KANA;
    mKeyNames["abnt_c1"] = OIS::KC_ABNT_C1;
    mKeyNames["convert"] = OIS::KC_CONVERT;
    mKeyNames["noconvert"] = OIS::KC_NOCONVERT;
    mKeyNames["yen"] = OIS::KC_YEN;
    mKeyNames["abnt_c2"] = OIS::KC_ABNT_C2;
    mKeyNames["numpadequals"] = OIS::KC_NUMPADEQUALS;
    mKeyNames["prevtrack"] = OIS::KC_PREVTRACK;
    mKeyNames["at"] = OIS::KC_AT;
    mKeyNames["colon"] = OIS::KC_COLON;
    mKeyNames["underline"] = OIS::KC_UNDERLINE;
    mKeyNames["kanji"] = OIS::KC_KANJI;
    mKeyNames["stop"] = OIS::KC_STOP;
    mKeyNames["ax"] = OIS::KC_AX;
    mKeyNames["unlabeled"] = OIS::KC_UNLABELED;
    mKeyNames["nexttrack"] = OIS::KC_NEXTTRACK;
    mKeyNames["numpadenter"] = OIS::KC_NUMPADENTER;
    mKeyNames["rcontrol"] = OIS::KC_RCONTROL;
    mKeyNames["mute"] = OIS::KC_MUTE;
    mKeyNames["calculator"] = OIS::KC_CALCULATOR;
    mKeyNames["playpause"] = OIS::KC_PLAYPAUSE;
    mKeyNames["mediastop"] = OIS::KC_MEDIASTOP;
    mKeyNames["volumedown"] = OIS::KC_VOLUMEDOWN;
    mKeyNames["volumeup"] = OIS::KC_VOLUMEUP;
    mKeyNames["webhome"] = OIS::KC_WEBHOME;
    mKeyNames["numpadcomma"] = OIS::KC_NUMPADCOMMA;

    mKeyNames["divide"] = OIS::KC_DIVIDE;
    mKeyNames["sysrq"] = OIS::KC_SYSRQ;
    mKeyNames["ralt"] = OIS::KC_RMENU;
    mKeyNames["pause"] = OIS::KC_PAUSE;
    mKeyNames["home"] = OIS::KC_HOME;
    mKeyNames["up"] = OIS::KC_UP;
    mKeyNames["pgup"] = OIS::KC_PGUP;
    mKeyNames["left"] = OIS::KC_LEFT;
    mKeyNames["right"] = OIS::KC_RIGHT;
    mKeyNames["end"] = OIS::KC_END;
    mKeyNames["down"] = OIS::KC_DOWN;
    mKeyNames["pgdown"] = OIS::KC_PGDOWN;
    mKeyNames["insert"] = OIS::KC_INSERT;
    mKeyNames["delete"] = OIS::KC_DELETE;
    mKeyNames["lwin"] = OIS::KC_LWIN;
    mKeyNames["rwin"] = OIS::KC_RWIN;
    mKeyNames["apps"] = OIS::KC_APPS;
    mKeyNames["power"] = OIS::KC_POWER;
    mKeyNames["sleep"] = OIS::KC_SLEEP;
    mKeyNames["wake"] = OIS::KC_WAKE;
    mKeyNames["websearch"] = OIS::KC_WEBSEARCH;
    mKeyNames["webfavorites"] = OIS::KC_WEBFAVORITES;
    mKeyNames["webrefresh"] = OIS::KC_WEBREFRESH;
    mKeyNames["webstop"] = OIS::KC_WEBSTOP;
    mKeyNames["webforward"] = OIS::KC_WEBFORWARD;
    mKeyNames["webback"] = OIS::KC_WEBBACK;
    mKeyNames["mycomputer"] = OIS::KC_MYCOMPUTER;
    mKeyNames["mail"] = OIS::KC_MAIL;
    mKeyNames["mediaselect"] = OIS::KC_MEDIASELECT;

    mKeyModifierNames["alt"] = OIS::Keyboard::Alt;
    mKeyModifierNames["ctrl"] = OIS::Keyboard::Ctrl;
    mKeyModifierNames["shift"] = OIS::Keyboard::Shift;

    mMouseComponentNames["left"] = MouseEvent::Component::CPNT_LEFT;
    mMouseComponentNames["right"] = MouseEvent::Component::CPNT_RIGHT;
    mMouseComponentNames["middle"] = MouseEvent::Component::CPNT_MIDDLE;
    mMouseComponentNames["button3"] = MouseEvent::Component::CPNT_BUTTON3;
    mMouseComponentNames["button4"] = MouseEvent::Component::CPNT_BUTTON4;
    mMouseComponentNames["button5"] = MouseEvent::Component::CPNT_BUTTON5;
    mMouseComponentNames["button6"] = MouseEvent::Component::CPNT_BUTTON6;
    mMouseComponentNames["button7"] = MouseEvent::Component::CPNT_BUTTON7;
    mMouseComponentNames["axis_x"] = MouseEvent::Component::CPNT_AXIS_X;
    mMouseComponentNames["axis_y"] = MouseEvent::Component::CPNT_AXIS_Y;
    mMouseComponentNames["axis_z"] = MouseEvent::Component::CPNT_AXIS_Z;

    mJoyStickComponentNames["button"] = OIS::OIS_Button;
    mJoyStickComponentNames["axis"] = OIS::OIS_Axis;
    mJoyStickComponentNames["slider"] = OIS::OIS_Slider;
    mJoyStickComponentNames["pov"] = OIS::OIS_POV;
    mJoyStickComponentNames["vector3"] = OIS::OIS_Vector3;

    mKeyboardDeviceNames.emplace_back("k");
    mKeyboardDeviceNames.emplace_back("kb");
    mKeyboardDeviceNames.emplace_back("key");
    mKeyboardDeviceNames.emplace_back("keyboard");
    
    mMouseDeviceNames.emplace_back("m");
    mMouseDeviceNames.emplace_back("ms");
    mMouseDeviceNames.emplace_back("mouse");
    
    mJoyStickDeviceNames.emplace_back("j");
    mJoyStickDeviceNames.emplace_back("js");
    mJoyStickDeviceNames.emplace_back("joystick");
}


void SimpleSerializer::loadBinding(NamedBindingMap& map)
{
    File fr(mPath+g_map_filename);
    if (!fr.isOpen()) return;
    std::string name, devName;

    while (fr.nextLine())
    {
        if (!fr.nextWord(name)) continue;
        if (!fr.nextWord(devName)) continue;
        
        Bind* b = map.getBinding(name, false);
        
        // Search for any name a device can have
        if (linear_search(mKeyboardDeviceNames, devName)) addKey(b, fr);
        else if (linear_search(mMouseDeviceNames, devName)) addMouse(b, fr);
        else if (linear_search(mJoyStickDeviceNames, devName)) addJoyStick(b, fr);
        else log::log("Invalid device name: "+devName, log::Level::Error);
    }
}


void SimpleSerializer::addKey(Bind* b, File& fr) const
{
    // A space to delimit multiple combination
    std::string word;
    while (fr.nextWord(word))
    {
        unsigned mod = 0;
        unsigned key = 0;
        bool rev = false;

        // Separator for key combination
        std::vector<std::string> splits;
        split_word('+', word, splits);

        for (std::string& keyName : splits)
        {
            if (is_negative(keyName)) rev = true;

            auto kmnIt = mKeyModifierNames.find(keyName);
            if (kmnIt != mKeyModifierNames.end())
            {
                KeyEvent::addModifier(mod, kmnIt->second);
                continue;
            }

            auto knIt = mKeyNames.find(keyName);
            if (knIt != mKeyNames.end())
            {
                // Only one key can be binded
                key = knIt->second;
                continue;
            }

            log::log("Invalid key name:"+keyName, log::Level::Error);
        }

        b->addKeyEvent(KeyEvent::create(key, mod, rev));
    }
}


void SimpleSerializer::addMouse(Bind* b, File& f) const
{
    // A space to delimit multiple combination
    std::string word;
    while (f.nextWord(word))
    {
        bool rev = is_negative(word);

        auto it = mMouseComponentNames.find(word);
        if (it == mMouseComponentNames.end())
        {
            log::log("Invalid mouse event name: "+word, log::Level::Warning);
            continue;
        }

        b->addMouseEvent(MouseEvent::create(it->second, rev));
    }
}


void SimpleSerializer::addJoyStick(Bind* b, File& f) const
{
    // Only one binding per line
    // Format: [joystick number] [component] [component id]

    int joystickNum;
    if (!f.nextNumber(joystickNum))
    {
        log::log("Invalid joystick number\n", log::Level::Warning);
        return;
    }

    std::string componentName;
    if (!f.nextWord(componentName)) 
    {
        log::log("Invalid joystick event: "+componentName, log::Level::Warning);
        return;
    }

    bool rev = is_negative(componentName);

    auto cpntIt = mJoyStickComponentNames.find(componentName);
    if (cpntIt == mJoyStickComponentNames.end())
    {
        log::log("Invalid joystick component: "+componentName, log::Level::Warning);
        return;
    }

    int componentId;
    if (!f.nextNumber(componentId))
    {
        log::log("Invalid joystick component id", log::Level::Warning);
        return;
    }

    b->addJoyStickEvent(JoyStickEvent::create(cpntIt->second, componentId, joystickNum, rev));
}


void SimpleSerializer::saveBinding(const NamedBindingMap& bs)
{
    File f(mPath+g_map_filename);
    for (auto& pair : bs.map)
    {
        const std::string& name = pair.first;
        Bind* b = pair.second;

        for (auto& evt : b->getKeyEvents())
            f << name << " keyboard " << keyEventToString(evt) << '\n';;
        for (auto& evt : b->getMouseEvents())
            f << name << " mouse " << mouseEventToString(evt) << '\n';
        for (auto& evt : b->getJoyStickEvents())
            f << name << " joystick " << joyStickEventToString(evt) << '\n';
    }
}


void SimpleSerializer::doConfig(Handler::Configuration* c, File::KeyValueOperation oper)
{
    File file(mPath+g_conf_filename);
    file.keyValuePair("mouse_sensivity_axis_x", c->mouseSensivityAxisX, oper);
    file.keyValuePair("mouse_sensivity_axis_y", c->mouseSensivityAxisY, oper);
    file.keyValuePair("mouse_sensivity_axis_z", c->mouseSensivityAxisZ, oper);
}


void SimpleSerializer::loadConfig(Handler::Configuration* c)
{
    doConfig(c, File::KeyValueOperation::KVO_Read);
}


void SimpleSerializer::saveConfig(Handler::Configuration* c)
{
    doConfig(c, File::KeyValueOperation::KVO_Write);
}


std::string SimpleSerializer::keyEventToString(const InputEvent::Type& evt)
{
    std::stringstream ss;
    if (InputEvent::getReverse(evt)) ss << '-';

    unsigned mod = KeyEvent::getModifier(evt);
    if (mod & OIS::Keyboard::Alt) ss << enum_to_string(mKeyModifierNames, OIS::Keyboard::Alt) << '+';
    if (mod & OIS::Keyboard::Ctrl) ss << enum_to_string(mKeyModifierNames, OIS::Keyboard::Ctrl) << '+';
    if (mod & OIS::Keyboard::Shift) ss << enum_to_string(mKeyModifierNames, OIS::Keyboard::Shift) << '+';

    ss << enum_to_string(mKeyNames, KeyEvent::getKey(evt));

    return ss.str();
}


std::string SimpleSerializer::mouseEventToString(const InputEvent::Type& evt)
{
    std::stringstream ss;

    if (InputEvent::getReverse(evt)) ss << '-';
    ss << enum_to_string(mMouseComponentNames, MouseEvent::getComponent(evt));

    return ss.str();
}


std::string SimpleSerializer::joyStickEventToString(const InputEvent::Type& evt)
{
    std::stringstream ss;

    ss << JoyStickEvent::getJoystickNumber(evt) << ' ';
    if (InputEvent::getReverse(evt)) ss << '-';
    ss << enum_to_string(mJoyStickComponentNames, JoyStickEvent::getComponent(evt)) << ' ';
    ss << JoyStickEvent::getComponentId(evt);

    return ss.str();
}
