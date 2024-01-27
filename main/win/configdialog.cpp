/***************************************************************************
						  configdialog.c  -  description
							 -------------------
	copyright            : (C) 2003 by ShadowPrince
	email                : shadow@emulation64.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
*/

#include <shlobj.h>
#include <cstdio>
#include <vector>

#include "../lua/LuaConsole.h"
#include "main_win.h"
#include "../../winproject/resource.h"
#include "../plugin.hpp"
#include "features/RomBrowser.hpp"
#include "timers.h"
#include "Config.hpp"
#include "wrapper/PersistentPathDialog.h"
#include "../main/helpers/string_helpers.h"
#include "../main/helpers/win_helpers.h"
#include "../r4300/r4300.h"
#include "configdialog.h"
#include <vcr.h>
#include <cassert>

std::vector<t_plugin*> available_plugins;

BOOL CALLBACK other_options_proc(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
	NMHDR FAR* l_nmhdr = nullptr;
    memcpy(&l_nmhdr, &l_param, sizeof(NMHDR FAR*));
    switch (message)
    {
    case WM_INITDIALOG:
        {
	        set_checkbox_state(hwnd, IDC_ALERTMOVIESERRORS, Config.is_rom_movie_compatibility_check_enabled);

            static const char* clock_speed_multiplier_names[] = {
                "1 - Legacy Mupen Lag Emulation", "2 - 'Lagless'", "3", "4", "5", "6"
            };

            // Populate CPU Clock Speed Multiplier Dropdown Menu
            for (auto& clock_speed_multiplier_name :clock_speed_multiplier_names) {
                SendDlgItemMessage(hwnd, IDC_COMBO_CLOCK_SPD_MULT, CB_ADDSTRING, 0,
                                   (LPARAM)clock_speed_multiplier_name);
            }
	        const int index = SendDlgItemMessage(hwnd, IDC_COMBO_CLOCK_SPD_MULT, CB_FINDSTRINGEXACT, -1,
	                                             (LPARAM)clock_speed_multiplier_names[Config.cpu_clock_speed_multiplier - 1]);
            SendDlgItemMessage(hwnd, IDC_COMBO_CLOCK_SPD_MULT, CB_SETCURSEL, index, 0);

            switch (Config.synchronization_mode)
            {
            case VCR_SYNC_AUDIO_DUPL:
                CheckDlgButton(hwnd, IDC_AV_AUDIOSYNC, BST_CHECKED);
                break;
            case VCR_SYNC_VIDEO_SNDROP:
                CheckDlgButton(hwnd, IDC_AV_VIDEOSYNC, BST_CHECKED);
                break;
            case VCR_SYNC_NONE:
                CheckDlgButton(hwnd, IDC_AV_NOSYNC, BST_CHECKED);
                break;
            default:
                break;
            }

            EnableWindow(GetDlgItem(hwnd, IDC_AV_AUDIOSYNC), !vcr_is_capturing());
            EnableWindow(GetDlgItem(hwnd, IDC_AV_VIDEOSYNC), !vcr_is_capturing());
            EnableWindow(GetDlgItem(hwnd, IDC_AV_NOSYNC), !vcr_is_capturing());

            return TRUE;
        }
    case WM_COMMAND:
        {
            char buf[50];
            // TODO: move into wm_notify psn_apply
            switch (LOWORD(w_param))
            {
            case IDC_AV_AUDIOSYNC:
                Config.synchronization_mode = VCR_SYNC_AUDIO_DUPL;
                break;
            case IDC_AV_VIDEOSYNC:
                Config.synchronization_mode = VCR_SYNC_VIDEO_SNDROP;
                break;
            case IDC_AV_NOSYNC:
                Config.synchronization_mode = VCR_SYNC_NONE;
                break;
            case IDC_COMBO_CLOCK_SPD_MULT:
                read_combo_box_value(hwnd, IDC_COMBO_CLOCK_SPD_MULT, buf);
                Config.cpu_clock_speed_multiplier = atoi(&buf[0]);
                break;
            case IDC_ALERTMOVIESERRORS:
            	Config.is_rom_movie_compatibility_check_enabled = get_checkbox_state(hwnd, IDC_ALERTMOVIESERRORS);
            	printf("%d\n", Config.is_rom_movie_compatibility_check_enabled);
            	break;
            default:
                break;
            }

            break;
        }
    default:
        return FALSE;
    }
    return TRUE;
}




