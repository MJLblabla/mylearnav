package com.codebouy.mediasouprtc.socket;

import org.protoojs.droid.Message;
import org.protoojs.droid.transports.AbsWebSocketTransport;

public interface SocketLister extends AbsWebSocketTransport.Listener {

        /** @param message {@link Message} */
        void onMessage(String message);

}
