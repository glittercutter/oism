// Licensed under the zlib License
// Copyright (C) 2012 Sebastien Raymond

#pragma once 

#include <OISEvents.h>
#include <OISInputManager.h>
#include <OISJoyStick.h>
#include <OISKeyboard.h>
#include <OISMouse.h>

#include <array>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

// NOTE: Bit shifting assume at least 32bits, little-endian machine
static_assert(sizeof(unsigned) >= 4, "unsigned type need to be at least 4 bytes wide");

namespace oism
{

// Forward decl
class Bind;
class Handler;
class Serializer;


struct InputEvent
{
    typedef unsigned Type;
    static const unsigned FlagByte = 1;
    static const unsigned char ReverseFlag = 1;

    static bool getReverse(Type evt)
    {
        return getByte(evt, FlagByte) & ReverseFlag;
    }

    static void setReverse(Type& evt, bool rev)
    {
        unsigned flags = getByte(evt, FlagByte);
        if (rev) flags |= ReverseFlag;
        else flags &= ~ReverseFlag;
        setByte(evt, flags, FlagByte);
    }

    static Type getByte(Type evt, unsigned num)
    {
        unsigned ls = 8 * (num - 1);
        unsigned rs = 8 * 3;
        evt <<= ls;
        evt >>= rs;
        return evt;
    }

    static void setByte(Type& evt, unsigned data, unsigned num)
    {
        unsigned char* cevt = (unsigned char*)&evt;
        cevt += num - 1;
        *cevt = *((unsigned char*)&data);
    }
};


typedef std::deque<Bind*> BindingList;
typedef std::unordered_map<InputEvent::Type, BindingList> InputEventBindingListMap;


struct DefaultEvent
{
    DefaultEvent& addKey(InputEvent::Type evt)
    {
        keyEvents.push_back(evt);
        return *this;
    }

    DefaultEvent& addMouseButton(InputEvent::Type evt)
    {
        mouseEvents.push_back(evt);
        return *this;
    }

    DefaultEvent& addJoyStickEvent(InputEvent::Type evt)
    {
        joyStickEvents.push_back(evt);
        return *this;
    }
    
    bool isEmpty() const
    {
        return keyEvents.empty() &&
               mouseEvents.empty() &&
               joyStickEvents.empty();
    }

    std::list<InputEvent::Type> keyEvents;
    std::list<InputEvent::Type> mouseEvents;
    std::list<InputEvent::Type> joyStickEvents;
};


struct KeyEvent : public InputEvent
{
    static const unsigned ModifierByte = 3; // Modifier on third byte
    static const unsigned KeyByte = 4; // Key on fourth byte

    static InputEvent::Type create(unsigned key, unsigned mod, bool rev)
    {
        Type evt = 0;
        setKey(evt, key);
        setModifier(evt, mod);
        setReverse(evt, rev);
        return evt;
    }

    // NOTE: Implemented in 'OISMHandlerUgly.cpp'
    static Type create2(const OIS::KeyEvent& evt, OIS::Keyboard* kb, bool rev = false);

    static void setKey(InputEvent::Type& evt, unsigned key)
    {
        setByte(evt, key, KeyByte);
    }

    static unsigned getKey(InputEvent::Type evt)
    {
        return getByte(evt, KeyByte);
    }

    static void setModifier(InputEvent::Type& evt, unsigned mod)
    {
        setByte(evt, mod, ModifierByte);
    }

    static unsigned getModifier(InputEvent::Type evt)
    {
        return getByte(evt, ModifierByte);
    }

    static void addModifier(unsigned& mod, unsigned newMod)
    {
        mod |= newMod;;
    }
};


struct MouseEvent : public InputEvent
{
    // Component on the fourth byte
    static const unsigned ComponentByte = 4; // Component on fourth byte

    static Type create(unsigned component, bool rev = false)
    {
        Type evt = 0;
        setComponent(evt, component);
        setReverse(evt, rev);
        return evt;
    }

    static void setComponent(Type& evt, unsigned component)
    {
        setByte(evt, component, ComponentByte);
    }

    static unsigned getComponent(Type evt)
    {
        return getByte(evt, ComponentByte);
    }

    enum Component
    {
        CPNT_LEFT = OIS::MB_Left,
        CPNT_RIGHT = OIS::MB_Right,
        CPNT_MIDDLE = OIS::MB_Middle,
        CPNT_BUTTON3 = OIS::MB_Button3,
        CPNT_BUTTON4 = OIS::MB_Button4,
        CPNT_BUTTON5 = OIS::MB_Button5,
        CPNT_BUTTON6 = OIS::MB_Button6,
        CPNT_BUTTON7 = OIS::MB_Button7,
        CPNT_BUTTON_COUNT,