BOOL CALLBACK about_dlg_proc(const HWND hwnd, const UINT message, const WPARAM w_param, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwnd, IDB_LOGO, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_LOGO),
                                             IMAGE_BITMAP, 0, 0, 0));
        return TRUE;

    case WM_CLOSE:
        EndDialog(hwnd, IDOK);
        break;

    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDOK:
            EndDialog(hwnd, IDOK);
            break;

        case IDC_WEBSITE:
            ShellExecute(nullptr, nullptr, "http://mupen64.emulation64.com", nullptr, nullptr, SW_SHOW);
            break;
        case IDC_GITREPO:
            ShellExecute(nullptr, nullptr, "https://github.com/mkdasher/mupen64-rr-lua-/", nullptr, nullptr, SW_SHOW);
            break;
        default:
            break;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void configdialog_about()
{
	DialogBox(GetModuleHandle(NULL),
						MAKEINTRESOURCE(IDD_ABOUT), mainHWND,
						about_dlg_proc);
}

void build_rom_browser_path_list(const HWND dialog_hwnd)
{
	const HWND hwnd = GetDlgItem(dialog_hwnd, IDC_ROMBROWSER_DIR_LIST);

    SendMessage(hwnd, LB_RESETCONTENT, 0, 0);

    for (const std::string& str : Config.rombrowser_rom_paths)
    {
        SendMessage(hwnd, LB_ADDSTRING, 0,
                    (LPARAM)str.c_str());
    }
}

