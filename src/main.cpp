// system includes
#include <format>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>
#include <vector>

// local includes
#include "nvapiwrapper/nvapidrssession.h"
#include "nvapiwrapper/utils.h"

//--------------------------------------------------------------------------------------------------

bool parseFpsNumber(const std::string& arg, unsigned long& number)
{
    bool valid_number{false};

    try
    {
        number       = std::stoul(arg);
        valid_number = std::regex_search(arg, std::regex{R"(^\s*[0-9]+\s*$)"});
    }
    catch (const std::exception&)
    {
    }

    if (valid_number)
    {
        if ((number < FRL_FPS_MIN || number > FRL_FPS_MAX) && number != FRL_FPS_DISABLED)
        {
            throw std::runtime_error(std::format(
                "FPS value {} is outside the range of [{}, {}] and is not {} (for disabling)!", number,
                static_cast<int>(FRL_FPS_MIN), static_cast<int>(FRL_FPS_MAX), static_cast<int>(FRL_FPS_DISABLED)));
        }
    }

    return valid_number;
}

//--------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    try
    {
        const std::string status_value{"status"};
        unsigned long     fps_value{0};

        const std::vector<std::string> args(argv, argv + argc);

        if (args.size() != 2 || (args[1] != status_value && !parseFpsNumber(args[1], fps_value)))
        {
            // clang-format off
            std::cout << std::endl;
            std::cout << "  Usage example:" << std::endl;
            std::cout << "    frltoggle status    prints the current FRL value. Value " << FRL_FPS_DISABLED << " means it's disabled." << std::endl;
            std::cout << "    frltoggle " << FRL_FPS_DISABLED << "         turns off the framerate limiter." << std::endl;
            std::cout << "    frltoggle 60        sets the FPS limit to 60 (allowed values are [" << FRL_FPS_MIN <<  ", " << FRL_FPS_MAX << "])." << std::endl;
            std::cout << std::endl;
            // clang-format on
            return args.size() < 2 ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        NvApiWrapper       nvapi;
        NvApiDrsSession    drs_session;
        NvDRSProfileHandle drs_profile{nullptr};
        NVDRS_SETTING      drs_setting{};

        drs_setting.version = NVDRS_SETTING_VER;

        assertSuccess(nvapi.DRS_LoadSettings(drs_session), "Failed to load session settings!");
        assertSuccess(nvapi.DRS_GetBaseProfile(drs_session, &drs_profile), "Failed to get base profile!");

        // Handle special case of getting settings
        {
            const auto status{nvapi.DRS_GetSetting(drs_session, drs_profile, FRL_FPS_ID, &drs_setting)};
            if (status == NVAPI_SETTING_NOT_FOUND)
            {
                throw std::runtime_error("Failed to get FRL setting! Make sure that setting has been saved at least "
                                         "once via Nvidia control panel.");
            }
            assertSuccess(status, "Failed to set FRL setting!");
        }

        if (args[1] == status_value)
        {
            std::cout << drs_setting.u32CurrentValue << std::endl;
            return EXIT_SUCCESS;
        }

        drs_setting.u32CurrentValue = fps_value;

        assertSuccess(nvapi.DRS_SetSetting(drs_session, drs_profile, &drs_setting), "Failed to set FRL setting!");
        assertSuccess(nvapi.DRS_SaveSettings(drs_session), "Failed to save session settings!");
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
