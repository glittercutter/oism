// Licensed under the zlib License
// Copyright (C) 2012 Sebastien Raymond

#include "OISMHandler.h"
#include "OISMLog.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>

using namespace oism;


/*
=====================
NamedBindingMap
=====================
*/


// NOTE: Emit a warning if you want to use(not constructing)
// a non-existing binding.
Bind* NamedBindingMap::getBinding(const std::string& name, bool forUse/* = true*/)
{
    auto it = map.find(name);
    if (it == map.end())
    {
        ILog() << "New binding '" << name << "'";
        if (forUse) WLog() << "Binding: '" << name << "' has no input";
        it = map.insert(std::make_pair(name, new Bind())).first;
    }
    return it->second;
}


/*
=====================
Bind
=====================
*/


void Bind::setValue(Handler* handler, float val)
{
    float oldVal = mValue;
    mValue = val;

    float high = _getMaxValue(handler);
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


float Bind::_getMaxValue(Handler* handler) const
{
    float high = 0.f;
    auto kb = handler->getKeyboard();
    auto mouse = handler->getMouse();

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
        if (float tmp = handler->getJoyStickValue(evt))
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


Handler::Handler(unsigned long windowID, bool exclusive/* = true*/)
:   mMouseSmoothLastX(0.f), mMouseSmoothLastY(0.f),
    mMouseSmoothUpdatedX(false), mMouseSmoothUpdatedY(false),
    mSerializer(0), mWindowID(windowID)
{
    createOIS(exclusive);
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
    mMouse->capture();
    smoothMouseCleanup();
    mKeyboard->capture();
    for (auto& pair : mJoySticks) pair.first->capture();
}


void Handler::setExclusive(bool exclusive/* = true*/)
{
    destroyOIS();
    createOIS(exclusive);
}


void Handler::createOIS(bool exclusive/* = true*/)
{
    OIS::ParamList pl;
    pl.insert({"WINDOW", std::to_string(mWindowID)});
    
    if (!exclusive)
    {
#if defined OIS_WIN32_PLATFORM
        pl.insert({"w32_mouse",    "DISCL_FOREGROUND"});
        pl.insert({"w32_mouse",    "DISCL_NONEXCLUSIVE"});
        pl.insert({"w32_keyboard", "DISCL_FOREGROUND"});
        pl.insert({"w32_keyboard", "DISCL_NONEXCLUSIVE"});
#elif defined OIS_LINUX_PLATFORM
        pl.insert({"x11_mouse_grab",    "false"});
        pl.insert({"x11_mouse_hide",    "false"});
        pl.insert({"x11_keyboard_grab", "false"});
        pl.insert({"XAutoRepeatOn",     "true"});
#endif
    }

    mOIS = OIS::InputManager::createInputSystem(pl);
    ILog() << mOIS->inputSystemName();

    ILog() << "Available device:";
    for (auto& dev : mOIS->listFreeDevices())
        ILog() << "  - "<<convertOISDeviceTypeToString(dev.first)<<" ("<<dev.second<<")";

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
    if (numJoystick) ILog() << "Creating joystick:";

    for (int i = 0; i < mOIS->getNumberOfDevices(OIS::OISJoyStick); i++)
    {
        mJoySticks.push_back(std::make_pair(
            static_cast<OIS::JoyStick*>(mOIS->createInputObject(OIS::OISJoyStick, true)),
            new JoyStickListener(this, i)));

        auto js = mJoySticks.back().first; 
        js->setEventCallback(mJoySticks.back().second);

        // List specs
        ILog()<<"  - Joystick "<<i<<" -- Vendor: "<<js->vendor();
        ILog()<<"    - Unknown: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Unknown);
        ILog()<<"    - Button: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Button);
        ILog()<<"    - Axis: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Axis);
        ILog()<<"    - Slider: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Slider);
        ILog()<<"    - POV: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_POV);
        ILog()<<"    - Movement capture: "<<js->getNumberOfComponents(OIS::ComponentType::OIS_Vector3);
    }
}


void Handler::destroyOIS()
{
    mOIS->destroyInputObject(mMouse);
    mOIS->destroyInputObject(mKeyboard);

    for (auto& pair : mJoySticks)
    {
        mOIS->destroyInputObject(pair.first); // JoyStick
        delete pair.second; // JoyStickListener
    }

    OIS::InputManager::destroyInputSystem(mOIS);
}


void Handler::setMouseLimit(int w, int h)
{
    ILog()<<"Setting mouse limit: width="<<w<<" height="<<h; 
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
    mSerializer->loadConfig(&mConfig);
}


void Handler::_saveConfig()
{
    mSerializer->saveConfig(&mConfig);
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
        WLog() << __FUNCTION__<<" Invalid joystick number";
}


Bind::CallbackSharedPtr Handler::callback(const std::string& name, const Bind::Callback& cb, unsigned type/* = Bind::CT_ON_POSITIVE*/)
{
    return getBinding(name)->addCallback(type, Bind::CallbackSharedPtr(new Bind::Callback(cb)));   
}


Bind* Handler::getBinding(const std::string& name, bool forUse/* = true*/)
{
    return mBindings.getBinding(name, forUse);
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


void Handler::setBindingValue(InputEventBindingListMap& bindings, unsigned evt, float value)
{
    auto it = bindings.find(evt);
    if (it == bindings.end()) return;
    for (auto binding : it->second)
        binding->setValue(this, value);
}


void Handler::setMouseValue(unsigned cpnt, float value)
{
    if (cpnt == MouseEvent::CPNT_AXIS_X) smoothMouse(value, mMouseSmoothLastX);
    else if (cpnt == MouseEvent::CPNT_AXIS_Y) smoothMouse(value, mMouseSmoothLastY);

    auto evt = MouseEvent::create(cpnt);
    setBindingValue(mMouseEvents, evt, value);

    MouseEvent::setReverse(evt);
    setBindingValue(mMouseEvents, evt, -value);
}


void Handler::smoothMouse(float& curr, float& last)
{
    float tmp = curr;

    if (mConfig.mouseSmoothing)
        curr = (curr + (last * mConfig.mouseSmoothing)) / (mConfig.mouseSmoothing + 1.f);

    last = tmp;
}


void Handler::smoothMouseCleanup()
{
    if (!mMouseSmoothUpdatedX) setMouseValue(MouseEvent::CPNT_AXIS_X, 0);
    if (!mMouseSmoothUpdatedY) setMouseValue(MouseEvent::CPNT_AXIS_Y, 0);
    mMouseSmoothUpdatedX = mMouseSmoothUpdatedY = false;
}


void Handler::setKeyboardValue(const OIS::KeyEvent& key, float value)
{
    auto evt = KeyEvent::create2(key, mKeyboard);
    setBindingValue(mKeyEvents, evt, value);

    KeyEvent::setReverse(evt);
    setBindingValue(mKeyEvents, evt, -value);
}


void Handler::setJoyStickValue(OIS::ComponentType cpntType, unsigned cpnt, JoyStickListener* lnr, float value)
{
    auto evt = JoyStickEvent::create(cpntType, cpnt, lnr->getId());
    setBindingValue(mJoyStickEvents, evt, value);
    
    JoyStickEvent::setReverse(evt);
    setBindingValue(mJoyStickEvents, evt, -value);
}


// ================
// Inplement OIS::MouseListener
// ================


bool Handler::mouseMoved(const OIS::MouseEvent& evt)
{
    if (evt.state.X.rel) mMouseSmoothUpdatedX = true;
    if (evt.state.Y.rel) mMouseSmoothUpdatedY = true;

    float x = evt.state.X.rel * mConfig.mouseSensivityAxisX;
    float y = evt.state.Y.rel * mConfig.mouseSensivityAxisY;

    setMouseValue(MouseEvent::CPNT_AXIS_X, x);
    setMouseValue(MouseEvent::CPNT_AXIS_Y, y);

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
    setMouseValue((unsigned)id, 1.f);

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
    setMouseValue((unsigned)id, 0.f);

    // Notify listeners
    for (auto it = mMouseListeners.begin(); it != mMouseListeners.end(); it++)
    {
        std::shared_ptr<OIS::MouseListener> lnr = it->lock();
        if (!lnr.get()) it = mMouseListeners.erase(it);
        else lnr->mouseReleased(evt, id);
    }

    return true;
}


// ================
// Implement OIS::KeyListener
// ================


bool Handler::keyPressed(const OIS::KeyEvent& evt)
{
    setKeyboardValue(evt, 1.f);

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
    setKeyboardValue(evt, 0.f);

    // Notify listeners
    for (auto it = mKeyListeners.begin(); it != mKeyListeners.end(); it++)
    {
        std::shared_ptr<OIS::KeyListener> lnr = it->lock();
        if (!lnr.get()) it = mKeyListeners.erase(it);
        else lnr->keyReleased(evt);
    }

    return true;
}


// ================
// Callback from Input::JoyStickListener
// ================


void Handler::buttonPressed(unsigned button, JoyStickListener* lnr)
{
    setJoyStickValue(OIS::ComponentType::OIS_Button, button, lnr, 1.f);
}


void Handler::buttonReleased(unsigned button, JoyStickListener* lnr)
{
    setJoyStickValue(OIS::ComponentType::OIS_Button, button, lnr, 0.f);
}


void Handler::axisMoved(unsigned axis, float value, JoyStickListener* lnr)
{
    setJoyStickValue(OIS::ComponentType::OIS_Axis, axis, lnr, value);
}


void Handler::povMoved(unsigned idx, unsigned direction, JoyStickListener* lnr)
{
    float horizontalValue = JoyStickEvent::directionToHorizontal(direction);
    float verticalValue = JoyStickEvent::directionToVertical(direction);

    setJoyStickValue(OIS::ComponentType::OIS_POV, idx, lnr, horizontalValue);
    setJoyStickValue(OIS::ComponentType::OIS_POV, idx, lnr, verticalValue);
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
