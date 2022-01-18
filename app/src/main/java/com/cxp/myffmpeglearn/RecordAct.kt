package com.cxp.myffmpeglearn

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.BitmapFactory
import android.net.Uri
import android.util.Log
import android.util.Size
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.util.concurrent.Executors
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import com.cxp.nativelibffmpeg.MediaRecorderContext
import com.cxp.nativelibffmpeg.MediaRecorderContext.Companion.IMAGE_FORMAT_I420
import com.cxp.nativelibffmpeg.MediaRecorderContext.Companion.RECORDER_TYPE_AV
import kotlinx.android.synthetic.main.activity_main.*
import java.io.File
import java.nio.ByteBuffer
import java.text.SimpleDateFormat
import java.util.*
import java.util.concurrent.ExecutorService
import android.os.Build
import android.os.Environment
import android.view.Surface.ROTATION_0
import android.view.Surface.ROTATION_90
import com.cxp.myffmpeglearn.CameraUtil.*
import com.cxp.nativelibffmpeg.MediaRecorderContext.Companion.IMAGE_FORMAT_NV12
import com.cxp.nativelibffmpeg.MediaRecorderContext.Companion.IMAGE_FORMAT_NV21
import com.cxp.nativelibffmpeg.MediaRecorderContext.Companion.IMAGE_FORMAT_RGBA


class MainActivity : AppCompatActivity() {

    private val mediaRecorderContext by lazy {
        val context = MediaRecorderContext()
        context.native_CreateContext()
        context
    }
    private var imageCapture: ImageCapture? = null

    val rgbaProducer = RGBAProducer()
    val consumerBuffer = RGBAConsumerBuffer()
    private val audioRecorder by lazy {
        AudioRecorder(
            object : AudioRecorder.AudioRecorderCallback {
                override fun onAudioData(data: ByteArray?, dataSize: Int) {
                    Log.d("mjl", "onAudioData")
                    mediaRecorderContext.native_OnAudioData(data)
                }

                override fun onError(msg: String?) {
                    Log.d("mjl", "onAudioData onError")
                }
            }
        )
    }
    private val mLuminosityAnalyzer = object : ImageAnalysis.Analyzer {

        private fun ByteBuffer.toByteArray(): ByteArray {
            rewind()    // Rewind the buffer to zero
            val data = ByteArray(remaining())
            get(data)   // Copy the buffer into a byte array
            return data // Return the byte array
        }

        override fun analyze(image: ImageProxy) {
            var w = image.getWidth()
            var h = image.getHeight()
           // var byteArray = CameraUtil.getDataFromImage(image, CameraUtil.COLOR_FormatNV21)

            rgbaProducer.parseImg(image)
            consumerBuffer.addRgbQueen(rgbaProducer.rgbaByteArray!!,rgbaProducer.mWidth,rgbaProducer.mHeight,rgbaProducer.pixelStride,rgbaProducer.rowPadding)
            mediaRecorderContext.native_OnVideoData(IMAGE_FORMAT_RGBA, rgbaProducer.rgbaByteArray, w, h);
            Log.d("mjl", "onImageProxy finish ${w}  ${h} ${image.imageInfo.rotationDegrees}")

            image.close()
        }
    }
    private val outputDirectory: File by lazy {
        val mediaDir = externalMediaDirs.firstOrNull()?.let {
            File(it, resources.getString(R.string.app_name)).apply { mkdirs() }
        }
        if (mediaDir != null && mediaDir.exists())
            mediaDir else filesDir
    }
    private val cameraExecutor: ExecutorService by lazy {
        Executors.newSingleThreadExecutor()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Request camera permissions
        if (allPermissionsGranted()) {
            startCamera()
        } else {
            ActivityCompat.requestPermissions(
                this, REQUIRED_PERMISSIONS, REQUEST_CODE_PERMISSIONS
            )
        }

        // Set up the listener for take photo button
        camera_capture_button.setOnClickListener {
            takePhoto()
        }

        btStop.setOnClickListener {
            onBackPressed();

        }
    }

