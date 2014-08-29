// Licensed under the zlib License
// Copyright (C) 2012 Sebastien Raymond

// Accessing protected member OIS::Keyboard::mModifiers
#define protected public

#include "OISMHandler.h"
#include <OISKeyboard.h>

using namespace oism;


InputEvent::Type KeyEvent::create2(const OIS::KeyEvent& evt, OIS::Keyboard* kb, bool rev/* = false*/)
{
    log::log(__PRETTY_FUNCTION__+std::string("key=")+std::to_string(evt.key));
    return create(evt.key, kb->mModifiers, rev);
}
