// GLIDESUPPORT.JAVA
// Copyright (c) A.Sobolev 2022
// Поддержка загрузки изображения для библиотеки GLIDE в рамках проекта Stylo-Q
//
package ru.petroglif.styloq;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.bumptech.glide.Priority;
import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.Options;
import com.bumptech.glide.load.data.DataFetcher;
import com.bumptech.glide.load.model.ModelLoader;
import com.bumptech.glide.load.model.ModelLoaderFactory;
import com.bumptech.glide.load.model.MultiModelLoaderFactory;
import com.bumptech.glide.signature.ObjectKey;
import java.nio.ByteBuffer;

public class GlideSupport {
	public static final String ModelPrefix = "stqblobsignature:";
	public static final class StyloQModelLoader implements ModelLoader<String, ByteBuffer> {
		private StyloQApp AppCtx;
		StyloQModelLoader(StyloQApp ctx)
		{
			AppCtx = ctx;
		}
		@Nullable @Override public LoadData<ByteBuffer> buildLoadData(String model, int width, int height, Options options)
		{
			return new LoadData<>(new ObjectKey(model), new StyloQDataFetcher(AppCtx, model));
		}
		@Override public boolean handles(String model)
		{
			return model.startsWith(ModelPrefix);
		}
	}
	public static class StyloQDataFetcher implements DataFetcher<ByteBuffer> {
		private final String Model;
		private StyloQApp AppCtx;
		StyloQDataFetcher(StyloQApp appCtx, String model)
		{
			AppCtx = appCtx;
			Model = model;
		}
		@Override public void loadData(Priority priority, DataCallback <?super ByteBuffer> callback)
		{
			byte [] blob_bytes = null;
			String _eff_model = Model.startsWith(ModelPrefix) ? Model.substring(ModelPrefix.length()) : Model;
			//CacheDiskUtils cache = CacheDiskUtils.getInstance("stqblob");
			//if(cache != null) {
			//	blob_bytes = cache.getBytes(_eff_model);
			//}
			if(blob_bytes == null) {
				blob_bytes = StyloQInterchange.QueryBlob(AppCtx, _eff_model);
				//if(blob_bytes != null && cache != null) {
				//	cache.put(_eff_model, blob_bytes);
				//}
			}
			if(blob_bytes != null) {
				callback.onDataReady(ByteBuffer.wrap(blob_bytes));
			}
			else {
				callback.onLoadFailed(new StyloQException("Failed to load blob" + " " + Model));
			}
		}
		@Override public void cleanup()
		{
		}
		@Override public void cancel()
		{
		}
		@NonNull @Override public Class<ByteBuffer> getDataClass()
		{
			return ByteBuffer.class;
		}
		@NonNull @Override public DataSource getDataSource()
		{
			return DataSource.REMOTE;
		}
	}
	public static class StyloQModelLoaderFactory implements ModelLoaderFactory<String, ByteBuffer> {
		private StyloQApp AppCtx;
		StyloQModelLoaderFactory(Context ctx)
		{
			AppCtx = (ctx != null && ctx instanceof StyloQApp) ? (StyloQApp)ctx : null;
		}
		@Override public ModelLoader<String, ByteBuffer> build(MultiModelLoaderFactory unused)
		{
			return new StyloQModelLoader(AppCtx);
		}
		@Override public void teardown()
		{
			// Do nothing.
		}
	}
}
