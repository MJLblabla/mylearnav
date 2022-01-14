package com.codebouy.mediasouprtc.socket;

import org.java_websocket.client.WebSocketClient;
import org.java_websocket.drafts.Draft;
import org.java_websocket.drafts.Draft_6455;
import org.java_websocket.extensions.IExtension;
import org.java_websocket.handshake.ServerHandshake;
import org.java_websocket.protocols.IProtocol;
import org.java_websocket.protocols.Protocol;

import java.net.URI;
import java.util.ArrayList;
import java.util.Collections;



public class WSocketClient extends WebSocketClient {
    //private static volatile WSocketClient client = null;

    public WSocketClientDelegate delegate;


    public WSocketClient(URI serverUri, Draft protocolDraft) {
        super(serverUri, protocolDraft);
    }

    public static Draft getDraftInfo() {

        ArrayList<IProtocol> protocols = new ArrayList<IProtocol>();
        protocols.add(new Protocol("protoo"));
        Draft_6455 draft_6455 = new Draft_6455(Collections.<IExtension>emptyList(),
                protocols);
        return draft_6455;
    }

    // socket连接成功
    @Override
    public void onOpen(ServerHandshake handshakedata) {
        delegate.onOpen(handshakedata);
    }

    // 收到消息
    @Override
    public void onMessage(String message) {
        delegate.onMessage(message);
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
