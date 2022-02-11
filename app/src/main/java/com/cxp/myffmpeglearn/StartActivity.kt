package com.cxp.myffmpeglearn

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import kotlinx.android.synthetic.main.activity_start.*

class StartActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_start)
        record.setOnClickListener {
            startActivity(Intent(this,MainActivity::class.java))
        }
        player.setOnClickListener {
            startActivity(Intent(this,playeractivity::class.java))
        }
        pushrtmp.setOnClickListener {
            startActivity(Intent(this,RTMPActivity::class.java))
        }
    }
}