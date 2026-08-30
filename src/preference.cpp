#include "stdafx.h"
#include "resource.h"
#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>
#include "global_config.h"
#include "GUID.h"

class CAdlibPreferences : public CDialogImpl<CAdlibPreferences>, public preferences_page_instance {
private:
	preferences_page_callback::ptr m_callback;
	fb2k::CDarkModeHooks m_dark;
	int m_sel_sample_rate;
	int m_sel_core;
	bool m_is_surround;
public:
	CAdlibPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {};
	enum {IDD=IDD_DIALOG_ADLIB};

	t_uint32 get_state() {
		t_uint32 state = preferences_state::dark_mode_supported | preferences_state::resettable;
		if (HasChanged()) state |= preferences_state::changed;
		return state;
	}
	void apply() {
		cfg_adlib_samplerate = m_sel_sample_rate;
		cfg_adlib_core = m_sel_core;
		cfg_adlib_surround = m_is_surround;
		OnChanged();
	}
	void reset() {
		CComboBox sample_combo(GetDlgItem(IDC_COMBO_SAMPLE));
		CComboBox core_combo(GetDlgItem(IDC_COMBO_CORE));
		sample_combo.SetCurSel(default_sample_rate_idx);
		core_combo.SetCurSel(default_core_idx);
		m_sel_sample_rate = default_sample_rate_idx;
		m_sel_core = default_core_idx;
		m_is_surround = default_is_surround;
		OnChanged();
	}
	void OnChanged() {
		m_callback->on_state_changed();
	}
	bool HasChanged() {
		return m_sel_sample_rate != (int)cfg_adlib_samplerate || m_sel_core != (int)cfg_adlib_core || m_is_surround != (bool)cfg_adlib_surround;
	}
	BEGIN_MSG_MAP_EX(CAdlibPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_COMBO_SAMPLE,CBN_SELCHANGE,OnSampleRateChange)
		COMMAND_HANDLER_EX(IDC_COMBO_CORE,CBN_SELCHANGE,OnCoreChange)
		COMMAND_HANDLER_EX(IDC_CHECK_SURROUND,BN_CLICKED,IsSurround)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM) {
		m_dark.AddDialogWithControls(*this);
		m_sel_sample_rate = cfg_adlib_samplerate;
		m_sel_core = cfg_adlib_core;
		m_is_surround = cfg_adlib_surround;
		CComboBox sample_combo(GetDlgItem(IDC_COMBO_SAMPLE));
		for (int i = 0; i < table_count; i++) {
			wchar_t buffer[32];
			wsprintfW(buffer, L"%u Hz", sample_rate_table[i]);
			SendDlgItemMessage(IDC_COMBO_SAMPLE, CB_ADDSTRING, 0, (LPARAM)buffer);
		}

		CComboBox core_combo(GetDlgItem(IDC_COMBO_CORE));
		core_combo.AddString(L"Harekiet's");
		core_combo.AddString(L"Ken Sliverman's");
		core_combo.AddString(L"Jarek Buczy¨½ski's");
		core_combo.AddString(L"Tatsuyuki Satoh's");
		core_combo.AddString(L"Nuked OPL3");

		sample_combo.SetCurSel((int)cfg_adlib_samplerate);//defalut: 49716 Hz
		core_combo.SetCurSel((int)cfg_adlib_core);
		CheckDlgButton(IDC_CHECK_SURROUND, (bool)cfg_adlib_surround ? BST_CHECKED : BST_UNCHECKED);
		return FALSE;
	}
	void OnSampleRateChange(UINT, int ctl, CWindow) {
		CComboBox combo(GetDlgItem(ctl));
		m_sel_sample_rate = combo.GetCurSel();
		OnChanged();
	}
	void OnCoreChange(UINT, int ctl, CWindow) {
		CComboBox combo(GetDlgItem(ctl));
		m_sel_core = combo.GetCurSel();
		OnChanged();
	}
	void IsSurround(UINT, int, CWindow) {
		m_is_surround = IsDlgButtonChecked(IDC_CHECK_SURROUND);
		OnChanged();
	}
};

class preferences_page_adlib_decoder_impl : public preferences_page_impl<CAdlibPreferences> {
public:
	const char* get_name() { return "AdLib OPL Decoder"; }
	GUID get_guid() { return guid_preference; }
	GUID get_parent_guid() { return preferences_page::guid_input; }
};

static preferences_page_factory_t<preferences_page_adlib_decoder_impl> g_preferences_page_adlib_decoder_impl_factory;