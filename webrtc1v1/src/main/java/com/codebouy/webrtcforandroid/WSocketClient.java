package com.codebouy.webrtcforandroid;

import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;
import org.json.JSONException;

import java.net.URI;

/**
 * @Author: Codeboy 2685312867@qq.com
 * @Date: 2019-12-21 20:44
 */

public class WSocketClient extends WebSocketClient {

    private static volatile WSocketClient client = null;

    public WSocketClientDelegate delegate;


    public static WSocketClient getInstance(URI uri) {

        if (client == null) {
            synchronized (WSocketClient.class) {
                if (client == null) {
                    client = new WSocketClient(uri);
                }
            }
        }
        return client;
    }

    public WSocketClient(URI serverUri) {
        super(serverUri);
    }

    @Override
    public void onOpen(ServerHandshake handshakedata) {
        try {
            delegate.onOpen(handshakedata);
        } catch (JSONException e) {
            e.printStackTrace();
        }

    }

    @Override
    public void onMessage(String message) {
        try {
            delegate.onMessage(message);
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onClose(int code, String reason, boolean remote) {
        delegate.onClose(code, reason, remote);

    }

    @Override
    public void onError(Exception ex) {
        delegate.onError(ex);
    }
}
