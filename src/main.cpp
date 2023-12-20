// system includes
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <set>
#include <stdexcept>
#include <variant>
#include <vector>

// local includes
#include "nvapiwrapper/nvapidrssession.h"
#include "nvapiwrapper/utils.h"

//--------------------------------------------------------------------------------------------------

namespace
{
struct StatusOption
{
};

//--------------------------------------------------------------------------------------------------

struct SetFpsOption
{
    enum class SaveOption
    {
        No,
        SavePrevious,
        SavePreviousAndReuse
    };

    unsigned long m_fps_value{0};
    SaveOption    m_save{SaveOption::No};
};

//--------------------------------------------------------------------------------------------------

struct LoadFileOption
{
};

//--------------------------------------------------------------------------------------------------

std::string trim(std::string value)
{
    value.erase(std::find_if(value.rbegin(), value.rend(), [](auto ch) { return !std::isspace(ch); }).base(),
                value.end());

    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](auto ch) { return !std::isspace(ch); }));

    return value;
}

//--------------------------------------------------------------------------------------------------

std::optional<unsigned long> parseFpsNumber(const std::string& arg, bool no_throw = false)
{
    bool          valid_number{false};
    unsigned long number{0};

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
            if (no_throw)
            {
                return std::nullopt;
            }

            throw std::runtime_error(std::format(
                "FPS value {} is outside the range of [{}, {}] and is not {} (for disabling)!", number,
                static_cast<int>(FRL_FPS_MIN), static_cast<int>(FRL_FPS_MAX), static_cast<int>(FRL_FPS_DISABLED)));
        }

        return number;
    }

    return std::nullopt;
}

//--------------------------------------------------------------------------------------------------

auto parseArgs(const std::vector<std::string>& args)
    -> std::optional<std::variant<StatusOption, SetFpsOption, LoadFileOption>>
{
    if (args.size() < 2)
    {
        return std::nullopt;
    }

    const std::string first_arg{trim(args[1])};
    {
        if (first_arg == "status")
        {
            if (args.size() > 2)
            {
                return std::nullopt;
            }

            return StatusOption{};
        }

        if (first_arg == "load-file")
        {
            if (args.size() > 2)
            {
                return std::nullopt;
            }

            return LoadFileOption{};
        }

        if (auto fps_number = parseFpsNumber(first_arg); fps_number)
        {
            if (args.size() > 3)
            {
                return std::nullopt;
            }
            else if (args.size() < 3)
            {
                return SetFpsOption{*fps_number};
            }

            const std::string second_arg{trim(args[2])};
            if (second_arg == "--save-previous")
            {
                return SetFpsOption{*fps_number, SetFpsOption::SaveOption::SavePrevious};
            }

            if (second_arg == "--save-previous-or-reuse")
            {
                return SetFpsOption{*fps_number, SetFpsOption::SaveOption::SavePreviousAndReuse};
            }

            return std::nullopt;
        }
    }

    return std::nullopt;
}

//--------------------------------------------------------------------------------------------------

std::optional<unsigned long> read_fps_from_file(const std::string& filename, bool no_throw = false)
{
    std::ifstream file;
    file.open(filename);

    if (!file)
    {
        if (no_throw)
        {
            return std::nullopt;
        }

        throw std::runtime_error(std::format("Failed to open file {} for reading", filename));
    }

    std::string line;
    file >> line;

    return parseFpsNumber(trim(line), no_throw);
}

//--------------------------------------------------------------------------------------------------

void save_to_file(const std::string& filename, unsigned long fps_value)
{
    std::ofstream file;
    file.open(filename);

    if (!file)
    {
        throw std::runtime_error(std::format("Failed to open file {} for writting", filename));
    }

    file << fps_value;
}

//--------------------------------------------------------------------------------------------------

void remove_file(const std::string& filename)
{
    // Note: will throw on actual error
    std::filesystem::remove(filename);
}

//--------------------------------------------------------------------------------------------------

std::string get_fps_filename(const std::string& exec_filename)
{
    std::filesystem::path path{exec_filename);
    path.replace_extension("saved_fps");
    return path.generic_string();
}
}  // namespace

