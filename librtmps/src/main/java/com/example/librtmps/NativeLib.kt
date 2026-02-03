package com.example.librtmps

import android.util.Log
import android.view.Surface
import java.io.IOException
import java.nio.ByteBuffer
import java.nio.file.Files.size



class NativeLib {
    interface CallBack {
        fun onRtmpStatus(status: String)
    }

    private val TAG = NativeLib::class.java.simpleName
    private var rtmpPointer: Long = 0
    private var dataBuffer:ByteBuffer ?= null
    private var mBack:CallBack ?= null
    class RtmpIOException(val errorCode: Int) : IOException("RTMP error: $errorCode") {
        companion object {
            /**
             * RTMP client could not allocate memory for rtmp context structure
             */
            const val OPEN_ALLOC: Int = -2

            /**
             * RTMP client could not open the stream on server
             */
            const val OPEN_CONNECT_STREAM: Int = -3

            /**
             * Received an unknown option from the RTMP server
             */
            const val UNKNOWN_RTMP_OPTION: Int = -4

            /**
             * RTMP server sent a packet with unknown AMF type
             */
            const val UNKNOWN_RTMP_AMF_TYPE: Int = -5

            /**
             * DNS server is not reachable
             */
            const val DNS_NOT_REACHABLE: Int = -6

            /**
             * Could not establish a socket connection to the server
             */
            const val SOCKET_CONNECT_FAIL: Int = -7

            /**
             * SOCKS negotiation failed
             */
            const val SOCKS_NEGOTIATION_FAIL: Int = -8

            /**
             * Could not create a socket to connect to RTMP server
             */
            const val SOCKET_CREATE_FAIL: Int = -9

            /**
             * SSL connection requested but not supported by the client
             */
            const val NO_SSL_TLS_SUPP: Int = -10

            /**
             * Could not connect to the server for handshake
             */
            const val HANDSHAKE_CONNECT_FAIL: Int = -11

            /**
             * Handshake with the server failed
             */
            const val HANDSHAKE_FAIL: Int = -12

            /**
             * RTMP server connection failed
             */
            const val RTMP_CONNECT_FAIL: Int = -13

            /**
             * Connection to the server lost
             */
            const val CONNECTION_LOST: Int = -14

            /**
             * Received an unexpected timestamp from the server
             */
            const val RTMP_KEYFRAME_TS_MISMATCH: Int = -15

            /**
             * The RTMP stream received is corrupted
             */
            const val RTMP_READ_CORRUPT_STREAM: Int = -16

            /**
             * Memory allocation failed
             */
            const val RTMP_MEM_ALLOC_FAIL: Int = -17

            /**
             * Stream indicated a bad datasize, could be corrupted
             */
            const val RTMP_STREAM_BAD_DATASIZE: Int = -18

            /**
             * RTMP packet received is too small
             */
            const val RTMP_PACKET_TOO_SMALL: Int = -19

            /**
             * Could not send packet to RTMP server
             */
            const val RTMP_SEND_PACKET_FAIL: Int = -20

            /**
             * AMF Encode failed while preparing a packet
             */
            const val RTMP_AMF_ENCODE_FAIL: Int = -21

            /**
             * Missing a :// in the URL
             */
            const val URL_MISSING_PROTOCOL: Int = -22

            /**
             * Hostname is missing in the URL
             */
            const val URL_MISSING_HOSTNAME: Int = -23

            /**
             * The port number indicated in the URL is wrong
             */
            const val URL_INCORRECT_PORT: Int = -24

            /**
             * Error code used by JNI to return after throwing an exception
             */
            const val RTMP_IGNORED: Int = -25

            /**
             * RTMP client has encountered an unexpected error
             */
            const val RTMP_GENERIC_ERROR: Int = -26

            /**
             * A sanity check failed in the RTMP client
             */
            const val RTMP_SANITY_FAIL: Int = -27
        }
    }

    fun setCallBack(callBack: CallBack){
        mBack = callBack
    }

    fun initMem(): Boolean {
        if(dataBuffer!=null && rtmpPointer!= 0L) return true

        dataBuffer = ByteBuffer.allocateDirect(6220800)
        rtmpPointer = nativeAlloc()
        if (rtmpPointer == 0L || dataBuffer == null) {
            mBack?.onRtmpStatus("init_err")
            return false
        }
        return true
    }

    fun open(url: String, w:Int, h:Int, fps: Int):Boolean {
        if (rtmpPointer != 0L && dataBuffer != null) {
            nativeOpen(rtmpPointer, url, w, h, fps)
            val ret = isConnect()
            if(ret){
                mBack?.onRtmpStatus("rtmp_ok")
                return true
            }else{
                mBack?.onRtmpStatus("rtmp_failed")
                return false
            }
        }
        return false
    }

    fun close() {
        if (rtmpPointer != 0L && dataBuffer != null) {
            nativeClose(rtmpPointer)
        }
    }

    fun isConnect() :Boolean {
        if (rtmpPointer != 0L && dataBuffer != null) {
            return nativeIsConnected(rtmpPointer)
        }else{
            return false
        }
    }

    fun writeVideo(buffer: ByteBuffer, timestamp: Long): Int {

        if (rtmpPointer != 0L && dataBuffer != null) {
            val memory = dataBuffer
            if(memory!=null){
                memory.clear()
//            val capacity = memory.capacity() // 总大小
//            val remaining = buffer.remaining() // post到有效数据尾的大小
//            Log.v(TAG, "capacity:$capacity remaining:$remaining")
//            buffer.position(4)
                memory.put(buffer)
                buffer.rewind()
//            MediaUtils.logByteBuffer(buffer,"buffer")
                memory.flip()
//            MediaUtils.logByteBuffer(memory,"memory")
                dataBuffer?.rewind()
//            dataBuffer?.let { MediaUtils.logByteBuffer(it, "dataBuffer") }
//            return
                return nativeWriteVideo(rtmpPointer, memory, memory.remaining(), timestamp)
            }
        }
        return 0
    }

    /**
     * A native method that is implemented by the 'librtmps' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    private external fun nativeAlloc(): Long
    private external fun nativeOpen(pointer: Long, url:String, w:Int, h:Int, fps:Int)
    private external fun nativeClose(pointer: Long)
    private external fun nativeIsConnected(pointer: Long): Boolean
    private external fun nativeWriteVideo(pointer: Long, data: ByteBuffer, length: Int, timestamp: Long): Int

    companion object {
        // Used to load the 'librtmps' library on application startup.
        init {
            System.loadLibrary("librtmps")
        }
    }
}