package com.mapswithme.maps.routing;

import com.loopj.android.http.*;
import cz.msebera.android.httpclient.Header;

public class ExternalDisplay
{
    private final AsyncHttpClient mHttpClient = new AsyncHttpClient();

    private String mIpAddress;

    public ExternalDisplay()
    {
        mIpAddress = "192.168.4.1"
    }

    public updateVehicle(RoutingInfo info)
    {
        // text
        String url = mIpAddress + "/display?command=print";
        RequestParams params = new RequestParams();

        params.put(info.distToTurn + info.turnUnits);

        mHttpClient.post(this, url, params, "application/json", null);

    }

}