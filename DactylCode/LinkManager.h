#pragma once

#include <Arduino.h>

#include "KeymapResolver.h"
#include "RuntimeState.h"

namespace LinkManager {

void begin(LinkState& state);
void tick(LinkState& state);
void poll_incoming(LinkState& state, bool dummy);
bool has_primary_ble_peer();
bool dispatch_remote_action(const KeymapResolver::Action& action, const LinkState& state);

}