        CPNT_AXIS_X,
        CPNT_AXIS_Y,
    };
};


struct JoyStickEvent : public InputEvent
{
    static const unsigned JoystickNumberByte = 2; // Joystick number on second byte
    static const unsigned ComponentByte = 3; // Component on third byte
    static const unsigned ComponentIdByte = 4; // ComponentId on fourth byte

    static Type create(unsigned component, unsigned componentId, unsigned joystickNum, bool rev = false)
    {
        Type evt = 0;
        setJoystickNumber(evt, joystickNum);
        setComponent(evt, component);
        setComponentId(evt, componentId);
        setReverse(evt, rev);
        return evt;
    }
    
    static void setJoystickNumber(Type& evt, unsigned num)
    {
        setByte(evt, num, JoystickNumberByte);
    }

    static unsigned getJoystickNumber(Type evt)
    { 
        return getByte(evt, JoystickNumberByte);
    }

    static void setComponent(Type& evt, unsigned component)
    {
        setByte(evt, component, ComponentByte);
    }

    static unsigned getComponent(Type evt)
    {
        return getByte(evt, ComponentByte);
    }

    static void setComponentId(Type& evt, unsigned id)
    {
        setByte(evt, id, ComponentIdByte);
    }

    static unsigned getComponentId(Type evt)
    {
        return getByte(evt, ComponentIdByte);
    }

    static float getPovDirectionValue(unsigned dir, unsigned componentId)
    {
        return componentId & 1 ?
            directionToHorizontal(dir) : 
            directionToVertical(dir);
    }

    static float directionToHorizontal(unsigned dir)
    { 
        if (dir & OIS::Pov::West) return -1.f;
        if (dir & OIS::Pov::East) return 1.f;
        return 0.f;
    }

    static float directionToVertical(unsigned dir)
    {
        if (dir & OIS::Pov::South) return -1.f;
        if (dir & OIS::Pov::North) return 1.f;
        return 0.f;
    }

    static float clipOne(float v)
    {
        if (v >= 1.f) return 1.f;
        if (v <= -1.f) return -1.f;
        return v;
    }

    static float normalizeAxisValue(float value)
    {
        return clipOne(value / (float)OIS::JoyStick::MAX_AXIS);
    }
};


class JoyStickListener : public OIS::JoyStickListener
{
public:
    JoyStickListener(Handler* handler, int id)
    :   mHandler(handler), mID(id) {}

    void addListener(std::weak_ptr<OIS::JoyStickListener>);
    unsigned getID() { return mID; }

protected:
	bool buttonPressed(const OIS::JoyStickEvent& evt, int button);
	bool buttonReleased(const OIS::JoyStickEvent& evt, int button);
	bool axisMoved(const OIS::JoyStickEvent& evt, int axis);
	bool povMoved(const OIS::JoyStickEvent& evt, int idx);
    
    Handler* mHandler;
    int mID;
    std::list<std::weak_ptr<OIS::JoyStickListener>> mListeners;
};


class Bind
{
friend class Handler;

public:
    typedef std::function<void()> Callback;
    typedef std::shared_ptr<Callback> CallbackSharedPtr;
    typedef std::weak_ptr<Callback> CallbackWeakPtr;
    typedef std::list<CallbackWeakPtr> CallbackWeakPtrList;

    enum CallbackType
    {
        CT_ON_POSITIVE = 0,
        CT_ON_CENTER,
        CT_ON_NEGATIVE,
        CT_COUNT
    };

    Bind() : mValue(0.f) {}
    Bind(const DefaultEvent& def)
    :   mKeyEvents(def.keyEvents),
        mMouseEvents(def.mouseEvents),
        mJoyStickEvents(def.joyStickEvents),
        mValue(0.f)
    {}

    const CallbackSharedPtr& addCallback(unsigned callbackType, const CallbackSharedPtr& cb);
    float getValue() const { return mValue; }

    // Must sync modification with 'Handler::_buildBindingListMaps()'
    void addKeyEvent(InputEvent::Type evt);
    void addMouseEvent(InputEvent::Type evt);
    void addJoyStickEvent(InputEvent::Type evt);
    void removeKeyEvent(InputEvent::Type evt);
    void removeMouseEvent(InputEvent::Type evt);
    void removeJoyStickEvent(InputEvent::Type evt);

