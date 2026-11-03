"""FluxGarage RoboEyes, exposed as an ESPHome external component.

GPL-3.0-or-later, see LICENSE in this directory.

The component has no configuration of its own. Adding `esphome_roboeyes:` to a
config copies the headers into the build and #includes the adapter in
main.cpp, so display lambdas can use roboeyes::Face.
"""

import esphome.codegen as cg
import esphome.config_validation as cv

CODEOWNERS = ["@pfefferle"]
DEPENDENCIES = ["display"]

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    # Display lambdas are compiled into main.cpp; make RoboEyes visible there
    cg.add_global(
        cg.RawStatement(
            '#include "esphome/components/esphome_roboeyes/esphome_roboeyes.h"'
        )
    )
