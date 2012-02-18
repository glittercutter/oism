#include "OISMHandler.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>

using namespace oism;


#define SET_BINDING_VALUE(_MAP, _KEY, _VAL) \
{ \
    auto it = _MAP.find(_KEY); \
    if (it != _MAP.end()) \
        for (auto jt = it->second.begin(); jt != it->second.end(); jt++) \
            (*jt)->setValue(_VAL); \
}


/*
=====================
Bind
=====================
*/


void Bind::setValue(float val)
{
    float oldVal = mValue;
    mValue = val;

    float high = _getMaxValue();
    if (fabs(high) > fabs(mValue)) mValue = high;

    if (oldVal == 0.f)
    {
        if (mValue > 0.f) doCallback(CT_ON_POSITIVE);
        if (mValue < 0.f) doCallback(CT_ON_NEGATIVE);
    }
    else if (mValue == 0.f) doCallback(CT_ON_CENTER);
}


void Bind::doCallback(unsigned callbackType)
{
    CallbackWeakPtrList& ls = mCallbacks[callbackType];
    
    for (auto it = ls.begin(); it != ls.end(); it++)
    {
        std::shared_ptr<Callback> cb = it->lock();
        if (cb.get()) (*cb.get())();
        else it = ls.erase(it);
    }
}


const Bind::CallbackSharedPtr& Bind::addCallback(unsigned callbackType, const CallbackSharedPtr& cb)
{
    mCallbacks[callbackType].push_back(CallbackWeakPtr(cb));
    return cb;
}


void Bind::addKeyEvent(InputEvent::Type evt)
{
    mKeyEvents.push_back(evt);
}


void Bind::addMouseEvent(InputEvent::Type evt)
{
    mMouseEvents.push_back(evt);
}


void Bind::addJoyStickEvent(InputEvent::Type evt)
{
    mJoyStickEvents.push_back(evt);
}


void Bind::removeKeyEvent(InputEvent::Type evt)
{
    auto it = std::find(mKeyEvents.begin(), mKeyEvents.end(), evt);
    if (it != mKeyEvents.end()) mKeyEvents.erase(it);
}


void Bind::removeMouseEvent(InputEvent::Type evt)
{
    auto it = std::find(mMouseEvents.begin(), mMouseEvents.end(), evt);
    if (it != mMouseEvents.end()) mMouseEvents.erase(it);
}


void Bind::removeJoyStickEvent(InputEvent::Type evt)
{
    auto it = std::find(mJoyStickEvents.begin(), mJoyStickEvents.end(), evt);
    if (it != mJoyStickEvents.end()) mJoyStickEvents.erase(it);
}


float Bind::_getMaxValue() const
{
    float high = 0.f;
    Handler& handler = Handler::getInstance();
    auto kb = handler.getKeyboard();
    auto mouse = handler.getMouse();

    // Key on/off
    for (auto evt : mKeyEvents)
    {
        if (kb->isKeyDown((OIS::KeyCode)KeyEvent::getKey(evt)))
        {
            if (KeyEvent::getReverse(evt)) high = -1.f;
            else high = 1.f;
        }
    }

    for (auto& evt : mMouseEvents)
    {
        unsigned component = MouseEvent::getComponent(evt);
        if (component < MouseEvent::CPNT_BUTTON_COUNT)
        {
            // Mouse button on/off
            if (mouse->getMouseState().buttonDown((OIS::MouseButtonID)component))
            {
                if (MouseEvent::getReverse(evt)) high = -1.f;
                else high = 1.f;
            }
        }
        else
        {
            // Mouse axis

        }
    }

    // Joystick
    for (auto& evt : mJoyStickEvents)
        if (float tmp = handler.getJoyStickValue(evt))
           if (std::fabs(tmp) > std::fabs(high)) high = tmp;

    return high;
}


/*
=====================
JoyStickListener
=====================
*/


void JoyStickListener::addListener(std::weak_ptr<OIS::JoyStickListener> lnr)
{
    mListeners.push_back(lnr);
}


bool JoyStickListener::buttonPressed(const OIS::JoyStickEvent& evt, int button)
{
    mHandler->buttonPressed(button, this);

    for (auto it = mListeners.begin(); it != mListeners.end(); it++)
    {
        std::shared_ptr<OIS::JoyStickListener> lnr = it->lock();
        if (!lnr.get()) it = mListeners.erase(it); // Listener no longer exist
        else lnr->buttonPressed(evt, button);
    }

    return true;
}


