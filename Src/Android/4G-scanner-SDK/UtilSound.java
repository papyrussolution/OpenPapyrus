package com.example.testscan;

import android.content.Context;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import java.util.HashMap;
import java.util.Map;

public class UtilSound {
    public static SoundPool sp;
    public static Map<Integer, Integer> suondMap;
    public static Context context;

    //init sound pool
    public static void initSoundPool(Context context)
    {
        UtilSound.context = context;
        sp = new SoundPool (4, AudioManager.STREAM_MUSIC, 4);
        suondMap = new HashMap<Integer, Integer> ();
        suondMap.put(1,sp.load(context, R.raw.msg,1));
    }
    //play sound
    public static void play()
    {
        AudioManager am = (AudioManager) UtilSound.context.getSystemService(UtilSound.context.AUDIO_SERVICE);
        float audioMaxVolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        float audioCurrentVolume = am.getStreamVolume(AudioManager.STREAM_MUSIC);
        float volumnRatio = audioCurrentVolume / audioMaxVolume;
        //第一个参数soundID
        //第二个参数leftVolume为左侧音量值（范围= 0.0到1.0）
        //第三个参数rightVolume为右的音量值（范围= 0.0到1.0）
        //第四个参数priority 为流的优先级，值越大优先级高，影响当同时播放数量超出了最大支持数时SoundPool对该流的处理
        //第五个参数loop 为音频重复播放次数，0为值播放一次，-1为无限循环，其他值为播放loop+1次
        //第六个参数 rate为播放的速率，范围0.5-2.0(0.5为一半速率，1.0为正常速率，2.0为两倍速率)
        sp.play(1, volumnRatio, volumnRatio, 0, 0, 1f);//0.5-2.0 speed
    }
}