BOOL CALLBACK directories_cfg(const HWND hwnd, const UINT message, const WPARAM w_param, LPARAM l_param)
{
	[[maybe_unused]] LPITEMIDLIST pidl{};
    BROWSEINFO bi{};
    auto l_nmhdr = (NMHDR*)&l_param;
    switch (message)
    {
    case WM_INITDIALOG:

        build_rom_browser_path_list(hwnd);

        SendMessage(GetDlgItem(hwnd, IDC_RECURSION), BM_SETCHECK,
                    Config.is_rombrowser_recursion_enabled ? BST_CHECKED : BST_UNCHECKED, 0);

        if (Config.is_default_plugins_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_PLUGINS_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_PLUGINS_DIR), FALSE);
        }
        if (Config.is_default_saves_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_SAVES_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_SAVES_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SAVES_DIR), FALSE);
        }
        if (Config.is_default_screenshots_directory_used)
        {
            SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK), BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_SCREENSHOTS_DIR), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SCREENSHOTS_DIR), FALSE);
        }

        SetDlgItemText(hwnd, IDC_PLUGINS_DIR, Config.plugins_directory.c_str());
        SetDlgItemText(hwnd, IDC_SAVES_DIR, Config.saves_directory.c_str());
        SetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, Config.screenshots_directory.c_str());

        break;
    case WM_NOTIFY:
        if (l_nmhdr->code == PSN_APPLY)
        {
            int selected = SendDlgItemMessage(hwnd, IDC_DEFAULT_PLUGINS_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED
                               ? TRUE
                               : FALSE;
            GetDlgItemText(hwnd, IDC_PLUGINS_DIR, TempMessage, 200);
            Config.plugins_directory = std::string(TempMessage);
            Config.is_default_plugins_directory_used = selected;

            selected = SendDlgItemMessage(hwnd, IDC_DEFAULT_SAVES_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED
                           ? TRUE
                           : FALSE;
            GetDlgItemText(hwnd, IDC_SAVES_DIR, TempMessage, MAX_PATH);
            Config.saves_directory = std::string(TempMessage);
            Config.is_default_saves_directory_used = selected;

            selected = SendDlgItemMessage(hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED
                           ? TRUE
                           : FALSE;
            GetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, TempMessage, MAX_PATH);
            Config.screenshots_directory = std::string(TempMessage);
            Config.is_default_screenshots_directory_used = selected;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDC_RECURSION:
            Config.is_rombrowser_recursion_enabled = SendDlgItemMessage(hwnd, IDC_RECURSION, BM_GETCHECK, 0, 0) ==
                BST_CHECKED;
            break;
        case IDC_ADD_BROWSER_DIR:
            {
                const auto path = show_persistent_folder_dialog("f_roms", hwnd);
                if (path.empty())
                {
                    break;
                }
                Config.rombrowser_rom_paths.push_back(wstring_to_string(path));
                build_rom_browser_path_list(hwnd);
                break;
            }
        case IDC_REMOVE_BROWSER_DIR:
            {
	            if (const int32_t selected_index = SendMessage(GetDlgItem(hwnd, IDC_ROMBROWSER_DIR_LIST), LB_GETCURSEL, 0, 0); selected_index != -1)
                {
                    Config.rombrowser_rom_paths.erase(Config.rombrowser_rom_paths.begin() + selected_index);
                }
                build_rom_browser_path_list(hwnd);
                break;
            }

        case IDC_REMOVE_BROWSER_ALL:
            Config.rombrowser_rom_paths.clear();
            build_rom_browser_path_list(hwnd);
            break;

        case IDC_DEFAULT_PLUGINS_CHECK:
            {
                Config.is_default_plugins_directory_used = SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_PLUGINS_CHECK),
                                                                       BM_GETCHECK, 0, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_DIR), !Config.is_default_plugins_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_PLUGINS_DIR), !Config.is_default_plugins_directory_used);
            }
            break;
        case IDC_PLUGIN_DIRECTORY_HELP:
            {
                MessageBox(hwnd, "Changing the plugin directory may introduce bugs to some plugins.", "Info",
                           MB_ICONINFORMATION | MB_OK);
            }
            break;
        case IDC_CHOOSE_PLUGINS_DIR:
            {
                const auto path = show_persistent_folder_dialog("f_plugins", hwnd);
                if (path.empty())
                {
                    break;
                }
                Config.plugins_directory = wstring_to_string(path) + "\\";
                SetDlgItemText(hwnd, IDC_PLUGINS_DIR, Config.plugins_directory.c_str());
            }
            break;
        case IDC_DEFAULT_SAVES_CHECK:
            {
                Config.is_default_saves_directory_used = SendMessage(GetDlgItem(hwnd, IDC_DEFAULT_SAVES_CHECK),
                                                                     BM_GETCHECK, 0, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_SAVES_DIR), !Config.is_default_saves_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SAVES_DIR), !Config.is_default_saves_directory_used);
            }
            break;
        case IDC_CHOOSE_SAVES_DIR:
            {
                const auto path = show_persistent_folder_dialog("f_saves", hwnd);
                if (path.empty())
                {
                    break;
                }
                Config.saves_directory = wstring_to_string(path) + "\\";
                SetDlgItemText(hwnd, IDC_SAVES_DIR, Config.saves_directory.c_str());
            }
            break;
        case IDC_DEFAULT_SCREENSHOTS_CHECK:
            {
                Config.is_default_screenshots_directory_used = SendMessage(
                    GetDlgItem(hwnd, IDC_DEFAULT_SCREENSHOTS_CHECK), BM_GETCHECK, 0, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_SCREENSHOTS_DIR), !Config.is_default_screenshots_directory_used);
                EnableWindow(GetDlgItem(hwnd, IDC_CHOOSE_SCREENSHOTS_DIR),
                             !Config.is_default_screenshots_directory_used);
            }
            break;
        case IDC_CHOOSE_SCREENSHOTS_DIR:
            {
                const auto path = show_persistent_folder_dialog("f_screenshots", hwnd);
                if (path.empty())
                {
                    break;
                }
                Config.screenshots_directory = wstring_to_string(path) + "\\";
                SetDlgItemText(hwnd, IDC_SCREENSHOTS_DIR, Config.screenshots_directory.c_str());
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

void update_plugin_selection(const HWND hwnd, const int32_t id, const std::filesystem::path& path)
{
	for (int i = 0; i < SendDlgItemMessage(hwnd, id, CB_GETCOUNT, 0, 0); ++i)
	{
		if (const auto plugin = (t_plugin*)SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, i, 0); plugin->path == path)
		{
			SendDlgItemMessage(hwnd, id, CB_SETCURSEL, i, 0);
			break;
		}
	}
}

