package com.codebouy.mediasouprtc

import org.mediasoup.droid.Consumer

interface RtcEngineEventLister {
    fun onRemoteTrackPublish(consumer: Consumer)
    fun onRemoteTrackUnPublish(consumer: Consumer)
}