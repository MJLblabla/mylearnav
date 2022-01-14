package com.codebouy.mediasouprtc.socket;

import android.util.Log;


import com.codebouy.mediasouprtc.utils.RandomString;

import org.json.JSONException;
import org.json.JSONObject;
import org.protoojs.droid.Message;

import java.util.concurrent.Callable;
import java.util.concurrent.CopyOnWriteArraySet;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import kotlin.jvm.functions.Function0;


public class EchoSocket implements MessageObserver.Subscriber {
    private static final String TAG = "EchoSocket";

    private final CopyOnWriteArraySet<MessageObserver.Observer> mObservers;
    private CountDownLatch mLatch;
    private final ExecutorService mExecutorService;

    public WebSocketTransport mSocket;

    private SocketLister listener = new SocketLister() {

        @Override
        public void onOpen() {
            notifyObservers(ActionEvent.OPEN, 0, false, null);
        }

        /**
         * Connection could not be established in the first place.
         */
        @Override
        public void onFail() {
            Log.d("EchoSocket", "onFail");
        }

        /**
         * @param message {@link Message}
         */
        @Override
        public void onMessage(Message message) {

        }

        /**
         * @param message {@link Message}
         */
        @Override
        public void onMessage(String message) {
            Log.d(TAG, "onMessage text=" + message);
            try {
                JSONObject jsonObject = new JSONObject(message);
                int id = -1;
                if (!jsonObject.isNull("id")) {
                    id = jsonObject.getInt("id");
                }
                String method = getJsonString(jsonObject, "method");
                boolean notification = false;
                if (!jsonObject.isNull("notification")) {
                    notification = jsonObject.getBoolean("notification");
                }
                JSONObject data = jsonObject.getJSONObject("data");
                notifyObservers(method, id, notification, data);
            } catch (JSONException e) {
                Log.e(TAG, "Failed to handle message", e);
                e.printStackTrace();
            }
        }

        /**
         * A previously established connection was lost unexpected.
         */
        @Override
        public void onDisconnected() {
            Log.d("EchoSocket", "onDisconnected");
        }

        @Override
        public void onClose() {
            Log.d("EchoSocket", "onClose");
            mSocket = null;
        }
    };

    public EchoSocket() {
        mObservers = new CopyOnWriteArraySet<>();
        mExecutorService = Executors.newSingleThreadExecutor();
    }

    public void connect(String wsUrl, Function0 call) {
        Log.d(TAG, "connect wsUrl=" + wsUrl);
        if (!wsUrl.startsWith("ws://") && !wsUrl.startsWith("wss://")) {
            throw new RuntimeException("Socket url must start with ws/wss");
        }

        if (mSocket != null) {
            throw new IllegalStateException("Socket is already defined");
        }
        MessageObserver.Observer observer = (method, id, notification, data) -> {
            Log.d(TAG, "GOT EVENT " + method);
            if (method.equals("open")) {
                mObservers.remove(this);
                call.invoke();
            }
        };
        mObservers.add(observer);
        mSocket = new WebSocketTransport(wsUrl);
        mSocket.connect2(listener);
    }



    public void sendMessage(String method, JSONObject body) {
        sendMessage(method, body, RandomString.numlength(7));
    }

    public void sendMessage(String method, JSONObject body, int requestId) {
        JSONObject json = new JSONObject();
        try {
            json.put("request", true);
            json.put("method", method);
            json.put("id", requestId);
            json.put("data", body);
            send(json);
        } catch (JSONException e) {
            e.printStackTrace();
        }

    }

    public void send(JSONObject message) {
        Log.i(TAG, message.toString());
        mSocket.sendMessage(message);
    }

    public Future<JSONObject> sendWithFuture(String method, JSONObject body) {
        return new AckCall(method).sendAckRequest(body);
    }



    private String getJsonString(JSONObject jsonObject, String name) throws JSONException {
        String s = "";
        if (!jsonObject.isNull(name)) {
            s = jsonObject.getString(name);
        }
        return s;
    }


    @Override
    public void register(MessageObserver.Observer observer) {
        mObservers.add(observer);
    }

    @Override
    public void unregister(MessageObserver.Observer observer) {
        mObservers.remove(observer);
    }

    @Override
    public void notifyObservers(String method, int id, boolean notification, JSONObject data) {
        for (final MessageObserver.Observer observer : mObservers) {
            observer.on(method, id, notification, data);
        }
    }


    private class AckCall {
        private final String method;
        private final int id;

        private JSONObject mResponse;

        AckCall(String method) {
            id = RandomString.numlength(7);
            this.method = method;
        }

        Future<JSONObject> sendAckRequest(JSONObject body) {
            mLatch = new CountDownLatch(1);

            Callable<JSONObject> callable = () -> {
                MessageObserver.Observer observer = (method, id, notification, data) -> {
                    if (this.id == id) {
                        // Acknowledgement received
                        mResponse = data;
                        mObservers.remove(this);
                        mLatch.countDown();
                    } else {
                        Log.d(TAG, "Another event 1" + id);
                    }
                };

                mObservers.add(observer);
                sendMessage(method, body, id);
                mLatch.await();

                return mResponse;
            };
            return mExecutorService.submit(callable);
        }
    }
}
