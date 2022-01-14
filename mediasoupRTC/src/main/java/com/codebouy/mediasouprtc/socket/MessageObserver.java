package com.codebouy.mediasouprtc.socket;

import org.json.JSONObject;



public interface MessageObserver {
    interface Observer {
        void on(String method, int id, boolean notification,
                JSONObject data);
    }

    interface Subscriber {
        void register(Observer observer);

        void unregister(Observer observer);


        void notifyObservers(String method, int id, boolean notification,
                             JSONObject data);
    }
}