bool JoyStickListener::buttonReleased(const OIS::JoyStickEvent& evt, int button)
{
    mHandler->buttonReleased(button, this);

    for (auto it = mListeners.begin(); it != mListeners.end(); it++)
    {
        std::shared_ptr<OIS::JoyStickListener> lnr = it->lock();
        if (!lnr.get()) it = mListeners.erase(it); // Listener no longer exist
        else lnr->buttonReleased(evt, button);
    }

    return true;
}


bool JoyStickListener::axisMoved(const OIS::JoyStickEvent& evt, int axis)
{
    mHandler->axisMoved(
        axis,
        JoyStickEvent::normalizeAxisValue(evt.state.mAxes[axis].abs),
        this);

    for (auto it = mListeners.begin(); it != mListeners.end(); it++)
    {
        std::shared_ptr<OIS::JoyStickListener> lnr = it->lock();
        if (!lnr.get()) it = mListeners.erase(it); // Listener no longer exist
        else lnr->axisMoved(evt, axis);
    }
    
    return true;
}


bool JoyStickListener::povMoved(const OIS::JoyStickEvent& evt, int idx)
{
    mHandler->povMoved(idx, evt.state.mPOV[idx].direction, this);

    for (auto it = mListeners.begin(); it != mListeners.end(); it++)
    {
        std::shared_ptr<OIS::JoyStickListener> lnr = it->lock();
        if (!lnr.get()) it = mListeners.erase(it); // Listener no longer exist
        else lnr->povMoved(evt, idx);
    }
    
    return true;
}


/*
=====================
Handler
=====================
*/


Handler& Handler::getInstance()
{
    static Handler* instance = new Handler();
    return *instance;
}

Handler::~Handler()
{
    _saveBinding();
    _saveConfig();
    destroyOIS();
    delete mSerializer;
}


void Handler::update()
{
    // Reset mouse value
    SET_BINDING_VALUE(mMouseEvents, MouseEvent::CPNT_AXIS_X, 0);
    SET_BINDING_VALUE(mMouseEvents, MouseEvent::CPNT_AXIS_Y, 0);

    mMouse->capture();
    mKeyboard->capture();
    for (auto& pair : mJoySticks) pair.first->capture();
}


void Handler::setExclusive(bool state/* = true*/)
{
    destroyOIS();
    createOIS(state);
}


void Handler::createOIS(bool exclusive/* = true*/)
{
    OIS::ParamList pl;
    std::stringstream ss; ss << mHWnd;
    pl.insert(OIS::ParamList::value_type("WINDOW", ss.str()));
    
    if (!exclusive)
    {
#if defined OIS_WIN32_PLATFORM
        pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND")));
        pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_NONEXCLUSIVE")));
        pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_FOREGROUND")));
        pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_NONEXCLUSIVE")));
#elif defined OIS_LINUX_PLATFORM
        pl.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
        pl.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
        pl.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("false")));
        pl.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));
