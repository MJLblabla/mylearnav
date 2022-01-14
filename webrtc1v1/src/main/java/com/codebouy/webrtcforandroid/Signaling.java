package com.codebouy.webrtcforandroid;

import android.util.Log;

import org.java_websocket.handshake.ServerHandshake;
import org.json.JSONException;
import org.json.JSONObject;

import java.net.URI;
import java.net.URISyntaxException;

/**
 * @Author: Codeboy 2685312867@qq.com
 * @Date: 2019-12-21 20:47
 */

public class Signaling implements WSocketClientDelegate {

    WSocketClient client;

    String url;

    String selfId;

    SignalingDelegate delegate;

    public Signaling(String url, String selfId) {
        this.url = url;
        this.selfId = selfId;

        try {
            this.client = WSocketClient.getInstance(new URI(url));
            this.client.delegate = this;
            this.client.connect();
        } catch (URISyntaxException e) {
            e.printStackTrace();
        }
    }

    public void send(JSONObject object, String event) throws JSONException {
        object.put("type", event);
        Log.i("send", object.toString());
        client.send(object.toString());

    }

    @Override
    public void onOpen(ServerHandshake handshakedata) throws JSONException {
        Log.i("socket", "socket 连接成功");
        JSONObject jsonObject = new JSONObject();
        jsonObject.put("name", "hello");
        jsonObject.put("id", selfId);
        this.send(jsonObject, "new");
    }

    @Override
    public void onMessage(String message) throws JSONException {
        Log.i("msg", message);
        JSONObject jsonObject = new JSONObject(message);
        this.delegate.onMessage(jsonObject);
    }

    @Override
    public void onClose(int code, String reason, boolean remote) {
        Log.i("close", reason);
    }

    @Override
    public void onError(Exception ex) {
        Log.i("onError", ex.toString());
    }
}
