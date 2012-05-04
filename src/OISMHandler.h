// Licensed under the zlib License
// Copyright (C) 2012 Sebastien Raymond

#pragma once 

#include "mm/cache_map.hpp"
#include "OISMfwdcl.h"

#include <OISEvents.h>
#include <OISInputManager.h>
#include <OISJoyStick.h>
#include <OISKeyboard.h>
#include <OISMouse.h>

#include <array>
#include <deque>
#include <functional>
#include <queue>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <unordered_map>


// NOTE: Bit shifting assume at least 32bits, little-endian machine
static_assert(sizeof(unsigned) >= 4, "unsigned type need to be at least 4 bytes wide");


namespace oism
{


namespace log
{

enum class Level
{
    Info,
    Warning,
    Error
};

typedef std::function<void(const std::string&, Level)> Func_t;
extern Func_t g_Func;

/// Set logger
inline void set(const Func_t& func) { g_Func = func; }

#ifdef OISM_ENABLE_LOG
/// Safe to use on empty function + default parameter
inline void log(const std::string& msg, Level lvl = Level::Info)
{
    if (g_Func)
        g_Func(msg,lvl);
}
#else // OISM_ENABLE_LOG
inline void log(const std::string& msg, Level lvl = Level::Info) {}
#endif // OISM_ENABLE_LOG

inline std::string to_string(Level lvl)
{
    switch (lvl)
    {
        case Level::Info: return "";
        case Level::Warning: return "WARNING | ";
        case Level::Error: return "== ERROR == | ";
    }
    return "";
}

} // namespace log


class NonCopyable
{
protected:
    NonCopyable() {}
    ~NonCopyable() {}
private:
    NonCopyable & operator=(const NonCopyable&);
    NonCopyable(const NonCopyable&);
};


struct InputEvent
{
    // Byte number start from the right.
    typedef unsigned Type;
    static const unsigned FlagByte = 1;
    static const unsigned char ReverseFlag = 1 << 0;

    static bool getReverse(const Type& evt)
    {
        return getByte(evt, FlagByte) & ReverseFlag;
    }
    static void setReverse(Type& evt, bool rev = true)
    {
        unsigned flags = getByte(evt, FlagByte);
        if (rev) flags |= ReverseFlag;
        else flags &= ~ReverseFlag;
        setByte(evt, flags, FlagByte);
    }

    static inline unsigned char getByte(Type evt, unsigned num)
    {
        return ((unsigned char*)&evt)[4 - num];
    }
    static inline void setByte(Type& evt, unsigned char data, unsigned num)
    {
        ((unsigned char*)&evt)[4 - num] = data;
    }
};


struct KeyEvent : public InputEvent
{
    static const unsigned ModifierByte = 3;
    static const unsigned KeyByte = 4;

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
    static const unsigned ComponentByte = 4;

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
    static const unsigned JoystickNumberByte = 2;
    static const unsigned ComponentByte = 3;
    static const unsigned ComponentIdByte = 4;

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


struct DefaultEvent
{
    // Thoses can be chained
    DefaultEvent& key(InputEvent::Type evt)
    {
        keyEvents.push_back(evt);
        return *this;
    }
    DefaultEvent& mouse(InputEvent::Type evt)
    {
        mouseEvents.push_back(evt);
        return *this;
    }
    DefaultEvent& joyStick(InputEvent::Type evt)
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


class JoyStickListener : public OIS::JoyStickListener
{
public:
    JoyStickListener(Handler* handler, int id)
    :   mHandler(handler), mId(id) {}

    void addListener(OIS::JoyStickListener*);
    void removeListener(OIS::JoyStickListener*);
    unsigned getId() { return mId; }

    const std::set<OIS::JoyStickListener*>& _getListeners() { return mListeners; }
    void _setListeners(const std::set<OIS::JoyStickListener*>& lnr) { mListeners = lnr; }

protected:
    bool buttonPressed(const OIS::JoyStickEvent& evt, int button);
    bool buttonReleased(const OIS::JoyStickEvent& evt, int button);
    bool axisMoved(const OIS::JoyStickEvent& evt, int axis);
    bool povMoved(const OIS::JoyStickEvent& evt, int idx);

    Handler* mHandler;
    int mId;
    std::set<OIS::JoyStickListener*> mListeners;
};


class Bind
{
friend class Handler;

public:
    typedef std::function<void()> Callback;
    typedef std::shared_ptr<Callback> CallbackSharedPtr;
    typedef std::weak_ptr<Callback> CallbackWeakPtr;
    typedef std::list<CallbackWeakPtr> CallbackWeakPtrList;
    typedef std::list<InputEvent::Type> InputEventList;

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
    inline float getValue() const { return mValue; }

    /// @name Modifiers
    /// Apply change by calling 'Handler::_buildBindingListMaps()'.
    ///@{
    void addKeyEvent(InputEvent::Type evt);
    void addMouseEvent(InputEvent::Type evt);
    void addJoyStickEvent(InputEvent::Type evt);
    void removeKeyEvent(InputEvent::Type evt);
    void removeMouseEvent(InputEvent::Type evt);
    void removeJoyStickEvent(InputEvent::Type evt);
    ///@}

    const InputEventList& getKeyEvents() {return mKeyEvents;}
    const InputEventList& getMouseEvents() {return mMouseEvents;}
    const InputEventList& getJoyStickEvents() {return mJoyStickEvents;}

    float _getMaxValue(Handler*) const; //>! Return farthest value from zero

protected:
    void setValue(Handler*, float); //>! Used by friend class 'Handler'
    InputEventList mKeyEvents;
    InputEventList mMouseEvents;
    InputEventList mJoyStickEvents;

private:
    void doCallback(unsigned callbackType);
    void clipValue();

