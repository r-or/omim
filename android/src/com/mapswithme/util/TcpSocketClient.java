package com.mapswithme.util;


import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.Calendar;
import java.util.Timer;
import java.util.TimerTask;

public class TcpSocketClient {
  private final String TAG = "TcpSocketClient";

  private boolean DEBUG = true;
  private String mDbgStr =
      "{\"cDist\":\"170%20m\",\"cTurn\":11,\"cTurnExNum\":69,\"nTurn\":5,\"cStreet\":\"Harvard Street qjaj^$#@xyz\"," +
      "\"nStreet\":\"College Avenue - and some more\",\"tDist\":\"420%20km\",\"tPerc\"" +
      ":\"0\",\"tTimeLeft\":\"60\",\"cSpeed\":\"999.99km/h\"}";

  private boolean mRun;
  private volatile Socket mSocket;
  private InetSocketAddress mAddress;
  private boolean mConnecting;

  private final int mConnectionTimeoutMs = 5000;
  private final int mPingUpdateSpeedMs = 5000;
  private final int mTimeSyncRateMs = 60000;
  private final int mOutUpdateSpeedMs = 500;

  private boolean mConnTimeout;

  private boolean mPingScheduled;
  private boolean mRoutingInfoUpdateScheduled;
  private boolean mTimeSyncScheduled;
  private String mRoutingInfoJson;

  private enum Command {
    PING                (0x01),
    PONG                (0x02),
    UPDATE_TIME         (0x08),
    UPDATE_ROUTING_INFO (0x10);

    private final int val;

    Command(int val) {
      this.val = val;
    }

    public int getVal() {
      return val;
    }
  }

  public TcpSocketClient() {
    mRun = true;
    mConnecting = false;
    mAddress = InetSocketAddress.createUnresolved(Config.externalDisplayIp(), Integer.parseInt(Config.externalDisplayPort()));
    mConnTimeout = false;
    mPingScheduled = true;
    mTimeSyncScheduled = true;
    mRoutingInfoUpdateScheduled = false;
    if (DEBUG) {
      mRoutingInfoJson = mDbgStr;
    }
    else {
      mRoutingInfoJson = "";
    }
    final Timer outTimer = new Timer();
    TimerTask outTimerTask = new TimerTask() {
      @Override
      public void run() {
        if (!mRun) {
          outTimer.cancel();
          outTimer.purge();
          return;
        }
        if (mPingScheduled) {
          mPingScheduled = false;
          if (!checkSocket() || !keepAlivePing()) {
            mSocket = null;
            Log.e(TAG, "Ping: couldn't send ping; mSocket not established. Wrong IP / net?");
          }
        } else if (mSocket != null) {
          if (mTimeSyncScheduled) {
            mTimeSyncScheduled = false;
            sendTimeInfo();
          }
          else if (mRoutingInfoUpdateScheduled || DEBUG) {
            mRoutingInfoUpdateScheduled = false;
            sendJsonRoutingData(mRoutingInfoJson);
          }
        }
      }
    };
    outTimer.scheduleAtFixedRate(outTimerTask, mOutUpdateSpeedMs, mOutUpdateSpeedMs);

    final Timer pingTimer = new Timer();
    TimerTask pingTimerTask = new TimerTask() {
      @Override
      public void run() {
        if (!mRun) {
          pingTimer.cancel();
          pingTimer.purge();
          return;
        }
        mPingScheduled = true;
      }
    };
    pingTimer.scheduleAtFixedRate(pingTimerTask, mPingUpdateSpeedMs, mPingUpdateSpeedMs);

    final Timer timeSyncTimer = new Timer();
    TimerTask syncTimerTask = new TimerTask() {
      @Override
      public void run() {
        if (!mRun) {
          timeSyncTimer.cancel();
          timeSyncTimer.purge();
          return;
        }
        mTimeSyncScheduled = true;
      }
    };
    timeSyncTimer.scheduleAtFixedRate(syncTimerTask, mTimeSyncRateMs, mTimeSyncRateMs);
  }

  private class ClientThread implements Runnable {
    @Override
    public void run() {
      Log.d(TAG, "ClientThread: Trying to connect to " + Config.externalDisplayIp() + ":" + Config.externalDisplayPort());
      mAddress = new InetSocketAddress(Config.externalDisplayIp(), Integer.parseInt(Config.externalDisplayPort()));
      if (mAddress.getAddress() != null) {
        mConnecting = true;
        try {
          mSocket = new Socket(mAddress.getAddress(), mAddress.getPort());
          Log.d(TAG, "ClientThread: connected!");
          scheduleRoutingInfoUpdate("");
        } catch (Exception e) {
          /**/
        }
        mConnecting = false;
      }
    }
  }

  private ByteArrayOutputStream readFromSocket() {
    try {
      DataInputStream dIn = new DataInputStream(mSocket.getInputStream());
      ByteArrayOutputStream bIn = new ByteArrayOutputStream();
      mConnTimeout = false;
      Timer customConnTimer = new Timer();
      customConnTimer.schedule(new TimerTask() {
        @Override
        public void run() {
          mConnTimeout = true;
          try {
            if (mSocket != null)
              mSocket.close();
            else
              Log.d(TAG, "readFromSocket: couldn't close socket!");
          } catch (IOException e) {
            e.printStackTrace();
          }
          Log.d(TAG, "readFromSocket: reached custom timeout!");
        }
      }, mConnectionTimeoutMs);
      while (true) {
        byte cB;
        try {
          cB = dIn.readByte();
        } catch (IOException e) {
          e.printStackTrace();
          // broken connection
          try {
            mSocket.close();
            Log.d(TAG, "readFromSocket: connection broken, socket closed!");
          } catch (IOException e1) {
            e1.printStackTrace();
          }
          mConnTimeout = false;
          return null;
        }
        bIn.write(cB);
        if (cB == 0x00) {
          customConnTimer.cancel();
          break;
        }
        if (mConnTimeout) {
          mConnTimeout = false;
          return null;
        }
      }
      return bIn;
    } catch (IOException e) {
      e.printStackTrace();
    }
    return null;
  }

