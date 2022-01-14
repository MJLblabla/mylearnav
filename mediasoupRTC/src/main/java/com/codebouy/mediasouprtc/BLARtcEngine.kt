package com.codebouy.mediasouprtc

import android.content.Context
import android.util.Log
import com.codebouy.mediasouprtc.media.MediaCapturer
import com.codebouy.mediasouprtc.request.Request
import com.codebouy.mediasouprtc.socket.EchoSocket
import org.json.JSONException
import org.json.JSONObject
import org.mediasoup.droid.*
import org.webrtc.*
import java.lang.Exception
import java.lang.IllegalStateException
import java.util.ArrayList
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ExecutionException
import java.util.concurrent.TimeoutException

class BLARtcEngine(var mRtcEngineEventLister: RtcEngineEventLister) {

    private var uri = "wss://117.50.40.254:4443?"
    private val mProducers by lazy { ConcurrentHashMap<String, Producer>() }
    private val mConsumers by lazy { ConcurrentHashMap<String, Consumer>() }
    private val mMediaCapturer by lazy { MediaCapturer() }
    private var mJoined = false
    private val mConsumersInfo = ArrayList<JSONObject>()
    private val mDevice by lazy { Device() }

    private var mSendTransport: SendTransport? = null
    private var mRecvTransport: RecvTransport? = null
    val TAG = "BLARtcEngine"


    companion object {
        fun init(context: Context) {
            org.mediasoup.droid.Logger.setLogLevel(Logger.LogLevel.LOG_DEBUG)
            MediasoupClient.initialize(context)
        }
    }


    private val mSocket by lazy {
        EchoSocket().apply {
            register { method, id, notification, data ->
                if (method == "newConsumer") {
                    handleNewConsumerEvent(data)
                } else if (method == "consumerClosed") {
                    val id = data.getString("consumerId")
                    val consumer = mConsumers[id] ?: return@register
                    mConsumers.remove(id)
                    //todo mConsumersInfo.remove(id)
                    mRtcEngineEventLister.onRemoteTrackUnPublish(consumer)
                }
            }
        }
    }


    fun join(peerId: String, roomId: String,call:()->Unit) {
        if (mJoined) {
            return
        }

        mSocket.connect("${uri}roomId=${roomId}&peerId=${peerId}") {
            Thread {
                try {
                    val getRoomRtpCapabilitiesResponse: JSONObject =
                        Request.sendGetRouterRtpCapabilities(mSocket)
                    mDevice.load(getRoomRtpCapabilitiesResponse.toString())
                    createRecvTransport()
                    createSendTransport()
                    join()
                    call.invoke()
                } catch (e: InterruptedException) {
                    e.printStackTrace()
                } catch (e: ExecutionException) {
                    e.printStackTrace()
                } catch (e: TimeoutException) {
                    e.printStackTrace()
                } catch (e: Exception) {
                    e.printStackTrace()
                }

            }.start()
        }
    }

    @Throws(MediasoupException::class)
    fun produceAudio() {
        checkNotNull(mSendTransport) { "Send Transport not created" }
        check(mDevice.canProduce("audio")) { "Device cannot produce audio" }
        val codecOptions = "[{\"opusStereo\":true},{\"opusDtx\":true}]"
        createProducer(mMediaCapturer.createAudioTrack(), codecOptions, null)
        Log.d(
            TAG,
            "produceAudio() audio produce initialized"
        )
    }