#endif
    }

    mOIS = OIS::InputManager::createInputSystem(pl);
    lprintln(mOIS->inputSystemName());

    lprintln("List of available device:")
    for (auto& dev : mOIS->listFreeDevices())
        lprintln("  - "<<convertOISDeviceTypeToString(dev.first)<<" -- "<<dev.second);

    // Create devices

    if (mOIS->getNumberOfDevices(OIS::OISMouse))
    {
        mMouse = static_cast<OIS::Mouse*>(mOIS->createInputObject(OIS::OISMouse, true));
        mMouse->setEventCallback(this);
    }

    if (mOIS->getNumberOfDevices(OIS::OISKeyboard))
    {
        mKeyboard = static_cast<OIS::Keyboard*>(mOIS->createInputObject(OIS::OISKeyboard, true));
        mKeyboard->setEventCallback(this);
    }

    unsigned numJoystick = mOIS->getNumberOfDevices(OIS::OISJoyStick);
    if (numJoystick) lprintln("Creating joystick:");

    for (int i = 0; i < mOIS->getNumberOfDevices(OIS::OISJoyStick); i++)
    {
        mJoySticks.push_back(std::make_pair(
            static_cast<OIS::JoyStick*>(mOIS->createInputObject(OIS::OISJoyStick, true)),
            new JoyStickListener(this, i)));

        auto js = mJoySticks.back().first; 
        js->setEventCallback(mJoySticks.back().second);

        // List specs
        lprintln("  - Joystick "<<i<<" -- Vendor: "<<js->vendor());
        lprintln("    - Unknown: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Unknown));
        lprintln("    - Button: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Button));
        lprintln("    - Axis: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Axis));
        lprintln("    - Slider: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Slider));
        lprintln("    - POV: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_POV));
        lprintln("    - Movement capture: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Vector3));
    }
}


void Handler::destroyOIS()
{
    mOIS->destroyInputObject(mMouse);
    mOIS->destroyInputObject(mKeyboard);

    for (auto it = mJoySticks.begin(); it != mJoySticks.end(); it++)
    {
        mOIS->destroyInputObject(it->first); // JoyStick
        delete it->second; // JoyStickListener
    }

    OIS::InputManager::destroyInputSystem(mOIS);
}


void Handler::setMouseLimit(int w, int h)
{
	mMouse->getMouseState().width = w;
	mMouse->getMouseState().height = h;
}


void Handler::_loadBinding()
{
    mBindings.clear();
    mSerializer->loadBinding(mBindings);
}


void Handler::_saveBinding()
{
    mSerializer->saveBinding(mBindings);
}


void Handler::_loadConfig()
{
    mSerializer->loadConfig(mConfig);
}


void Handler::_saveConfig()
{
    mSerializer->saveConfig(mConfig);
}


void Handler::_buildBindingListMaps()
{
    mKeyEvents.clear();
    mMouseEvents.clear();
    mJoyStickEvents.clear();

    for (auto& pair : mBindings.map)
    {
        Bind* b = pair.second;
        for (auto evt : b->mKeyEvents)
            mKeyEvents[evt].push_back(b);

        for (auto evt : b->mMouseEvents)
            mMouseEvents[evt].push_back(b);

        for (auto evt : b->mJoyStickEvents)
            mJoyStickEvents[evt].push_back(b);
    }
}


void Handler::registerKeyListener(std::weak_ptr<OIS::KeyListener> lnr)
{
    mKeyListeners.push_back(lnr);
}


void Handler::registerMouseListener(std::weak_ptr<OIS::MouseListener> lnr)
{
    mMouseListeners.push_back(lnr);
}


void Handler::registerJoyStickListener(std::weak_ptr<OIS::JoyStickListener> lnr, int id)
{
    if (id >= 0 && mJoySticks.size() > (unsigned)id)
        mJoySticks[id].second->addListener(lnr);
    else
        std::cout << "===WARNING=== "<<__FUNCTION__<<" Wrong joystick ID " << std::endl;
}


Bind* Handler::getBinding(const std::string& name, const DefaultEvent& def/* = DefaultEvent()*/)
{
    return mBindings.getBinding(name);
}


float Handler::getJoyStickValue(InputEvent::Type evt) const
{
    float value = 0.f;
    unsigned joystickNum = JoyStickEvent::getJoystickNumber(evt);
    unsigned component = JoyStickEvent::getComponent(evt);
    unsigned componentId = JoyStickEvent::getComponentId(evt);

    if (joystickNum >= mJoySticks.size()) return value;
    OIS::JoyStick* js = mJoySticks[joystickNum].first;
    const OIS::JoyStickState& state = js->getJoyStickState();

lprintln(evt<<" "<<joystickNum<<" "<<component<<" "<<componentId);

    if (componentId >= (unsigned)js->getNumberOfComponents((OIS::ComponentType)component) &&
        (component != OIS::ComponentType::OIS_POV ||
        componentId/2 >= (unsigned)js->getNumberOfComponents((OIS::ComponentType)component)))
    {
        return value;
    }

    switch (component)
    {
        case OIS::ComponentType::OIS_Unknown:
            break;
        case OIS::ComponentType::OIS_Button:
            value = state.mButtons[componentId];
            break;
        case OIS::ComponentType::OIS_Axis:
            value = state.mAxes[componentId].abs;
            break;
        case OIS::ComponentType::OIS_Slider:
            //value = state.mPOV[componentId/2].direction;
            break;
        case OIS::ComponentType::OIS_POV:
            value = JoyStickEvent::getPovDirectionValue(
                state.mPOV[componentId/2].direction, componentId);
            break;
        case OIS::ComponentType::OIS_Vector3:
            //value = state.mVectors[componentId];
            break;
    }

    if (JoyStickEvent::getReverse(evt)) value = -value;
    return value;
}


// OIS::MouseListener


bool Handler::mouseMoved(const OIS::MouseEvent& evt)
{
    SET_BINDING_VALUE(
        mMouseEvents,
        MouseEvent::create(MouseEvent::CPNT_AXIS_X),
        evt.state.X.rel * mConfig.mouseSensivityAxisX);

    SET_BINDING_VALUE(
        mMouseEvents,
        MouseEvent::create(MouseEvent::CPNT_AXIS_Y),
        evt.state.Y.rel * mConfig.mouseSensivityAxisY);

    // Notify listeners
	for (auto it = mMouseListeners.begin(); it != mMouseListeners.end(); it++)
    {
        std::shared_ptr<OIS::MouseListener> lnr = it->lock();
        if (!lnr.get()) it = mMouseListeners.erase(it);
        else lnr->mouseMoved(evt);
    }

    return true;
}


bool Handler::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
    SET_BINDING_VALUE(mMouseEvents, MouseEvent::create((unsigned)id), 1.f);
    SET_BINDING_VALUE(mMouseEvents, MouseEvent::create((unsigned)id, true), -1.f);

    // Notify listeners
	for (auto it = mMouseListeners.begin(); it != mMouseListeners.end(); it++)
    {
        std::shared_ptr<OIS::MouseListener> lnr = it->lock();
        if (!lnr.get()) it = mMouseListeners.erase(it);
        else lnr->mousePressed(evt, id);
    }

    return true;
}


bool Handler::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
    SET_BINDING_VALUE(mMouseEvents, (unsigned)id, 0.f);

    // Notify listeners
	for (auto it = mMouseListeners.begin(); it != mMouseListeners.end(); it++)
    {
        std::shared_ptr<OIS::MouseListener> lnr = it->lock();
        if (!lnr.get()) it = mMouseListeners.erase(it);
        else lnr->mouseReleased(evt, id);
    }

    return true;
}


// OIS::KeyListener


bool Handler::keyPressed(const OIS::KeyEvent& evt)
{
    SET_BINDING_VALUE(mKeyEvents, KeyEvent::create2(evt, mKeyboard), 1.f);
    SET_BINDING_VALUE(mKeyEvents, KeyEvent::create2(evt, mKeyboard, true), -1.f);

    // Notify listeners
	for (auto it = mKeyListeners.begin(); it != mKeyListeners.end(); it++)
    {
        std::shared_ptr<OIS::KeyListener> lnr = it->lock();
        if (!lnr.get()) it = mKeyListeners.erase(it);
        else lnr->keyPressed(evt);
    }

    return true;
}


bool Handler::keyReleased(const OIS::KeyEvent& evt)
{
    SET_BINDING_VALUE(mKeyEvents, KeyEvent::create2(evt, mKeyboard), 0.f);
    SET_BINDING_VALUE(mKeyEvents, KeyEvent::create2(evt, mKeyboard, true), 0.f);

    // Notify listeners
	for (auto it = mKeyListeners.begin(); it != mKeyListeners.end(); it++)
    {
        std::shared_ptr<OIS::KeyListener> lnr = it->lock();
        if (!lnr.get()) it = mKeyListeners.erase(it);
        else lnr->keyReleased(evt);
    }

    return true;
}


// Callback from Input::JoyStickListener


void Handler::buttonPressed(unsigned button, JoyStickListener* lnr)
{
    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_Button, button, lnr->getID()),
        1.f);

    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_Button, button, lnr->getID(), true),
        -1.f);
}


