/*
    Copyright 2012-2013 Samplecount S.L.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef METHCLA_PLUGINS_PATCH_CABLE_H_INCLUDED
#define METHCLA_PLUGINS_PATCH_CABLE_H_INCLUDED

#include <methcla/plugin.h>

METHCLA_EXPORT const Methcla_Library* methcla_plugins_patch_cable(const Methcla_Host*, const char*);
#define METHCLA_PLUGINS_PATCH_CABLE_URI METHCLA_PLUGINS_URI "/patch-cable"
#define METHCLA_PLUGINS_PATCH_CABLE_LIB { methcla_plugins_patch_cable }

#endif // METHCLA_PLUGINS_PATCH_CABLE_H_INCLUDED