    // Return farthest value from zero -/+
    float _getMaxValue() const;

protected:
    // Used by friend class 'Handler'
    void setValue(float);
    std::list<InputEvent::Type> mKeyEvents;
    std::list<InputEvent::Type> mMouseEvents;
    std::list<InputEvent::Type> mJoyStickEvents;

private:
    void doCallback(unsigned callbackType);
    void clipValue();

    std::array<CallbackWeakPtrList, CT_COUNT> mCallbacks;
    float mValue;
};


struct NamedBindingMap
{
    Bind* getBinding(const std::string& name)
    {
        auto it = map.find(name);
        if (it == map.end()) it = map.insert(std::make_pair(name, new Bind())).first;
        return it->second;
    }

    void clear() { map.clear(); }

    std::unordered_map<std::string, Bind*> map;
};


class Handler : public OIS::MouseListener, public OIS::KeyListener
{
friend class JoyStickListener;

public:
    static Handler& getInstance();

    template<typename Serializer_t>
    void init(unsigned long hWnd, const std::string& filename, bool exclusive = true)
    {
        mHWnd = hWnd;
        mSerializer = new Serializer_t(filename);
        _loadBinding();
        _loadConfig();
        _buildBindingListMaps();
        createOIS(exclusive);
    }

    virtual ~Handler();

    void registerKeyListener(std::weak_ptr<OIS::KeyListener>);
    void registerMouseListener(std::weak_ptr<OIS::MouseListener>);
    void registerJoyStickListener(std::weak_ptr<OIS::JoyStickListener>, int id);

    OIS::Keyboard* getKeyboard() { return mKeyboard; }
    OIS::Mouse* getMouse() { return mMouse; }
    float getJoyStickValue(unsigned int) const;

    void update();
    Bind* getBinding(const std::string& name, const DefaultEvent& def = DefaultEvent());
    void setMouseLimit(int w, int h);
    void setExclusive(bool state = true);

    void _loadBinding();
    void _saveBinding();
    void _loadConfig();
    void _saveConfig();
    void _buildBindingListMaps(); // Sync

    static std::string convertOISDeviceTypeToString(OIS::Type type);

    struct Configuration
    {
        Configuration()
        :   // Assign default values
            mouseSensivityAxisX(0.2),
            mouseSensivityAxisY(0.2)
        {}

        float mouseSensivityAxisX;
        float mouseSensivityAxisY;
        std::vector<float> joystickDeadZones;
    };

protected:
    Handler() {}
    Handler(const Handler&); // No copy

    void createOIS(bool exclusive = true);
    void destroyOIS();

    // Implement OIS::MouseListener
    bool mouseMoved(const OIS::MouseEvent& evt);
    bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id);
    // Implement OIS::KeyListener
    bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id);
    bool keyPressed(const OIS::KeyEvent& evt);
    bool keyReleased(const OIS::KeyEvent& evt);

    // Callback from Input::JoyStickListener
    void buttonPressed(unsigned button, JoyStickListener* lnr);
    void buttonReleased(unsigned button, JoyStickListener* lnr);
    void axisMoved(unsigned axis, float value, JoyStickListener* lnr);
    void povMoved(unsigned idx, unsigned direction, JoyStickListener* lnr);

    InputEventBindingListMap mKeyEvents;
    InputEventBindingListMap mMouseEvents;
    InputEventBindingListMap mJoyStickEvents;
    NamedBindingMap mBindings;

    OIS::InputManager* mOIS;

    OIS::Mouse* mMouse;
    OIS::Keyboard* mKeyboard;
    std::vector<std::pair<OIS::JoyStick*, JoyStickListener*>> mJoySticks;
    std::deque<std::weak_ptr<OIS::KeyListener>> mKeyListeners;
    std::deque<std::weak_ptr<OIS::MouseListener>> mMouseListeners;

    Serializer* mSerializer;
    Configuration mConfig;

    unsigned long mHWnd;
};


class Serializer
{
public:
    Serializer(const std::string& path)
    :   mPath(path) {}
    virtual ~Serializer() {}

    virtual void loadBinding(NamedBindingMap&) = 0;
    virtual void loadConfig(Handler::Configuration&) = 0;
    virtual void saveBinding(const NamedBindingMap&) = 0;
    virtual void saveConfig(const Handler::Configuration&) = 0;

protected:
    std::string mPath;
};


} /* oism */


#define OISM_NEW_CALLBACK(_NAME, _TYPE, _CALLBACK) \
    oism::Handler::getInstance(). \
        getBinding(_NAME)->addCallback( \
        oism::Bind::_TYPE, \
        oism::Bind::CallbackSharedPtr( \
        new oism::Bind::Callback(_CALLBACK)))
