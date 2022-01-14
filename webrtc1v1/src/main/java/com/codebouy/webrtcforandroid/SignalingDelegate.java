package com.codebouy.webrtcforandroid;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * @Author: Codeboy 2685312867@qq.com
 * @Date: 2019-12-21 20:47
 */

public interface SignalingDelegate {
    public void onMessage(JSONObject data) throws JSONException;
}