    @Throws(Exception::class)
    fun produceVideo(
        context: Context,
        localVideoView: SurfaceViewRenderer,
        eglContext: EglBase.Context
    ): VideoTrack {
        checkNotNull(mSendTransport) { "Send Transport not created" }
        check(mDevice.canProduce("video")) { "Device cannot produce video" }
        mMediaCapturer.initCamera(context)
        val videoTrack = mMediaCapturer.createVideoTrack(
            context,
            localVideoView, eglContext
        )
        val codecOptions = "[{\"videoGoogleStartBitrate\":1000}]"
        val encodings: MutableList<RtpParameters.Encoding> = ArrayList()
        encodings.add(RTCUtils.genRtpEncodingParameters(false, 500000, 0, 60, 0, 0.0, 0L))
        encodings.add(RTCUtils.genRtpEncodingParameters(false, 1000000, 0, 60, 0, 0.0, 0L))
        encodings.add(RTCUtils.genRtpEncodingParameters(false, 1500000, 0, 60, 0, 0.0, 0L))
        createProducer(videoTrack, codecOptions, encodings)
        return videoTrack
    }


    @Throws(JSONException::class)
    fun subscriptRemoteConsumer(id: String) {
        Request.sendResumeConsumerRequest(mSocket, id)
    }


    @Throws(MediasoupException::class)
    private fun createProducer(
        track: MediaStreamTrack,
        codecOptions: String,
        encodings: List<RtpParameters.Encoding>?
    ) {
        val listener = Producer.Listener { producer: Producer? ->
            Log.d(
                TAG,
                "producer::onTransportClose kind=" + track.kind()
            )
        }
        val kindProducer = mSendTransport!!.produce(listener, track, encodings, codecOptions)
        mProducers[kindProducer.id] = kindProducer
    }


    @Throws(Exception::class)
    private fun join() {
        check(mDevice.isLoaded) { "Device is not loaded" }
        if (mJoined) {
            Log.w(
                TAG,
                "join() room already joined"
            )
            return
        }
        val device = JSONObject()
        device.put("name", "android")
        device.put("flag", "10")
        device.put("version", "10")
        Request.sendLoginRoomRequest(
            mSocket, "codeboy", device,
            JSONObject(mDevice.rtpCapabilities)
        )
        mJoined = true
        Log.d(TAG, "join() room joined")
    }


    @Throws(Exception::class)
    private fun createSendTransport() {
        // Do nothing if send transport is already created
        if (mSendTransport != null) {
            return
        }
        createWebRtcTransport("send")
    }

    @Throws(Exception::class)
    private fun createRecvTransport() {
        // Do nothing if recv transport is already created
        if (mRecvTransport != null) {
            return
        }
        createWebRtcTransport("recv")
    }

    @Throws(Exception::class)
    private fun createWebRtcTransport(direction: String) {
        val createWebRtcTransportResponse =
            Request.sendCreateWebRtcTransportRequest(mSocket, direction)
        Log.i(
            "createWebRtcTransport",
            createWebRtcTransportResponse.toString()
        )
        val id = createWebRtcTransportResponse.getString("id")
        val iceParametersString =
            createWebRtcTransportResponse.getJSONObject("iceParameters").toString()
        val iceCandidatesArrayString =
            createWebRtcTransportResponse.getJSONArray("iceCandidates").toString()
        val dtlsParametersString =
            createWebRtcTransportResponse.getJSONObject("dtlsParameters").toString()

        when (direction) {

            "send" -> createLocalWebRtcSendTransport(
                id,
                iceParametersString,
                iceCandidatesArrayString,
                dtlsParametersString
            )

            "recv" -> createLocalWebRtcRecvTransport(
                id,
                iceParametersString,
                iceCandidatesArrayString,
                dtlsParametersString
            )
            else -> throw IllegalStateException("Invalid Direction")
        }
    }

    @Throws(MediasoupException::class)
    private fun createLocalWebRtcRecvTransport(
        id: String,
        remoteIceParameters: String,
        remoteIceCandidatesArray: String,
        remoteDtlsParameters: String
    ) {
        val listener: RecvTransport.Listener = object : RecvTransport.Listener {
            override fun onConnect(transport: Transport, dtlsParameters: String) {
                Log.d(
                    TAG,
                    "recvTransport::onConnect"
                )
                handleLocalTransportConnectEvent(transport, dtlsParameters)
            }

            override fun onConnectionStateChange(transport: Transport, newState: String) {
                Log.d(
                    TAG,
                    "recvTransport::onConnectionStateChange newState=$newState"
                )
            }
        }
        mRecvTransport = mDevice.createRecvTransport(
            listener,
            id,
            remoteIceParameters,
            remoteIceCandidatesArray,
            remoteDtlsParameters
        )
        Log.d(
            TAG,
            "Recv Transport Created id=" + mRecvTransport?.getId()
        )
    }