    std::array<CallbackWeakPtrList, CT_COUNT> mCallbacks;
    float mValue;
};


struct NamedBindingMap
{
    Bind* getBinding(const std::string& name, bool forUse = true);
    void clear() { map.clear(); }

    std::unordered_map<std::string, Bind*> map;
};


class Handler : NonCopyable, public OIS::MouseListener, public OIS::KeyListener
{
friend class JoyStickListener;

public:
    Handler(unsigned long windowID, bool exclusive = true);
    virtual ~Handler();

    template<typename Serializer_t>
    void load(const std::string& path)
    {
        Serializer_t s(path);
        _loadBinding(s);
        _loadConfig(s);
        _buildBindingListMaps();
    }

    template<typename Serializer_t>
    void save(const std::string& path)
    {
        Serializer_t s(path);
        _saveBinding(s);
        _saveConfig(s);
    }

    void setMouseLimit(int w, int h);
    void setExclusive(bool exclusive = true);
    bool isExclusive() const { return mIsExclusive; }

    void update();
    Bind::CallbackSharedPtr callback(const std::string& name, const Bind::Callback& cb, unsigned type = Bind::CT_ON_POSITIVE);
    Bind* getBinding(const std::string& name, bool forUse = true);

    void addKeyListener(OIS::KeyListener*);
    void removeKeyListener(OIS::KeyListener*);
    void addMouseListener(OIS::MouseListener*);
    void removeMouseListener(OIS::MouseListener*);
    void addJoyStickListener(OIS::JoyStickListener*, int id);
    void removeJoyStickListener(OIS::JoyStickListener*);

    OIS::Keyboard* getKeyboard() { return mKeyboard; }
    OIS::Mouse* getMouse() { return mMouse; }
    float getJoyStickValue(unsigned int) const;

    void _buildBindingListMaps(); //>! Apply change made to bindings

    static std::string convertOISDeviceTypeToString(OIS::Type type);

    struct Configuration
    {
        Configuration()
        :   mouseSensivityAxisX(0.2f),
            mouseSensivityAxisY(0.2f),
            mouseSmoothing(0.1f)
        {}

        float mouseSensivityAxisX;
        float mouseSensivityAxisY;
        float mouseSmoothing;
        std::vector<float> joystickDeadZones;
    };

protected:
    typedef std::deque<Bind*> BindingList;
    typedef mm::cache_map<InputEvent::Type, BindingList> InputEventBindingListMap;

    void createOIS(bool exclusive = true);
    void destroyOIS();

    void setBindingValue(InputEventBindingListMap& bindings, unsigned evt, float value);
    void setMouseValue(unsigned cnpt, float value);
    void smoothMouseCleanup();
    void smoothMouse(float& curr, float& old);
    void setKeyboardValue(const OIS::KeyEvent& key, float value);
    void setJoyStickValue(OIS::ComponentType cpntType, unsigned cpnt, JoyStickListener* lnr, float value);

    void _loadBinding(Serializer&);
    void _saveBinding(Serializer&);
    void _loadConfig(Serializer&);
    void _saveConfig(Serializer&);

    void processInternalCallback();
    void _setExclusive(bool exclusive); //!< Internal callback

    /// @name OIS::MouseListener
    //@{
    bool mouseMoved(const OIS::MouseEvent& evt);
    bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id);
    //@}

    /// @name OIS::KeyListener
    //@{
    bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id);
    bool keyPressed(const OIS::KeyEvent& evt);
    bool keyReleased(const OIS::KeyEvent& evt);
    //@}


    /// @name Callback from oism::JoyStickListener
    //@{
    void buttonPressed(unsigned button, JoyStickListener* lnr);
    void buttonReleased(unsigned button, JoyStickListener* lnr);
    void axisMoved(unsigned axis, float value, JoyStickListener* lnr);
    void povMoved(unsigned idx, unsigned direction, JoyStickListener* lnr);
    //@}

    InputEventBindingListMap mKeyEvents;
    InputEventBindingListMap mMouseEvents;
    InputEventBindingListMap mJoyStickEvents;
    NamedBindingMap mBindings;

    OIS::InputManager* mOIS;
    OIS::Mouse* mMouse;
    OIS::Keyboard* mKeyboard;
    std::vector<std::pair<OIS::JoyStick*, JoyStickListener*>> mJoySticks;
    std::unordered_set<OIS::KeyListener*> mKeyListeners;
    std::unordered_set<OIS::MouseListener*> mMouseListeners;

    Configuration mConfig;

    float mMouseSmoothLastX, mMouseSmoothLastY;
    bool mMouseSmoothUpdatedX, mMouseSmoothUpdatedY;

    unsigned long mWindowID;
    bool mIsExclusive;
    std::queue<std::function<void()>> mInternalCallbacks;
};


//! Helper container keeping callbacks alive
class CallbackList
{
public:
    CallbackList(Handler* h) : mHandler(h) {}

    void add(const std::string& name, const Bind::Callback& cb)
        {mCallbacks.insert(std::make_pair(name,mHandler->callback(name,cb)));}
    void remove(const std::string& name)
        {mCallbacks.erase(name);}

    std::unordered_map<std::string,Bind::CallbackSharedPtr> mCallbacks;
    Handler* mHandler;
};


class Serializer
{
public:
    Serializer(const std::string& path)
    :   mPath(path) {}
    virtual ~Serializer() {}

    virtual void loadBinding(NamedBindingMap&) = 0;
    virtual void loadConfig(Handler::Configuration*) = 0;
    virtual void saveBinding(const NamedBindingMap&) = 0;
    virtual void saveConfig(Handler::Configuration*) = 0;

protected:
    std::string mPath;
};


} // namespace oism
