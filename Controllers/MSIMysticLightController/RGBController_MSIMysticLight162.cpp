/*-----------------------------------------*\
|  RGBController_MSIMysticLight162.cpp      |
|                                           |
|  Generic RGB Interface for OpenRGB        |
|  MSI Mystic Light (162-byte) USB Driver   |
|                                           |
|  T-bond 3/4/2020                          |
\*-----------------------------------------*/

#include "RGBController_MSIMysticLight162.h"


struct ZoneDescription
{
    std::string name;
    MSI_ZONE    zone_type;
};

#define NUMOF_ZONES     (sizeof(led_zones) / sizeof(ZoneDescription))

const ZoneDescription led_zones[] =
{
    ZoneDescription{ "JRGB1",        MSI_ZONE_J_RGB_1 },
    ZoneDescription{ "JRGB2",        MSI_ZONE_J_RGB_2 },
    ZoneDescription{ "JRAINBOW1",    MSI_ZONE_J_RAINBOW_1 },
    ZoneDescription{ "JRAINBOW2",    MSI_ZONE_J_RAINBOW_2 },
    ZoneDescription{ "JCORSAIR",     MSI_ZONE_J_CORSAIR },
    ZoneDescription{ "Onboard LEDs", MSI_ZONE_ON_BOARD_LED_0 }
};


static std::vector<const ZoneDescription*> zone_description;



/**------------------------------------------------------------------*\
    @name MSI Mystic Light (162 Byte)
    @category Motherboard
    @type USB
    @save :robot:
    @direct :white_check_mark:
    @effects :white_check_mark:
    @detectors DetectMSIMysticLightControllers
    @comment
\*-------------------------------------------------------------------*/

RGBController_MSIMysticLight162::RGBController_MSIMysticLight162
    (
    MSIMysticLight162Controller* controller_ptr
    )
{
    controller = controller_ptr;

    name        = controller->GetDeviceName();
    vendor      = "MSI";
    type        = DEVICE_TYPE_MOTHERBOARD;
    description = "MSI Mystic Light Device (162-byte)";
    version     = controller->GetFWVersion();
    location    = controller->GetDeviceLocation();
    serial      = controller->GetSerial();

    const std::vector<MSI_ZONE>* supported_zones = controller->GetSupportedZones();

    for(std::size_t i = 0; i < supported_zones->size(); ++i)
    {
        for(std::size_t j = 0; j < NUMOF_ZONES; ++j)
        {
            if(led_zones[j].zone_type == (*supported_zones)[i])
            {
                zone_description.push_back(&led_zones[j]);
                break;
            }
        }
    }

    SetupModes();
    SetupZones();
    SetupColors();
}

RGBController_MSIMysticLight162::~RGBController_MSIMysticLight162()
{
    zone_description.clear();
    delete controller;
}

void RGBController_MSIMysticLight162::SetupZones()
{
    /*---------------------------------------------------------*\
    | Set up zones                                              |
    \*---------------------------------------------------------*/
    for(std::size_t zone_idx = 0; zone_idx < zone_description.size(); ++zone_idx)
    {
        const ZoneDescription* zd = zone_description[zone_idx];

        zone new_zone;

        new_zone.name           = zd->name;

        /*--------------------------------------------------\
        | 162-byte MSI does not have resizable zones, but   |
        | onboard LED zones have multiple LEDs              |
        \*-------------------------------------------------*/
        if(zd->zone_type == MSI_ZONE_ON_BOARD_LED_0)
        {
            new_zone.leds_max   = (int)controller->GetMaxOnboardLeds();
        }
        else
        {
            new_zone.leds_max   = 1;
        }

        new_zone.leds_min       = new_zone.leds_max;
        new_zone.leds_count     = new_zone.leds_max;

        /*-------------------------------------------------*\
        | Determine zone type based on max number of LEDs   |
        \*-------------------------------------------------*/
        if(new_zone.leds_max == 1)
        {
            new_zone.type       = ZONE_TYPE_SINGLE;
        }
        else
        {
            new_zone.type       = ZONE_TYPE_LINEAR;
        }

        new_zone.matrix_map     = NULL;
        zones.push_back(new_zone);
	}

    /*---------------------------------------------------------*\
    | Set up LEDs                                               |
    \*---------------------------------------------------------*/
    for(std::size_t zone_idx = 0; zone_idx < zone_description.size(); ++zone_idx)
    {
        for(std::size_t led_idx = 0; led_idx < zones[zone_idx].leds_count; ++led_idx)
        {
            led new_led;

            new_led.name = zones[zone_idx].name + " LED ";

            if(zones[zone_idx].leds_count > 1)
            {
                new_led.name.append(std::to_string(led_idx + 1));
            }

            new_led.value = (unsigned int)(zone_description[zone_idx]->zone_type + led_idx);
            leds.push_back(new_led);
        }
    }
}

