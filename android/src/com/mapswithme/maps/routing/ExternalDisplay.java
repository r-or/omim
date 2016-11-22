package com.mapswithme.maps.routing;

import android.util.Pair;
import com.mapswithme.util.Config;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.TcpSocketClient;
import com.mapswithme.util.UiUtils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.text.DateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Timer;
import java.util.TimerTask;

public class ExternalDisplay
{
  private static TcpSocketClient mTcpClient;
  private Pair<String, String> mSpeed;
  private boolean mOutValid = true;
  private final int mUpdateSpeedMs = 500;

  public ExternalDisplay()
  {
    mSpeed = StringUtils.nativeFormatSpeedAndUnits(0.0);
    mTcpClient = new TcpSocketClient();
    Timer outTimer = new Timer();
    TimerTask outTimerTask = new TimerTask() {
      @Override
      public void run() {
        mOutValid = true;
      }
    };
    outTimer.scheduleAtFixedRate(outTimerTask, mUpdateSpeedMs, mUpdateSpeedMs);
  }

  public void updateVehicle(RoutingInfo info)
  {
    if (!mOutValid)
      return;

    final Calendar currentTime = Calendar.getInstance();

    JSONObject rqParams = new JSONObject();
    try {
      rqParams.put("cDist", info.distToTurn + "%20" + info.turnUnits);
      rqParams.put("cTurn", info.vehicleTurnDirection.ordinal());
      if (RoutingInfo.VehicleTurnDirection.isRoundAbout(info.vehicleTurnDirection))
        rqParams.put("cTurnExNum", info.exitNum);
      if (info.vehicleNextTurnDirection.containsNextTurn())
        rqParams.put("nTurn", info.vehicleNextTurnDirection.ordinal());
      rqParams.put("cStreet", info.currentStreet);
      rqParams.put("nStreet", info.nextStreet);
      rqParams.put("tDist", info.distToTarget + "%20" + info.targetUnits);
      rqParams.put("tPerc", Integer.toString((int) (info.completionPercent + 0.5)));
      rqParams.put("tTimeLeft", info.totalTimeInSeconds);
      rqParams.put("cTime", currentTime.getTimeInMillis() / 1000);
      rqParams.put("cSpeed", mSpeed.first + mSpeed.second);
    } catch (JSONException e) {
      e.printStackTrace();
    }

    mTcpClient.scheduleRoutingInfoUpdate(rqParams.toString());
    mOutValid = false;
  }

  public void setSpeed(Pair<String, String> speed)
  {
    mSpeed = speed;
  }

  public void close() {
    mTcpClient.close();
  }
}