t_plugin* get_selected_plugin(const HWND hwnd, const int id)
{
    const int i = SendDlgItemMessage(hwnd, id, CB_GETCURSEL, 0, 0);
    const auto res = SendDlgItemMessage(hwnd, id, CB_GETITEMDATA, i, 0);
	return res == CB_ERR ? nullptr : (t_plugin*)res;
}

BOOL CALLBACK plugins_cfg(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
	[[maybe_unused]] char path_buffer[_MAX_PATH];
    NMHDR FAR* l_nmhdr = nullptr;
    memcpy(&l_nmhdr, &l_param, sizeof(NMHDR FAR*));
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(hwnd, IDOK);
        break;
    case WM_INITDIALOG:

    	// When the emu is running, we know that the plugins are locked and the comboboxes can't reveal new items, so we reuse the previous results
    	// There's also the possibility that we don't have any "previous results", so we do the search normally in that case
	    if (!emu_launched || available_plugins.empty())
	    {
    		for (auto plugin : available_plugins)
    		{
    			plugin_destroy(&plugin);
    		}

        	available_plugins = get_available_plugins();
	    }

        for (const auto& plugin : available_plugins)
        {
            int32_t id = 0;

            switch (plugin->type)
            {
            case plugin_type::video:
                id = IDC_COMBO_GFX;
                break;
            case plugin_type::audio:
                id = IDC_COMBO_SOUND;
                break;
            case plugin_type::input:
                id = IDC_COMBO_INPUT;
                break;
            case plugin_type::rsp:
                id = IDC_COMBO_RSP;
                break;
            default:
                assert(false);
                break;
            }
            // we add the string and associate a pointer to the plugin with the item
            const int i = SendDlgItemMessage(hwnd, id, CB_GETCOUNT, 0, 0);

            SendDlgItemMessage(hwnd, id, CB_ADDSTRING, 0, (LPARAM)plugin->name.c_str());
            SendDlgItemMessage(hwnd, id, CB_SETITEMDATA, i, (LPARAM)plugin);
        }

        update_plugin_selection(hwnd, IDC_COMBO_GFX, Config.selected_video_plugin);
        update_plugin_selection(hwnd, IDC_COMBO_SOUND, Config.selected_audio_plugin);
        update_plugin_selection(hwnd, IDC_COMBO_INPUT, Config.selected_input_plugin);
        update_plugin_selection(hwnd, IDC_COMBO_RSP, Config.selected_rsp_plugin);

        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_GFX), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_INPUT), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_SOUND), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_COMBO_RSP), !emu_launched);

        SendDlgItemMessage(hwnd, IDB_DISPLAY, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_DISPLAY),
                                             IMAGE_BITMAP, 0, 0, 0));
        SendDlgItemMessage(hwnd, IDB_CONTROL, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_CONTROL),
                                             IMAGE_BITMAP, 0, 0, 0));
        SendDlgItemMessage(hwnd, IDB_SOUND, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_SOUND),
                                             IMAGE_BITMAP, 0, 0, 0));
        SendDlgItemMessage(hwnd, IDB_RSP, STM_SETIMAGE, IMAGE_BITMAP,
                           (LPARAM)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDB_RSP),
                                             IMAGE_BITMAP, 0, 0, 0));
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDGFXCONFIG:
            hwnd_plug = hwnd;
            plugin_config(get_selected_plugin(hwnd, IDC_COMBO_GFX));
            break;
        case IDGFXTEST:
            hwnd_plug = hwnd;
            plugin_test(get_selected_plugin(hwnd, IDC_COMBO_GFX));
            break;
        case IDGFXABOUT:
            hwnd_plug = hwnd;
            plugin_about(get_selected_plugin(hwnd, IDC_COMBO_GFX));
            break;
        case IDINPUTCONFIG:
            hwnd_plug = hwnd;
            plugin_config(get_selected_plugin(hwnd, IDC_COMBO_INPUT));
            break;
        case IDINPUTTEST:
            hwnd_plug = hwnd;
            plugin_test(get_selected_plugin(hwnd, IDC_COMBO_INPUT));
            break;
        case IDINPUTABOUT:
            hwnd_plug = hwnd;
            plugin_about(get_selected_plugin(hwnd, IDC_COMBO_INPUT));
            break;
        case IDSOUNDCONFIG:
            hwnd_plug = hwnd;
            plugin_config(get_selected_plugin(hwnd, IDC_COMBO_SOUND));
            break;
        case IDSOUNDTEST:
            hwnd_plug = hwnd;
            plugin_test(get_selected_plugin(hwnd, IDC_COMBO_SOUND));
            break;
        case IDSOUNDABOUT:
            hwnd_plug = hwnd;
            plugin_about(get_selected_plugin(hwnd, IDC_COMBO_SOUND));
            break;
        case IDRSPCONFIG:
            hwnd_plug = hwnd;
            plugin_config(get_selected_plugin(hwnd, IDC_COMBO_RSP));
            break;
        case IDRSPTEST:
            hwnd_plug = hwnd;
            plugin_test(get_selected_plugin(hwnd, IDC_COMBO_RSP));
            break;
        case IDRSPABOUT:
            hwnd_plug = hwnd;
            plugin_about(get_selected_plugin(hwnd, IDC_COMBO_RSP));
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
        if (l_nmhdr->code == PSN_APPLY)
        {
	        if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_GFX); plugin != nullptr)
	        {
	        	Config.selected_video_plugin = plugin->path.string();
	        }
        	if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_SOUND); plugin != nullptr)
        	{
        		Config.selected_audio_plugin = plugin->path.string();
        	}
        	if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_INPUT); plugin != nullptr)
        	{
        		Config.selected_input_plugin = plugin->path.string();
        	}
        	if (const auto plugin = get_selected_plugin(hwnd, IDC_COMBO_RSP); plugin != nullptr)
        	{
        		Config.selected_rsp_plugin = plugin->path.string();
        	}
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK general_cfg(const HWND hwnd, const UINT message, const WPARAM w_param, const LPARAM l_param)
{
    NMHDR FAR* l_nmhdr = nullptr;
    memcpy(&l_nmhdr, &l_param, sizeof(NMHDR FAR*));

    switch (message)
    {
    case WM_INITDIALOG:
        set_checkbox_state(hwnd, IDC_SHOWFPS, Config.show_fps);
        set_checkbox_state(hwnd, IDC_SHOWVIS, Config.show_vis_per_second);
        set_checkbox_state(hwnd, IDC_ALERTSAVESTATEWARNINGS, Config.is_savestate_warning_enabled);
        SetDlgItemInt(hwnd, IDC_SKIPFREQ, Config.frame_skip_frequency, 0);
        set_checkbox_state(hwnd, IDC_ALLOW_ARBITRARY_SAVESTATE_LOADING,
                           Config.is_state_independent_state_loading_allowed);

        CheckDlgButton(hwnd, IDC_INTERP, Config.core_type == 0 ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_RECOMP, Config.core_type == 1 ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_PURE_INTERP, Config.core_type == 2 ? BST_CHECKED : BST_UNCHECKED);

        EnableWindow(GetDlgItem(hwnd, IDC_INTERP), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_RECOMP), !emu_launched);
        EnableWindow(GetDlgItem(hwnd, IDC_PURE_INTERP), !emu_launched);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDC_SKIPFREQUENCY_HELP:
            MessageBox(hwnd, "0 = Skip all frames, 1 = Show all frames, n = show every nth frame", "Info",
                       MB_OK | MB_ICONINFORMATION);
            break;
        case IDC_INTERP:
            if (!emu_launched)
            {
                Config.core_type = 0;
            }
            break;
        case IDC_RECOMP:
            if (!emu_launched)
            {
                Config.core_type = 1;
            }
            break;
        case IDC_PURE_INTERP:
            if (!emu_launched)
            {
                Config.core_type = 2;
            }
            break;
        default:
            break;
        }
        break;

    case WM_NOTIFY:
        if (l_nmhdr->code == PSN_APPLY)
        {
            Config.show_fps = get_checkbox_state(hwnd, IDC_SHOWFPS);
            Config.show_vis_per_second = get_checkbox_state(hwnd, IDC_SHOWVIS);
            Config.is_savestate_warning_enabled = get_checkbox_state(hwnd, IDC_ALERTSAVESTATEWARNINGS);
            Config.frame_skip_frequency = (int)GetDlgItemInt(hwnd, IDC_SKIPFREQ, nullptr, 0);
            Config.is_state_independent_state_loading_allowed = get_checkbox_state(
                hwnd, IDC_ALLOW_ARBITRARY_SAVESTATE_LOADING);
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}


