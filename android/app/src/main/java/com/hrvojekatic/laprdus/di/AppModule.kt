package com.hrvojekatic.laprdus.di

import android.content.Context
import com.hrvojekatic.laprdus.audio.AudioPlayer
import com.hrvojekatic.laprdus.data.SettingsRepository
import com.hrvojekatic.laprdus.tts.LaprdusTTS
import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.android.qualifiers.ApplicationContext
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

/**
 * Hilt module providing application-wide dependencies.
 */
@Module
@InstallIn(SingletonComponent::class)
object AppModule {

    /**
     * Provides the singleton LaprdusTTS instance.
     */
    @Provides
    @Singleton
    fun provideLaprdusTTS(): LaprdusTTS {
        return LaprdusTTS.getInstance()
    }

    /**
     * Provides the SettingsRepository for persisting TTS settings.
     */
    @Provides
    @Singleton
    fun provideSettingsRepository(
        @ApplicationContext context: Context
    ): SettingsRepository {
        return SettingsRepository(context)
    }

    /**
     * Provides the AudioPlayer for audio playback.
     */
    @Provides
    @Singleton
    fun provideAudioPlayer(): AudioPlayer {
        return AudioPlayer()
    }
}