//--------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    try
    {
        const std::vector<std::string> args(argv, argv + argc);
        const auto                     parsed_args{parseArgs(args)};
        const std::string              fps_filename{get_fps_filename(args.at(0))};

        if (!parsed_args)
        {
            // clang-format off
            std::cout << std::endl;
            std::cout << "  Usage example:" << std::endl;
            std::cout << "    frltoggle status                          prints the current FRL value. Value " << FRL_FPS_DISABLED << " means it's disabled." << std::endl;
            std::cout << "    frltoggle " << FRL_FPS_DISABLED << "                               turns off the framerate limiter." << std::endl;
            std::cout << "    frltoggle 60                              sets the FPS limit to 60 (allowed values are [" << FRL_FPS_MIN <<  ", " << FRL_FPS_MAX << "])." << std::endl;
            std::cout << "    frltoggle 60 --save-previous              sets the FPS limit to 60 and saves the previous value to a file." << std::endl;
            std::cout << "    frltoggle 60 --save-previous-or-reuse     sets the FPS limit to 60 and saves the previous value to a file." << std::endl << 
                         "                                              If the file already exists, its value will be validated and reused instead." << std::endl <<
                         "                                              This is useful in case the system has crashed and we want to reuse the value from before the crash." << std::endl;
            std::cout << "    frltoggle load-file                       loads the value from file (e.g., saved using \"--save-previous\") and uses it to set FRL." << std::endl << 
                         "                                              File is removed afterwards if no errors occur." << std::endl;
            std::cout << std::endl;
            // clang-format on
            return args.size() < 2 ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        NvApiWrapper       nvapi;
        NvApiDrsSession    drs_session;
        NvDRSProfileHandle drs_profile{nullptr};
        NVDRS_SETTING      drs_setting{};
        const auto         update_value = [&](unsigned long fps_value)
        {
            drs_setting.u32CurrentValue = fps_value;
            assertSuccess(nvapi.DRS_SetSetting(drs_session, drs_profile, &drs_setting), "Failed to set FRL setting!");
            assertSuccess(nvapi.DRS_SaveSettings(drs_session), "Failed to save session settings!");
        };

        drs_setting.version = NVDRS_SETTING_VER;

        assertSuccess(nvapi.DRS_LoadSettings(drs_session), "Failed to load session settings!");
        assertSuccess(nvapi.DRS_GetBaseProfile(drs_session, &drs_profile), "Failed to get base profile!");

        // Handle special case of getting settings
        {
            const auto status{nvapi.DRS_GetSetting(drs_session, drs_profile, FRL_FPS_ID, &drs_setting)};
            if (status == NVAPI_SETTING_NOT_FOUND)
            {
                throw std::runtime_error("Failed to get FRL setting! Make sure that setting has been saved at least "
                                         "once via NVIDIA Control Panel.");
            }
            assertSuccess(status, "Failed to get FRL setting!");
        }

        if (std::get_if<StatusOption>(&*parsed_args))
        {
            std::cout << drs_setting.u32CurrentValue << std::endl;
            return EXIT_SUCCESS;
        }

        if (const auto* option = std::get_if<SetFpsOption>(&*parsed_args))
        {
            switch (option->m_save)
            {
                case SetFpsOption::SaveOption::SavePrevious:
                {
                    save_to_file(fps_filename, drs_setting.u32CurrentValue);
                    break;
                }
                case SetFpsOption::SaveOption::SavePreviousAndReuse:
                {
                    const auto fps_value{read_fps_from_file(fps_filename, true)};
                    if (!fps_value)
                    {
                        save_to_file(fps_filename, drs_setting.u32CurrentValue);
                    }
                    break;
                }
                default:
                    break;
            }

            update_value(option->m_fps_value);
            return EXIT_SUCCESS;
        }

        if (std::get_if<LoadFileOption>(&*parsed_args))
        {
            const auto fps_value{read_fps_from_file(fps_filename)};

            update_value(*fps_value);
            remove_file(fps_filename);

            return EXIT_SUCCESS;
        }

        throw std::runtime_error("Unhandled code path!");
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
    }

    return EXIT_FAILURE;
}