BOOL CALLBACK advanced_settings_proc(const HWND hwnd, const UINT message, WPARAM, const LPARAM l_param)
{
    NMHDR FAR* l_nmhdr = nullptr;
    memcpy(&l_nmhdr, &l_param, sizeof(NMHDR FAR*));
    switch (message)
    {
    case WM_INITDIALOG:
       set_checkbox_state(hwnd, IDC_PAUSENOTACTIVE, Config.is_unfocused_pause_enabled);
       set_checkbox_state(hwnd, IDC_GUI_TOOLBAR, Config.is_toolbar_enabled);
       set_checkbox_state(hwnd, IDC_GUI_STATUSBAR, Config.is_statusbar_enabled);
       set_checkbox_state(hwnd, IDC_USESUMMERCART, Config.use_summercart);
       set_checkbox_state(hwnd, IDC_ROUNDTOZERO, Config.is_round_towards_zero_enabled);
       set_checkbox_state(hwnd, IDC_EMULATEFLOATCRASHES, Config.is_float_exception_propagation_enabled);
       set_checkbox_state(hwnd, IDC_CLUADOUBLEBUFFER, Config.is_lua_double_buffered);
       set_checkbox_state(hwnd, IDC_ENABLE_AUDIO_DELAY, Config.is_audio_delay_enabled);
       set_checkbox_state(hwnd, IDC_ENABLE_COMPILED_JUMP, Config.is_compiled_jump_enabled);
       set_checkbox_state(hwnd, IDC_RECORD_RESETS, Config.is_reset_recording_enabled);
       set_checkbox_state(hwnd, IDC_FORCEINTERNAL, Config.is_internal_capture_forced);
       set_checkbox_state(hwnd, IDC_CAPTUREOTHER, Config.is_capture_cropped_screen_dc);
       SetDlgItemInt(hwnd, IDC_CAPTUREDELAY, Config.capture_delay, 0);
        return TRUE;


    case WM_NOTIFY:
        if (l_nmhdr->code == PSN_APPLY)
        {
            Config.is_unfocused_pause_enabled = get_checkbox_state(hwnd, IDC_PAUSENOTACTIVE);
            Config.use_summercart = get_checkbox_state(hwnd, IDC_USESUMMERCART);
            Config.is_round_towards_zero_enabled = get_checkbox_state(hwnd, IDC_ROUNDTOZERO);
            Config.is_float_exception_propagation_enabled = get_checkbox_state(hwnd, IDC_EMULATEFLOATCRASHES);
            Config.is_lua_double_buffered = get_checkbox_state(hwnd, IDC_CLUADOUBLEBUFFER);
            Config.is_audio_delay_enabled = get_checkbox_state(hwnd, IDC_ENABLE_AUDIO_DELAY);
            Config.is_compiled_jump_enabled = get_checkbox_state(hwnd, IDC_ENABLE_COMPILED_JUMP);
            Config.is_reset_recording_enabled = get_checkbox_state(hwnd, IDC_RECORD_RESETS);
            Config.is_internal_capture_forced = get_checkbox_state(hwnd, IDC_FORCEINTERNAL);
            Config.is_capture_cropped_screen_dc = get_checkbox_state(hwnd, IDC_CAPTUREOTHER);
            Config.capture_delay = GetDlgItemInt(hwnd, IDC_CAPTUREDELAY, nullptr, 0);

            rombrowser_build();
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

std::string hotkey_to_string_overview(t_hotkey* hotkey)
{
    return hotkey->identifier + " (" + hotkey_to_string(hotkey) + ")";
}

void on_hotkey_selection_changed(const HWND dialog_hwnd)
{
    HWND list_hwnd = GetDlgItem(dialog_hwnd, IDC_HOTKEY_LIST);
    HWND enable_hwnd = GetDlgItem(dialog_hwnd, IDC_HOTKEY_CLEAR);
    HWND edit_hwnd = GetDlgItem(dialog_hwnd, IDC_SELECTED_HOTKEY_TEXT);
    HWND assign_hwnd = GetDlgItem(dialog_hwnd, IDC_HOTKEY_ASSIGN_SELECTED);

    char selected_identifier[MAX_PATH] = {0};
    SendMessage(list_hwnd, LB_GETTEXT, SendMessage(list_hwnd, LB_GETCURSEL, 0, 0), (LPARAM)selected_identifier);

	int32_t selected_index = ListBox_GetCurSel(list_hwnd);

	EnableWindow(enable_hwnd, selected_index != -1);
	EnableWindow(edit_hwnd, selected_index != -1);
	EnableWindow(assign_hwnd, selected_index != -1);

	SetWindowText(assign_hwnd, "Assign...");

    if (selected_index == -1)
    {
    	SetWindowText(edit_hwnd, "");
    	return;
    }

	auto hotkey = (t_hotkey*)ListBox_GetItemData(list_hwnd, selected_index);
	SetWindowText(edit_hwnd, hotkey_to_string(hotkey).c_str());
}

void build_hotkey_list(HWND hwnd)
{
	HWND list_hwnd = GetDlgItem(hwnd, IDC_HOTKEY_LIST);

	char search_query_c[MAX_PATH] = {0};
	GetDlgItemText(hwnd, IDC_HOTKEY_SEARCH, search_query_c, std::size(search_query_c));
	std::string search_query = search_query_c;

    ListBox_ResetContent(list_hwnd);

    for (t_hotkey* hotkey : hotkeys)
    {
        std::string hotkey_string = hotkey_to_string_overview(hotkey);

        if (!search_query.empty())
        {
            if (!contains(to_lower(hotkey_string), to_lower(search_query))
                && !contains(to_lower(hotkey->identifier), to_lower(search_query)))
            {
                continue;
            }
        }

    	int32_t index = ListBox_GetCount(list_hwnd);
    	ListBox_AddString(list_hwnd, hotkey_string.c_str());
		ListBox_SetItemData(list_hwnd, index, hotkey);
    }
}

BOOL CALLBACK hotkeys_proc(const HWND hwnd, const UINT message, const WPARAM w_param, LPARAM)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            SetDlgItemText(hwnd, IDC_HOTKEY_SEARCH, "");
            on_hotkey_selection_changed(hwnd);
            return TRUE;
        }
    case WM_COMMAND:
        {
    		HWND list_hwnd = GetDlgItem(hwnd, IDC_HOTKEY_LIST);
    		const int event = HIWORD(w_param);
    		const int id = LOWORD(w_param);
	        switch (id)
	        {
	        case IDC_HOTKEY_LIST:
		        if (event == LBN_SELCHANGE)
		        {
		        	on_hotkey_selection_changed(hwnd);
		        }
	        	if (event == LBN_DBLCLK)
	        	{
	        		on_hotkey_selection_changed(hwnd);
	        		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_HOTKEY_ASSIGN_SELECTED, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDC_HOTKEY_ASSIGN_SELECTED));
	        	}
	        	break;
	        case IDC_HOTKEY_ASSIGN_SELECTED:
		        {
	        		int32_t index = ListBox_GetCurSel(list_hwnd);
			        if (index == -1)
			        {
				        break;
			        }
	        		auto hotkey = (t_hotkey*)ListBox_GetItemData(list_hwnd, index);
	        		SetDlgItemText(hwnd, id, "...");
	        		get_user_hotkey(hotkey);

	        		build_hotkey_list(hwnd);
	        		ListBox_SetCurSel(list_hwnd, (index + 1) % ListBox_GetCount(list_hwnd));
	        		on_hotkey_selection_changed(hwnd);
		        }
	        	break;
	        case IDC_HOTKEY_SEARCH:
		        {
	        		build_hotkey_list(hwnd);
		        }
	        	break;
	        case IDC_HOTKEY_CLEAR:
	        	{
	        		int32_t index = ListBox_GetCurSel(list_hwnd);
	        		if (index == -1)
	        		{
	        			break;
	        		}
	        		auto hotkey = (t_hotkey*)ListBox_GetItemData(list_hwnd, index);
	        		hotkey->key = 0;
	        		hotkey->ctrl = 0;
	        		hotkey->shift = 0;
	        		hotkey->alt = 0;
	        		build_hotkey_list(hwnd);
	        		on_hotkey_selection_changed(hwnd);
	        	}
	        	break;
	        }
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void configdialog_show()
{
    PROPSHEETPAGE psp[6] = {0};
    for (auto& i : psp)
    {
	    i.dwSize = sizeof(PROPSHEETPAGE);
	    i.dwFlags = PSP_USETITLE;
	    i.hInstance = app_instance;
    }

	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_MAIN);
    psp[0].pfnDlgProc = plugins_cfg;
    psp[0].pszTitle = "Plugins";

    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_DIRECTORIES);
    psp[1].pfnDlgProc = directories_cfg;
    psp[1].pszTitle = "Directories";

    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_MESSAGES);
    psp[2].pfnDlgProc = general_cfg;
    psp[2].pszTitle = "General";

    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_ADVANCED_OPTIONS);
    psp[3].pfnDlgProc = advanced_settings_proc;
    psp[3].pszTitle = "Advanced";

    psp[4].pszTemplate = MAKEINTRESOURCE(IDD_NEW_HOTKEY_DIALOG);
    psp[4].pfnDlgProc = hotkeys_proc;
    psp[4].pszTitle = "Hotkeys";

    psp[5].pszTemplate = MAKEINTRESOURCE(IDD_OTHER_OPTIONS_DIALOG);
    psp[5].pfnDlgProc = other_options_proc;
    psp[5].pszTitle = "Other";

	PROPSHEETHEADER psh = {0};
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    psh.hwndParent = mainHWND;
    psh.hInstance = app_instance;
    psh.pszCaption = "Settings";
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE)&psp;

    const CONFIG old_config = Config;

    if (!PropertySheet(&psh))
    {
        Config = old_config;
    }

    save_config();
    rombrowser_build();
    update_menu_hotkey_labels();
}


