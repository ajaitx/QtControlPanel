#pragma once

#include <stdbool.h>

#define MAX_PRODUCT_PARAMS_COUNT        3
#define MAX_SPEED_PARAMS_COUNT          5
#define RPM_TO_VOLTAGEE_DIVIDE_FACTOR       150
#define COLOR_PULSES_PER_STEP_FACTOR        10

#define ANALOG_VOLTAGE_MAX          10
#define EXTRUDER_RPM_MIN            0
#define CATERPILLAR_RPM_MIN         0
#define STEPPER_COLOR_FACTOR_MIN    0.0
#define STEPPER_COLOR_FACTOR_MAX    100.0

#if defined(__aarch64__)
#define SYSTEM_SETTINGS_FILE "/home/pi/settings.bin"
#else
#define SYSTEM_SETTINGS_FILE "./settings.bin"
#endif

#pragma pack(1)

enum SpeedProfiles {
    eSPEED_ONE = 0,
    eSPEED_TWO,
    eSPEED_THREE,
    eSPEED_FOUR,
    eSPEED_FIVE
};

enum RunStates {
    eSTATE_STARTED,
    eSTATE_STOPPED,
    eSTATE_UNKNOWN
};

enum ControlPanelScreens {
    eRUN_SCREEN = 0,
    eSETTINGS_SCREEN,
    eINPUT_TEXT_SCREEN,
    eFACTORS_EDIT_SCREEN,
    eUNKWON_SCREEN
};

enum ParameterTypes {
    eERPM = 0,
    eCRPM,
    eCOLOR,
    eALL_PARAMS
};

struct SpeedParams
{
    int erpm;
    float crpm;
    float color;
};

struct ProductParams
{
    char name[32];
    SpeedParams params[MAX_SPEED_PARAMS_COUNT];
};

struct ControlPanelConfig
{
    int product_idx;
    int speed_idx;
    float analog_factor_value;
    float color_factor_value;
    struct ProductParams m_products[MAX_PRODUCT_PARAMS_COUNT];
};
