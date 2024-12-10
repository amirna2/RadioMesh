#pragma once

/**
 * @file RadioMesh.h
 * @brief Main include file for the RadioMesh framework
 */
#include "UserOptions.h"

// Common includes
#include <common/inc/Definitions.h>
#include <common/inc/Errors.h>
#include <common/inc/Logger.h>
#include <common/inc/Options.h>
#include <common/inc/RadioConfigs.h>

// Protocol includes
#include <core/protocol/inc/packet/Callbacks.h>
#include <core/protocol/inc/packet/Packet.h>
#include <core/protocol/inc/packet/Topics.h>

// Framework interfaces
#include <framework/interfaces/ICrypto.h>
#include <framework/interfaces/IDevice.h>
#include <framework/interfaces/IDisplay.h>
#include <framework/interfaces/IRadio.h>
#include <framework/interfaces/IWifiAccessPoint.h>
#include <framework/interfaces/IWifiConnector.h>

// Builder
#include <framework/builder/inc/DeviceBuilder.h>

// Version
#include "RadioMeshVersion.h"
