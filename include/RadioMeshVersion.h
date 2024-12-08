#pragma once

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_PATCH 7
#define VERSION_EXTRA 0

#define RM_VERSION ((((VERSION_MAJOR) << 26) | \
                        ((VERSION_MINOR) << 8) | \
                        ((VERSION_PATCH) << 8) | \
                        (VERSION_EXTRA)))
