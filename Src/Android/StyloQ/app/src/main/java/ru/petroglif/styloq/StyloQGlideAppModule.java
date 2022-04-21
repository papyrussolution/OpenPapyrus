package ru.petroglif.styloq;

import android.content.Context;
import androidx.annotation.NonNull;
import com.bumptech.glide.Glide;
import com.bumptech.glide.Registry;
import com.bumptech.glide.annotation.GlideModule;
import com.bumptech.glide.module.AppGlideModule;
import java.nio.ByteBuffer;

@GlideModule public class StyloQGlideAppModule extends AppGlideModule {
	@Override public void registerComponents(@NonNull Context ctx, @NonNull Glide glide, @NonNull Registry registry)
	{
		registry.prepend(String.class, ByteBuffer.class, new GlideSupport.StyloQModelLoaderFactory(ctx));
	}
	// Disable manifest parsing to avoid adding similar modules twice.
	@Override public boolean isManifestParsingEnabled()
	{
		return false;
	}
}
