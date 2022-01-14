package com.cxp.myffmpeglearn

import android.Manifest
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.View
import androidx.recyclerview.widget.GridLayoutManager
import com.chad.library.adapter.base.BaseQuickAdapter
import com.chad.library.adapter.base.BaseViewHolder
import com.codebouy.mediasouprtc.BLARtcEngine
import com.codebouy.mediasouprtc.RtcEngineEventLister
import com.codebouy.mediasouprtc.utils.RandomString
import com.tbruyelle.rxpermissions3.RxPermissions
import kotlinx.android.synthetic.main.activity_rtc.*
import kotlinx.android.synthetic.main.item_remote.view.*
import org.json.JSONException
import org.webrtc.EglBase
import org.webrtc.RendererCommon
import org.webrtc.SurfaceViewRenderer
import org.webrtc.VideoTrack
import java.util.ArrayList

class RtcActivity : AppCompatActivity() {


    class RendererData {
        var viewRenderer: SurfaceViewRenderer? = null
        var id: String? = null
    }

    private val mRootEglBase: EglBase by lazy {  EglBase.create()};
    private val mRoomAdapter = object : BaseQuickAdapter<RendererData, BaseViewHolder>(
        R.layout.item_remote,
        ArrayList<RendererData>()
    ) {
        override fun convert(helper: BaseViewHolder, item: RendererData) {
            helper.itemView.itemContent.removeAllViews()
            helper.itemView.itemContent.addView(item.viewRenderer)
        }
    }
    private val mBLARtcEngine by lazy {
        BLARtcEngine(object : RtcEngineEventLister {
            override fun onRemoteTrackPublish(consumer: org.mediasoup.droid.Consumer) {
                if (consumer.kind == "video") {
                    val videoTrack = consumer.track as VideoTrack
                    videoTrack.setEnabled(true)
                    runOnUiThread {
                        val remoteView = createRemoteView()
                        videoTrack.addSink(remoteView)
                        val data = RendererData()
                        data.id = consumer.id
                        data.viewRenderer = remoteView
                        mRoomAdapter.addData(data)
                    }
                }

                try {
                    resumeRemoteConsumer(consumer.id)
                } catch (je: JSONException) {
                    Log.e("rtcActivity", "Failed to send resume consumer request", je)
                }
            }

            override fun onRemoteTrackUnPublish(consumer: org.mediasoup.droid.Consumer) {
                runOnUiThread {
                    var toRemoveIndex = -1
                    mRoomAdapter.data.forEachIndexed { index, rendererData ->
                        if (rendererData.id == (consumer.id)) {
                            toRemoveIndex = index
                        }
                    }
                    if (toRemoveIndex > -1) {
                        mRoomAdapter.remove(toRemoveIndex)
                    }
                }
            }
        })
    }


    fun resumeRemoteConsumer(id: String) {
        mBLARtcEngine.subscriptRemoteConsumer(id)
    }

    fun createRemoteView(): SurfaceViewRenderer? {
        val remoteView = SurfaceViewRenderer(this)
        remoteView.init(mRootEglBase.getEglBaseContext(), null) // 初始化
        remoteView.setScalingType(RendererCommon.ScalingType.SCALE_ASPECT_FILL) // 填充方式
        remoteView.setMirror(true) // 镜面显示
        remoteView.setEnableHardwareScaler(true)
        remoteView.visibility = View.VISIBLE
        return remoteView
    }


//    var socketTransport: WebSocketTransport =
//        WebSocketTransport("wss://117.50.40.254:4443?roomId=2121&peerId=Mo7FAwn")
//    private val listener: org.protoojs.droid.transports.AbsWebSocketTransport.Listener =
//        object : org.protoojs.droid.transports.AbsWebSocketTransport.Listener {
//            override fun onOpen() {
//                val j = JSONObject()
//                socketTransport.sed("{\"request\":true,\"method\":\"getRouterRtpCapabilities\",\"id\":4614197,\"data\":{}}")
//            }
//
//            /**
//             * Connection could not be established in the first place.
//             */
//            override fun onFail() {
//                Log.d("EchoSocket", "onFail")
//            }
//
//            /**
//             * @param message [Message]
//             */
//            override fun onMessage(message: org.protoojs.droid.Message) {
//                Log.d("EchoSocket", "onMessage text=$message")
//            }
//
//            /**
//             * A previously established connection was lost unexpected.
//             */
//            override fun onDisconnected() {
//                Log.d("EchoSocket", "onDisconnected")
//            }
//
//            override fun onClose() {
//                Log.d("EchoSocket", "onClose")
//            }
//        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_rtc)
        BLARtcEngine.init(this)
        RxPermissions(this).request(
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.CAMERA,
            Manifest.permission.RECORD_AUDIO
        ).doFinally {

            // mBinding = DataBindingUtil.setContentView(this, R.layout.activity_room);
            //  createRoom();
            //checkPermission();
         //   Thread { socketTransport.connect(listener) }.start()
            mBLARtcEngine.join(RandomString.length(7), "2121"){
                localView.post {
                    localView.init(mRootEglBase.eglBaseContext,null)
                    mBLARtcEngine.produceAudio()
                    mBLARtcEngine.produceVideo(this, localView, mRootEglBase.eglBaseContext)
                }
            }
        }.subscribe {

        }
        recyRemote.layoutManager = GridLayoutManager(this,2)
        recyRemote.adapter = mRoomAdapter

    }
}