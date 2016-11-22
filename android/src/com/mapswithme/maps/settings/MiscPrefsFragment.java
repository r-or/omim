package com.mapswithme.maps.settings;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.TwoStatePreference;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Config;
import com.mapswithme.util.statistics.MytargetHelper;
import com.mapswithme.util.statistics.Statistics;

public class MiscPrefsFragment extends BaseXmlSettingsFragment
{
  @Override
  protected int getXmlResources()
  {
    return R.xml.prefs_misc;
  }

  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    Preference pref = findPreference(getString(R.string.pref_send_statistics));
    ((TwoStatePreference)pref).setChecked(Config.isStatisticsEnabled());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue)
      {
        Statistics.INSTANCE.setStatEnabled((Boolean) newValue);
        return true;
      }
    });

    pref = findPreference(getString(R.string.pref_play_services));

    if (GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(MwmApplication.get()) != ConnectionResult.SUCCESS)
      getPreferenceScreen().removePreference(pref);
    else
    {
      ((TwoStatePreference) pref).setChecked(Config.useGoogleServices());
      pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
      {
        @Override
        public boolean onPreferenceChange(Preference preference, Object newValue)
        {
          boolean oldVal = Config.useGoogleServices();
          boolean newVal = ((Boolean) newValue).booleanValue();
          if (oldVal != newVal)
          {
            Config.setUseGoogleService(newVal);
            LocationHelper.INSTANCE.initProvider(false /* forceNative */);
          }
          return true;
        }
      });
    }

    pref = findPreference(getString(R.string.pref_ext_disp));
    ((TwoStatePreference) pref).setChecked(Config.useExternalDisplay());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue) {
        boolean oldVal = Config.useExternalDisplay();
        boolean newVal = (Boolean) newValue;
        if (oldVal != newVal)
        {
          Config.setUseExternalDisplay(newVal);
          Preference ip = findPreference(getString(R.string.pref_ext_disp_server_ip));
          Preference port = findPreference(getString(R.string.pref_ext_disp_server_port));
          ip.setEnabled(newVal);
          ip.setSelectable(newVal);
          port.setEnabled(newVal);
          port.setSelectable(newVal);
        }
        return true;
      }
    });

    pref = findPreference(getString(R.string.pref_ext_disp_server_ip));
    if (!Config.externalDisplayIp().equals(""))
      pref.setSummary(Config.externalDisplayIp());
    pref.setEnabled(Config.useExternalDisplay());
    pref.setSelectable(Config.useExternalDisplay());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue) {
        String oldVal = Config.externalDisplayIp();
        String newVal = (String) newValue;
        if (!oldVal.equals(newVal))
        {
          Config.setExternalDisplayIp(newVal);
          preference.setSummary(newVal);
        }
        return true;
      }
    });

    pref = findPreference(getString(R.string.pref_ext_disp_server_port));
    if (!Config.externalDisplayPort().equals(""))
      pref.setSummary(Config.externalDisplayPort());
    pref.setEnabled(Config.useExternalDisplay());
    pref.setSelectable(Config.useExternalDisplay());
    pref.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener()
    {
      @Override
      public boolean onPreferenceChange(Preference preference, Object newValue) {
        String oldVal = Config.externalDisplayPort();
        String newVal = (String) newValue;
        if (!oldVal.equals(newVal))
        {
          Config.setExternalDisplayPort(newVal);
          preference.setSummary(newVal);
        }
        return true;
      }
    });

    if (!MytargetHelper.isShowcaseSwitchedOnServer())
      getPreferenceScreen().removePreference(findPreference(getString(R.string.pref_showcase_switched_on)));
  }
}