    @Throws(MediasoupException::class)
    private fun createLocalWebRtcSendTransport(
        id: String,
        remoteIceParameters: String,
        remoteIceCandidatesArray: String,
        remoteDtlsParameters: String
    ) {
        val listener: SendTransport.Listener = object : SendTransport.Listener {
            override fun onConnect(transport: Transport, dtlsParameters: String) {
                Log.d(
                    TAG,
                    "sendTransport::onConnect"
                )
                handleLocalTransportConnectEvent(transport, dtlsParameters)
            }

            override fun onProduce(
                transport: Transport,
                kind: String,
                rtpParameters: String,
                appData: String
            ): String {
                Log.d(
                    TAG,
                    "sendTransport::onProduce kind=$kind"
                )
                return handleLocalTransportProduceEvent(transport, kind, rtpParameters, appData)
                    ?: ""
            }

            override fun onConnectionStateChange(transport: Transport, connectionState: String) {}
        }
        mSendTransport = mDevice.createSendTransport(
            listener,
            id,
            remoteIceParameters,
            remoteIceCandidatesArray,
            remoteDtlsParameters
        )
        Log.d(
            TAG,
            "Send Transport Created id=" + mSendTransport!!.id
        )
    }

    private fun handleLocalTransportConnectEvent(transport: Transport, dtlsParameters: String) {
        try {
            Request.sendConnectWebRtcTransportRequest(mSocket, transport.id, dtlsParameters)
        } catch (e: JSONException) {
            e.printStackTrace()
        }
    }

    private fun handleLocalTransportProduceEvent(
        transport: Transport,
        kind: String,
        rtpParameters: String,
        s2: String
    ): String? {
        return try {
            val transportProduceResponse = Request.sendProduceWebRtcTransportRequest(
                mSocket,
                transport.id,
                kind,
                rtpParameters
            )
            transportProduceResponse.getString("id")
        } catch (e: Exception) {
            Log.e(
                TAG,
                "transport::onProduce failed",
                e
            )
            null
        }
    }


    private fun handleNewConsumerEvent(consumerInfo: JSONObject) {
        try {
            Log.d(TAG, "handleNewConsumerEvent info =$consumerInfo")
            consumeTrack(consumerInfo)
        } catch (e: Exception) {
            e.printStackTrace()
            Log.e(TAG, "Failed to consume remote track", e)
        }
    }


    @Throws(JSONException::class, MediasoupException::class)
    private fun consumeTrack(consumerInfo: JSONObject) {
        if (mRecvTransport == null) {
            // User has not yet created a transport for receiving so temporarily store it
            // and play it when the recv transport is created
            //mConsumersInfo.add(consumerInfo);
            mConsumersInfo.add(consumerInfo)
            return
        }
        val kind = consumerInfo.getString("kind")
        val id = consumerInfo.getString("id")
        val producerId = consumerInfo.getString("producerId")
        val rtpParameters = consumerInfo.getJSONObject("rtpParameters").toString()
        val listener =
            Consumer.Listener { consumer: Consumer? ->
                Log.d(
                    TAG,
                    "consumer::onTransportClose"
                )
            }
        val kindConsumer = mRecvTransport!!.consume(listener, id, producerId, kind, rtpParameters)
        mConsumers[kindConsumer.id] = kindConsumer
        Log.d(
            TAG,
            "consumerTrack() consuming id=" + kindConsumer.id
        )
        mRtcEngineEventLister.onRemoteTrackPublish(kindConsumer)
    }


}