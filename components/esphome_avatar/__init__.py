"""Stack-chan avatar, exposed as an ESPHome external component.

The component has no configuration of its own. Adding `esphome_avatar:`
to a config copies esphome_avatar.h into the build and #includes it in
main.cpp, so display lambdas can use stackchan_avatar::Avatar without a
local copy of the header.
"""

import esphome.codegen as cg
import esphome.config_validation as cv

CODEOWNERS = ["@pfefferle"]
DEPENDENCIES = ["display"]

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    # Display lambdas are compiled into main.cpp; make the avatar visible there
    cg.add_global(
        cg.RawStatement(
            '#include "esphome/components/esphome_avatar/esphome_avatar.h"'
        )
    )
