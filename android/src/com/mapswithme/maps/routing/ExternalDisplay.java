package com.mapswithme.maps.routing;

import android.util.Pair;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.TcpSocketClient;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Timer;
import java.util.TimerTask;

public class ExternalDisplay
{
  private static TcpSocketClient mTcpClient;
  private Pair<String, String> mSpeed;
  private boolean mOutValid = true;
  private final int mUpdateSpeedMs = 240;

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

    JSONObject rqParams = new JSONObject();
    try {
      rqParams.put("cDist", info.distToTurn + info.turnUnits);
      rqParams.put("cTurn", info.vehicleTurnDirection.ordinal());
      if (RoutingInfo.VehicleTurnDirection.isRoundAbout(info.vehicleTurnDirection))
        rqParams.put("cTurnExNum", info.exitNum);
      if (info.vehicleNextTurnDirection.containsNextTurn())
        rqParams.put("nTurn", info.vehicleNextTurnDirection.ordinal());
      rqParams.put("cStreet", info.currentStreet);
      rqParams.put("nStreet", info.nextStreet);
      rqParams.put("tDist", info.distToTarget + info.targetUnits);
      rqParams.put("tPerc", Integer.toString((int) (info.completionPercent + 0.5)));
      rqParams.put("tTimeLeft", info.totalTimeInSeconds);
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

  public void close()
  {
    mTcpClient.close();
  }
}