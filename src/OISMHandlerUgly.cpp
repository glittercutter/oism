// Licensed under the zlib License
// Copyright (C) 2012 Sebastien Raymond

// Accessing protected member OIS::Keyboard::mModifiers
#define protected public

#include "OISMHandler.h"
#include <OIS/OISKeyboard.h>

using namespace oism;

InputEvent::Type KeyEvent::create2(const OIS::KeyEvent& evt, OIS::Keyboard* kb, bool rev/* = false*/)
{
    return create(evt.key, kb->mModifiers, rev);
}