void Handler::buttonReleased(unsigned button, JoyStickListener* lnr)
{
    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_Button, button, lnr->getID()),
        0.f);

    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_Button, button, lnr->getID(), true),
        0.f);
}


void Handler::axisMoved(unsigned axis, float value, JoyStickListener* lnr)
{
    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_Axis, axis, lnr->getID()),
        value);

    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_Axis, axis, lnr->getID(), true),
        -value);
}


void Handler::povMoved(unsigned idx, unsigned direction, JoyStickListener* lnr)
{
    float horizontalValue = JoyStickEvent::directionToHorizontal(direction);
    float verticalValue = JoyStickEvent::directionToVertical(direction);

    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_POV, idx, lnr->getID()),
        horizontalValue);

    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_POV, idx, lnr->getID(), true),
        -horizontalValue);

    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_POV, idx, lnr->getID()),
        verticalValue);

    SET_BINDING_VALUE(
        mJoyStickEvents,
        JoyStickEvent::create(OIS::ComponentType::OIS_POV, idx, lnr->getID(), true),
        -verticalValue);
}


std::string Handler::convertOISDeviceTypeToString(OIS::Type type)
{
    switch (type)
    {
        case OIS::OISUnknown: return "Unknown";
        case OIS::OISKeyboard: return "Keyboard";
        case OIS::OISMouse: return "Mouse";
        case OIS::OISJoyStick: return "Joystick";
        case OIS::OISTablet: return "Tablet";
        case OIS::OISMultiTouch: return "MultiTouch";
    }
    return "";
}