void RGBController_MSIMysticLight162::ResizeZone
    (
    int /*zone*/,
    int /*new_size*/
    )
{
}

void RGBController_MSIMysticLight162::DeviceUpdateLEDs()
{
    for(std::size_t zone_idx = 0; zone_idx < zones.size(); ++zone_idx)
    {
        for(int led_idx = zones[zone_idx].leds_count - 1; led_idx >= 0; led_idx--)
        {
            UpdateLed((int)zone_idx, led_idx);
        }
    }
    controller->Update((modes[active_mode].flags & MODE_FLAG_AUTOMATIC_SAVE) != 0);
}

void RGBController_MSIMysticLight162::UpdateZoneLEDs
    (
    int zone
    )
{
    for(int led_idx = zones[zone].leds_count - 1; led_idx >= 0; led_idx--)
    {
        UpdateLed(zone, led_idx);
    }
    controller->Update((modes[active_mode].flags & MODE_FLAG_AUTOMATIC_SAVE) != 0);
}

void RGBController_MSIMysticLight162::UpdateSingleLED
    (
    int led
    )
{
    UpdateLed(leds[led].value, led);
    controller->Update((modes[active_mode].flags & MODE_FLAG_AUTOMATIC_SAVE) != 0);
}

void RGBController_MSIMysticLight162::DeviceUpdateMode()
{
    if(modes[active_mode].value == MSI_MODE_DIRECT_DUMMY)
    {
        controller->SetDirectMode(true);
    }
    else
    {
        controller->SetDirectMode(false);
        DeviceUpdateLEDs();
    }
}

void RGBController_MSIMysticLight162::DeviceSaveMode()
{
     controller->Update(true);
}

void RGBController_MSIMysticLight162::SetupModes()
{
    constexpr unsigned int PER_LED_ONLY = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_MANUAL_SAVE;
    constexpr unsigned int RANDOM_ONLY  = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_RANDOM_COLOR | MODE_FLAG_MANUAL_SAVE;
    constexpr unsigned int COMMON       = RANDOM_ONLY | MODE_FLAG_HAS_PER_LED_COLOR;

    SetupMode("Direct",                     MSI_MODE_DIRECT_DUMMY,                  MODE_FLAG_HAS_PER_LED_COLOR);
    SetupMode("Static",                     MSI_MODE_STATIC,                        MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_MANUAL_SAVE);
    // SetupMode("Off",                        MSI_MODE_DISABLE,                       0);
    SetupMode("Breathing",                  MSI_MODE_BREATHING,                     PER_LED_ONLY);
    SetupMode("Flashing",                   MSI_MODE_FLASHING,                      COMMON);
    SetupMode("Double flashing",            MSI_MODE_DOUBLE_FLASHING,               COMMON);
    SetupMode("Lightning",                  MSI_MODE_LIGHTNING,                     PER_LED_ONLY);
    // SetupMode("MSI Marquee",                MSI_MODE_MSI_MARQUEE,                   COMMON);
    SetupMode("Meteor",                     MSI_MODE_METEOR,                        COMMON);
    SetupMode("Stack",                      MSI_MODE_WATER_DROP,                    COMMON);
    // SetupMode("MSI Rainbow",                MSI_MODE_MSI_RAINBOW,                   RANDOM_ONLY);
    // SetupMode("Pop",                        MSI_MODE_POP,                           COMMON);
    // SetupMode("Rap",                        MSI_MODE_RAP,                           COMMON);
    // SetupMode("Jazz",                       MSI_MODE_JAZZ,                          COMMON);
    // SetupMode("Play",                       MSI_MODE_PLAY,                          COMMON);
    // SetupMode("Movie",                      MSI_MODE_MOVIE,                         COMMON);
    SetupMode("Rainbow",                    MSI_MODE_COLOR_RING,                    COMMON);
    SetupMode("Planetary",                  MSI_MODE_PLANETARY,                     RANDOM_ONLY);
    SetupMode("Double meteor",              MSI_MODE_DOUBLE_METEOR,                 RANDOM_ONLY);
    SetupMode("Energy",                     MSI_MODE_ENERGY,                        RANDOM_ONLY);
    SetupMode("Blink",                      MSI_MODE_BLINK,                         COMMON);
    SetupMode("Clock",                      MSI_MODE_CLOCK,                         RANDOM_ONLY);
    SetupMode("Color pulse",                MSI_MODE_COLOR_PULSE,                   COMMON);
    SetupMode("Color shift",                MSI_MODE_COLOR_SHIFT,                   RANDOM_ONLY);
    SetupMode("Color wave",                 MSI_MODE_COLOR_WAVE,                    COMMON);
    SetupMode("Marquee",                    MSI_MODE_MARQUEE,                       PER_LED_ONLY);
    // SetupMode("Rainbow",                    MSI_MODE_RAINBOW,                       COMMON);
    SetupMode("Rainbow wave",               MSI_MODE_RAINBOW_WAVE,                  RANDOM_ONLY);
    SetupMode("Visor",                      MSI_MODE_VISOR,                         COMMON);
    // SetupMode("JRainbow",                   MSI_MODE_JRAINBOW,                      COMMON);
    SetupMode("Rainbow flashing",           MSI_MODE_RAINBOW_FLASHING,              RANDOM_ONLY);
    SetupMode("Rainbow double flashing",    MSI_MODE_RAINBOW_DOUBLE_FLASHING,       RANDOM_ONLY);
    // SetupMode("Random",                     MSI_MODE_RANDOM,                        COMMON);
    // SetupMode("Fan control",                MSI_MODE_FAN_CONTROL,                   COMMON);
    // SetupMode("Off 2",                      MSI_MODE_DISABLE_2,                     COMMON);
    // SetupMode("Color ring flashing",        MSI_MODE_COLOR_RING_FLASHING,           COMMON);
    // SetupMode("Color ring double flashing", MSI_MODE_COLOR_RING_DOUBLE_FLASHING,    COMMON);
    // SetupMode("Stack",                      MSI_MODE_STACK,                         COMMON);
    // SetupMode("Corsair Que",                MSI_MODE_CORSAIR_QUE,                   COMMON);
    // SetupMode("Fire",                       MSI_MODE_FIRE,                          COMMON);
    // SetupMode("Lava",                       MSI_MODE_LAVA,                          COMMON);
}

