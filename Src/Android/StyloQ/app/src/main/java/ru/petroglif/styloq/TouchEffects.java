// TouchEffects.java
//
package ru.petroglif.styloq;

import android.animation.Animator;
import android.animation.AnimatorSet;
import android.animation.ArgbEvaluator;
import android.animation.ValueAnimator;
import android.app.Activity;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.os.Build;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.animation.DecelerateInterpolator;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringDef;
import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.widget.AppCompatImageButton;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.appcompat.widget.AppCompatTextView;
import androidx.appcompat.widget.ContentFrameLayout;
import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.view.LayoutInflaterCompat;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class TouchEffects {
	public enum TouchEffectsExtraType {
		AspectRatio,
		;
	}
	public static class SingleHandler extends Handler {
		private static SingleHandler mInstance;
		private SingleHandler()
		{
		}
		public static SingleHandler getInstance()
		{
			if(mInstance == null) {
				synchronized (SingleHandler.class) {
					if(mInstance == null) {
						mInstance = new SingleHandler();
					}
				}
			}
			return mInstance;
		}
	}
	public static List<String> getAllViewTypes()
	{
		ArrayList<String> viewTypes = new ArrayList<>();
		viewTypes.add(ViewType.TextView);
		viewTypes.add(ViewType.Button);
		viewTypes.add(ViewType.ImageView);
		viewTypes.add(ViewType.ImageButton);
		viewTypes.add(ViewType.FrameLayout);
		viewTypes.add(ViewType.LinearLayout);
		viewTypes.add(ViewType.RelativeLayout);
		viewTypes.add(ViewType.ConstraintLayout);
		return viewTypes;
	}
	public static boolean isContainsExtraType(TouchEffectsExtraType extraType)
	{
		HashMap<TouchEffectsExtraType, BaseExtraBean> map = Manager.getExtraTypeMap();
		return map.containsKey(extraType);
	}
	public static interface TouchEffectsType {
	}
	public static enum TouchEffectsSingleType implements TouchEffectsType {
		SHAKE,
	}
	public static enum TouchEffectsWholeType implements TouchEffectsType {
		NONE,
		SCALE,
		RIPPLE,
		STATE,
		RIPPLE_1,
		//    ALL_SCALE,
		//    ALL_RIPPLE,
		//    ALL_STATE,
	}
	@Retention(RetentionPolicy.SOURCE) @Target({ElementType.ANNOTATION_TYPE}) public static @interface ConstraintType {
		TouchEffectsWholeType[] value() default {};
	}
	@ConstraintType({TouchEffectsWholeType.NONE, TouchEffectsWholeType.SCALE}) public static @interface DefaultType {
	}
	@ConstraintType({TouchEffectsWholeType.RIPPLE, TouchEffectsWholeType.STATE}) public static @interface ColorType {
	}
	//
	//
	//
	@Retention(RetentionPolicy.SOURCE)
	@StringDef({ViewType.ALL,
		ViewType.TextView,
		ViewType.Button,
		ViewType.ImageView,
		ViewType.ImageButton,
		ViewType.FrameLayout,
		ViewType.LinearLayout,
		ViewType.RelativeLayout,
		ViewType.ConstraintLayout})
	public static @interface ViewType {
		String ALL = "ALL";
		String TextView = "TextView";
		String Button = "Button";
		String ImageView = "ImageView";
		String ImageButton = "ImageButton";
		String FrameLayout = "FrameLayout";
		String LinearLayout = "LinearLayout";
		String RelativeLayout = "RelativeLayout";
		String ConstraintLayout = "android.support.constraint.ConstraintLayout";
		//    ALL("ALL"),
		//    TextView("TextView"),
		//    Button("Button"),
		//    ImageView("ImageView"),
		//    ImageButton("ImageButton"),
		//    FrameLayout("FrameLayout"),
		//    LinearLayout("LinearLayout"),
		//    RelativeLayout("RelativeLayout"),
		//    ConstraintLayout("android.support.constraint.ConstraintLayout");
		//
		//
		//
		//    private String mViewName;
		//
		//    public String getViewName() {
		//        return mViewName;
		//    }
		//
		//    ViewType(String viewName) {
		//        this.mViewName = viewName;
		//    }
	}
	//
	//
	//
	public static abstract class EffectsAdapter {
		public abstract void initAttr(Context context, AttributeSet attrs);
		public abstract void runAnimator(View view, Canvas canvas);
		public abstract void dispatchDraw(View view, Canvas canvas);
		public abstract void drawForeground(View view, Canvas canvas);
		public abstract boolean onTouch(View view, MotionEvent motionEvent, View.OnClickListener onClickListener, View.OnLongClickListener onLongClickListener);
		protected abstract Animator createEngineAnimator(View view);
		protected abstract Animator createExtinctAnimator(View view);

		protected final int LONG_DURATION = 850;
		protected final int CLICK_DURATION = 100;
		protected int mAnimationDuration = 100;
		protected Animator mEngineAnimator;
		protected Animator mExtinctAnimator;
		protected boolean isPointLeaveView;
		protected boolean isLongClick;
		protected int mViewWidth,mViewHeight;
		protected Runnable mClickRunnable = null;
		protected Runnable mLongClickRunnable = null;
		public void measuredSize(int width,int height)
		{
			mViewWidth = width;
			mViewHeight = height;
		}
		protected void engineAnimator(View view)
		{
			if(mExtinctAnimator != null && mExtinctAnimator.isRunning()){
				mExtinctAnimator.cancel();
			}
			if(mEngineAnimator == null) {
				mEngineAnimator = createEngineAnimator(view);
			}
			if(mEngineAnimator != null){
				mEngineAnimator.start();
			}
		}
		protected void extinctAnimator(View view)
		{
			if(mEngineAnimator != null && mEngineAnimator.isRunning()){
				mEngineAnimator.cancel();
			}
			if(mExtinctAnimator == null){
				mExtinctAnimator = createExtinctAnimator(view);
			}
			if(mExtinctAnimator != null){
				mExtinctAnimator.start();
			}
		}
		protected boolean touchView(View view, MotionEvent event, View.OnClickListener onClickListener)
		{
			if(onClickListener == null){
				return false;
			}
			switch (event.getAction()){
				case MotionEvent.ACTION_DOWN:
					isPointLeaveView = false;
					isLongClick = false;
					engineAnimator(view);
					if(mLongClickRunnable != null){
						SingleHandler.getInstance().postDelayed(mLongClickRunnable,LONG_DURATION);
					}
					break;
				case MotionEvent.ACTION_MOVE:
					if(!pointInView(view,event.getX(), event.getY(),0) && !isPointLeaveView){
						isPointLeaveView = true;
						extinctAnimator(view);
						if(mLongClickRunnable != null){
							isLongClick = false;
							SingleHandler.getInstance().removeCallbacks(mLongClickRunnable);
						}
					}
					break;
				case MotionEvent.ACTION_UP:
					if(onClickListener != null && !isPointLeaveView && !isLongClick){
						if(mClickRunnable == null){
							createClick(view,onClickListener);
						}
						SingleHandler.getInstance().postDelayed(mClickRunnable,CLICK_DURATION);
//                    onClickListener.onClick(view);
					}
					//不能加break
				case MotionEvent.ACTION_CANCEL:
					if(!isPointLeaveView){
						isPointLeaveView = true;
						extinctAnimator(view);
					}
					if(mLongClickRunnable != null){
						isLongClick = false;
						SingleHandler.getInstance().removeCallbacks(mLongClickRunnable);
					}
					break;
			}
			return true;
		}
		public void createClick(View view, View.OnClickListener onClickListener)
		{
			mClickRunnable = () -> {
				if(onClickListener != null){
					onClickListener.onClick(view);
				}
			};
		}
		public void createLongClick(View view, View.OnLongClickListener onLongClickListener)
		{
			mLongClickRunnable = () -> {
				if(onLongClickListener != null){
					isLongClick = onLongClickListener.onLongClick(view);
				}
			};
		}
		protected boolean pointInView(View view, float localX, float localY, float slop)
		{
			return localX >= -slop && localY >= -slop && localX < ((view.getRight() - view.getLeft()) + slop) &&
					localY < ((view.getBottom() - view.getTop()) + slop);
		}
	}
	//
	public static abstract class AnimatorEndListener implements Animator.AnimatorListener {
		public abstract void onAnimatorEnd(Animator animation);
		@Override public void onAnimationStart(Animator animation)
		{
		}
		@Override public void onAnimationEnd(Animator animation) 
		{
			onAnimatorEnd(animation);
		}
		@Override public void onAnimationCancel(Animator animation)
		{
		}
		@Override public void onAnimationRepeat(Animator animation)
		{
		}
	}

	public static class ColorBean {
		private int pressedColor;
		private int normalColor;
		private RadiusBean mRadiusBean;
		public ColorBean(int normalColor,int pressedColor)
		{
			this.pressedColor = pressedColor;
			this.normalColor = normalColor;
		}
		public ColorBean(int normalColor,int pressedColor, RadiusBean radiusBean)
		{
			this.pressedColor = pressedColor;
			this.normalColor = normalColor;
			mRadiusBean = radiusBean;
		}
		public void setPressedColor(int pressedColor) { this.pressedColor = pressedColor; }
		public void setNormalColor(int normalColor) { this.normalColor = normalColor; }
		public void setRadiusBean(RadiusBean radiusBean) { mRadiusBean = radiusBean; }
		public int getPressedColor() { return pressedColor; }
		public int getNormalColor() { return normalColor; }
		public RadiusBean getRadiusBean() { return mRadiusBean; }
	}
	public static class RadiusBean {
		private int mTopLeftRadius;
		private int mTopRightRadius;
		private int mBottomLeftRadius;
		private int mBottomRightRadius;

		public RadiusBean(int topLeftRadius, int topRightRadius, int bottomLeftRadius, int bottomRightRadius)
		{
			mTopLeftRadius = topLeftRadius;
			mTopRightRadius = topRightRadius;
			mBottomLeftRadius = bottomLeftRadius;
			mBottomRightRadius = bottomRightRadius;
		}
		public void setTopLeftRadius(int topLeftRadius) { mTopLeftRadius = topLeftRadius; }
		public void setTopRightRadius(int topRightRadius) { mTopRightRadius = topRightRadius; }
		public void setBottomLeftRadius(int bottomLeftRadius) { mBottomLeftRadius = bottomLeftRadius; }
		public void setBottomRightRadius(int bottomRightRadius) { mBottomRightRadius = bottomRightRadius; }
		public int getTopLeftRadius() { return mTopLeftRadius; }
		public int getTopRightRadius() { return mTopRightRadius; }
		public int getBottomLeftRadius() { return mBottomLeftRadius; }
		public int getBottomRightRadius() { return mBottomRightRadius; }
	}
	public class ScaleBean {
		private int mAnimationDuration;
		private int mShakeScale;
		public ScaleBean(int animationDuration, int shakeScale)
		{
			mAnimationDuration = animationDuration;
			mShakeScale = shakeScale;
		}
		public int getAnimationDuration() { return mAnimationDuration; }
		public int getShakeScale() { return mShakeScale; }
	}
	public static class BaseExtraBean {
		protected TouchEffectsWholeType mWholeType;
		public void setWholeType(TouchEffectsWholeType wholeType)
		{
			mWholeType = wholeType;
		}
		public TouchEffectsWholeType getWholeType() { return mWholeType; }
	}
	public static class ExtraAspectRatioBean extends BaseExtraBean {
		private float mWidth;
		private float mHeight;
		public ExtraAspectRatioBean(TouchEffectsWholeType wholeType,float width,float height)
		{
			mWholeType = wholeType;
			mWidth = width;
			mHeight = height;
		}
		public float getWidth() { return mWidth; }
		public float getHeight() { return mHeight; }
	}
	//
	public static class TouchRippleAdapter extends EffectsAdapter {
		private final int TRANSPARENT_COLOR = 0x00000000;
		private int mPressedColor;
		private int mNormalColor;
		private int mCurrentColor;
		private int mRippleColor;
		private float mRadius;
		private ColorBean mColorBean;
		private float mColorValue;
		private float mCircleRadius;
		private float mTouchX,mTouchY;
		private Paint mPaint;
		private RectF mRect;
		private Path mPath;
		private float[] mRadiusArray;

		public TouchRippleAdapter(ColorBean colorBean)
		{
			mColorBean = colorBean;
		}
		@Override public void initAttr(Context context, AttributeSet attrs)
		{
			TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.TouchEffectsView);
			mPressedColor = ta.getColor(R.styleable.TouchEffectsView_touch_effects_pressed_color, mColorBean != null? mColorBean.getPressedColor():0);
			mNormalColor = ta.getColor(R.styleable.TouchEffectsView_touch_effects_normal_color, mColorBean != null? mColorBean.getNormalColor():0);
			if(mNormalColor == 0){
				mNormalColor = mPressedColor;
			}
			mRippleColor = mNormalColor;
			mRadius = ta.getDimension(R.styleable.TouchEffectsView_touch_effects_radius,0);
			if(mRadius != 0){
				mRadiusArray = new float[8];
				mRadiusArray[0] = mRadius;
				mRadiusArray[1] = mRadius;
				mRadiusArray[2] = mRadius;
				mRadiusArray[3] = mRadius;
				mRadiusArray[4] = mRadius;
				mRadiusArray[5] = mRadius;
				mRadiusArray[6] = mRadius;
				mRadiusArray[7] = mRadius;
			}
			mPaint = new Paint();
			mPaint.setAntiAlias(true);

		}
		@Override public void measuredSize(int width, int height)
		{
			super.measuredSize(width, height);
			if(mRadiusArray != null){
				mPath = new Path();
				mPath.addRoundRect(new RectF(0, 0, width, height), mRadiusArray, Path.Direction.CW);
			}
			mRect = new RectF(0,0,width,height);
		}
		@Override public void runAnimator(View view, Canvas canvas)
		{
		}
		@Override public void dispatchDraw(View view, Canvas canvas)
		{
			if(mPath != null){
				canvas.clipPath(mPath);
			}
			mPaint.setColor(mCurrentColor);
			canvas.drawRoundRect(mRect,mRadius,mRadius,mPaint);
			if(mCircleRadius != 0){
				mPaint.setColor(mRippleColor);
				canvas.drawCircle(mTouchX,mTouchY,mCircleRadius,mPaint);
			}
		}
		@Override public void drawForeground(View view, Canvas canvas)
		{
			dispatchDraw(view,canvas);
		}
		@Override public boolean onTouch(View view, MotionEvent event, View.OnClickListener onClickListener,View.OnLongClickListener onLongClickListener)
		{
			switch (event.getAction()){
				case MotionEvent.ACTION_DOWN:
					mTouchX = event.getX();
					mTouchY = event.getY();
					break;
				case MotionEvent.ACTION_MOVE:
					if(pointInView(view,event.getX(), event.getY(),0)){
						mTouchX = event.getX();
						mTouchY = event.getY();
					}
					break;
			}
			return touchView(view,event,onClickListener);
		}
		@Override protected Animator createEngineAnimator(View view)
		{
			ArgbEvaluator argbEvaluator = new ArgbEvaluator();//渐变色计算类
			ValueAnimator valueAnimator = ValueAnimator.ofFloat(0f,1f);
			valueAnimator.setDuration(850);
//                valueAnimator.setInterpolator(new DecelerateInterpolator());
			valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
				@Override
				public void onAnimationUpdate(ValueAnimator animation) {
					mColorValue = (float) animation.getAnimatedValue();
					mCurrentColor = (int) (argbEvaluator.evaluate(mColorValue, TRANSPARENT_COLOR, mPressedColor));
					view.invalidate();
				}
			});
			return valueAnimator;
		}
		@Override protected Animator createExtinctAnimator(View view)
		{
			ArgbEvaluator argbEvaluator = new ArgbEvaluator();//渐变色计算类
			ValueAnimator valueAnimator;
			if(mColorValue < 0.5f){
				valueAnimator = ValueAnimator.ofFloat(mColorValue,0.8f,0.6f,0.4f,0.2f,0f);
			}
			else{
				valueAnimator = ValueAnimator.ofFloat(mColorValue,0f);
			}
			valueAnimator.setDuration(450);
			valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
				@Override public void onAnimationUpdate(ValueAnimator animation) 
				{
					mColorValue = (float) animation.getAnimatedValue();
					mCurrentColor = (int) (argbEvaluator.evaluate(mColorValue, TRANSPARENT_COLOR, mNormalColor));
					view.invalidate();
				}
			});
			ValueAnimator valueAnimator1 = ValueAnimator.ofFloat(0f,Math.max(view.getWidth(),view.getHeight())/1.25f);
			valueAnimator1.setDuration(400);
			valueAnimator1.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
				@Override public void onAnimationUpdate(ValueAnimator animation)
				{
					float value = (float) animation.getAnimatedValue();
					mCircleRadius = value;
				}
			});
			valueAnimator1.setInterpolator(new DecelerateInterpolator());
			valueAnimator1.addListener(new AnimatorEndListener() {
				@Override public void onAnimatorEnd(Animator animation) 
				{
					mCircleRadius = 0;
					view.invalidate();
				}
			});
			ValueAnimator valueAnimator2 = ValueAnimator.ofFloat(1f,0f);
			valueAnimator2.setDuration(400);
			valueAnimator2.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
				@Override public void onAnimationUpdate(ValueAnimator animation)
				{
					float value = (float) animation.getAnimatedValue();
					mRippleColor = (int) (argbEvaluator.evaluate(value, TRANSPARENT_COLOR, mNormalColor));
				}
			});
			AnimatorSet animatorSet = new AnimatorSet();
			animatorSet.playTogether(valueAnimator,valueAnimator1,valueAnimator2);
			return animatorSet;
		}
	}
	public static class TouchRipple1Adapter extends EffectsAdapter {
		private final int TRANSPARENT_COLOR = 0x00000000;
		private int mPressedColor;
		private int mNormalColor;
		private int mCurrentColor;
		private float mRadius;
		private ColorBean mColorBean;
		private float mCircleRadius;
		private float mTouchX,mTouchY;
		private Paint mPaint;
		private RectF mRect;
		private Path mPath;
		private float[] mRadiusArray;
		public TouchRipple1Adapter(ColorBean colorBean)
		{
			mColorBean = colorBean;
		}
		@Override public void initAttr(Context context, AttributeSet attrs)
		{
			TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.TouchEffectsView);
			mPressedColor = ta.getColor(R.styleable.TouchEffectsView_touch_effects_pressed_color, mColorBean != null? mColorBean.getPressedColor():0);
			mNormalColor = ta.getColor(R.styleable.TouchEffectsView_touch_effects_normal_color, mColorBean != null? mColorBean.getNormalColor():0);
			if(mNormalColor == 0){
				mNormalColor = mPressedColor;
			}
			mRadius = ta.getDimension(R.styleable.TouchEffectsView_touch_effects_radius,0);
			if(mRadius != 0){
				mRadiusArray = new float[8];
				mRadiusArray[0] = mRadius;
				mRadiusArray[1] = mRadius;
				mRadiusArray[2] = mRadius;
				mRadiusArray[3] = mRadius;
				mRadiusArray[4] = mRadius;
				mRadiusArray[5] = mRadius;
				mRadiusArray[6] = mRadius;
				mRadiusArray[7] = mRadius;
			}
			mPaint = new Paint();
			mPaint.setAntiAlias(true);
		}
		@Override public void measuredSize(int width, int height)
		{
			super.measuredSize(width, height);
			if(mRadiusArray != null){
				mPath = new Path();
				mPath.addRoundRect(new RectF(0, 0, width, height), mRadiusArray, Path.Direction.CW);
			}
			mRect = new RectF(0,0,width,height);
		}
		@Override public void runAnimator(View view, Canvas canvas)
		{
		}
		@Override public void dispatchDraw(View view, Canvas canvas)
		{
			if(mPath != null)
				canvas.clipPath(mPath);
			mPaint.setColor(mCurrentColor);
			canvas.drawCircle(mTouchX,mTouchY,mCircleRadius,mPaint);
		}
		@Override public void drawForeground(View view, Canvas canvas)
		{
			dispatchDraw(view,canvas);
		}
		@Override public boolean onTouch(View view, MotionEvent event, View.OnClickListener onClickListener,View.OnLongClickListener onLongClickListener)
		{
			switch(event.getAction()){
				case MotionEvent.ACTION_DOWN:
					mCircleRadius = 0;
					mCurrentColor = mPressedColor;
					mTouchX = event.getX();
					mTouchY = event.getY();
					break;
				case MotionEvent.ACTION_MOVE:
					if(pointInView(view,event.getX(), event.getY(),0)){
						mTouchX = event.getX();
						mTouchY = event.getY();
					}
					break;
			}
			return touchView(view,event,onClickListener);
		}
		@Override protected void engineAnimator(View view)
		{
			if(mEngineAnimator != null){
				mEngineAnimator.setDuration((long) getTimeScale(view));
			}
			super.engineAnimator(view);
		}
		@Override protected void extinctAnimator(View view)
		{
			if(mExtinctAnimator == null){
				mExtinctAnimator = createExtinctAnimator(view);
			}
			if(mExtinctAnimator != null){
				if(mExtinctAnimator.isRunning()){
					mExtinctAnimator.cancel();
				}
				mExtinctAnimator.start();
			}
		}
		@Override protected Animator createEngineAnimator(View view)
		{
			ValueAnimator valueAnimator = ValueAnimator.ofFloat(0f,Math.max(view.getWidth(),view.getHeight()) * 1.42f);
			valueAnimator.setDuration((long) getTimeScale(view));
			valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
				@Override public void onAnimationUpdate(ValueAnimator animation)
				{
					float value = (float) animation.getAnimatedValue();
					mCircleRadius = value;
					view.invalidate();
				}
			});
			return valueAnimator;
		}
		@Override protected Animator createExtinctAnimator(View view)
		{
			ArgbEvaluator argbEvaluator = new ArgbEvaluator();//渐变色计算类
			ValueAnimator valueAnimator;
			valueAnimator = ValueAnimator.ofFloat(1f,0f);
			valueAnimator.setDuration(450);
			valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
				@Override public void onAnimationUpdate(ValueAnimator animation)
				{
					mCurrentColor = (int) (argbEvaluator.evaluate((float)animation.getAnimatedValue(), TRANSPARENT_COLOR, mNormalColor));
					view.invalidate();
				}
			});
			return valueAnimator;
		}
		private float getTimeScale(View view)
		{
			float sizeX = Math.abs(mTouchX - view.getWidth()/2f);
			float sizeY = Math.abs(mTouchY - view.getHeight()/2f);
			float timeScale = 0f;
			if(sizeX != 0 && sizeY != 0){
				timeScale = Math.min(sizeX,sizeY)/(sizeX<sizeY?view.getWidth()/2f:view.getHeight()/2f);
			}
			return 450f - (200f * timeScale);
		}
	}
	public static class TouchStateAdapter extends EffectsAdapter {
		private final int TRANSPARENT_COLOR = 0x00000000;
		private int mPressedColor;
		private int mNormalColor;
		private int mCurrentColor;
		private float mRadius;
		private ColorBean mColorBean;
		private float mColorValue;
		private Paint mPaint;
		private RectF mRect;

		public TouchStateAdapter(ColorBean colorBean) { mColorBean = colorBean; }
		@Override public void initAttr(Context context, AttributeSet attrs)
		{
			TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.TouchEffectsView);
			mPressedColor = ta.getColor(R.styleable.TouchEffectsView_touch_effects_pressed_color, mColorBean != null? mColorBean.getPressedColor():0);
			mNormalColor = ta.getColor(R.styleable.TouchEffectsView_touch_effects_normal_color, mColorBean != null? mColorBean.getNormalColor():0);
			if(mNormalColor == 0)
				mNormalColor = mPressedColor;
			mRadius = ta.getDimension(R.styleable.TouchEffectsView_touch_effects_radius,0);
			mPaint = new Paint();
			mPaint.setAntiAlias(true);
		}
		@Override public void measuredSize(int width, int height)
		{
			super.measuredSize(width, height);
			mRect = new RectF(0,0,width,height);
		}
		@Override public void runAnimator(View view, Canvas canvas)
		{
		}
		@Override public void dispatchDraw(View view, Canvas canvas)
		{
			mPaint.setColor(mCurrentColor);
//        mPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.SRC_IN));
			canvas.drawRoundRect(mRect,mRadius,mRadius,mPaint);
		}
		@Override public void drawForeground(View view, Canvas canvas) {
			dispatchDraw(view,canvas);
		}
		@Override public boolean onTouch(View view, MotionEvent event, View.OnClickListener onClickListener, View.OnLongClickListener onLongClickListener)
		{
			return touchView(view,event,onClickListener);
		}
		@Override protected Animator createEngineAnimator(View view)
		{
			ArgbEvaluator argbEvaluator = new ArgbEvaluator();//渐变色计算类
			ValueAnimator valueAnimator = ValueAnimator.ofFloat(0f,1f);
			valueAnimator.setDuration(850);
			valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
				@Override
				public void onAnimationUpdate(ValueAnimator animation) {
					mColorValue = (float) animation.getAnimatedValue();
					mCurrentColor = (int) (argbEvaluator.evaluate(mColorValue, TRANSPARENT_COLOR, mPressedColor));
					view.invalidate();
				}
			});
			return valueAnimator;
		}
		@Override protected Animator createExtinctAnimator(View view)
		{
			ArgbEvaluator argbEvaluator = new ArgbEvaluator();//渐变色计算类
			ValueAnimator valueAnimator;
			if(mColorValue < 0.5f){
				valueAnimator = ValueAnimator.ofFloat(mColorValue,0.8f,0.6f,0.4f,0.2f,0f);
			}
			else {
				valueAnimator = ValueAnimator.ofFloat(mColorValue,0f);
			}
			valueAnimator.setDuration(450);
			valueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
				@Override public void onAnimationUpdate(ValueAnimator animation)
				{
					mColorValue = (float) animation.getAnimatedValue();
					mCurrentColor = (int) (argbEvaluator.evaluate(mColorValue, TRANSPARENT_COLOR, mNormalColor));
					view.invalidate();
				}
			});
			return valueAnimator;
		}
	}
	public static class TouchScaleAdapter extends EffectsAdapter {
		private float mShakeScale = 0.85f;
		private float mCurrentScaleX = 1.0f,mCurrentScaleY = 1.0f;
		public TouchScaleAdapter(ScaleBean scaleBean)
		{
			if(scaleBean != null){
				mShakeScale = scaleBean.getShakeScale();
				mAnimationDuration = scaleBean.getAnimationDuration();
			}
		}
		@Override public void initAttr(Context context, AttributeSet attrs)
		{
			TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.TouchEffectsView);
			mAnimationDuration = ta.getResourceId(R.styleable.TouchEffectsView_animation_duration,100);
			mShakeScale = ta.getFloat(R.styleable.TouchEffectsView_shake_view_scale,0.85f);
		}
		@Override public void runAnimator(View view,Canvas canvas)
		{
			canvas.scale(mCurrentScaleX,mCurrentScaleY,view.getMeasuredWidth()/2f,view.getMeasuredHeight()/2f);
		}
		@Override public void dispatchDraw(View view, Canvas canvas)
		{
		}
		@Override public void drawForeground(View view, Canvas canvas)
		{
		}
		@Override public boolean onTouch(View view, MotionEvent motionEvent, View.OnClickListener onClickListener, View.OnLongClickListener onLongClickListener)
		{
			return touchView(view,motionEvent,onClickListener);
		}
		@Override protected Animator createEngineAnimator(View view)
		{
			ValueAnimator valueAnimator = ValueAnimator.ofFloat(1.0f, mShakeScale);
			valueAnimator.setDuration(mAnimationDuration);
			valueAnimator.addUpdateListener(animation -> {
				mCurrentScaleX = (float) animation.getAnimatedValue();
				mCurrentScaleY = mCurrentScaleX;
				view.invalidate();
			});
			return valueAnimator;
		}
		@Override protected Animator createExtinctAnimator(View view)
		{
			ValueAnimator valueAnimator = ValueAnimator.ofFloat(mShakeScale,1.0f);
			valueAnimator.setDuration(mAnimationDuration);
			valueAnimator.addUpdateListener(animation -> {
				mCurrentScaleX = (float) animation.getAnimatedValue();
				mCurrentScaleY = mCurrentScaleX;
				view.invalidate();
			});
			return valueAnimator;
		}
	}
	public static class TouchShakeAdapter extends EffectsAdapter {
		private float mTranslateValue;
		@Override public void initAttr(Context context, AttributeSet attrs)
		{
			TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.TouchEffectsView);
			mAnimationDuration = ta.getResourceId(R.styleable.TouchEffectsView_animation_duration,150);
		}
		@Override public void runAnimator(View view, Canvas canvas)
		{
			canvas.translate(view.getWidth() * mTranslateValue,0);
		}
		@Override public void dispatchDraw(View view, Canvas canvas)
		{
		}
		@Override public void drawForeground(View view, Canvas canvas)
		{
		}
		@Override public boolean onTouch(View view, MotionEvent motionEvent, View.OnClickListener onClickListener,View.OnLongClickListener onLongClickListener)
		{
			switch(motionEvent.getAction()) {
				case MotionEvent.ACTION_DOWN:
					engineAnimator(view);
					break;
			}
			return false;
		}
		@Override protected Animator createEngineAnimator(View view)
		{
			ValueAnimator valueAnimator = ValueAnimator.ofFloat(0f,-0.1f,0f,0.1f,0f,-0.1f,0f);
			valueAnimator.setDuration(mAnimationDuration);
			valueAnimator.addUpdateListener(animation -> {
				mTranslateValue = (float) animation.getAnimatedValue();
				view.invalidate();
			});
			return valueAnimator;
		}
		@Override protected Animator createExtinctAnimator(View view) { return null; }
	}
	//
	//
	//
	public static class BaseEffectsProxy {
		protected EffectsAdapter mAdapter;
		protected AttributeSet mAttributeSet;
		protected Context mContext;
		public BaseEffectsProxy(EffectsAdapter adapter) 
		{
			mAdapter = adapter;
		}
		public void replaceEffectAdapter(EffectsAdapter adapter)
		{
			mAdapter = adapter;
		}
		public EffectsAdapter getAdapter() {
			return mAdapter;
		}
		public void initAttr(Context context, AttributeSet attrs)
		{
			mContext = context;
			mAttributeSet = attrs;
			if(mAdapter != null){
				mAdapter.initAttr(context,attrs);
			}
		}
		public void measuredSize(int measuredWidth, int measuredHeight)
		{
			if(mAdapter != null){
				mAdapter.measuredSize(measuredWidth,measuredHeight);
			}
		}
	}
	public static class AspectRatioEffectsProxy extends BaseEffectsProxy {
		public AspectRatioEffectsProxy(EffectsAdapter adapter)
		{
			super(adapter);
		}
		@Override public void measuredSize(int measuredWidth, int measuredHeight)
		{
			BaseExtraBean extraBean = Manager.getExtraTypeMap().get(TouchEffectsExtraType.AspectRatio);
			boolean isChange = false;
			if(extraBean != null){
				ExtraAspectRatioBean aspectRatioBean = (ExtraAspectRatioBean) extraBean;
				float width = aspectRatioBean.getWidth();
				float height = aspectRatioBean.getHeight();
				//启动了大小判断，当宽或高大小超过，就转换模式
				if((width > 1f && measuredWidth > width) || (height > 1f && measuredHeight > height)){
					isChange = true;
				}
				else if(width <= 1f && height <= 1f){
					//比例判断
					if(width > height && width/height < measuredWidth/(float)measuredHeight){
						isChange = true;
					}
					else if(height > width && width/height > measuredWidth/(float)measuredHeight){
						isChange = true;
					}
				}
			}
			if(isChange) {
				mAdapter = getAdapter(extraBean.getWholeType());
				mAdapter.initAttr(mContext,mAttributeSet);
				mAdapter.measuredSize(measuredWidth,measuredHeight);
			}
			else {
				super.measuredSize(measuredWidth, measuredHeight);
			}
		}
		private EffectsAdapter getAdapter(TouchEffectsType type)
		{
			if(type instanceof TouchEffectsSingleType){
				TouchEffectsSingleType singleType = (TouchEffectsSingleType)type;
				switch(singleType){
					case SHAKE: return new TouchShakeAdapter();
				}
			}
			else {
				TouchEffectsWholeType wholeType = (TouchEffectsWholeType) type;
				switch(wholeType){
					case SCALE: return new TouchScaleAdapter(Manager.getScaleBean());
					case RIPPLE: return new TouchRippleAdapter(Manager.getColorBean());
					case STATE: return new TouchStateAdapter(Manager.getColorBean());
					case RIPPLE_1: return new TouchRipple1Adapter(Manager.getColorBean());
				}
			}
			return null;
		}
	}
	/**
	 * Created by lky on 2018/9/14
	 */
	public static class TouchEffectsButton extends AppCompatButton {
		private BaseEffectsProxy mEffectsProxy;
		public TouchEffectsButton(Context context) {
			this(context,null);
		}
		public TouchEffectsButton(Context context, @Nullable AttributeSet attrs)
		{
			super(context, attrs);
		}
		public TouchEffectsButton(Context context, @Nullable AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			super(context, attrs);
			mEffectsProxy = effectsProxy;
			mEffectsProxy.initAttr(context,attrs);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
			mEffectsProxy.measuredSize(getMeasuredWidth(),getMeasuredHeight());
		}
		@Override protected void onDraw(Canvas canvas)
		{
			mEffectsProxy.getAdapter().runAnimator(this,canvas);
			super.onDraw(canvas);
		}
		@Override protected void dispatchDraw(Canvas canvas)
		{
			super.dispatchDraw(canvas);
			if(Build.VERSION.SDK_INT < Build.VERSION_CODES.M){
				mEffectsProxy.getAdapter().dispatchDraw(this,canvas);
			}
		}
		@Override public void onDrawForeground(Canvas canvas)
		{
			super.onDrawForeground(canvas);
			mEffectsProxy.getAdapter().drawForeground(this,canvas);
		}
		@Override public boolean onTouchEvent(MotionEvent event)
		{
			if(mOnClickListener == null && mOnLongClickListener == null || !isEnabled()){
				return super.onTouchEvent(event);
			}
			return mEffectsProxy.getAdapter().onTouch(this,event,mOnClickListener,mOnLongClickListener);
		}
		public OnClickListener mOnClickListener;
		@Override public void setOnClickListener(@Nullable OnClickListener l) {
			mOnClickListener = l;
		}
		public OnLongClickListener mOnLongClickListener;
		@Override public void setOnLongClickListener(OnLongClickListener onLongClickListener)
		{
			mOnLongClickListener = onLongClickListener;
			if(mOnLongClickListener != null){
				mEffectsProxy.getAdapter().createLongClick(this,mOnLongClickListener);
			}
		}
	}

	public static class TouchEffectsImageButton extends AppCompatImageButton {
		private BaseEffectsProxy mEffectsProxy;
		public TouchEffectsImageButton(Context context) {
			this(context,null);
		}
		public TouchEffectsImageButton(Context context, @Nullable AttributeSet attrs)
		{
			super(context, attrs);
		}
		public TouchEffectsImageButton(Context context, @Nullable AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			super(context, attrs,0);
			mEffectsProxy = effectsProxy;
			mEffectsProxy.initAttr(context,attrs);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
			mEffectsProxy.measuredSize(getMeasuredWidth(),getMeasuredHeight());
		}
		@Override protected void onDraw(Canvas canvas)
		{
			mEffectsProxy.getAdapter().runAnimator(this,canvas);
			super.onDraw(canvas);
		}
		@Override protected void dispatchDraw(Canvas canvas)
		{
			super.dispatchDraw(canvas);
			if(Build.VERSION.SDK_INT < Build.VERSION_CODES.M){
				mEffectsProxy.getAdapter().dispatchDraw(this,canvas);
			}
		}
		@Override public void onDrawForeground(Canvas canvas)
		{
			super.onDrawForeground(canvas);
			mEffectsProxy.getAdapter().drawForeground(this,canvas);
		}
		@Override public boolean onTouchEvent(MotionEvent event)
		{
			if(mOnClickListener == null && mOnLongClickListener == null || !isEnabled()){
				return super.onTouchEvent(event);
			}
			return mEffectsProxy.getAdapter().onTouch(this,event,mOnClickListener,mOnLongClickListener);
		}
		public OnClickListener mOnClickListener;
		@Override public void setOnClickListener(@Nullable OnClickListener l) {
			mOnClickListener = l;
		}
		public OnLongClickListener mOnLongClickListener;
		@Override public void setOnLongClickListener(OnLongClickListener onLongClickListener)
		{
			mOnLongClickListener = onLongClickListener;
			if(mOnLongClickListener != null){
				mEffectsProxy.getAdapter().createLongClick(this,mOnLongClickListener);
			}
		}
	}
	/**
	 * Created by lky on 2018/8/23
	 */
	public static class TouchEffectsImageView extends AppCompatImageView {
		private BaseEffectsProxy mEffectsProxy;
		public TouchEffectsImageView(Context context) {
			this(context,null);
		}
		public TouchEffectsImageView(Context context, @Nullable AttributeSet attrs) {
			super(context, attrs);
		}
		public TouchEffectsImageView(Context context, @Nullable AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			super(context, attrs,0);
			mEffectsProxy = effectsProxy;
			mEffectsProxy.initAttr(context,attrs);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
			mEffectsProxy.measuredSize(getMeasuredWidth(),getMeasuredHeight());
		}
		@Override protected void onDraw(Canvas canvas)
		{
			mEffectsProxy.getAdapter().runAnimator(this,canvas);
			super.onDraw(canvas);
		}
		@Override protected void dispatchDraw(Canvas canvas)
		{
			super.dispatchDraw(canvas);
			if(Build.VERSION.SDK_INT < Build.VERSION_CODES.M){
				mEffectsProxy.getAdapter().dispatchDraw(this,canvas);
			}
		}
		@Override public void onDrawForeground(Canvas canvas)
		{
			super.onDrawForeground(canvas);
			mEffectsProxy.getAdapter().drawForeground(this,canvas);
		}
		@Override public boolean onTouchEvent(MotionEvent event)
		{
			if(mOnClickListener == null && mOnLongClickListener == null || !isEnabled()){
				return super.onTouchEvent(event);
			}
			return mEffectsProxy.getAdapter().onTouch(this,event,mOnClickListener,mOnLongClickListener);
		}
		public OnClickListener mOnClickListener;
		@Override public void setOnClickListener(@Nullable OnClickListener l) {
			mOnClickListener = l;
		}
		public OnLongClickListener mOnLongClickListener;
		@Override public void setOnLongClickListener(OnLongClickListener onLongClickListener)
		{
			mOnLongClickListener = onLongClickListener;
			if(mOnLongClickListener != null){
				mEffectsProxy.getAdapter().createLongClick(this,mOnLongClickListener);
			}
		}
	}
	/**
	 * Created by lky on 2018/8/27
	 */
	public static class TouchEffectsTextView extends AppCompatTextView {
		private BaseEffectsProxy mEffectsProxy;
		public TouchEffectsTextView(Context context) {
			this(context,null);
		}
		public TouchEffectsTextView(Context context, @Nullable AttributeSet attrs) {
			super(context, attrs);
		}
		public TouchEffectsTextView(Context context, @Nullable AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			super(context, attrs,0);
			mEffectsProxy = effectsProxy;
			mEffectsProxy.initAttr(context,attrs);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
			mEffectsProxy.measuredSize(getMeasuredWidth(),getMeasuredHeight());
		}
		@Override protected void onDraw(Canvas canvas)
		{
			mEffectsProxy.getAdapter().runAnimator(this,canvas);
			super.onDraw(canvas);
		}
		@Override protected void dispatchDraw(Canvas canvas)
		{
			super.dispatchDraw(canvas);
			if(Build.VERSION.SDK_INT < Build.VERSION_CODES.M){
				mEffectsProxy.getAdapter().dispatchDraw(this,canvas);
			}
		}
		@Override public void onDrawForeground(Canvas canvas)
		{
			super.onDrawForeground(canvas);
			mEffectsProxy.getAdapter().drawForeground(this,canvas);
		}
		@Override public boolean onTouchEvent(MotionEvent event)
		{
			if(mOnClickListener == null && mOnLongClickListener == null || !isEnabled()){
				return super.onTouchEvent(event);
			}
			return mEffectsProxy.getAdapter().onTouch(this,event,mOnClickListener,mOnLongClickListener);
		}
		public OnClickListener mOnClickListener;
		@Override public void setOnClickListener(@Nullable OnClickListener l) {
			mOnClickListener = l;
		}
		public OnLongClickListener mOnLongClickListener;
		@Override public void setOnLongClickListener(OnLongClickListener onLongClickListener)
		{
			mOnLongClickListener = onLongClickListener;
			if(mOnLongClickListener != null){
				mEffectsProxy.getAdapter().createLongClick(this,mOnLongClickListener);
			}
		}
	}
	/**
	 * Created by lky on 2018/8/27
	 */
	public static class TouchEffectsLinearLayout extends LinearLayout {
		private BaseEffectsProxy mEffectsProxy;
		public TouchEffectsLinearLayout(Context context) {
			this(context,null);
		}
		public TouchEffectsLinearLayout(Context context, @Nullable AttributeSet attrs)
		{
			super(context, attrs);
		}
		public TouchEffectsLinearLayout(Context context, @Nullable AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			super(context, attrs,0);
			setWillNotDraw(false);
			mEffectsProxy = effectsProxy;
			mEffectsProxy.initAttr(context,attrs);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
			mEffectsProxy.measuredSize(getMeasuredWidth(),getMeasuredHeight());
		}
		@Override protected void onDraw(Canvas canvas)
		{
			mEffectsProxy.getAdapter().runAnimator(this,canvas);
			super.onDraw(canvas);
		}
		@Override protected void dispatchDraw(Canvas canvas)
		{
			super.dispatchDraw(canvas);
			if(Build.VERSION.SDK_INT < Build.VERSION_CODES.M){
				mEffectsProxy.getAdapter().dispatchDraw(this,canvas);
			}
		}
		@Override public void onDrawForeground(Canvas canvas)
		{
			super.onDrawForeground(canvas);
			mEffectsProxy.getAdapter().drawForeground(this,canvas);
		}
		@Override public boolean onTouchEvent(MotionEvent event)
		{
			if(mOnClickListener == null && mOnLongClickListener == null || !isEnabled()){
				return super.onTouchEvent(event);
			}
			return mEffectsProxy.getAdapter().onTouch(this,event,mOnClickListener,mOnLongClickListener);
		}
		public OnClickListener mOnClickListener;
		@Override public void setOnClickListener(@Nullable OnClickListener l) {
			mOnClickListener = l;
		}
		public OnLongClickListener mOnLongClickListener;
		@Override public void setOnLongClickListener(OnLongClickListener onLongClickListener)
		{
			mOnLongClickListener = onLongClickListener;
			if(mOnLongClickListener != null){
				mEffectsProxy.getAdapter().createLongClick(this,mOnLongClickListener);
			}
		}
	}
	/**
	 * Created by lky on 2018/9/4
	 */
	public static class TouchEffectsFrameLayout extends ContentFrameLayout {
		private BaseEffectsProxy mEffectsProxy;
		public TouchEffectsFrameLayout(Context context) {
			this(context,null);
		}
		public TouchEffectsFrameLayout(Context context, @Nullable AttributeSet attrs)
		{
			super(context, attrs);
		}
		public TouchEffectsFrameLayout(Context context, @Nullable AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			super(context, attrs,0);
			setWillNotDraw(false);
			mEffectsProxy = effectsProxy;
			mEffectsProxy.initAttr(context,attrs);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
			mEffectsProxy.measuredSize(getMeasuredWidth(),getMeasuredHeight());
		}
		@Override protected void onDraw(Canvas canvas)
		{
			mEffectsProxy.getAdapter().runAnimator(this,canvas);
			super.onDraw(canvas);
		}
		@Override protected void dispatchDraw(Canvas canvas)
		{
			super.dispatchDraw(canvas);
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M){
				mEffectsProxy.getAdapter().dispatchDraw(this,canvas);
			}
		}
		@Override public void onDrawForeground(Canvas canvas)
		{
			super.onDrawForeground(canvas);
			mEffectsProxy.getAdapter().drawForeground(this,canvas);
		}
		@Override public boolean onTouchEvent(MotionEvent event)
		{
			if(mOnClickListener == null && mOnLongClickListener == null || !isEnabled()){
				return super.onTouchEvent(event);
			}
			return mEffectsProxy.getAdapter().onTouch(this,event,mOnClickListener,mOnLongClickListener);
		}
		public OnClickListener mOnClickListener;
		@Override public void setOnClickListener(@Nullable OnClickListener l) {
			mOnClickListener = l;
		}
		public OnLongClickListener mOnLongClickListener;
		@Override public void setOnLongClickListener(OnLongClickListener onLongClickListener)
		{
			mOnLongClickListener = onLongClickListener;
			if(mOnLongClickListener != null){
				mEffectsProxy.getAdapter().createLongClick(this,mOnLongClickListener);
			}
		}
	}
	public static class TouchEffectsConstraintLayout extends ConstraintLayout {
		private BaseEffectsProxy mEffectsProxy;
		public TouchEffectsConstraintLayout(Context context) {
			this(context,null);
		}
		public TouchEffectsConstraintLayout(Context context, @Nullable AttributeSet attrs)
		{
			super(context, attrs);
		}
		public TouchEffectsConstraintLayout(Context context, @Nullable AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			super(context, attrs,0);
			setWillNotDraw(false);
			mEffectsProxy = effectsProxy;
			mEffectsProxy.initAttr(context,attrs);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
			mEffectsProxy.measuredSize(getMeasuredWidth(),getMeasuredHeight());
		}
		@Override protected void onDraw(Canvas canvas)
		{
			mEffectsProxy.getAdapter().runAnimator(this,canvas);
			super.onDraw(canvas);
		}
		@Override public void dispatchDraw(Canvas canvas)
		{
			super.dispatchDraw(canvas);
			if(Build.VERSION.SDK_INT < Build.VERSION_CODES.M){
				mEffectsProxy.getAdapter().dispatchDraw(this,canvas);
			}
		}
		@Override public void onDrawForeground(Canvas canvas)
		{
			super.onDrawForeground(canvas);
			mEffectsProxy.getAdapter().drawForeground(this,canvas);
		}
		@Override public boolean onTouchEvent(MotionEvent event)
		{
			if(mOnClickListener == null && mOnLongClickListener == null || !isEnabled()){
				return super.onTouchEvent(event);
			}
			return mEffectsProxy.getAdapter().onTouch(this,event,mOnClickListener,mOnLongClickListener);
		}
		public OnClickListener mOnClickListener;
		@Override public void setOnClickListener(@Nullable OnClickListener l) {
			mOnClickListener = l;
		}
		public OnLongClickListener mOnLongClickListener;
		@Override public void setOnLongClickListener(OnLongClickListener onLongClickListener)
		{
			mOnLongClickListener = onLongClickListener;
			if(mOnLongClickListener != null){
				mEffectsProxy.getAdapter().createLongClick(this,mOnLongClickListener);
			}
		}
	}
	public static class TouchEffectsRelativeLayout extends RelativeLayout {
		private BaseEffectsProxy mEffectsProxy;
		public TouchEffectsRelativeLayout(Context context) {
			this(context,null);
		}
		public TouchEffectsRelativeLayout(Context context, @Nullable AttributeSet attrs)
		{
			super(context, attrs);
		}
		public TouchEffectsRelativeLayout(Context context, @Nullable AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			super(context, attrs,0);
			setWillNotDraw(false);
			mEffectsProxy = effectsProxy;
			mEffectsProxy.initAttr(context,attrs);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			super.onMeasure(widthMeasureSpec, heightMeasureSpec);
			mEffectsProxy.measuredSize(getMeasuredWidth(),getMeasuredHeight());
		}
		@Override protected void onDraw(Canvas canvas)
		{
			mEffectsProxy.getAdapter().runAnimator(this,canvas);
			super.onDraw(canvas);
		}
		@Override protected void dispatchDraw(Canvas canvas)
		{
			super.dispatchDraw(canvas);
			if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M){
				mEffectsProxy.getAdapter().dispatchDraw(this,canvas);
			}
		}
		@Override public void onDrawForeground(Canvas canvas)
		{
			super.onDrawForeground(canvas);
			mEffectsProxy.getAdapter().drawForeground(this,canvas);
		}
		@Override public boolean onTouchEvent(MotionEvent event)
		{
			if(mOnClickListener == null && mOnLongClickListener == null || !isEnabled()){
				return super.onTouchEvent(event);
			}
			return mEffectsProxy.getAdapter().onTouch(this,event,mOnClickListener,mOnLongClickListener);
		}
		public OnClickListener mOnClickListener;
		@Override public void setOnClickListener(@Nullable OnClickListener l) {
			mOnClickListener = l;
		}
		public OnLongClickListener mOnLongClickListener;
		@Override public void setOnLongClickListener(OnLongClickListener onLongClickListener)
		{
			mOnLongClickListener = onLongClickListener;
			if(mOnLongClickListener != null){
				mEffectsProxy.getAdapter().createLongClick(this,mOnLongClickListener);
			}
		}
	}
	//
	//
	//
	public static interface TouchEffectsViewSubject {
		View createView(View parent, String name, Context context, AttributeSet attrs);
	}

	public static class ViewProxy implements TouchEffectsViewSubject {
		private TouchEffectsViewSubject mTouchEffectsViewSubject;
		public ViewProxy(TouchEffectsViewSubject touchEffectsViewSubject)
		{
			mTouchEffectsViewSubject = touchEffectsViewSubject;
		}
		@Override public View createView(View parent, String name, Context context, AttributeSet attrs)
		{
			return mTouchEffectsViewSubject.createView(parent,name,context,attrs);
		}
	}
	public static class TouchEffectsCreateViewSubject implements TouchEffectsViewSubject {
		private TouchEffectsWholeType mTouchEffectsWholeType;
		private ScaleBean mScaleBean;
		private ColorBean mColorBean;
		public TouchEffectsCreateViewSubject(TouchEffectsWholeType touchEffectsWholeType)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
		}
		public TouchEffectsCreateViewSubject(TouchEffectsWholeType touchEffectsWholeType, ScaleBean scaleBean)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
			mScaleBean = scaleBean;
		}
		public TouchEffectsCreateViewSubject(TouchEffectsWholeType touchEffectsWholeType, ColorBean colorBean)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
			mColorBean = colorBean;
		}
		@Override public View createView(View parent, String name, Context context, AttributeSet attrs)
		{
			View view = null;
			if(!name.contains(".") || name.startsWith("android")) {
				TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.TouchEffectsView);
				int type = ta.getInt(R.styleable.TouchEffectsView_touch_effects_type, Manager.NONE_TYPE);
				boolean is_forbid = ta.getBoolean(R.styleable.TouchEffectsView_touch_effects_forbid, false);
				if(!is_forbid) {
					switch(type) {
						case Manager.NONE_TYPE:
							if(Manager.getViewTypes().get(name) != null)
								view = parseTypeView(Manager.getViewTypes().get(name), parent, name, context, attrs);
							else
								view = parseTypeView(mTouchEffectsWholeType, parent, name, context, attrs);
							break;
						case Manager.RIPPLE_TYPE:
							view = parseTypeView(TouchEffectsWholeType.RIPPLE, parent, name, context, attrs);
							break;
						case Manager.STATE_TYPE:
							view = parseTypeView(TouchEffectsWholeType.STATE, parent, name, context, attrs);
							break;
						case Manager.SHAKE_TYPE:
							view = parseTypeView(TouchEffectsSingleType.SHAKE, parent, name, context, attrs);
							break;
						case Manager.RIPPLE_1_TYPE:
							view = parseTypeView(TouchEffectsWholeType.RIPPLE_1, parent, name, context, attrs);
							break;
						default:
							view = parseTypeView(TouchEffectsWholeType.SCALE, parent, name, context, attrs);
							break;
					}
				}
			}
			return view;
		}
		private View parseTypeView(TouchEffectsType type, View parent, String name, Context context, AttributeSet attrs)
		{
			View view = parseEffectsView(parent,type,name,context,attrs);
			return view;
		}
		/**
		 * 解析缩放模式下的View
		 * @param name
		 * @param context
		 * @param attrs
		 * @return
		 */
		private View parseEffectsView(View parent, TouchEffectsType wholeType,String name, Context context, AttributeSet attrs)
		{
			return findSystemViewChange(parent,name,wholeType,context,attrs);
		}
		/**
		 * 如果是支持的View，则转换成TouchShake系列的View
		 * @param name
		 * @param context
		 * @param attrs
		 * @return
		 */
		private View findSystemViewChange(View parent, String name, TouchEffectsType wholeType, Context context, AttributeSet attrs)
		{
			View view = null;
			EffectsAdapter adapter = null;
			if(Manager.getListWholeType() != null && parent != null && (parent instanceof ListView || parent.getClass().getName().contains("widget.RecyclerView"))){ //父控件为列表，使用定义的类型
				adapter = getAdapter(Manager.getListWholeType());
			}
			else if(wholeType != null){
				adapter = getAdapter(wholeType);
			}
			if(adapter != null) {
				BaseEffectsProxy baseEffectsProxy;
				if(isContainsExtraType(TouchEffectsExtraType.AspectRatio))
					baseEffectsProxy = new AspectRatioEffectsProxy(adapter);
				else
					baseEffectsProxy = new BaseEffectsProxy(adapter);
				view = checkViewName(name, context, attrs, baseEffectsProxy);
			}
			return view;
		}
		private View checkViewName(String name,Context context, AttributeSet attrs,BaseEffectsProxy effectsProxy)
		{
			if(name.startsWith("androidx.")){
				return checkExtendView(name,context,attrs,effectsProxy);
			}
			else {
				return checkView(name,context,attrs,effectsProxy);
			}
		}
		private View checkView(String name,Context context, AttributeSet attrs,BaseEffectsProxy effectsProxy)
		{
			View view = null;
			switch(name) {
				case "TextView":
					view = new TouchEffectsTextView(context, attrs,effectsProxy);
					break;
				case "Button":
					view = new TouchEffectsButton(context, attrs,effectsProxy);
					break;
				case "ImageView":
					view = new TouchEffectsImageView(context, attrs,effectsProxy);
					break;
				case "ImageButton":
					view = new TouchEffectsImageButton(context, attrs,effectsProxy);
					break;
				case "FrameLayout":
					view = new TouchEffectsFrameLayout(context, attrs,effectsProxy);
					break;
				case "LinearLayout":
					view = new TouchEffectsLinearLayout(context, attrs,effectsProxy);
					break;
				case "RelativeLayout":
					view = new TouchEffectsRelativeLayout(context, attrs,effectsProxy);
					break;
				case "android.support.constraint.ConstraintLayout":
					view = new TouchEffectsConstraintLayout(context,attrs,effectsProxy);
					break;
			}
			return view;
		}
		private View checkExtendView(String name,Context context, AttributeSet attrs,BaseEffectsProxy effectsProxy)
		{
			if(name.contains("TextView")){
				return new TouchEffectsTextView(context, attrs,effectsProxy);
			}
			if(name.contains("Button")){
				return new TouchEffectsButton(context, attrs,effectsProxy);
			}
			if(name.contains("ImageView")){
				return new TouchEffectsImageView(context, attrs,effectsProxy);
			}
			if(name.contains("ImageButton")){
				return new TouchEffectsImageButton(context, attrs,effectsProxy);
			}
			if(name.contains("FrameLayout")){
				return new TouchEffectsFrameLayout(context, attrs,effectsProxy);
			}
			if(name.contains("LinearLayout")){
				return new TouchEffectsLinearLayout(context, attrs,effectsProxy);
			}
			if(name.contains("RelativeLayout")){
				return new TouchEffectsRelativeLayout(context, attrs,effectsProxy);
			}
			if(name.contains("ConstraintLayout")){
				return new TouchEffectsConstraintLayout(context,attrs,effectsProxy);
			}
			return null;
		}
		private EffectsAdapter getAdapter(TouchEffectsType type)
		{
			if(type instanceof TouchEffectsSingleType){
				TouchEffectsSingleType singleType = (TouchEffectsSingleType) type;
				switch(singleType){
					case SHAKE: return new TouchShakeAdapter();
				}
			}
			else {
				TouchEffectsWholeType wholeType = (TouchEffectsWholeType) type;
				switch(wholeType){
					case SCALE: return new TouchScaleAdapter(mScaleBean);
					case RIPPLE: return new TouchRippleAdapter(Manager.getColorBean());
					case STATE: return new TouchStateAdapter(Manager.getColorBean());
					case RIPPLE_1: return new TouchRipple1Adapter(Manager.getColorBean());
				}
			}
			return null;
		}
	}
	/**
	 * 页面中单独设置Type的Subject，全局设置及列表设置无效
	 */
	public static class TouchEffectsCreateViewPageSubject implements TouchEffectsViewSubject {
		private TouchEffectsWholeType mTouchEffectsWholeType;
		private ScaleBean mScaleBean;
		private ColorBean mColorBean;
		private final String[] sClassPrefixList = { "android.widget.", "android.view.", "android.webkit." };
		public TouchEffectsCreateViewPageSubject(TouchEffectsWholeType touchEffectsWholeType)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
		}
		public TouchEffectsCreateViewPageSubject(TouchEffectsWholeType touchEffectsWholeType, ScaleBean scaleBean)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
			mScaleBean = scaleBean;
		}
		public TouchEffectsCreateViewPageSubject(TouchEffectsWholeType touchEffectsWholeType, ColorBean colorBean)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
			mColorBean = colorBean;
		}
		@Override public View createView(View parent, String name, Context context, AttributeSet attrs)
		{
			View view = null;
			if(name.contains(".") && !name.startsWith("android")) {//不处理自定义控件
				return view;
			}
			TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.TouchEffectsView);
			int type = ta.getInt(R.styleable.TouchEffectsView_touch_effects_type, Manager.NONE_TYPE);
			boolean isForbid = ta.getBoolean(R.styleable.TouchEffectsView_touch_effects_forbid,false);
			if(isForbid) {//如果设置了禁用,则优先级最高，不处理
				return view;
			}
			if(type != Manager.NONE_TYPE) {//只要在View中设置了type，除非是none，否则不论全局何种模式，都优先根据type
				if(type == Manager.RIPPLE_TYPE) {
					view = parseTypeView(TouchEffectsWholeType.RIPPLE,parent,name, context, attrs);
				}
				else if (type == Manager.RIPPLE_1_TYPE) {
					view = parseTypeView(TouchEffectsWholeType.RIPPLE_1,parent,name, context, attrs);
				}
				else if (type == Manager.STATE_TYPE) {
					view = parseTypeView(TouchEffectsWholeType.STATE,parent,name, context, attrs);
				}
				else if (type == Manager.SHAKE_TYPE) {
					view = parseTypeView(TouchEffectsSingleType.SHAKE,parent,name, context, attrs);
				}
				else {
					view = parseTypeView(TouchEffectsWholeType.SCALE,parent,name, context, attrs);
				}
			}
			else{
				view = parseTypeView(mTouchEffectsWholeType, parent,name,context,attrs);
			}
			return view;
		}
		private View parseTypeView(TouchEffectsType type, View parent, String name, Context context, AttributeSet attrs)
		{
			View view = parseEffectsView(parent,type,name,context,attrs);
			return view;
		}
		/**
		 * 解析缩放模式下的View
		 * @param name
		 * @param context
		 * @param attrs
		 * @return
		 */
		private View parseEffectsView(View parent, TouchEffectsType wholeType, String name, Context context, AttributeSet attrs)
		{
			return findSystemViewChange(name, wholeType, context,attrs);
		}
		/**
		 * 如果是支持的View，则转换成TouchShake系列的View
		 * @param name
		 * @param context
		 * @param attrs
		 * @return
		 */
		private View findSystemViewChange(String name, TouchEffectsType wholeType, Context context, AttributeSet attrs)
		{
			View view = null;
			EffectsAdapter adapter = null;
			if(wholeType != null) {
				adapter = getAdapter(wholeType);
			}
			if(adapter == null){
				return null;
			}
			BaseEffectsProxy baseEffectsProxy;
			if(isContainsExtraType(TouchEffectsExtraType.AspectRatio)){
				baseEffectsProxy = new AspectRatioEffectsProxy(adapter);
			}
			else {
				baseEffectsProxy = new BaseEffectsProxy(adapter);
			}
			view = checkViewName(name,context,attrs,baseEffectsProxy);
			return view;
		}
		private View checkViewName(String name, Context context, AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			if(name.startsWith("androidx.")){
				return checkExtendView(name,context,attrs,effectsProxy);
			}
			else {
				return checkView(name,context,attrs,effectsProxy);
			}
		}
		private View checkView(String name,Context context, AttributeSet attrs,BaseEffectsProxy effectsProxy)
		{
			View view = null;
			switch(name) {
				case "TextView": view = new TouchEffectsTextView(context, attrs,effectsProxy); break;
				case "Button": view = new TouchEffectsButton(context, attrs,effectsProxy); break;
				case "ImageView": view = new TouchEffectsImageView(context, attrs,effectsProxy); break;
				case "ImageButton": view = new TouchEffectsImageButton(context, attrs,effectsProxy); break;
				case "FrameLayout": view = new TouchEffectsFrameLayout(context, attrs,effectsProxy); break;
				case "LinearLayout": view = new TouchEffectsLinearLayout(context, attrs,effectsProxy); break;
				case "RelativeLayout": view = new TouchEffectsRelativeLayout(context, attrs,effectsProxy); break;
				case "android.support.constraint.ConstraintLayout": view = new TouchEffectsConstraintLayout(context,attrs,effectsProxy); break;
			}
			return view;
		}

		private View checkExtendView(String name,Context context, AttributeSet attrs, BaseEffectsProxy effectsProxy)
		{
			if(name.contains("TextView")){
				return new TouchEffectsTextView(context, attrs,effectsProxy);
			}
			if(name.contains("Button")){
				return new TouchEffectsButton(context, attrs,effectsProxy);
			}
			if(name.contains("ImageView")){
				return new TouchEffectsImageView(context, attrs,effectsProxy);
			}
			if(name.contains("ImageButton")){
				return new TouchEffectsImageButton(context, attrs,effectsProxy);
			}
			if(name.contains("FrameLayout")){
				return new TouchEffectsFrameLayout(context, attrs,effectsProxy);
			}
			if(name.contains("LinearLayout")){
				return new TouchEffectsLinearLayout(context, attrs,effectsProxy);
			}
			if(name.contains("RelativeLayout")){
				return new TouchEffectsRelativeLayout(context, attrs,effectsProxy);
			}
			if(name.contains("ConstraintLayout")){
				return new TouchEffectsConstraintLayout(context,attrs,effectsProxy);
			}
			return null;
		}
		private EffectsAdapter getAdapter(TouchEffectsType type)
		{
			if(type instanceof TouchEffectsSingleType){
				TouchEffectsSingleType singleType = (TouchEffectsSingleType)type;
				switch(singleType){
					case SHAKE: return new TouchShakeAdapter();
				}
			}
			else {
				TouchEffectsWholeType wholeType = (TouchEffectsWholeType) type;
				switch(wholeType){
					case SCALE: return new TouchScaleAdapter(mScaleBean);
					case RIPPLE: return new TouchRippleAdapter(Manager.getColorBean());
					case STATE: return new TouchStateAdapter(Manager.getColorBean());
					case RIPPLE_1: return new TouchRipple1Adapter(Manager.getColorBean());
				}
			}
			return null;
		}
	}
	//
	//
	//
	public static class InflaterFactory implements LayoutInflater.Factory2 {
		private TouchEffectsWholeType mTouchEffectsWholeType;
		private ViewProxy mTouchEffectsViewProxy;
		public InflaterFactory()
		{
		}
		public InflaterFactory(TouchEffectsWholeType touchEffectsWholeType)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
			mTouchEffectsViewProxy = new ViewProxy(new TouchEffectsCreateViewPageSubject(mTouchEffectsWholeType));
		}
		@Override public View onCreateView(View parent, String name, Context context, AttributeSet attrs)
		{
			if(mTouchEffectsViewProxy != null){
				return mTouchEffectsViewProxy.createView(parent,name,context,attrs);
			}
			return Manager.getViewSubject().createView(parent,name,context,attrs);
		}
		@Override public View onCreateView(String name, Context context, AttributeSet attrs)
		{
			if(mTouchEffectsViewProxy != null){
				return mTouchEffectsViewProxy.createView(null,name,context,attrs);
			}
			return Manager.getViewSubject().createView(null,name,context,attrs);
		}
	}

	public static class Factory {
		public static void initTouchEffects(@NonNull Activity activity)
		{
			LayoutInflaterCompat.setFactory2(activity.getLayoutInflater(),new InflaterFactory());
		}
		public static void initTouchEffects(@NonNull Activity activity, TouchEffectsWholeType touchEffectsWholeType)
		{
			LayoutInflaterCompat.setFactory2(activity.getLayoutInflater(),new InflaterFactory(touchEffectsWholeType));
		}
	}
	//
	//
	//
	public static class Manager {
		public static final int NONE_TYPE = -1;
		public static final int SCALE_TYPE = 0;
		public static final int RIPPLE_TYPE = 1;
		public static final int STATE_TYPE = 2;
		public static final int SHAKE_TYPE = 3;
		public static final int RIPPLE_1_TYPE = 4;

		private static volatile Manager mInstance;
		private static TouchEffectsWholeType mTouchEffectsWholeType;
		private static HashMap<String, TouchEffectsWholeType> mViewTypes;
		private static ViewProxy mViewSubject;
		private static ColorBean mColorBean;
		private static ScaleBean mScaleBean;
		private static TouchEffectsWholeType mListWholeType;
		private static HashMap<TouchEffectsExtraType, BaseExtraBean> mExtraTypeMap = new HashMap<>();

		private Manager()
		{
			this.mViewTypes = new HashMap<>();
			mViewSubject = new ViewProxy(new TouchEffectsCreateViewSubject(mTouchEffectsWholeType,mColorBean));
		}
		private static Manager getInstance()
		{
			if(mInstance == null){
				synchronized (Manager.class){
					if(mInstance == null){
						mInstance = new Manager();
					}
				}
			}
			return mInstance;
		}
		/**
		 * 创建Manager，初始化
		 * @param touchEffectsWholeType 全局使用什么模式{@link TouchEffectsWholeType}
		 * @return
		 */
		public static Manager build(@NonNull TouchEffectsWholeType touchEffectsWholeType)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
			if(mColorBean == null){
				mColorBean = new ColorBean(0x3D000000,0x3D000000);
			}
			return getInstance();
		}
		/**
		 * 创建Manager，初始化
		 * @param touchEffectsWholeType 全局使用什么模式{@link TouchEffectsWholeType}
		 * @return
		 */
		public static Manager build(@NonNull TouchEffectsWholeType touchEffectsWholeType, int normalColor, int pressedColor)
		{
			mTouchEffectsWholeType = touchEffectsWholeType;
			if(mColorBean == null){
				mColorBean = new ColorBean(normalColor,pressedColor);
			}
			else{
				mColorBean.setNormalColor(normalColor);
				mColorBean.setPressedColor(pressedColor);
			}
			return getInstance();
		}
		/**
		 * 添加支持的视图的类型
		 * @param viewTypes 支持的类型 {@link ViewType}
		 * @return
		 */
		public Manager addViewTypes(@ViewType String... viewTypes)
		{
			addViewTypes(mTouchEffectsWholeType, viewTypes);
			return mInstance;
		}
		/**
		 * 添加支持的视图的类型
		 * @param wholeType 支持的类型中适用于什么模式（如TextView使用EFFECTS,Button使用RIPPLE）{@link TouchEffectsWholeType}
		 * @param viewTypes 支持的类型 {@link ViewType}
		 * @return
		 */
		public Manager addViewTypes(TouchEffectsWholeType wholeType, @ViewType String... viewTypes){
			for(String viewType:viewTypes){
				addViewType(wholeType,viewType);
			}
			return mInstance;
		}
		/**
		 * 添加支持的视图的类型
		 * @param viewType 支持的类型 {@link ViewType}
		 * @return
		 */
		public Manager addViewType(@ViewType String viewType)
		{
			addViewType(mTouchEffectsWholeType,viewType);
			return mInstance;
		}
		/**
		 * 添加支持的视图的类型
		 * @param wholeType 支持的类型中适用于什么模式（如TextView使用EFFECTS,Button使用RIPPLE）{@link TouchEffectsWholeType}
		 * @param viewType 支持的类型 {@link ViewType}
		 * @return
		 */
		public Manager addViewType(TouchEffectsWholeType wholeType, @ViewType String viewType)
		{
			if(viewType.equals(ViewType.ALL)){
				mViewTypes.clear();
				for(String viewType1:getAllViewTypes()) {
					mViewTypes.put(viewType1,wholeType);
				}
			}else{
				mViewTypes.put(viewType,wholeType);
			}
			return mInstance;
		}
		public Manager setListWholeType(TouchEffectsWholeType listWholeType)
		{
			mListWholeType = listWholeType;
			return mInstance;
		}
		/**
		 * 设置View在某种模式中，宽高比达到什么程度时，使用另外的模式
		 * @param width 宽度，width < 1f : 使用比例  width > 1 : 不使用比例，当大于这个值的时候触发，与height同时生效
		 * @param height 宽度，height < 1f : 使用比例  height > 1 : 不使用比例，当大于这个值的时候触发，与width同时生效
		 * @param wholeType 更换的模式
		 *                  ！！！注意，优先级大于listType
		 * @return
		 */
		public Manager setAspectRatioType(float width, float height, TouchEffectsWholeType wholeType)
		{
			ExtraAspectRatioBean extraAspectRatioBean = new ExtraAspectRatioBean(wholeType, width, height);
			mExtraTypeMap.put(TouchEffectsExtraType.AspectRatio,extraAspectRatioBean);
			return mInstance;
		}
		public Manager setAspectRatioType(float aspectRatio, TouchEffectsWholeType wholeType)
		{
			if(aspectRatio > 10 || aspectRatio <= 0){
				throw new IllegalArgumentException("宽高比不能大于10或小于等于0");
			}
			float height = 1f;
			float width = aspectRatio;
			if(aspectRatio > 1f){
				width = aspectRatio / 10f;
				height = 1f / 10f;
			}
			ExtraAspectRatioBean extraAspectRatioBean = new ExtraAspectRatioBean(wholeType,width,height);
			mExtraTypeMap.put(TouchEffectsExtraType.AspectRatio,extraAspectRatioBean);
			return mInstance;
		}
		public static HashMap<TouchEffectsExtraType, BaseExtraBean> getExtraTypeMap() { return mExtraTypeMap; }
		public static TouchEffectsWholeType getListWholeType() { return mListWholeType; }
		public static HashMap<String, TouchEffectsWholeType> getViewTypes()
		{
			if(mViewTypes == null){
				throw new RuntimeException("please initialize in Application");
			}
			return mViewTypes;
		}
		public static ViewProxy getViewSubject() { return mViewSubject; }
		public static ColorBean getColorBean() { return mColorBean; }
		public static ScaleBean getScaleBean() { return mScaleBean; }
	}
}
