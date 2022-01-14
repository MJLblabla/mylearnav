package com.codebouy.webrtcforandroid;

import org.java_websocket.handshake.ServerHandshake;
import org.json.JSONException;

/**
 * @Author: Codeboy 2685312867@qq.com
 * @Date: 2019-12-21 20:45
 */

public interface WSocketClientDelegate {


    public void onOpen(ServerHandshake handshakedata) throws JSONException;


    public void onMessage(String message) throws JSONException;


    public void onClose(int code, String reason, boolean remote);


    public void onError(Exception ex);
}