    private fun takePhoto() {
        // Get a stable reference of the modifiable image capture use case
        val imageCapture = imageCapture ?: return

        // Create time-stamped output file to hold the image
        val photoFile = File(
            outputDirectory,
            SimpleDateFormat(
                FILENAME_FORMAT, Locale.US
            ).format(System.currentTimeMillis()) + ".jpg"
        )

        // Create output options object which contains file + metadata
        val outputOptions = ImageCapture.OutputFileOptions.Builder(photoFile).build()

        // Set up image capture listener, which is triggered after photo has
        // been taken
        imageCapture.takePicture(
            outputOptions,
            ContextCompat.getMainExecutor(this),
            object : ImageCapture.OnImageSavedCallback {
                override fun onError(exc: ImageCaptureException) {
                    Log.e(TAG, "Photo capture failed: ${exc.message}", exc)
                }

                override fun onImageSaved(output: ImageCapture.OutputFileResults) {
                    val savedUri = Uri.fromFile(photoFile)
                    val msg = "Photo capture succeeded: $savedUri"
                    Toast.makeText(baseContext, msg, Toast.LENGTH_SHORT).show()
                    Log.d(TAG, msg)
                }
            })
    }

    private fun startCamera() {

        val imageAnalyzer = ImageAnalysis.Builder()

            .setTargetResolution(Size(480, 640))
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .setOutputImageFormat(ImageAnalysis.OUTPUT_IMAGE_FORMAT_RGBA_8888)
            .build()

            .also {
                it.setAnalyzer(cameraExecutor, mLuminosityAnalyzer)
            }

        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener(Runnable {
            // Used to bind the lifecycle of cameras to the lifecycle owner
            val cameraProvider: ProcessCameraProvider = cameraProviderFuture.get()

            // Preview
            val preview = Preview.Builder()

                .build()
                .also {
                    it.setSurfaceProvider(viewFinder.surfaceProvider)
                }


            // Select back camera as a default
            val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA

            try {
                // Unbind use cases before rebinding
                cameraProvider.unbindAll()

                // Bind use cases to camera
                cameraProvider.bindToLifecycle(
                    this, cameraSelector, preview, imageAnalyzer
                )

            } catch (exc: Exception) {
                Log.e(TAG, "Use case binding failed", exc)
            }

        }, ContextCompat.getMainExecutor(this))
        audioRecorder.run()
        val frameWidth: Int = 640
        val frameHeight: Int = 480
        val fps = 25
        val bitRate = (frameWidth * frameHeight * fps * 0.3).toLong()
        var url: String? = ""
        url = if (Build.VERSION.SDK_INT > 29) {
            cacheDir.absolutePath
        } else {
            Environment.getExternalStorageDirectory().getAbsolutePath().toString() + "/A"
        } + "/${System.currentTimeMillis()}.mp4"

        val file = File(url)
        if (!file.exists()) {
            file.createNewFile()
        }
        mediaRecorderContext.native_StartRecord(
            RECORDER_TYPE_AV,
            url,
            frameWidth,
            frameHeight,
            bitRate,
            fps
        )
    }

    private fun allPermissionsGranted() = REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(
            baseContext, it
        ) == PackageManager.PERMISSION_GRANTED
    }


    override fun onDestroy() {
        super.onDestroy()

    }

    override fun onBackPressed() {
        cameraExecutor.shutdown()
        audioRecorder.stop()
        mediaRecorderContext.native_StopRecord()
        mediaRecorderContext.native_DestroyContext()
        System.gc()
        super.onBackPressed()

    }


    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults:
        IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            if (allPermissionsGranted()) {
                startCamera()
            } else {
                Toast.makeText(
                    this,
                    "Permissions not granted by the user.",
                    Toast.LENGTH_SHORT
                ).show()
                finish()
            }
        }
    }


    companion object {
        private const val TAG = "CameraXBasic"
        private const val FILENAME_FORMAT = "yyyy-MM-dd-HH-mm-ss-SSS"
        private const val REQUEST_CODE_PERMISSIONS = 10
        private val REQUIRED_PERMISSIONS = arrayOf(
            Manifest.permission.CAMERA, Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.RECORD_AUDIO
        )
    }
}