"""esp32-eyes (Cozmo-style animated eyes), exposed as an ESPHome external component.

AGPL-3.0-or-later, see LICENSE.AGPL in this directory.

The component has no configuration of its own. Adding `esphome_esp32eyes:`
to a config copies the sources into the build and #includes the adapter in
main.cpp, so display lambdas can use esp32eyes::Eyes.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import CORE

CODEOWNERS = ["@pfefferle"]
DEPENDENCIES = ["display"]

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    # The upstream sources include <Arduino.h> and <U8g2lib.h>; the
    # stand-ins for both live in this directory, so it goes on the include
    # path. Native ESP-IDF builds only pass -D/-W from add_build_flag, but
    # C++ flags unfiltered (all upstream sources are C++).
    include = f"-I{CORE.relative_src_path('esphome', 'components', 'esphome_esp32eyes')}"
    cg.add_build_flag(include)
    cg.add_cxx_build_flag(include)
    # Display lambdas are compiled into main.cpp; make the adapter visible there
    cg.add_global(
        cg.RawStatement(
            '#include "esphome/components/esphome_esp32eyes/esphome_esp32eyes.h"'
        )
    )
