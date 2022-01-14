package com.codebouy.webrtcforandroid;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;

import com.codebouy.webrtc1v1.R;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.AudioTrack;
import org.webrtc.EglBase;
import org.webrtc.IceCandidate;
import org.webrtc.MediaStream;
import org.webrtc.PeerConnection;
import org.webrtc.RendererCommon;
import org.webrtc.SessionDescription;
import org.webrtc.SurfaceViewRenderer;
import org.webrtc.VideoTrack;

import java.util.HashMap;
import java.util.Map;

public class CallVideoActivity extends AppCompatActivity implements SignalingDelegate {

    SurfaceViewRenderer localSurfaceView;
    SurfaceViewRenderer remoteSurfaceView;

    VideoTrack videoTrack;
    AudioTrack audioTrack;

    MediaCapturer mediaCapturer;
    Signaling signaling;

    Map<String, PConnection> peerConnections = new HashMap<>();
    EglBase mRootEglBase;

    String sessionId;
    String selfId;
    JSONObject peer;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_call_video);

        selfId = RandomString.length(12);
        signaling = new Signaling("ws://mancangyun.com:7080", selfId);
        signaling.delegate = this;

        mRootEglBase = EglBase.create();
        mediaCapturer = new MediaCapturer(getBaseContext(), mRootEglBase);

        localSurfaceView = findViewById(R.id.LocalSurfaceView);
        localSurfaceView.init(mRootEglBase.getEglBaseContext(), null);
        localSurfaceView.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FILL);
        localSurfaceView.setZOrderMediaOverlay(true);

        try {
            mediaCapturer.initCamera(getBaseContext());
            videoTrack = mediaCapturer.createVideoTrack(getBaseContext(),
                    localSurfaceView, mRootEglBase.getEglBaseContext());
            audioTrack = mediaCapturer.createAudioTrack();
        } catch (Exception e) {
            e.printStackTrace();
        }

        remoteSurfaceView = findViewById(R.id.RemoteSurfaceView);
        remoteSurfaceView.init(mRootEglBase.getEglBaseContext(), null);
        remoteSurfaceView.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FILL);
        remoteSurfaceView.setMirror(true);
        remoteSurfaceView.setVisibility(View.VISIBLE);

        findViewById(R.id.Connect).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                try {
                    startCall(peer);
                } catch (JSONException e) {
                    e.printStackTrace();
                }
            }
        });


    }


    void startCall(JSONObject peer) throws JSONException {
        String peerId = peer.getString("id");
        sessionId = selfId + peerId;
        PConnection pc = createPeerConnection(peerId);
        pc.createOffer(new PConnection.CompletionHandler() {
            @Override
            public void success(SessionDescription sessionDescription) {
                JSONObject jsep = new JSONObject();
                try {
                    jsep.put("type", sessionDescription.type.toString());
                    jsep.put("sdp", sessionDescription.description);

                    JSONObject object = new JSONObject();
                    object.put("to", peerId);
                    object.put("description", jsep);
                    object.put("session_id", sessionId);
                    signaling.send(object, "offer");

                } catch (JSONException e) {
                    e.printStackTrace();
                }

            }
        });

    }


    PConnection createPeerConnection(String peerId) {
        PConnection pc = new PConnection();
        pc.createPeerConnection(mediaCapturer.mPeerConnectionFactory, audioTrack, videoTrack);
        peerConnections.put(peerId, pc);
        pc.callback = new PConnection.Callback() {
            @Override
            public void onAddStream(PeerConnection peerConnection, MediaStream mediaStream) {

                VideoTrack remoteVideoTrack = mediaStream.videoTracks.get(0);
                remoteVideoTrack.setEnabled(true);
                remoteVideoTrack.addSink(remoteSurfaceView);
                AudioTrack audioTrack=mediaStream.audioTracks.get(0);
                audioTrack.setEnabled(true);
            }

            @Override
            public void onIceCandidate(PeerConnection peerConnection, IceCandidate candidate) {
                Log.i("收集候选者后", "start");
                JSONObject candidateObject = new JSONObject();

                try {
                    candidateObject.put("candidate", candidate.sdp);
                    candidateObject.put("sdpMid", candidate.sdpMid);
                    candidateObject.put("sdpMLineIndex", candidate.sdpMLineIndex);

                    JSONObject object = new JSONObject();
                    object.put("to", peerId);
                    object.put("candidate", candidateObject);
                    object.put("session_id", sessionId);

                    signaling.send(object, "candidate");

                } catch (JSONException e) {
                    e.printStackTrace();
                }

            }
        };
        return pc;
    }

    @Override
    public void onMessage(JSONObject object) throws JSONException {
        String type = object.getString("type");
        if (type.equals("peers")) {
            JSONArray peers = object.getJSONArray("data");
            for (int i = 0; i < peers.length(); i++) {
                JSONObject o = peers.getJSONObject(i);
                if (!o.getString("id").equals(selfId)) {
                    peer = o;
                }
            }
        } else if (type.equals("offer")) {
            JSONObject data = object.getJSONObject("data");
            JSONObject description = data.getJSONObject("description");

            sessionId = data.getString("session_id");
            String peerId = data.getString("from");
            PConnection pc = createPeerConnection(peerId);
            pc.onRemoteAnswer(description);
            pc.createAnswer(new PConnection.CompletionHandler() {
                @Override
                public void success(SessionDescription sessionDescription) {
                    JSONObject jsonObject = new JSONObject();
                    JSONObject sdp = new JSONObject();
                    try {
                        sdp.put("sdp", sessionDescription.description);
                        sdp.put("type", "answer");
                        jsonObject.put("to", peerId);
                        jsonObject.put("description", sdp);
                        jsonObject.put("session_id", sessionId);
                        signaling.send(jsonObject, "answer");
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }

                }
            });
        } else if (type.equals("answer")) {
            JSONObject data = object.getJSONObject("data");
            String peerId = data.getString("from");
            PConnection pc = peerConnections.get(peerId);
            JSONObject description = data.getJSONObject("description");
            pc.onRemoteAnswer(description);
        } else if (type.equals("candidate")) {
            JSONObject data = object.getJSONObject("data");
            String peerId = data.getString("from");
            JSONObject candidateObj = data.getJSONObject("candidate");
            IceCandidate candidate = new IceCandidate(
                    candidateObj.getString("sdpMid"),
                    candidateObj.getInt("sdpMLineIndex"),
                    candidateObj.getString("candidate"));

            PConnection pc = peerConnections.get(peerId);
            pc.peerConnection.addIceCandidate(candidate);
        }

    }
}