  private boolean keepAlivePing() {
    Log.i(TAG, "Ping: starting...");
    long startTime = System.nanoTime();
    if (!sendBarr(Command.PING, null)) {
      // broken connection!
      try {
        mSocket.close();
        Log.i(TAG, "Ping: socket closed!");
      } catch (IOException e1) {
        e1.printStackTrace();
      }
      return false;
    }

    ByteArrayOutputStream bIn = readFromSocket();
    if (bIn == null || bIn.toByteArray().length < 4)
      return false;

    long roundTrip = System.nanoTime() - startTime;
    byte[] bytesReceived = bIn.toByteArray();
    if (bytesReceived[2] == Command.PONG.getVal())
      Log.i(TAG, "Ping: received PONG! Round trip time: " + String.format("%.2fms", (double) roundTrip / 1000000.0));
    else
      Log.i(TAG, "Ping: wrong answer, expected PONG!");

    return true;
  }

  private boolean checkSocket() {
    if (mSocket != null && mSocket.isClosed()) {
      Log.d(TAG, "Socked is closed - deleting");
      mSocket = null;
    }

    if (mSocket != null && (!mSocket.getInetAddress().getHostAddress().equals(Config.externalDisplayIp())
        || mSocket.getPort() != Integer.parseInt(Config.externalDisplayPort()))) {
      // Config changed! Shutdown mSocket...
      try {
        mSocket.close();
      } catch (IOException e) {
        e.printStackTrace();
      }
      Log.d(TAG, "Closed mSocket: " + mSocket);
      mSocket = null;
    }

    if (mSocket == null && !mConnecting) {
      new Thread(new ClientThread()).start();
      try {
        Thread.sleep(mConnectionTimeoutMs);
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
    }
    return mSocket != null && mSocket.isConnected();
  }

  private void sendJsonRoutingData(String text) {
    try {
      sendBarr(Command.UPDATE_ROUTING_INFO, text.getBytes("ISO-8859-1"));
    } catch (UnsupportedEncodingException e) {
      e.printStackTrace();
    }
  }

  private void sendTimeInfo() {
    Calendar cCal = Calendar.getInstance();
    long cTime =  cCal.get(Calendar.HOUR_OF_DAY) * 3600000
        + cCal.get(Calendar.MINUTE) * 60000
        + cCal.get(Calendar.SECOND) * 1000
        + cCal.get(Calendar.MILLISECOND);
    Log.d(TAG, "Updating time: " + Long.toString(cTime) + "ms today...");
    try {
      sendBarr(Command.UPDATE_TIME, Long.toString(cTime).getBytes("ISO-8859-1"));
    } catch (UnsupportedEncodingException e) {
      e.printStackTrace();
    }
  }

  private ByteArrayOutputStream constructPackage(Command cmd, byte[] payload) {
    ByteArrayOutputStream bOut = new ByteArrayOutputStream();
    int outLen = (payload == null) ? 4 : payload.length + 4;
    if (outLen > (int) Math.pow(2.0, 15.0)) {
      Log.e(TAG, "Output length exceeds 15 bits! (length = " + Integer.toString(outLen) + ")");
      return null;
    }
    bOut.write((byte) ((outLen >>> 8) | 0x80));                // high byte first: 0b1xxxxxxx
    bOut.write((byte) (outLen & 0xFF));                        // low byte         0bxxxxxxxx
    bOut.write((byte) cmd.getVal());                          // command
    if (payload != null) {
      try {
        bOut.write(payload);                                       // payload
      } catch (IOException e) {
        e.printStackTrace();
        return null;
      }
    }
    bOut.write((byte)0);                                      // zero termination
    return bOut;
  }

  private boolean sendBarr(Command cmd, byte[] payload) {
    ByteArrayOutputStream bOut = constructPackage(cmd, payload);
    if (bOut == null)
      return false;
    //Log.d(TAG, "Sending len: " + Integer.toString(bOut.size()) + "; '" + bOut.toString());

    try {
      Log.d(TAG, "sending: '" + bOut.toString() + "'");
      DataOutputStream dOut = new DataOutputStream(mSocket.getOutputStream());
      dOut.write(bOut.toByteArray());
      dOut.flush();
    } catch (IOException e) {
      e.printStackTrace();
      return false;
    }
    return true;
  }

  public void scheduleRoutingInfoUpdate(String jsonTxt) {
    if (!jsonTxt.equals(mRoutingInfoJson) || DEBUG) {
      if (!jsonTxt.equals(""))
        mRoutingInfoJson = jsonTxt;
      mRoutingInfoUpdateScheduled = true;
    }
  }

  public void close() {
    mRun = false;
    if (mSocket != null) {
      try {
        mSocket.close();
      } catch (IOException e) {
        e.printStackTrace();
      }
    }
    mSocket = null;
  }
}