void RGBController_MSIMysticLight162::UpdateLed
    (
    int zone,
    int led
    )
{
    unsigned char red = RGBGetRValue(zones[zone].colors[led]);
    unsigned char grn = RGBGetGValue(zones[zone].colors[led]);
    unsigned char blu = RGBGetBValue(zones[zone].colors[led]);

    if(controller->IsDirectModeActive())
    {
        controller->SetLedColor((MSI_ZONE)zones[zone].leds[led].value, red, grn, blu);
    }
    else
    {
        bool           random     = modes[active_mode].color_mode == MODE_COLORS_RANDOM;
        MSI_MODE       mode       = (MSI_MODE)modes[active_mode].value;
        MSI_SPEED      speed      = (MSI_SPEED)modes[active_mode].speed;
        MSI_BRIGHTNESS brightness = (MSI_BRIGHTNESS)modes[active_mode].brightness;

        controller->SetMode((MSI_ZONE)zones[zone].leds[led].value, mode, speed, brightness, random);
        controller->SetZoneColor((MSI_ZONE)zones[zone].leds[led].value, red, grn, blu, red, grn, blu);
    }
}

void RGBController_MSIMysticLight162::SetupMode
    (
    const char      *name,
    MSI_MODE        mod,
    unsigned int    flags
    )
{
    mode Mode;
    Mode.name   = name;
    Mode.value  = mod;
    Mode.flags  = flags;

    if(flags & MODE_FLAG_HAS_PER_LED_COLOR)
    {
        Mode.color_mode = MODE_COLORS_PER_LED;
    }
    else
    {
        Mode.color_mode = MODE_COLORS_RANDOM;
    }

    if(flags & MODE_FLAG_HAS_SPEED)
    {
        Mode.speed      = MSI_SPEED_MEDIUM;
        Mode.speed_max  = MSI_SPEED_HIGH;
        Mode.speed_min  = MSI_SPEED_LOW;
    }
    else
    {
        /*---------------------------------------------------------*\
        | For modes without speed this needs to be set to avoid     |
        | bad values in the saved profile which in turn corrupts    |
        | the brightness calculation when loading the profile       |
        \*---------------------------------------------------------*/
        Mode.speed      = 0;
        Mode.speed_max  = 0;
        Mode.speed_min  = 0;
    }

    if(flags & MODE_FLAG_HAS_BRIGHTNESS)
    {
        Mode.brightness      = MSI_BRIGHTNESS_LEVEL_100;
        Mode.brightness_max  = MSI_BRIGHTNESS_LEVEL_100;
        Mode.brightness_min  = MSI_BRIGHTNESS_OFF;
    }
    else
    {
        Mode.brightness      = MSI_BRIGHTNESS_LEVEL_100;
        Mode.brightness_max  = MSI_BRIGHTNESS_LEVEL_100;
        Mode.brightness_min  = MSI_BRIGHTNESS_LEVEL_100;
    }

    modes.push_back(Mode);
}
