package com.codebouy.mediasouprtc.request;



import com.codebouy.mediasouprtc.socket.ActionEvent;
import com.codebouy.mediasouprtc.socket.EchoSocket;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;


public class Request {
    private static final int REQUEST_TIMEOUT_SECONDS = 3000;

    public static JSONObject sendGetRouterRtpCapabilities(EchoSocket socket) throws InterruptedException, ExecutionException, TimeoutException {
        JSONObject getRoomRtpCapabilitiesRequest = new JSONObject();
        return socket.sendWithFuture("getRouterRtpCapabilities", getRoomRtpCapabilitiesRequest)
                .get(Request.REQUEST_TIMEOUT_SECONDS, TimeUnit.SECONDS);
    }

    public static JSONObject sendLoginRoomRequest(EchoSocket socket, String displayName, JSONObject device,
                                                  JSONObject deviceRtpCapabilities) throws InterruptedException, ExecutionException, TimeoutException {
        JSONObject loginRoomRequest = new JSONObject();
        try {
            loginRoomRequest.put("displayName", displayName);
            loginRoomRequest.put("device", device);
            loginRoomRequest.put("rtpCapabilities", deviceRtpCapabilities);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return socket.sendWithFuture(ActionEvent.join, loginRoomRequest)
                .get(Request.REQUEST_TIMEOUT_SECONDS, TimeUnit.SECONDS);
    }

    public static JSONObject sendCreateWebRtcTransportRequest(EchoSocket socket,
                                                              String direction) throws JSONException, InterruptedException, ExecutionException, TimeoutException {
        JSONObject createWebRtcTransportRequest = new JSONObject();
        createWebRtcTransportRequest.put("forceTcp", false);
        createWebRtcTransportRequest.put("producing",
                direction.equals("send") ? true : false);
        createWebRtcTransportRequest.put("consuming",
                direction.equals("send") ? false : true);
        return socket.sendWithFuture(ActionEvent.createWebRtcTransport,
                createWebRtcTransportRequest)
                .get(Request.REQUEST_TIMEOUT_SECONDS, TimeUnit.SECONDS);
    }

    public static void sendConnectWebRtcTransportRequest(EchoSocket socket,
                                                         String transportId,
                                                         String dtlsParameters)
            throws JSONException {
        JSONObject connectWebRtcTransportRequest = new JSONObject();
        connectWebRtcTransportRequest.put("transportId", transportId);
        connectWebRtcTransportRequest.put("dtlsParameters", new JSONObject(dtlsParameters));
        socket.sendMessage(ActionEvent.connectWebRtcTransport, connectWebRtcTransportRequest);
    }

    // Send produce request
    public static JSONObject sendProduceWebRtcTransportRequest(EchoSocket socket,
                                                               String transportId,
                                                               String kind,
                                                               String rtpParameters)
            throws JSONException, InterruptedException, ExecutionException, TimeoutException {
        JSONObject produceWebRtcTransportRequest = new JSONObject();
        produceWebRtcTransportRequest.put("transportId", transportId);
        produceWebRtcTransportRequest.put("kind", kind);
        produceWebRtcTransportRequest.put("rtpParameters", new JSONObject(rtpParameters));
        return socket.sendWithFuture(ActionEvent.produce, produceWebRtcTransportRequest)
                .get(Request.REQUEST_TIMEOUT_SECONDS, TimeUnit.SECONDS);
    }

    // resume consumer
    public static void sendResumeConsumerRequest(EchoSocket socket, String consumerId)
            throws JSONException {
        JSONObject resumeConsumerRequest = new JSONObject();

        resumeConsumerRequest.put("consumerId", consumerId);

        socket.sendMessage(ActionEvent.resumeConsumer, resumeConsumerRequest);
    }

}
