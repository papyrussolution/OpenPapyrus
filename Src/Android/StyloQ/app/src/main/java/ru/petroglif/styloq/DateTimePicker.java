// DateTimePicker.java
//
package ru.petroglif.styloq;

import android.animation.AnimatorSet;
import android.animation.Keyframe;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.app.ActionBar;
import android.app.Activity;
import android.app.DatePickerDialog;
import android.app.DialogFragment;
import android.app.Service;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.StateListDrawable;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.os.Vibrator;
import android.provider.Settings;
import android.text.TextUtils;
import android.text.format.DateFormat;
import android.text.format.DateUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.widget.AbsListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TabHost;
import android.widget.TextView;
import android.widget.ViewAnimator;
import androidx.annotation.NonNull;
import androidx.collection.SimpleArrayMap;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.customview.widget.ExploreByTouchHelper;
import java.security.InvalidParameterException;
import java.text.DateFormatSymbols;
import java.text.SimpleDateFormat;
import java.time.Month;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Formatter;
import java.util.GregorianCalendar;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;

public class DateTimePicker {
	//
	// Fake Button class, used so TextViews can announce themselves as Buttons, for accessibility.
	//
	public static class AccessibleLinearLayout extends LinearLayout {
		public AccessibleLinearLayout(Context context, AttributeSet attrs) {
			super(context, attrs);
		}
		@Override public void onInitializeAccessibilityEvent(AccessibilityEvent event)
		{
			super.onInitializeAccessibilityEvent(event);
			event.setClassName(Button.class.getName());
		}
		@Override public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info)
		{
			super.onInitializeAccessibilityNodeInfo(info);
			info.setClassName(Button.class.getName());
		}
	}
	//
	// Fake Button class, used so TextViews can announce themselves as Buttons, for accessibility.
	//
	public static class AccessibleTextView extends TextView {
		public AccessibleTextView(Context context, AttributeSet attrs) {
			super(context, attrs);
		}
		@Override public void onInitializeAccessibilityEvent(AccessibilityEvent event)
		{
			super.onInitializeAccessibilityEvent(event);
			event.setClassName(Button.class.getName());
		}
		@Override public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info)
		{
			super.onInitializeAccessibilityNodeInfo(info);
			info.setClassName(Button.class.getName());
		}
	}
	//
	// A simple utility class to handle haptic feedback.
	//
	public static class HapticFeedbackController {
		private static final int VIBRATE_DELAY_MS = 125;
		private static final int VIBRATE_LENGTH_MS = 50;

		private static boolean checkGlobalSetting(Context context)
		{
			return Settings.System.getInt(context.getContentResolver(), Settings.System.HAPTIC_FEEDBACK_ENABLED, 0) == 1;
		}
		private final Context mContext;
		private final ContentObserver mContentObserver;

		private Vibrator mVibrator;
		private boolean mIsGloballyEnabled;
		private long mLastVibrate;

		public HapticFeedbackController(Context context)
		{
			mContext = context;
			mContentObserver = new ContentObserver(null) {
				@Override public void onChange(boolean selfChange)
				{
					mIsGloballyEnabled = checkGlobalSetting(mContext);
				}
			};
		}
		//
		// Call to setup the controller.
		//
		public void start()
		{
			if(hasVibratePermission(mContext)) {
				mVibrator = (Vibrator) mContext.getSystemService(Service.VIBRATOR_SERVICE);
			}
			// Setup a listener for changes in haptic feedback settings
			mIsGloballyEnabled = checkGlobalSetting(mContext);
			Uri uri = Settings.System.getUriFor(Settings.System.HAPTIC_FEEDBACK_ENABLED);
			mContext.getContentResolver().registerContentObserver(uri, false, mContentObserver);
		}
		//
		// Method to verify that vibrate permission has been granted.
		//
		// Allows users of the library to disabled vibrate support if desired.
		// @return true if Vibrate permission has been granted
		//
		private boolean hasVibratePermission(Context context)
		{
			PackageManager pm = context.getPackageManager();
			int hasPerm = pm.checkPermission(android.Manifest.permission.VIBRATE, context.getPackageName());
			return hasPerm == PackageManager.PERMISSION_GRANTED;
		}
		//
		// Call this when you don't need the controller anymore.
		//
		public void stop()
		{
			mVibrator = null;
			mContext.getContentResolver().unregisterContentObserver(mContentObserver);
		}
		//
		// Try to vibrate. To prevent this becoming a single continuous vibration, nothing will
		// happen if we have vibrated very recently.
		//
		public void tryVibrate()
		{
			if(mVibrator != null && mIsGloballyEnabled) {
				long now = SystemClock.uptimeMillis();
				// We want to try to vibrate each individual tick discretely.
				if((now - mLastVibrate) >= VIBRATE_DELAY_MS) {
					mVibrator.vibrate(VIBRATE_LENGTH_MS);
					mLastVibrate = now;
				}
			}
		}
	}
	//
	// Each call to Typeface.createFromAsset will load a new instance of the typeface into memory,
	// and this memory is not consistently get garbage collected
	// http://code.google.com/p/android/issues/detail?id=9904
	// (It states released but even on Lollipop you can see the typefaces accumulate even after
	// multiple GC passes)
	// You can detect this by running:
	// adb shell dumpsys meminfo com.your.packagenage
	// You will see output like:
	//   Asset Allocations
	//      zip:/data/app/com.your.packagenage-1.apk:/assets/Roboto-Medium.ttf: 125K
	//      zip:/data/app/com.your.packagenage-1.apk:/assets/Roboto-Medium.ttf: 125K
	//      zip:/data/app/com.your.packagenage-1.apk:/assets/Roboto-Medium.ttf: 125K
	//      zip:/data/app/com.your.packagenage-1.apk:/assets/Roboto-Regular.ttf: 123K
	//      zip:/data/app/com.your.packagenage-1.apk:/assets/Roboto-Medium.ttf: 125K
	//
	public static class TypefaceHelper {
		private static final SimpleArrayMap<String, Typeface> cache = new SimpleArrayMap<>();
		public static Typeface get(Context c, String name)
		{
			synchronized(cache) {
				if(!cache.containsKey(name)) {
					Typeface t = Typeface.createFromAsset(c.getAssets(), String.format("fonts/%s.ttf", name));
					cache.put(name, t);
					return t;
				}
				return cache.get(name);
			}
		}
	}
	//
	// Utility helper functions for time and date pickers.
	//
	public static class Utils {
		//public static final int MONDAY_BEFORE_JULIAN_EPOCH = Time.EPOCH_JULIAN_DAY - 3;
		public static final int PULSE_ANIMATOR_DURATION = 544;
		// Alpha level for time picker selection.
		public static final int SELECTED_ALPHA = 255;
		public static final int SELECTED_ALPHA_THEME_DARK = 255;
		// Alpha level for fully opaque.
		public static final int FULL_ALPHA = 255;
		public static boolean isJellybeanOrLater() { return Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN; }
		//
		// Try to speak the specified text, for accessibility. Only available on JB or later.
		// @param text Text to announce.
		//
		@SuppressLint("NewApi") public static void tryAccessibilityAnnounce(View view, CharSequence text)
		{
			if(isJellybeanOrLater() && view != null && text != null) {
				view.announceForAccessibility(text);
			}
		}
		/**
		 * Takes a number of weeks since the epoch and calculates the Julian day of
		 * the Monday for that week.
		 *
		 * This assumes that the week containing the {@link Time#EPOCH_JULIAN_DAY}
		 * is considered week 0. It returns the Julian day for the Monday
		 * {@code week} weeks after the Monday of the week containing the epoch.
		 *
		 * @param week Number of weeks since the epoch
		 * @return The julian day for the Monday of the given week since the epoch
		 */
		// public static int getJulianMondayFromWeeksSinceEpoch(int week) { return MONDAY_BEFORE_JULIAN_EPOCH + week * 7; }
		/**
		 * Returns the week since {@link Time#EPOCH_JULIAN_DAY} (Jan 1, 1970)
		 * adjusted for first day of week.
		 *
		 * This takes a julian day and the week start day and calculates which
		 * week since {@link Time#EPOCH_JULIAN_DAY} that day occurs in, starting
		 * at 0. *Do not* use this to compute the ISO week number for the year.
		 *
		 * @param julianDay The julian day to calculate the week number for
		 * @param firstDayOfWeek Which week day is the first day of the week,
		 *          see {@link Time#SUNDAY}
		 * @return Weeks since the epoch
		 */
		/**
		 public static int getWeeksSinceEpochFromJulianDay(int julianDay, int firstDayOfWeek) {
		 int diff = Time.THURSDAY - firstDayOfWeek;
		 if(diff < 0) {
		 diff += 7;
		 }
		 int refDay = Time.EPOCH_JULIAN_DAY - diff;
		 return (julianDay - refDay) / 7;
		 }
		 */
		//
		// Render an animator to pulsate a view in place.
		// @param labelToAnimate the view to pulsate.
		// @return The animator object. Use .start() to begin.
		//
		public static ObjectAnimator getPulseAnimator(View labelToAnimate, float decreaseRatio, float increaseRatio)
		{
			Keyframe k0 = Keyframe.ofFloat(0f, 1f);
			Keyframe k1 = Keyframe.ofFloat(0.275f, decreaseRatio);
			Keyframe k2 = Keyframe.ofFloat(0.69f, increaseRatio);
			Keyframe k3 = Keyframe.ofFloat(1f, 1f);
			PropertyValuesHolder scaleX = PropertyValuesHolder.ofKeyframe("scaleX", k0, k1, k2, k3);
			PropertyValuesHolder scaleY = PropertyValuesHolder.ofKeyframe("scaleY", k0, k1, k2, k3);
			ObjectAnimator pulseAnimator = ObjectAnimator.ofPropertyValuesHolder(labelToAnimate, scaleX, scaleY);
			pulseAnimator.setDuration(PULSE_ANIMATOR_DURATION);
			return pulseAnimator;
		}
		//
		// Convert Dp to Pixel
		//
		@SuppressWarnings("unused") public static int dpToPx(float dp, Resources resources)
		{
			float px = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, resources.getDisplayMetrics());
			return (int) px;
		}
		public static int darkenColor(int color)
		{
			float[] hsv = new float[3];
			Color.colorToHSV(color, hsv);
			hsv[2] = hsv[2] * 0.8f; // value component
			return Color.HSVToColor(hsv);
		}
		//
		// Gets the colorAccent from the current context, if possible/available
		// @param context
		// @return -1 if accent color invalid, otherwise the accent color of the current context
		//
		public static int getAccentColorFromThemeIfAvailable(Context context)
		{
			TypedValue typedValue = new TypedValue();
			//First, try the android:colorAccent
			if(Build.VERSION.SDK_INT >= 21) {
				context.getTheme().resolveAttribute(android.R.attr.colorAccent, typedValue, true);
				return typedValue.data;
			}
			//Next, try colorAccent from support lib
			int colorAccentResId = context.getResources().getIdentifier("colorAccent", "attr", context.getPackageName());
			if(colorAccentResId == 0) {
				return -1;
			}
			if(!context.getTheme().resolveAttribute(colorAccentResId, typedValue, true)) {
				return -1;
			}
			return typedValue.data;
		}
	}
	//
	// Controller class to communicate among the various components of the date picker dialog.
	//
	public interface DatePickerController {
		void onYearSelected(int year);
		void onDayOfMonthSelected(int year, int month, int day);
		void registerOnDateChangedListener(DatePickerDialog.OnDateChangedListener listener);
		void unregisterOnDateChangedListener(DatePickerDialog.OnDateChangedListener listener);
		MonthAdapter.CalendarDay getSelectedDay();
		boolean isThemeDark();
		Calendar[] getHighlightedDays();
		Calendar[] getSelectableDays();
		int getFirstDayOfWeek();
		int getMinYear();
		int getMaxYear();
		Calendar getMinDate();
		Calendar getMaxDate();
		void tryVibrate();
	}
	//
	// DATE
	//
	public static class AccessibleDateAnimator extends ViewAnimator {
		private long mDateMillis;
		public AccessibleDateAnimator(Context context, AttributeSet attrs)
		{
			super(context, attrs);
		}
		public void setDateMillis(long dateMillis)
		{
			mDateMillis = dateMillis;
		}
		//
		// Announce the currently-selected date when launched.
		//
		@Override public boolean dispatchPopulateAccessibilityEvent(AccessibilityEvent event)
		{
			if(event.getEventType() == AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED) {
				// Clear the event's current text so that only the current date will be spoken.
				event.getText().clear();
				int flags = DateUtils.FORMAT_SHOW_DATE | DateUtils.FORMAT_SHOW_YEAR|DateUtils.FORMAT_SHOW_WEEKDAY;
				String dateString = DateUtils.formatDateTime(getContext(), mDateMillis, flags);
				event.getText().add(dateString);
				return true;
			}
			return super.dispatchPopulateAccessibilityEvent(event);
		}
	}
	//
	//
	//
	public static class SimpleMonthView extends MonthView {
		public SimpleMonthView(Context context, AttributeSet attr, DatePickerController controller)
		{
			super(context, attr, controller);
		}
		@Override public void drawMonthDay(Canvas canvas, int year, int month, int day, int x, int y, int startX, int stopX, int startY, int stopY)
		{
			if(mSelectedDay == day) {
				canvas.drawCircle(x , y - (MINI_DAY_NUMBER_TEXT_SIZE / 3), DAY_SELECTED_CIRCLE_SIZE, mSelectedCirclePaint);
			}
			if(isHighlighted(year, month, day)) {
				mMonthNumPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
			}
			else {
				mMonthNumPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.NORMAL));
			}
			// If we have a mindate or maxdate, gray out the day number if it's outside the range.
			if(isOutOfRange(year, month, day)) {
				mMonthNumPaint.setColor(mDisabledDayTextColor);
			}
			else if(mSelectedDay == day) {
				mMonthNumPaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
				mMonthNumPaint.setColor(mSelectedDayTextColor);
			}
			else if(mHasToday && mToday == day) {
				mMonthNumPaint.setColor(mTodayNumberColor);
			}
			else {
				mMonthNumPaint.setColor(isHighlighted(year, month, day) ? mHighlightedDayTextColor : mDayTextColor);
			}
			canvas.drawText(String.format("%d", day), x, y, mMonthNumPaint);
		}
	}
	//
	// An adapter for a list of {@link SimpleMonthView} items.
	//
	public static class SimpleMonthAdapter extends MonthAdapter {
		public SimpleMonthAdapter(Context context, DatePickerController controller)
		{
			super(context, controller);
		}
		@Override public MonthView createMonthView(Context context)
		{
			final MonthView monthView = new SimpleMonthView(context, null, mController);
			return monthView;
		}
	}
	//
	// A DayPickerView customized for {@link SimpleMonthAdapter}
	//
	public static class SimpleDayPickerView extends DayPickerView {
		public SimpleDayPickerView(Context context, AttributeSet attrs) {
			super(context, attrs);
		}
		public SimpleDayPickerView(Context context, DatePickerController controller)
		{
			super(context, controller);
		}
		@Override public MonthAdapter createMonthAdapter(Context context, DatePickerController controller)
		{
			return new SimpleMonthAdapter(context, controller);
		}
	}
	//
	// A calendar-like view displaying a specified month and the appropriate selectable day numbers
	// within the specified month.
	//
	public static abstract class MonthView extends View {
		private static final String TAG = "MonthView";
		//
		// These params can be passed into the view to control how it appears.
		// {@link #VIEW_PARAMS_WEEK} is the only required field, though the default
		// values are unlikely to fit most layouts correctly.
		//
		public static final String VIEW_PARAMS_HEIGHT = "height"; // This sets the height of this week in pixels
		public static final String VIEW_PARAMS_MONTH = "month"; // This specifies the position (or weeks since the epoch) of this week.
		public static final String VIEW_PARAMS_YEAR = "year"; // This specifies the position (or weeks since the epoch) of this week.
		//
		// This sets one of the days in this view as selected {@link Calendar#SUNDAY} through {@link Calendar#SATURDAY}.
		//
		public static final String VIEW_PARAMS_SELECTED_DAY = "selected_day";
		//
		// Which day the week should start on. {@link Calendar#SUNDAY} through {@link Calendar#SATURDAY}.
		//
		public static final String VIEW_PARAMS_WEEK_START = "week_start";
		//
		// How many days to display at a time. Days will be displayed starting with {@link #mWeekStart}.
		//
		public static final String VIEW_PARAMS_NUM_DAYS = "num_days";
		public static final String VIEW_PARAMS_FOCUS_MONTH = "focus_month"; // Which month is currently in focus, as defined by {@link Calendar#MONTH} [0-11].
		public static final String VIEW_PARAMS_SHOW_WK_NUM = "show_wk_num"; // If this month should display week numbers. false if 0, true otherwise.
		protected static int DEFAULT_HEIGHT = 32;
		protected static int MIN_HEIGHT = 10;
		protected static final int DEFAULT_SELECTED_DAY = -1;
		protected static final int DEFAULT_WEEK_START = Calendar.SUNDAY;
		protected static final int DEFAULT_NUM_DAYS = 7;
		protected static final int DEFAULT_SHOW_WK_NUM = 0;
		protected static final int DEFAULT_FOCUS_MONTH = -1;
		protected static final int DEFAULT_NUM_ROWS = 6;
		protected static final int MAX_NUM_ROWS = 6;
		private static final int SELECTED_CIRCLE_ALPHA = 255;
		protected static int DAY_SEPARATOR_WIDTH = 1;
		protected static int MINI_DAY_NUMBER_TEXT_SIZE;
		protected static int MONTH_LABEL_TEXT_SIZE;
		protected static int MONTH_DAY_LABEL_TEXT_SIZE;
		protected static int MONTH_HEADER_SIZE;
		protected static int DAY_SELECTED_CIRCLE_SIZE;
		protected static float mScale = 0; // used for scaling to the device density
		protected DatePickerController mController;
		protected int mEdgePadding = 0; // affects the padding on the sides of this view
		private String mDayOfWeekTypeface;
		private String mMonthTitleTypeface;
		protected Paint mMonthNumPaint;
		protected Paint mMonthTitlePaint;
		protected Paint mSelectedCirclePaint;
		protected Paint mMonthDayLabelPaint;
		private final Formatter mFormatter;
		private final StringBuilder mStringBuilder;
		protected int mFirstJulianDay = -1; // The Julian day of the first day displayed by this item
		protected int mFirstMonth = -1; // The month of the first day in this week
		protected int mLastMonth = -1; // The month of the last day in this week
		protected int mMonth;
		protected int mYear;
		protected int mWidth; // Quick reference to the width of this view, matches parent
		protected int mRowHeight = DEFAULT_HEIGHT; // The height this view should draw at in pixels, set by height param
		protected boolean mHasToday = false; // If this view contains the today
		protected int mSelectedDay = -1; // Which day is selected [0-6] or -1 if no day is selected
		protected int mToday = DEFAULT_SELECTED_DAY; // Which day is today [0-6] or -1 if no day is today
		protected int mWeekStart = DEFAULT_WEEK_START; // Which day of the week to start on [0-6]
		protected int mNumDays = DEFAULT_NUM_DAYS; // How many days to display
		protected int mNumCells = mNumDays; // The number of days + a spot for week number if it is displayed
		protected int mSelectedLeft = -1; // The left edge of the selected day
		protected int mSelectedRight = -1; // The right edge of the selected day
		private final Calendar mCalendar;
		protected final Calendar mDayLabelCalendar;
		private final MonthViewTouchHelper mTouchHelper;
		protected int mNumRows = DEFAULT_NUM_ROWS;
		protected OnDayClickListener mOnDayClickListener; // Optional listener for handling day click actions
		private boolean mLockAccessibilityDelegate; // Whether to prevent setting the accessibility delegate
		protected int mDayTextColor;
		protected int mSelectedDayTextColor;
		protected int mMonthDayTextColor;
		protected int mTodayNumberColor;
		protected int mHighlightedDayTextColor;
		protected int mDisabledDayTextColor;
		protected int mMonthTitleColor;
		public MonthView(Context context)
		{
			this(context, null, null);
		}
		public MonthView(Context context, AttributeSet attr, DatePickerController controller)
		{
			super(context, attr);
			mController = controller;
			Resources res = context.getResources();
			mDayLabelCalendar = Calendar.getInstance();
			mCalendar = Calendar.getInstance();
			mDayOfWeekTypeface = "sans-serif";//res.getString(R.string.range_day_of_week_label_typeface);
			mMonthTitleTypeface = "sans-serif";//res.getString(R.string.range_sans_serif);
			boolean darkTheme = mController != null && mController.isThemeDark();
			if(darkTheme) {
				mDayTextColor = res.getColor(R.color.range_date_picker_text_normal_dark_theme);
				mMonthDayTextColor = res.getColor(R.color.range_date_picker_month_day_dark_theme);
				mDisabledDayTextColor = res.getColor(R.color.range_date_picker_text_disabled_dark_theme);
				mHighlightedDayTextColor = res.getColor(R.color.range_date_picker_text_highlighted_dark_theme);
			}
			else {
				mDayTextColor = res.getColor(R.color.range_date_picker_text_normal);
				mMonthDayTextColor = res.getColor(R.color.range_date_picker_month_day);
				mDisabledDayTextColor = res.getColor(R.color.range_date_picker_text_disabled);
				mHighlightedDayTextColor = res.getColor(R.color.range_date_picker_text_highlighted);
			}
			mSelectedDayTextColor = res.getColor(R.color.White);
			mTodayNumberColor = res.getColor(R.color.range_accent_color);
			mMonthTitleColor = res.getColor(R.color.White);
			mStringBuilder = new StringBuilder(50);
			mFormatter = new Formatter(mStringBuilder, Locale.getDefault());
			MINI_DAY_NUMBER_TEXT_SIZE = res.getDimensionPixelSize(R.dimen.range_day_number_size);
			MONTH_LABEL_TEXT_SIZE = res.getDimensionPixelSize(R.dimen.range_month_label_size);
			MONTH_DAY_LABEL_TEXT_SIZE = res.getDimensionPixelSize(R.dimen.range_month_day_label_text_size);
			MONTH_HEADER_SIZE = res.getDimensionPixelOffset(R.dimen.range_month_list_item_header_height);
			DAY_SELECTED_CIRCLE_SIZE = res.getDimensionPixelSize(R.dimen.range_day_number_select_circle_radius);
			mRowHeight = (res.getDimensionPixelOffset(R.dimen.range_date_picker_view_animator_height) - getMonthHeaderSize()) / MAX_NUM_ROWS;
			// Set up accessibility components.
			mTouchHelper = getMonthViewTouchHelper();
			ViewCompat.setAccessibilityDelegate(this, mTouchHelper);
			ViewCompat.setImportantForAccessibility(this, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
			mLockAccessibilityDelegate = true;
			// Sets up any standard paints that will be used
			initView();
		}
		public void setDatePickerController(DatePickerController controller)
		{
			mController = controller;
		}
		protected MonthViewTouchHelper getMonthViewTouchHelper() { return new MonthViewTouchHelper(this); }
		@Override public void setAccessibilityDelegate(AccessibilityDelegate delegate)
		{
			// Workaround for a JB MR1 issue where accessibility delegates on
			// top-level ListView items are overwritten.
			if(!mLockAccessibilityDelegate) {
				super.setAccessibilityDelegate(delegate);
			}
		}
		public void setOnDayClickListener(OnDayClickListener listener)
		{
			mOnDayClickListener = listener;
		}
		@Override public boolean dispatchHoverEvent(@NonNull MotionEvent event)
		{
			// First right-of-refusal goes the touch exploration helper.
			if(mTouchHelper.dispatchHoverEvent(event)) {
				return true;
			}
			return super.dispatchHoverEvent(event);
		}
		@Override public boolean onTouchEvent(@NonNull MotionEvent event)
		{
			switch(event.getAction()) {
				case MotionEvent.ACTION_UP:
					final int day = getDayFromLocation(event.getX(), event.getY());
					if(day >= 0) {
						onDayClick(day);
					}
					break;
			}
			return true;
		}
		//
		// Sets up the text and style properties for painting. Override this if you
		// want to use a different paint.
		//
		protected void initView()
		{
			mMonthTitlePaint = new Paint();
			mMonthTitlePaint.setFakeBoldText(true);
			mMonthTitlePaint.setAntiAlias(true);
			mMonthTitlePaint.setTextSize(MONTH_LABEL_TEXT_SIZE);
			mMonthTitlePaint.setTypeface(Typeface.create(mMonthTitleTypeface, Typeface.BOLD));
			mMonthTitlePaint.setColor(mDayTextColor);
			mMonthTitlePaint.setTextAlign(Paint.Align.CENTER);
			mMonthTitlePaint.setStyle(Paint.Style.FILL);
			mSelectedCirclePaint = new Paint();
			mSelectedCirclePaint.setFakeBoldText(true);
			mSelectedCirclePaint.setAntiAlias(true);
			mSelectedCirclePaint.setColor(mTodayNumberColor);
			mSelectedCirclePaint.setTextAlign(Paint.Align.CENTER);
			mSelectedCirclePaint.setStyle(Paint.Style.FILL);
			mSelectedCirclePaint.setAlpha(SELECTED_CIRCLE_ALPHA);
			mMonthDayLabelPaint = new Paint();
			mMonthDayLabelPaint.setAntiAlias(true);
			mMonthDayLabelPaint.setTextSize(MONTH_DAY_LABEL_TEXT_SIZE);
			mMonthDayLabelPaint.setColor(mMonthDayTextColor);
			mMonthDayLabelPaint.setTypeface(TypefaceHelper.get(getContext(),"Roboto-Medium"));
			mMonthDayLabelPaint.setStyle(Paint.Style.FILL);
			mMonthDayLabelPaint.setTextAlign(Paint.Align.CENTER);
			mMonthDayLabelPaint.setFakeBoldText(true);
			mMonthNumPaint = new Paint();
			mMonthNumPaint.setAntiAlias(true);
			mMonthNumPaint.setTextSize(MINI_DAY_NUMBER_TEXT_SIZE);
			mMonthNumPaint.setStyle(Paint.Style.FILL);
			mMonthNumPaint.setTextAlign(Paint.Align.CENTER);
			mMonthNumPaint.setFakeBoldText(false);
		}
		public void setAccentColor(int color)
		{
			mTodayNumberColor = color;
			mSelectedCirclePaint.setColor(color);
		}
		@Override protected void onDraw(Canvas canvas)
		{
			drawMonthTitle(canvas);
			drawMonthDayLabels(canvas);
			drawMonthNums(canvas);
		}
		private int mDayOfWeekStart = 0;
		/**
		 * Sets all the parameters for displaying this week. The only required
		 * parameter is the week number. Other parameters have a default value and
		 * will only update if a new value is included, except for focus month,
		 * which will always default to no focus month if no value is passed in. See
		 * {@link #VIEW_PARAMS_HEIGHT} for more info on parameters.
		 *
		 * @param params A map of the new parameters, see
		 *            {@link #VIEW_PARAMS_HEIGHT}
		 */
		public void setMonthParams(HashMap<String, Integer> params)
		{
			if(!params.containsKey(VIEW_PARAMS_MONTH) && !params.containsKey(VIEW_PARAMS_YEAR)) {
				throw new InvalidParameterException("You must specify month and year for this view");
			}
			setTag(params);
			// We keep the current value for any params not present
			if(params.containsKey(VIEW_PARAMS_HEIGHT)) {
				mRowHeight = params.get(VIEW_PARAMS_HEIGHT);
				if(mRowHeight < MIN_HEIGHT) {
					mRowHeight = MIN_HEIGHT;
				}
			}
			if(params.containsKey(VIEW_PARAMS_SELECTED_DAY)) {
				mSelectedDay = params.get(VIEW_PARAMS_SELECTED_DAY);
			}
			// Allocate space for caching the day numbers and focus values
			mMonth = params.get(VIEW_PARAMS_MONTH);
			mYear = params.get(VIEW_PARAMS_YEAR);
			// Figure out what day today is
			//final Time today = new Time(Time.getCurrentTimezone());
			//today.setToNow();
			final Calendar today = Calendar.getInstance();
			mHasToday = false;
			mToday = -1;
			mCalendar.set(Calendar.MONTH, mMonth);
			mCalendar.set(Calendar.YEAR, mYear);
			mCalendar.set(Calendar.DAY_OF_MONTH, 1);
			mDayOfWeekStart = mCalendar.get(Calendar.DAY_OF_WEEK);
			if(params.containsKey(VIEW_PARAMS_WEEK_START)) {
				mWeekStart = params.get(VIEW_PARAMS_WEEK_START);
			}
			else {
				mWeekStart = mCalendar.getFirstDayOfWeek();
			}
			mNumCells = mCalendar.getActualMaximum(Calendar.DAY_OF_MONTH);
			for(int i = 0; i < mNumCells; i++) {
				final int day = i + 1;
				if(sameDay(day, today)) {
					mHasToday = true;
					mToday = day;
				}
			}
			mNumRows = calculateNumRows();
			// Invalidate cached accessibility information.
			mTouchHelper.invalidateRoot();
		}
		public void setSelectedDay(int day)
		{
			mSelectedDay = day;
		}
		public void reuse()
		{
			mNumRows = DEFAULT_NUM_ROWS;
			requestLayout();
		}
		private int calculateNumRows()
		{
			int offset = findDayOffset();
			int dividend = (offset + mNumCells) / mNumDays;
			int remainder = (offset + mNumCells) % mNumDays;
			return (dividend + (remainder > 0 ? 1 : 0));
		}
		private boolean sameDay(int day, Calendar today)
		{
			return mYear == today.get(Calendar.YEAR) && mMonth == today.get(Calendar.MONTH) && day == today.get(Calendar.DAY_OF_MONTH);
		}
		@Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
		{
			setMeasuredDimension(MeasureSpec.getSize(widthMeasureSpec), mRowHeight * mNumRows + getMonthHeaderSize() + 5);
		}
		@Override protected void onSizeChanged(int w, int h, int oldw, int oldh)
		{
			mWidth = w;
			mTouchHelper.invalidateRoot(); // Invalidate cached accessibility information.
		}
		public int getMonth() { return mMonth; }
		public int getYear() { return mYear; }
		//
		// A wrapper to the MonthHeaderSize to allow override it in children
		//
		protected int getMonthHeaderSize() { return MONTH_HEADER_SIZE; }
		private String getMonthAndYearString()
		{
			int flags = DateUtils.FORMAT_SHOW_DATE | DateUtils.FORMAT_SHOW_YEAR | DateUtils.FORMAT_NO_MONTH_DAY;
			mStringBuilder.setLength(0);
			long millis = mCalendar.getTimeInMillis();
			return DateUtils.formatDateRange(getContext(), mFormatter, millis, millis, flags, null).toString();
		}
		protected void drawMonthTitle(Canvas canvas)
		{
			int x = (mWidth + 2 * mEdgePadding) / 2;
			int y = (getMonthHeaderSize() - MONTH_DAY_LABEL_TEXT_SIZE) / 2;
			canvas.drawText(getMonthAndYearString(), x, y, mMonthTitlePaint);
		}
		protected void drawMonthDayLabels(Canvas canvas)
		{
			int y = getMonthHeaderSize() - (MONTH_DAY_LABEL_TEXT_SIZE / 2);
			int dayWidthHalf = (mWidth - mEdgePadding * 2) / (mNumDays * 2);
			for(int i = 0; i < mNumDays; i++) {
				int x = (2 * i + 1) * dayWidthHalf + mEdgePadding;
				int calendarDay = (i + mWeekStart) % mNumDays;
				mDayLabelCalendar.set(Calendar.DAY_OF_WEEK, calendarDay);
				Locale locale = Locale.getDefault();
				String localWeekDisplayName = mDayLabelCalendar.getDisplayName(Calendar.DAY_OF_WEEK, Calendar.SHORT, locale);
				String weekString = localWeekDisplayName.toUpperCase(locale).substring(0, 1);
				if(locale.equals(Locale.CHINA) || locale.equals(Locale.CHINESE) || locale.equals(Locale.SIMPLIFIED_CHINESE) || locale.equals(Locale.TRADITIONAL_CHINESE)) {
					int len = localWeekDisplayName.length();
					weekString = localWeekDisplayName.substring(len -1, len);
				}
				if(locale.getLanguage().equals("he") || locale.getLanguage().equals("iw")) {
					if(mDayLabelCalendar.get(Calendar.DAY_OF_WEEK) != Calendar.SATURDAY) {
						int len = localWeekDisplayName.length();
						weekString = localWeekDisplayName.substring(len - 2, len - 1);
					}
					else {
						// I know this is duplication, but it makes the code easier to grok by
						// having all hebrew code in the same block
						weekString = localWeekDisplayName.toUpperCase(locale).substring(0, 1);
					}
				}
				canvas.drawText(weekString, x, y, mMonthDayLabelPaint);
			}
		}
		//
		// Draws the week and month day numbers for this week. Override this method
		// if you need different placement.
		//
		// @param canvas The canvas to draw on
		//
		protected void drawMonthNums(Canvas canvas)
		{
			int y = (((mRowHeight + MINI_DAY_NUMBER_TEXT_SIZE) / 2) - DAY_SEPARATOR_WIDTH) + getMonthHeaderSize();
			final float dayWidthHalf = (mWidth - mEdgePadding * 2) / (mNumDays * 2.0f);
			int j = findDayOffset();
			for(int dayNumber = 1; dayNumber <= mNumCells; dayNumber++) {
				final int x = (int)((2 * j + 1) * dayWidthHalf + mEdgePadding);
				int yRelativeToDay = (mRowHeight + MINI_DAY_NUMBER_TEXT_SIZE) / 2 - DAY_SEPARATOR_WIDTH;
				final int startX = (int)(x - dayWidthHalf);
				final int stopX = (int)(x + dayWidthHalf);
				final int startY = (int)(y - yRelativeToDay);
				final int stopY = (int)(startY + mRowHeight);
				drawMonthDay(canvas, mYear, mMonth, dayNumber, x, y, startX, stopX, startY, stopY);
				j++;
				if(j == mNumDays) {
					j = 0;
					y += mRowHeight;
				}
			}
		}
		//
		// This method should draw the month day.  Implemented by sub-classes to allow customization.
		//
		// @param canvas  The canvas to draw on
		// @param year  The year of this month day
		// @param month  The month of this month day
		// @param day  The day number of this month day
		// @param x  The default x position to draw the day number
		// @param y  The default y position to draw the day number
		// @param startX  The left boundary of the day number rect
		// @param stopX  The right boundary of the day number rect
		// @param startY  The top boundary of the day number rect
		// @param stopY  The bottom boundary of the day number rect
		//
		public abstract void drawMonthDay(Canvas canvas, int year, int month, int day, int x, int y, int startX, int stopX, int startY, int stopY);
		protected int findDayOffset()
		{
			return (mDayOfWeekStart < mWeekStart ? (mDayOfWeekStart + mNumDays) : mDayOfWeekStart) - mWeekStart;
		}
		//
		// Calculates the day that the given x position is in, accounting for week
		// number. Returns the day or -1 if the position wasn't in a day.
		//
		// @param x The x position of the touch event
		// @return The day number, or -1 if the position wasn't in a day
		//
		public int getDayFromLocation(float x, float y)
		{
			final int day = getInternalDayFromLocation(x, y);
			return (day < 1 || day > mNumCells) ? -1 : day;
		}
		//
		// Calculates the day that the given x position is in, accounting for week number.
		//
		// @param x The x position of the touch event
		// @return The day number
		//
		protected int getInternalDayFromLocation(float x, float y)
		{
			int dayStart = mEdgePadding;
			if(x < dayStart || x > mWidth - mEdgePadding) {
				return -1;
			}
			else {
				// Selection is (x - start) / (pixels/day) == (x -s) * day / pixels
				int row = (int) (y - getMonthHeaderSize()) / mRowHeight;
				int column = (int) ((x - dayStart) * mNumDays / (mWidth - dayStart - mEdgePadding));
				int day = column - findDayOffset() + 1;
				day += row * mNumDays;
				return day;
			}
		}
		//
		// Called when the user clicks on a day. Handles callbacks to the
		// {@link OnDayClickListener} if one is set.
		// <p/>
		// If the day is out of the range set by minDate and/or maxDate, this is a no-op.
		//
		// @param day The day that was clicked
		//
		private void onDayClick(int day)
		{
			// If the min / max date are set, only process the click if it's a valid selection.
			if(!isOutOfRange(mYear, mMonth, day)) {
				if (mOnDayClickListener != null)
					mOnDayClickListener.onDayClick(this, new MonthAdapter.CalendarDay(mYear, mMonth, day));
				// This is a no-op if accessibility is turned off.
				mTouchHelper.sendEventForVirtualView(day, AccessibilityEvent.TYPE_VIEW_CLICKED);
			}
		}
		//
		// @return true if the specified year/month/day are within the selectable days or the range set by minDate and maxDate.
		// If one or either have not been set, they are considered as Integer.MIN_VALUE and
		// Integer.MAX_VALUE.
		//
		protected boolean isOutOfRange(int year, int month, int day)
		{
			if(mController.getSelectableDays() != null)
				return !isSelectable(year, month, day);
			else if(isBeforeMin(year, month, day))
				return true;
			else if(isAfterMax(year, month, day))
				return true;
			else
				return false;
		}
		private boolean isSelectable(int year, int month, int day)
		{
			Calendar[] selectableDays = mController.getSelectableDays();
			for(Calendar c : selectableDays) {
				if(year < c.get(Calendar.YEAR)) break;
				if(year > c.get(Calendar.YEAR)) continue;
				if(month < c.get(Calendar.MONTH)) break;
				if(month > c.get(Calendar.MONTH)) continue;
				if(day < c.get(Calendar.DAY_OF_MONTH)) break;
				if(day > c.get(Calendar.DAY_OF_MONTH)) continue;
				return true;
			}
			return false;
		}
		private boolean isBeforeMin(int year, int month, int day)
		{
			if(mController == null) {
				return false;
			}
			Calendar minDate = mController.getMinDate();
			if(minDate == null) {
				return false;
			}
			if(year < minDate.get(Calendar.YEAR)) {
				return true;
			}
			else if(year > minDate.get(Calendar.YEAR)) {
				return false;
			}
			if(month < minDate.get(Calendar.MONTH)) {
				return true;
			}
			else if(month > minDate.get(Calendar.MONTH)) {
				return false;
			}
			if(day < minDate.get(Calendar.DAY_OF_MONTH)) {
				return true;
			}
			else {
				return false;
			}
		}
		private boolean isAfterMax(int year, int month, int day)
		{
			if(mController == null) {
				return false;
			}
			Calendar maxDate = mController.getMaxDate();
			if(maxDate == null) {
				return false;
			}
			if(year > maxDate.get(Calendar.YEAR)) {
				return true;
			}
			else if(year < maxDate.get(Calendar.YEAR)) {
				return false;
			}
			if(month > maxDate.get(Calendar.MONTH)) {
				return true;
			}
			else if(month < maxDate.get(Calendar.MONTH)) {
				return false;
			}
			if(day > maxDate.get(Calendar.DAY_OF_MONTH)) {
				return true;
			}
			else {
				return false;
			}
		}
		//
		// @param year
		// @param month
		// @param day
		// @return true if the given date should be highlighted
		//
		protected boolean isHighlighted(int year, int month, int day)
		{
			Calendar[] highlightedDays = mController.getHighlightedDays();
			if(highlightedDays == null) return false;
			for(Calendar c : highlightedDays) {
				if(year != c.get(Calendar.YEAR)) continue;
				if(month != c.get(Calendar.MONTH)) continue;
				if(day != c.get(Calendar.DAY_OF_MONTH)) continue;
				return true;
			}
			return false;
		}
		//
		// @return The date that has accessibility focus, or {@code null} if no date has focus
		//
		public MonthAdapter.CalendarDay getAccessibilityFocus()
		{
			final int day = mTouchHelper.getFocusedVirtualView();
			return (day >= 0) ? new MonthAdapter.CalendarDay(mYear, mMonth, day) : null;
		}
		//
		// Clears accessibility focus within the view. No-op if the view does not
		// contain accessibility focus.
		//
		public void clearAccessibilityFocus() { mTouchHelper.clearFocusedVirtualView(); }
		//
		// Attempts to restore accessibility focus to the specified date.
		//
		// @param day The date which should receive focus
		// @return {@code false} if the date is not valid for this month view, or {@code true} if the date received focus
		//
		public boolean restoreAccessibilityFocus(MonthAdapter.CalendarDay day)
		{
			if((day.year != mYear) || (day.month != mMonth) || (day.day > mNumCells)) {
				return false;
			}
			else {
				mTouchHelper.setFocusedVirtualView(day.day);
				return true;
			}
		}
		//
		// Provides a virtual view hierarchy for interfacing with an accessibility service.
		//
		protected class MonthViewTouchHelper extends ExploreByTouchHelper {
			private static final String DATE_FORMAT = "dd MMMM yyyy";
			private final Rect mTempRect = new Rect();
			private final Calendar mTempCalendar = Calendar.getInstance();
			public MonthViewTouchHelper(View host)
			{
				super(host);
			}
			public void setFocusedVirtualView(int virtualViewId)
			{
				getAccessibilityNodeProvider(MonthView.this).performAction(
						virtualViewId, AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS, null);
			}
			public void clearFocusedVirtualView()
			{
				final int focusedVirtualView = getFocusedVirtualView();
				if(focusedVirtualView != ExploreByTouchHelper.INVALID_ID) {
					getAccessibilityNodeProvider(MonthView.this).performAction(
							focusedVirtualView,
							AccessibilityNodeInfoCompat.ACTION_CLEAR_ACCESSIBILITY_FOCUS,
							null);
				}
			}
			@Override protected int getVirtualViewAt(float x, float y)
			{
				final int day = getDayFromLocation(x, y);
				if(day >= 0) {
					return day;
				}
				return ExploreByTouchHelper.INVALID_ID;
			}
			@Override protected void getVisibleVirtualViews(List<Integer> virtualViewIds)
			{
				for(int day = 1; day <= mNumCells; day++) {
					virtualViewIds.add(day);
				}
			}
			@Override protected void onPopulateEventForVirtualView(int virtualViewId, AccessibilityEvent event)
			{
				event.setContentDescription(getItemDescription(virtualViewId));
			}
			@Override protected void onPopulateNodeForVirtualView(int virtualViewId, AccessibilityNodeInfoCompat node)
			{
				getItemBounds(virtualViewId, mTempRect);
				node.setContentDescription(getItemDescription(virtualViewId));
				node.setBoundsInParent(mTempRect);
				node.addAction(AccessibilityNodeInfo.ACTION_CLICK);
				if(virtualViewId == mSelectedDay) {
					node.setSelected(true);
				}

			}
			@Override protected boolean onPerformActionForVirtualView(int virtualViewId, int action, Bundle arguments)
			{
				switch(action) {
					case AccessibilityNodeInfo.ACTION_CLICK:
						onDayClick(virtualViewId);
						return true;
				}
				return false;
			}
			//
			// Calculates the bounding rectangle of a given time object.
			//
			// @param day The day to calculate bounds for
			// @param rect The rectangle in which to store the bounds
			//
			protected void getItemBounds(int day, Rect rect)
			{
				final int offsetX = mEdgePadding;
				final int offsetY = getMonthHeaderSize();
				final int cellHeight = mRowHeight;
				final int cellWidth = ((mWidth - (2 * mEdgePadding)) / mNumDays);
				final int index = ((day - 1) + findDayOffset());
				final int row = (index / mNumDays);
				final int column = (index % mNumDays);
				final int x = (offsetX + (column * cellWidth));
				final int y = (offsetY + (row * cellHeight));
				rect.set(x, y, (x + cellWidth), (y + cellHeight));
			}
			//
			// Generates a description for a given time object. Since this
			// description will be spoken, the components are ordered by descending
			// specificity as DAY MONTH YEAR.
			//
			// @param day The day to generate a description for
			// @return A description of the time object
			//
			protected CharSequence getItemDescription(int day)
			{
				mTempCalendar.set(mYear, mMonth, day);
				final CharSequence date = DateFormat.format(DATE_FORMAT, mTempCalendar.getTimeInMillis());
				if(day == mSelectedDay) {
					// @sobolev return getContext().getString(R.string.range_item_is_selected, date);
					return date; // @sobolev @todo
				}
				return date;
			}
		}
		//
		// Handles callbacks when the user clicks on a time object.
		//
		public interface OnDayClickListener {
			void onDayClick(MonthView view, MonthAdapter.CalendarDay day);
		}
	}
	//
	// An adapter for a list of {@link MonthView} items.
	//
	public static abstract class MonthAdapter extends BaseAdapter implements MonthView.OnDayClickListener {
		private static final String TAG = "SimpleMonthAdapter";
		private final Context mContext;
		protected final DatePickerController mController;
		private CalendarDay mSelectedDay;
		private int mAccentColor = -1;
		protected static int WEEK_7_OVERHANG_HEIGHT = 7;
		protected static final int MONTHS_IN_YEAR = 12;
		//
		// A convenience class to represent a specific date.
		//
		public static class CalendarDay {
			private Calendar calendar;
			int year;
			int month;
			int day;
			public CalendarDay()
			{
				setTime(System.currentTimeMillis());
			}
			public CalendarDay(long timeInMillis)
			{
				setTime(timeInMillis);
			}
			public CalendarDay(Calendar calendar)
			{
				year = calendar.get(Calendar.YEAR);
				month = calendar.get(Calendar.MONTH);
				day = calendar.get(Calendar.DAY_OF_MONTH);
			}
			public CalendarDay(int year, int month, int day)
			{
				setDay(year, month, day);
			}
			public void set(CalendarDay date)
			{
				year = date.year;
				month = date.month;
				day = date.day;
			}
			public void setDay(int year, int month, int day)
			{
				this.year = year;
				this.month = month;
				this.day = day;
			}
			private void setTime(long timeInMillis)
			{
				if(calendar == null) {
					calendar = Calendar.getInstance();
				}
				calendar.setTimeInMillis(timeInMillis);
				month = calendar.get(Calendar.MONTH);
				year = calendar.get(Calendar.YEAR);
				day = calendar.get(Calendar.DAY_OF_MONTH);
			}
			public int getYear() { return year; }
			public int getMonth() { return month; }
			public int getDay() { return day; }
		}
		public MonthAdapter(Context context, DatePickerController controller)
		{
			mContext = context;
			mController = controller;
			init();
			setSelectedDay(mController.getSelectedDay());
		}
		public void setAccentColor(int color) { mAccentColor = color; }
		//
		// Updates the selected day and related parameters.
		//
		// @param day The day to highlight
		//
		public void setSelectedDay(CalendarDay day)
		{
			mSelectedDay = day;
			notifyDataSetChanged();
		}
		public CalendarDay getSelectedDay() { return mSelectedDay; }
		//
		//Set up the gesture detector and selected time
		//
		protected void init()
		{
			mSelectedDay = new CalendarDay(System.currentTimeMillis());
		}
		@Override public int getCount()
		{
			return ((mController.getMaxYear() - mController.getMinYear()) + 1) * MONTHS_IN_YEAR;
		}
		@Override public Object getItem(int position) { return null; }
		@Override public long getItemId(int position) { return position; }
		@Override public boolean hasStableIds() { return true; }
		@SuppressLint("NewApi") @SuppressWarnings("unchecked") @Override public View getView(int position, View convertView, ViewGroup parent)
		{
			MonthView v;
			HashMap<String, Integer> drawingParams = null;
			if(convertView != null) {
				v = (MonthView) convertView;
				// We store the drawing parameters in the view so it can be recycled
				drawingParams = (HashMap<String, Integer>) v.getTag();
			}
			else {
				v = createMonthView(mContext);
				// Set up the new view
				AbsListView.LayoutParams params = new AbsListView.LayoutParams(AbsListView.LayoutParams.MATCH_PARENT, AbsListView.LayoutParams.MATCH_PARENT);
				v.setLayoutParams(params);
				v.setClickable(true);
				v.setOnDayClickListener(this);
				if(mAccentColor != -1) {
					v.setAccentColor(mAccentColor);
				}
			}
			if(drawingParams == null) {
				drawingParams = new HashMap<String, Integer>();
			}
			drawingParams.clear();
			final int month = position % MONTHS_IN_YEAR;
			final int year = position / MONTHS_IN_YEAR + mController.getMinYear();
			int selectedDay = -1;
			if(isSelectedDayInMonth(year, month)) {
				selectedDay = mSelectedDay.day;
			}
			// Invokes requestLayout() to ensure that the recycled view is set with the appropriate
			// height/number of weeks before being displayed.
			v.reuse();
			drawingParams.put(MonthView.VIEW_PARAMS_SELECTED_DAY, selectedDay);
			drawingParams.put(MonthView.VIEW_PARAMS_YEAR, year);
			drawingParams.put(MonthView.VIEW_PARAMS_MONTH, month);
			drawingParams.put(MonthView.VIEW_PARAMS_WEEK_START, mController.getFirstDayOfWeek());
			v.setMonthParams(drawingParams);
			v.invalidate();
			return v;
		}
		public abstract MonthView createMonthView(Context context);
		private boolean isSelectedDayInMonth(int year, int month)
		{
			return mSelectedDay.year == year && mSelectedDay.month == month;
		}
		@Override public void onDayClick(MonthView view, CalendarDay day)
		{
			if(day != null)
				onDayTapped(day);
		}
		//
		// Maintains the same hour/min/sec but moves the day to the tapped day.
		//
		// @param day The day that was tapped
		//
		protected void onDayTapped(CalendarDay day)
		{
			mController.tryVibrate();
			mController.onDayOfMonthSelected(day.year, day.month, day.day);
			setSelectedDay(day);
		}
	}
	//
	// This displays a list of months in a calendar format with selectable days.
	//
	public static abstract class DayPickerView extends ListView implements AbsListView.OnScrollListener, DatePickerDialog.OnDateChangedListener {
		private static final String TAG = "MonthFragment";
		protected static final int SCROLL_HYST_WEEKS = 2; // Affects when the month selection will change while scrolling up
		protected static final int GOTO_SCROLL_DURATION = 250; // How long the GoTo fling animation should last
		protected static final int SCROLL_CHANGE_DELAY = 40; // How long to wait after receiving an onScrollStateChanged notification before acting on it
		public static final int DAYS_PER_WEEK = 7; // The number of days to display in each week
		public static int LIST_TOP_OFFSET = -1; // so that the top line will be
		// under the separator
		// You can override these numbers to get a different appearance
		protected int mNumWeeks = 6;
		protected boolean mShowWeekNumber = false;
		protected int mDaysPerWeek = 7;
		private static SimpleDateFormat YEAR_FORMAT = new SimpleDateFormat("yyyy", Locale.getDefault());
		protected float mFriction = 1.0f; // These affect the scroll speed and feel
		protected Context mContext;
		protected Handler mHandler;
		// highlighted time
		protected MonthAdapter.CalendarDay mSelectedDay = new MonthAdapter.CalendarDay();
		protected MonthAdapter mAdapter;
		protected MonthAdapter.CalendarDay mTempDay = new MonthAdapter.CalendarDay();
		protected int mFirstDayOfWeek; // When the week starts; numbered like Time.<WEEKDAY> (e.g. SUNDAY=0).
		protected CharSequence mPrevMonthName; // The last name announced by accessibility
		protected int mCurrentMonthDisplayed; // which month should be displayed/highlighted [0-11]
		protected long mPreviousScrollPosition; // used for tracking during a scroll
		protected int mPreviousScrollState = OnScrollListener.SCROLL_STATE_IDLE; // used for tracking what state listview is in
		protected int mCurrentScrollState = OnScrollListener.SCROLL_STATE_IDLE; // used for tracking what state listview is in
		private DatePickerController mController;
		private boolean mPerformingScroll;
		public DayPickerView(Context context, AttributeSet attrs)
		{
			super(context, attrs);
			init(context);
		}
		public DayPickerView(Context context, DatePickerController controller)
		{
			super(context);
			init(context);
			setController(controller);
		}
		public void setController(DatePickerController controller)
		{
			mController = controller;
			mController.registerOnDateChangedListener(this);
			refreshAdapter();
			onDateChanged();
		}
		public void init(Context context)
		{
			mHandler = new Handler();
			setLayoutParams(new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));
			setDrawSelectorOnTop(false);
			mContext = context;
			setUpListView();
		}
		public void onChange()
		{
			refreshAdapter();
		}
		//
		// Creates a new adapter if necessary and sets up its parameters. Override
		// this method to provide a custom adapter.
		//
		protected void refreshAdapter()
		{
			if(mAdapter == null)
				mAdapter = createMonthAdapter(getContext(), mController);
			else
				mAdapter.setSelectedDay(mSelectedDay);
			setAdapter(mAdapter); // refresh the view with the new parameters
		}
		public abstract MonthAdapter createMonthAdapter(Context context, DatePickerController controller);
		//
		// Sets all the required fields for the list view. Override this method to
		// set a different list view behavior.
		//
		protected void setUpListView()
		{
			setCacheColorHint(0); // Transparent background on scroll
			setDivider(null); // No dividers
			setItemsCanFocus(true); // Items are clickable
			setFastScrollEnabled(false); // The thumb gets in the way, so disable it
			setVerticalScrollBarEnabled(false);
			setOnScrollListener(this);
			setFadingEdgeLength(0);
			setFriction(ViewConfiguration.getScrollFriction() * mFriction); // Make the scrolling behavior nicer
		}
		//
		// This moves to the specified time in the view. If the time is not already
		// in range it will move the list so that the first of the month containing
		// the time is at the top of the view. If the new time is already in view
		// the list will not be scrolled unless forceScroll is true. This time may
		// optionally be highlighted as selected as well.
		//
		// @param day The day to move to
		// @param animate Whether to scroll to the given time or just redraw at the new location
		// @param setSelected Whether to set the given time as selected
		// @param forceScroll Whether to recenter even if the time is already visible
		// @return Whether or not the view animated to the new location
		//
		public boolean goTo(MonthAdapter.CalendarDay day, boolean animate, boolean setSelected, boolean forceScroll)
		{
			// Set the selected day
			if(setSelected) {
				mSelectedDay.set(day);
			}
			mTempDay.set(day);
			final int position = (day.year - mController.getMinYear()) * MonthAdapter.MONTHS_IN_YEAR + day.month;
			View child;
			int i = 0;
			int top = 0;
			// Find a child that's completely in the view
			do {
				child = getChildAt(i++);
				if(child == null) {
					break;
				}
				top = child.getTop();
				if(Log.isLoggable(TAG, Log.DEBUG)) {
					Log.d(TAG, "child at " + (i - 1) + " has top " + top);
				}
			} while(top < 0);
			// Compute the first and last position visible
			int selectedPosition = (child != null) ? getPositionForView(child) : 0;
			if(setSelected) {
				mAdapter.setSelectedDay(mSelectedDay);
			}
			if(Log.isLoggable(TAG, Log.DEBUG)) {
				Log.d(TAG, "GoTo position " + position);
			}
			// Check if the selected day is now outside of our visible range
			// and if so scroll to the month that contains it
			if(position != selectedPosition || forceScroll) {
				setMonthDisplayed(mTempDay);
				mPreviousScrollState = OnScrollListener.SCROLL_STATE_FLING;
				if(animate) {
					smoothScrollToPosition(position);
					return true;
				}
				else
					postSetSelection(position);
			}
			else if(setSelected)
				setMonthDisplayed(mSelectedDay);
			return false;
		}
		public void postSetSelection(final int position)
		{
			clearFocus();
			post(new Runnable() {
				@Override public void run() { setSelection(position); }
			});
			onScrollStateChanged(this, OnScrollListener.SCROLL_STATE_IDLE);
		}
		//
		// Updates the title and selected month if the view has moved to a new month.
		//
		@Override public void onScroll(AbsListView view, int firstVisibleItem, int visibleItemCount, int totalItemCount)
		{
			MonthView child = (MonthView) view.getChildAt(0);
			if(child != null) {
				// Figure out where we are
				long currScroll = view.getFirstVisiblePosition() * child.getHeight() - child.getBottom();
				mPreviousScrollPosition = currScroll;
				mPreviousScrollState = mCurrentScrollState;
			}
		}
		//
		// Sets the month displayed at the top of this view based on time. Override
		// to add custom events when the title is changed.
		//
		protected void setMonthDisplayed(MonthAdapter.CalendarDay date)
		{
			mCurrentMonthDisplayed = date.month;
			invalidateViews();
		}
		public void setAccentColor(int accentColor)
		{
			mAdapter.setAccentColor(accentColor);
		}
		@Override public void onScrollStateChanged(AbsListView view, int scrollState)
		{
			// use a post to prevent re-entering onScrollStateChanged before it exits
			mScrollStateChangedRunnable.doScrollStateChange(view, scrollState);
		}
		protected ScrollStateRunnable mScrollStateChangedRunnable = new ScrollStateRunnable();

		protected class ScrollStateRunnable implements Runnable {
			private int mNewState;
			//
			// Sets up the runnable with a short delay in case the scroll state
			// immediately changes again.
			// @param view The list view that changed state
			// @param scrollState The new state it changed to
			//
			public void doScrollStateChange(AbsListView view, int scrollState)
			{
				mHandler.removeCallbacks(this);
				mNewState = scrollState;
				mHandler.postDelayed(this, SCROLL_CHANGE_DELAY);
			}
			@Override public void run()
			{
				mCurrentScrollState = mNewState;
				if(Log.isLoggable(TAG, Log.DEBUG)) {
					Log.d(TAG,"new scroll state: " + mNewState + " old state: " + mPreviousScrollState);
				}
				// Fix the position after a scroll or a fling ends
				if(mNewState == OnScrollListener.SCROLL_STATE_IDLE
						&& mPreviousScrollState != OnScrollListener.SCROLL_STATE_IDLE
						&& mPreviousScrollState != OnScrollListener.SCROLL_STATE_TOUCH_SCROLL) {
					mPreviousScrollState = mNewState;
					int i = 0;
					View child = getChildAt(i);
					while(child != null && child.getBottom() <= 0) {
						child = getChildAt(++i);
					}
					if(child == null) {
						return; // The view is no longer visible, just return
					}
					int firstPosition = getFirstVisiblePosition();
					int lastPosition = getLastVisiblePosition();
					boolean scroll = firstPosition != 0 && lastPosition != getCount() - 1;
					final int top = child.getTop();
					final int bottom = child.getBottom();
					final int midpoint = getHeight() / 2;
					if(scroll && top < LIST_TOP_OFFSET) {
						if(bottom > midpoint) {
							smoothScrollBy(top, GOTO_SCROLL_DURATION);
						}
						else {
							smoothScrollBy(bottom, GOTO_SCROLL_DURATION);
						}
					}
				}
				else {
					mPreviousScrollState = mNewState;
				}
			}
		}
		/**
		 * Gets the position of the view that is most prominently displayed within the list view.
		 */
		public int getMostVisiblePosition()
		{
			final int firstPosition = getFirstVisiblePosition();
			final int height = getHeight();
			int maxDisplayedHeight = 0;
			int mostVisibleIndex = 0;
			int i=0;
			int bottom = 0;
			while(bottom < height) {
				View child = getChildAt(i);
				if(child == null) {
					break;
				}
				bottom = child.getBottom();
				int displayedHeight = Math.min(bottom, height) - Math.max(0, child.getTop());
				if(displayedHeight > maxDisplayedHeight) {
					mostVisibleIndex = i;
					maxDisplayedHeight = displayedHeight;
				}
				i++;
			}
			return firstPosition + mostVisibleIndex;
		}
		@Override public void onDateChanged()
		{
			goTo(mController.getSelectedDay(), false, true, true);
		}
		/**
		 * Attempts to return the date that has accessibility focus.
		 *
		 * @return The date that has accessibility focus, or {@code null} if no date
		 *         has focus.
		 */
		private MonthAdapter.CalendarDay findAccessibilityFocus()
		{
			final int childCount = getChildCount();
			for(int i = 0; i < childCount; i++) {
				final View child = getChildAt(i);
				if(child instanceof MonthView) {
					final MonthAdapter.CalendarDay focus = ((MonthView) child).getAccessibilityFocus();
					if(focus != null) {
						if(Build.VERSION.SDK_INT == Build.VERSION_CODES.JELLY_BEAN_MR1) {
							// Clear focus to avoid ListView bug in Jelly Bean MR1.
							((MonthView) child).clearAccessibilityFocus();
						}
						return focus;
					}
				}
			}
			return null;
		}
		/**
		 * Attempts to restore accessibility focus to a given date. No-op if
		 * {@code day} is {@code null}.
		 *
		 * @param day The date that should receive accessibility focus
		 * @return {@code true} if focus was restored
		 */
		private boolean restoreAccessibilityFocus(MonthAdapter.CalendarDay day)
		{
			if(day == null) {
				return false;
			}
			final int childCount = getChildCount();
			for(int i = 0; i < childCount; i++) {
				final View child = getChildAt(i);
				if(child instanceof MonthView) {
					if (((MonthView) child).restoreAccessibilityFocus(day)) {
						return true;
					}
				}
			}
			return false;
		}
		@Override protected void layoutChildren()
		{
			final MonthAdapter.CalendarDay focusedDay = findAccessibilityFocus();
			super.layoutChildren();
			if(mPerformingScroll) {
				mPerformingScroll = false;
			}
			else {
				restoreAccessibilityFocus(focusedDay);
			}
		}
		@Override public void onInitializeAccessibilityEvent(@NonNull AccessibilityEvent event)
		{
			super.onInitializeAccessibilityEvent(event);
			event.setItemCount(-1);
		}
		private static String getMonthAndYearString(MonthAdapter.CalendarDay day)
		{
			Calendar cal = Calendar.getInstance();
			cal.set(day.year, day.month, day.day);
			String sbuf = "";
			sbuf += cal.getDisplayName(Calendar.MONTH, Calendar.LONG, Locale.getDefault());
			sbuf += " ";
			sbuf += YEAR_FORMAT.format(cal.getTime());
			return sbuf;
		}
		//
		// Necessary for accessibility, to ensure we support "scrolling" forward and backward in the month list.
		//
		@Override @SuppressWarnings("deprecation") public void onInitializeAccessibilityNodeInfo(@NonNull AccessibilityNodeInfo info)
		{
			super.onInitializeAccessibilityNodeInfo(info);
			if(Build.VERSION.SDK_INT >= 21) {
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_BACKWARD);
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_FORWARD);
			}
			else {
				info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
				info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
			}
		}
		//
		// When scroll forward/backward events are received, announce the newly scrolled-to month.
		//
		@SuppressLint("NewApi") @Override public boolean performAccessibilityAction(int action, Bundle arguments)
		{
			if(action != AccessibilityNodeInfo.ACTION_SCROLL_FORWARD &&
					action != AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD) {
				return super.performAccessibilityAction(action, arguments);
			}
			// Figure out what month is showing.
			int firstVisiblePosition = getFirstVisiblePosition();
			int month = firstVisiblePosition % 12;
			int year = firstVisiblePosition / 12 + mController.getMinYear();
			MonthAdapter.CalendarDay day = new MonthAdapter.CalendarDay(year, month, 1);
			// Scroll either forward or backward one month.
			if(action == AccessibilityNodeInfo.ACTION_SCROLL_FORWARD) {
				day.month++;
				if(day.month == 12) {
					day.month = 0;
					day.year++;
				}
			}
			else if(action == AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD) {
				View firstVisibleView = getChildAt(0);
				// If the view is fully visible, jump one month back. Otherwise, we'll just jump
				// to the first day of first visible month.
				if(firstVisibleView != null && firstVisibleView.getTop() >= -1) {
					// There's an off-by-one somewhere, so the top of the first visible item will
					// actually be -1 when it's at the exact top.
					day.month--;
					if(day.month == -1) {
						day.month = 11;
						day.year--;
					}
				}
			}
			// Go to that month.
			Utils.tryAccessibilityAnnounce(this, getMonthAndYearString(day));
			goTo(day, true, false, true);
			mPerformingScroll = true;
			return true;
		}
	}
	//
	// Dialog allowing users to select a date.
	//
	public static class DatePickerDialog extends DialogFragment implements View.OnClickListener, DatePickerController {
		private static final String TAG = "DatePickerDialog";
		private static final int UNINITIALIZED = -1;
		private static final int MONTH_AND_DAY_VIEW = 0;
		private static final int YEAR_VIEW = 1;
		private static final String KEY_SELECTED_YEAR = "year";
		private static final String KEY_SELECTED_YEAR_END = "year_end";
		private static final String KEY_SELECTED_MONTH = "month";
		private static final String KEY_SELECTED_MONTH_END = "month_end";
		private static final String KEY_SELECTED_DAY = "day";
		private static final String KEY_SELECTED_DAY_END = "day_end";
		private static final String KEY_LIST_POSITION = "list_position";
		private static final String KEY_LIST_POSITION_END = "list_position_end";
		private static final String KEY_WEEK_START = "week_start";
		private static final String KEY_WEEK_START_END = "week_start_end";
		private static final String KEY_YEAR_START = "year_start";
		private static final String KEY_YEAR_START_END = "year_start_end";
		private static final String KEY_MAX_YEAR = "max_year";
		private static final String KEY_MAX_YEAR_END = "max_year_end";
		private static final String KEY_CURRENT_VIEW = "current_view";
		private static final String KEY_CURRENT_VIEW_END = "current_view_end";
		private static final String KEY_LIST_POSITION_OFFSET = "list_position_offset";
		private static final String KEY_LIST_POSITION_OFFSET_END = "list_position_offset_end";
		private static final String KEY_MIN_DATE = "min_date";
		private static final String KEY_MAX_DATE = "max_date";
		private static final String KEY_HIGHLIGHTED_DAYS = "highlighted_days";
		private static final String KEY_SELECTABLE_DAYS = "selectable_days";
		private static final String KEY_MIN_DATE_END = "min_date_end";
		private static final String KEY_MAX_DATE_END = "max_date_end";
		private static final String KEY_HIGHLIGHTED_DAYS_END = "highlighted_days_end";
		private static final String KEY_SELECTABLE_DAYS_END = "selectable_days_end";
		private static final String KEY_THEME_DARK = "theme_dark";
		private static final String KEY_ACCENT = "accent";
		private static final String KEY_VIBRATE = "vibrate";
		private static final String KEY_DISMISS = "dismiss";
		private static final int DEFAULT_START_YEAR = 1900;
		private static final int DEFAULT_END_YEAR = 2100;
		private static final int ANIMATION_DURATION = 300;
		private static final int ANIMATION_DELAY = 500;
		private static SimpleDateFormat YEAR_FORMAT = new SimpleDateFormat("yyyy", Locale.getDefault());
		private static SimpleDateFormat DAY_FORMAT = new SimpleDateFormat("dd", Locale.getDefault());
		private Calendar mCalendar = Calendar.getInstance();
		private Calendar mCalendarEnd = Calendar.getInstance();
		// @sobolev private OnDateSetListener mCallBack;
		private SLib.EventHandler Handler; // @sobolev
		private HashSet<OnDateChangedListener> mListeners = new HashSet<>();
		private DialogInterface.OnCancelListener mOnCancelListener;
		private DialogInterface.OnDismissListener mOnDismissListener;

		private AccessibleDateAnimator mAnimator;
		private TextView mDayOfWeekView;
		private LinearLayout mMonthAndDayView;
		private TextView mSelectedMonthTextView;
		private TextView mSelectedDayTextView;
		private TextView mYearView;
		private DayPickerView mDayPickerView;
		private YearPickerView mYearPickerView;
		private int mCurrentView = UNINITIALIZED;
		private int mCurrentViewEnd = UNINITIALIZED;
		private int mWeekStart = mCalendar.getFirstDayOfWeek();
		private int mWeekStartEnd = mCalendarEnd.getFirstDayOfWeek();
		private int mMinYear = DEFAULT_START_YEAR;
		private int mMaxYear = DEFAULT_END_YEAR;
		private Calendar mMinDate;
		private Calendar mMaxDate;
		private Calendar[] highlightedDays;
		private Calendar[] selectableDays;
		private Calendar mMinDateEnd;
		private Calendar mMaxDateEnd;
		private Calendar[] highlightedDaysEnd;
		private Calendar[] selectableDaysEnd;
		private boolean mAutoHighlight = false;
		private boolean mThemeDark;
		private int mAccentColor = -1;
		private boolean mVibrate;
		private boolean mDismissOnPause;
		private HapticFeedbackController mHapticFeedbackController;
		private boolean mDelayAnimation = true;
		// Accessibility strings.
		private String mDayPickerDescription;
		private String mSelectDay;
		private String mYearPickerDescription;
		private String mSelectYear;
		private TabHost tabHost;
		private LinearLayout mMonthAndDayViewEnd;
		private TextView mSelectedMonthTextViewEnd;
		private TextView mSelectedDayTextViewEnd;
		private TextView mYearViewEnd;
		private SimpleDayPickerView mDayPickerViewEnd;
		private YearPickerView mYearPickerViewEnd;
		private AccessibleDateAnimator mAnimatorEnd;
		private int tabTag=1;
		private String startTitle;
		private String endTitle;
		//
		// The callback used to indicate the user is done filling in the date.
		//
		/* @sobolev public interface OnDateSetListener {
			//
			// @param view The view associated with this listener.
			// @param year The year that was set.
			// @param monthOfYear The month that was set (0-11) for compatibility with {@link Calendar}.
			// @param dayOfMonth The day of the month that was set.
			//
			void onDateSet(DatePickerDialog view, int year, int monthOfYear, int dayOfMonth,int yearEnd, int monthOfYearEnd, int dayOfMonthEnd);
		}*/
		//
		// The callback used to notify other date picker components of a change in selected date.
		//
		public interface OnDateChangedListener {
			void onDateChanged();
		}
		public DatePickerDialog()
		{
			// Empty constructor required for dialog fragment.
		}

		//
		// @param callBack How the parent is notified that the date is set.
		// @param year The initial year of the dialog.
		// @param monthOfYear The initial month of the dialog.
		// @param dayOfMonth The initial day of the dialog.
		//
		public static DatePickerDialog newInstance(SLib.EventHandler handler, int year, int monthOfYear, int dayOfMonth)
		{
			DatePickerDialog ret = new DatePickerDialog();
			ret.initialize(handler, year, monthOfYear, dayOfMonth);
			return ret;
		}
		//
		// @param callBack How the parent is notified that the date is set.
		// @param year The initial year of the dialog.
		// @param monthOfYear The initial month of the dialog.
		// @param dayOfMonth The initial day of the dialog.
		// @param yearEnd The end year of the dialog.
		// @param montOfYearEnd The end month of the dialog.
		// @param dayOfMonthEnd  The end day of the dialog.
		//
		public static DatePickerDialog newInstance(SLib.EventHandler handler, int year, int monthOfYear, int dayOfMonth,
			int yearEnd, int montOfYearEnd, int dayOfMonthEnd)
		{
			DatePickerDialog ret = new DatePickerDialog();
			ret.initialize(handler, year, monthOfYear, dayOfMonth, yearEnd, montOfYearEnd, dayOfMonthEnd);
			return ret;
		}
		public void initialize(SLib.EventHandler handler, int year, int monthOfYear, int dayOfMonth)
		{
			initialize(handler, year, monthOfYear, dayOfMonth, year, monthOfYear, dayOfMonth);
		}
		public void initialize(SLib.EventHandler handler, int year, int monthOfYear, int dayOfMonth, int yearEnd, int montOfYearEnd, int dayOfMonthEnd)
		{
			Handler = handler;
			mCalendar.set(Calendar.YEAR, year);
			mCalendar.set(Calendar.MONTH, monthOfYear);
			mCalendar.set(Calendar.DAY_OF_MONTH, dayOfMonth);
			mCalendarEnd.set(Calendar.YEAR, yearEnd);
			mCalendarEnd.set(Calendar.MONTH, montOfYearEnd);
			mCalendarEnd.set(Calendar.DAY_OF_MONTH, dayOfMonthEnd);
			mThemeDark = false;
			mAccentColor = -1;
			mVibrate = true;
			mDismissOnPause = false;
		}
		@Override public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
			final Activity activity = getActivity();
			activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
			if(savedInstanceState != null) {
				mCalendar.set(Calendar.YEAR, savedInstanceState.getInt(KEY_SELECTED_YEAR));
				mCalendar.set(Calendar.MONTH, savedInstanceState.getInt(KEY_SELECTED_MONTH));
				mCalendar.set(Calendar.DAY_OF_MONTH, savedInstanceState.getInt(KEY_SELECTED_DAY));
				mCalendarEnd.set(Calendar.YEAR, savedInstanceState.getInt(KEY_SELECTED_YEAR_END));
				mCalendarEnd.set(Calendar.MONTH, savedInstanceState.getInt(KEY_SELECTED_MONTH_END));
				mCalendarEnd.set(Calendar.DAY_OF_MONTH, savedInstanceState.getInt(KEY_SELECTED_DAY_END));
			}
		}
		@Override public void onSaveInstanceState(@NonNull Bundle outState)
		{
			super.onSaveInstanceState(outState);
			outState.putInt(KEY_SELECTED_YEAR, mCalendar.get(Calendar.YEAR));
			outState.putInt(KEY_SELECTED_MONTH, mCalendar.get(Calendar.MONTH));
			outState.putInt(KEY_SELECTED_DAY, mCalendar.get(Calendar.DAY_OF_MONTH));
			outState.putInt(KEY_WEEK_START, mWeekStart);
			outState.putInt(KEY_YEAR_START, mMinYear);
			outState.putInt(KEY_MAX_YEAR, mMaxYear);
			outState.putInt(KEY_CURRENT_VIEW, mCurrentView);
			outState.putInt(KEY_SELECTED_YEAR_END, mCalendarEnd.get(Calendar.YEAR));
			outState.putInt(KEY_SELECTED_MONTH_END, mCalendarEnd.get(Calendar.MONTH));
			outState.putInt(KEY_SELECTED_DAY_END, mCalendarEnd.get(Calendar.DAY_OF_MONTH));
			outState.putInt(KEY_WEEK_START_END, mWeekStartEnd);
			outState.putInt(KEY_YEAR_START_END, mMinYear);
			outState.putInt(KEY_MAX_YEAR_END, mMaxYear);
			outState.putInt(KEY_CURRENT_VIEW_END, mCurrentViewEnd);
			int listPosition = -1;
			int listPositionEnd = -1;
			if(mCurrentView == MONTH_AND_DAY_VIEW||mCurrentViewEnd==MONTH_AND_DAY_VIEW) {
				listPosition = mDayPickerView.getMostVisiblePosition();
				listPositionEnd = mDayPickerViewEnd.getMostVisiblePosition();

			}
			else if(mCurrentView == YEAR_VIEW||mCurrentViewEnd==YEAR_VIEW) {
				listPosition = mYearPickerView.getFirstVisiblePosition();
				listPositionEnd = mYearPickerViewEnd.getFirstVisiblePosition();
				outState.putInt(KEY_LIST_POSITION_OFFSET, mYearPickerView.getFirstPositionOffset());
				outState.putInt(KEY_LIST_POSITION_OFFSET_END, mYearPickerViewEnd.getFirstPositionOffset());
			}
			outState.putInt(KEY_LIST_POSITION, listPosition);
			outState.putInt(KEY_LIST_POSITION_END, listPositionEnd);
			outState.putSerializable(KEY_MIN_DATE, mMinDate);
			outState.putSerializable(KEY_MAX_DATE, mMaxDate);
			outState.putSerializable(KEY_MIN_DATE_END, mMinDateEnd);
			outState.putSerializable(KEY_MAX_DATE_END, mMaxDateEnd);
			outState.putSerializable(KEY_HIGHLIGHTED_DAYS, highlightedDays);
			outState.putSerializable(KEY_SELECTABLE_DAYS, selectableDays);
			outState.putSerializable(KEY_HIGHLIGHTED_DAYS_END, highlightedDaysEnd);
			outState.putSerializable(KEY_SELECTABLE_DAYS_END, selectableDaysEnd);
			outState.putBoolean(KEY_THEME_DARK, mThemeDark);
			outState.putInt(KEY_ACCENT, mAccentColor);
			outState.putBoolean(KEY_VIBRATE, mVibrate);
			outState.putBoolean(KEY_DISMISS, mDismissOnPause);
		}
		@Override public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
		{
			String text_from = "";
			String text_to = "";
			getDialog().getWindow().requestFeature(Window.FEATURE_NO_TITLE);
			View view = inflater.inflate(R.layout.range_date_picker_dialog, null);
			tabHost = (TabHost) view.findViewById(R.id.range_tabHost);
			tabHost.findViewById(R.id.range_tabHost);
			tabHost.setup();

			final Activity activity = getActivity();
			Context _ctx = getContext();
			if(_ctx != null && _ctx instanceof StyloQApp) {
				StyloQApp app_ctx = (StyloQApp)_ctx;
				text_from = app_ctx.GetString("from");
				text_to = app_ctx.GetString("to");
				mDayPickerDescription = app_ctx.GetString("calendar"); // R.string.range_day_picker_description
				mYearPickerDescription = ""; // @todo R.string.range_year_picker_description
				mSelectDay = app_ctx.GetString("selectmonthandday"); // R.string.range_select_day
				mSelectYear = app_ctx.GetString("selectyear"); // R.string.range_select_year
			}
			TabHost.TabSpec startDatePage = tabHost.newTabSpec("start");
			startDatePage.setContent(R.id.start_date_group);
			startDatePage.setIndicator((startTitle != null && !startTitle.isEmpty()) ? startTitle : text_from/*activity.getResources().getString(R.string.range_from)*/);
			TabHost.TabSpec endDatePage = tabHost.newTabSpec("end");
			endDatePage.setContent(R.id.range_end_date_group);
			endDatePage.setIndicator((endTitle!=null&&!endTitle.isEmpty())?endTitle : text_to/*activity.getResources().getString(R.string.range_to)*/);
			tabHost.addTab(startDatePage);
			tabHost.addTab(endDatePage);
			mDayOfWeekView = (TextView) view.findViewById(R.id.range_date_picker_header);
			mMonthAndDayView = (LinearLayout) view.findViewById(R.id.range_date_picker_month_and_day);
			mMonthAndDayViewEnd = (LinearLayout) view.findViewById(R.id.range_date_picker_month_and_day_end);
			mMonthAndDayView.setOnClickListener(this);
			mMonthAndDayViewEnd.setOnClickListener(this);
			mSelectedMonthTextView = (TextView) view.findViewById(R.id.range_date_picker_month);
			mSelectedMonthTextViewEnd = (TextView) view.findViewById(R.id.range_date_picker_month_end);
			mSelectedDayTextView = (TextView) view.findViewById(R.id.range_date_picker_day);
			mSelectedDayTextViewEnd = (TextView) view.findViewById(R.id.range_date_picker_day_end);
			mYearView = (TextView) view.findViewById(R.id.range_date_picker_year);
			mYearViewEnd = (TextView) view.findViewById(R.id.range_date_picker_year_end);
			mYearView.setOnClickListener(this);
			mYearViewEnd.setOnClickListener(this);
			int listPosition = -1;
			int listPositionOffset = 0;
			int listPositionEnd = -1;
			int listPositionOffsetEnd = 0;
			int currentView = MONTH_AND_DAY_VIEW;
			int currentViewEnd = MONTH_AND_DAY_VIEW;
			if(savedInstanceState != null) {
				mWeekStart = savedInstanceState.getInt(KEY_WEEK_START);
				mWeekStartEnd = savedInstanceState.getInt(KEY_WEEK_START_END);
				mMinYear = savedInstanceState.getInt(KEY_YEAR_START);
				mMaxYear = savedInstanceState.getInt(KEY_MAX_YEAR);
				currentView = savedInstanceState.getInt(KEY_CURRENT_VIEW);
				currentViewEnd = savedInstanceState.getInt(KEY_CURRENT_VIEW_END);
				listPosition = savedInstanceState.getInt(KEY_LIST_POSITION);
				listPositionOffset = savedInstanceState.getInt(KEY_LIST_POSITION_OFFSET);
				listPositionEnd = savedInstanceState.getInt(KEY_LIST_POSITION_END);
				listPositionOffsetEnd = savedInstanceState.getInt(KEY_LIST_POSITION_OFFSET_END);
				mMinDate = (Calendar)savedInstanceState.getSerializable(KEY_MIN_DATE);
				mMaxDate = (Calendar)savedInstanceState.getSerializable(KEY_MAX_DATE);
				mMinDateEnd = (Calendar)savedInstanceState.getSerializable(KEY_MIN_DATE_END);
				mMaxDateEnd= (Calendar)savedInstanceState.getSerializable(KEY_MAX_DATE_END);
				highlightedDays = (Calendar[])savedInstanceState.getSerializable(KEY_HIGHLIGHTED_DAYS);
				selectableDays = (Calendar[])savedInstanceState.getSerializable(KEY_SELECTABLE_DAYS);
				highlightedDaysEnd = (Calendar[])savedInstanceState.getSerializable(KEY_HIGHLIGHTED_DAYS_END);
				selectableDaysEnd = (Calendar[])savedInstanceState.getSerializable(KEY_SELECTABLE_DAYS_END);
				mThemeDark = savedInstanceState.getBoolean(KEY_THEME_DARK);
				mAccentColor = savedInstanceState.getInt(KEY_ACCENT);
				mVibrate = savedInstanceState.getBoolean(KEY_VIBRATE);
				mDismissOnPause = savedInstanceState.getBoolean(KEY_DISMISS);
			}
			mDayPickerView = new SimpleDayPickerView(activity, this);
			mYearPickerView = new YearPickerView(activity, this);
			mDayPickerViewEnd = new SimpleDayPickerView(activity, this);
			mYearPickerViewEnd = new YearPickerView(activity, this);
			{
				//Context _ctx = getContext();
				//if(_ctx != null && _ctx instanceof StyloQApp) {
				//	StyloQApp app_ctx = (StyloQApp)_ctx;
				//	mDayPickerDescription = app_ctx.GetString("calendar"); // R.string.range_day_picker_description
				//	mYearPickerDescription = ""; // @todo R.string.range_year_picker_description
				//	mSelectDay = app_ctx.GetString("selectmonthandday"); // R.string.range_select_day
				//	mSelectYear = app_ctx.GetString("selectyear"); // R.string.range_select_year
				//}
				//
				//Resources res = getResources();
				//mDayPickerDescription = res.getString(R.string.range_day_picker_description);
				//mSelectDay = res.getString(R.string.range_select_day);
				//mYearPickerDescription = res.getString(R.string.range_year_picker_description);
				//mSelectYear = res.getString(R.string.range_select_year);
			}
			int bgColorResource = mThemeDark ? R.color.range_date_picker_view_animator_dark_theme : R.color.range_date_picker_view_animator;
			view.setBackgroundColor(ContextCompat.getColor(activity, bgColorResource));
			mAnimator = (AccessibleDateAnimator) view.findViewById(R.id.range_animator);
			mAnimatorEnd = (AccessibleDateAnimator) view.findViewById(R.id.range_animator_end);
			mAnimator.addView(mDayPickerView);
			mAnimator.addView(mYearPickerView);
			mAnimator.setDateMillis(mCalendar.getTimeInMillis());
			// TODO: Replace with animation decided upon by the design team.
			Animation animation = new AlphaAnimation(0.0f, 1.0f);
			animation.setDuration(ANIMATION_DURATION);
			mAnimator.setInAnimation(animation);
			// TODO: Replace with animation decided upon by the design team.
			Animation animation2 = new AlphaAnimation(1.0f, 0.0f);
			animation2.setDuration(ANIMATION_DURATION);
			mAnimator.setOutAnimation(animation2);
			mAnimatorEnd.addView(mDayPickerViewEnd);
			mAnimatorEnd.addView(mYearPickerViewEnd);
			mAnimatorEnd.setDateMillis(mCalendarEnd.getTimeInMillis());
			// TODO: Replace with animation decided upon by the design team.
			Animation animationEnd = new AlphaAnimation(0.0f, 1.0f);
			animationEnd.setDuration(ANIMATION_DURATION);
			mAnimatorEnd.setInAnimation(animation);
			// TODO: Replace with animation decided upon by the design team.
			Animation animation2End = new AlphaAnimation(1.0f, 0.0f);
			animation2End.setDuration(ANIMATION_DURATION);
			mAnimatorEnd.setOutAnimation(animation2);
			Button okButton = (Button) view.findViewById(R.id.range_ok);
			okButton.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v)
				{
					tryVibrate();
					if(Handler != null) {
						/*
						mCallBack.onDateSet(DatePickerDialog.this, mCalendar.get(Calendar.YEAR),
								mCalendar.get(Calendar.MONTH), mCalendar.get(Calendar.DAY_OF_MONTH),mCalendarEnd.get(Calendar.YEAR),
								mCalendarEnd.get(Calendar.MONTH), mCalendarEnd.get(Calendar.DAY_OF_MONTH));
						 */
						SLib.DateRange result = new SLib.DateRange();
						result.Low = new SLib.LDATE(mCalendar);
						result.Upp = new SLib.LDATE(mCalendarEnd);
						Handler.HandleEvent(SLib.EV_DATETIMEPICKERREPLY, this, result);
					}
					dismiss();
				}
			});
			okButton.setTypeface(TypefaceHelper.get(activity, "Roboto-Medium"));
			Button cancelButton = (Button) view.findViewById(R.id.range_cancel);
			cancelButton.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v)
				{
					tryVibrate();
					if(getDialog() != null)
						getDialog().cancel();
				}
			});
			cancelButton.setTypeface(TypefaceHelper.get(activity,"Roboto-Medium"));
			cancelButton.setVisibility(isCancelable() ? View.VISIBLE : View.GONE);
			//If an accent color has not been set manually, try and get it from the context
			if(mAccentColor == -1) {
				int accentColor = Utils.getAccentColorFromThemeIfAvailable(getActivity());
				if(accentColor != -1) {
					mAccentColor = accentColor;
				}
			}
			if(mAccentColor != -1) {
				if(mDayOfWeekView != null)
					mDayOfWeekView.setBackgroundColor(Utils.darkenColor(mAccentColor));
				view.findViewById(R.id.range_day_picker_selected_date_layout).setBackgroundColor(mAccentColor);
				view.findViewById(R.id.range_day_picker_selected_date_layout_end).setBackgroundColor(mAccentColor);
				okButton.setTextColor(mAccentColor);
				cancelButton.setTextColor(mAccentColor);
				mYearPickerView.setAccentColor(mAccentColor);
				mDayPickerView.setAccentColor(mAccentColor);
				mYearPickerViewEnd.setAccentColor(mAccentColor);
				mDayPickerViewEnd.setAccentColor(mAccentColor);
			}
			updateDisplay(false);
			setCurrentView(currentView);
			if(listPosition != -1) {
				if(currentView == MONTH_AND_DAY_VIEW) {
					mDayPickerView.postSetSelection(listPosition);
				}
				else if(currentView == YEAR_VIEW) {
					mYearPickerView.postSetSelectionFromTop(listPosition, listPositionOffset);
				}
			}
			if(listPositionEnd != -1) {
				if(currentViewEnd == MONTH_AND_DAY_VIEW) {
					mDayPickerViewEnd.postSetSelection(listPositionEnd);
				}
				else if(currentViewEnd == YEAR_VIEW) {
					mYearPickerViewEnd.postSetSelectionFromTop(listPositionEnd, listPositionOffsetEnd);
				}
			}
			mHapticFeedbackController = new HapticFeedbackController(activity);
			tabHost.setOnTabChangedListener(new TabHost.OnTabChangeListener() {
				@Override public void onTabChanged(String tabId) {
					MonthAdapter.CalendarDay calendarDay;
					if(tabId.equals("start")){
						calendarDay = new MonthAdapter.CalendarDay(mCalendar.getTimeInMillis());
						mDayPickerView.goTo(calendarDay,true,true,false);
					}
					else{
						calendarDay = new MonthAdapter.CalendarDay(mCalendarEnd.getTimeInMillis());
						mDayPickerViewEnd.goTo(calendarDay,true,true,false);
					}
				}
			});
			return view;
		}
		@Override public void onResume()
		{
			super.onResume();
			mHapticFeedbackController.start();
		}
		@Override public void onPause()
		{
			super.onPause();
			mHapticFeedbackController.stop();
			if(mDismissOnPause) dismiss();
		}
		@Override public void onCancel(DialogInterface dialog)
		{
			super.onCancel(dialog);
			if(mOnCancelListener != null) mOnCancelListener.onCancel(dialog);
		}
		@Override public void onDismiss(DialogInterface dialog)
		{
			super.onDismiss(dialog);
			if(mOnDismissListener != null) mOnDismissListener.onDismiss(dialog);
		}
		//
		// Get whether auto highlighting is turned on or not
		// @return true if on, false otherwise
		//
		@SuppressWarnings("unused") public boolean isAutoHighlight()
		{
			return mAutoHighlight;
		}
		//
		// If set to true, all days between {@link #highlightedDays} and {@link #highlightedDaysEnd} will be highlighted.
		// This will reset manually inserted days to highlight using {@link #setHighlightedDays(Calendar[], Calendar[])}
		// @param autoHighlight Set true to turn on auto highlighting, false otherwise
		//
		@SuppressWarnings("unused") public void setAutoHighlight(boolean autoHighlight)
		{
			this.mAutoHighlight = autoHighlight;
			if(autoHighlight) {
				calculateHighlightedDays();
			}
			else {
				highlightedDays = null;
				highlightedDaysEnd = null;
			}
		}
		private void setCurrentView(final int viewIndex)
		{
			long millis = mCalendar.getTimeInMillis();
			long millisEnd = mCalendarEnd.getTimeInMillis();
			switch(viewIndex) {
				case MONTH_AND_DAY_VIEW:
					ObjectAnimator pulseAnimator = Utils.getPulseAnimator(mMonthAndDayView, 0.9f,1.05f);
					ObjectAnimator pulseAnimatorTwo = Utils.getPulseAnimator(mMonthAndDayViewEnd, 0.9f,1.05f);
					if(mDelayAnimation) {
						pulseAnimator.setStartDelay(ANIMATION_DELAY);
						pulseAnimatorTwo.setStartDelay(ANIMATION_DELAY);
						mDelayAnimation = false;
					}
					mDayPickerView.onDateChanged();
					if(mCurrentView != viewIndex) {
						mMonthAndDayView.setSelected(true);
						mMonthAndDayViewEnd.setSelected(true);
						mYearView.setSelected(false);
						mYearViewEnd.setSelected(false);
						mAnimator.setDisplayedChild(MONTH_AND_DAY_VIEW);
						mAnimatorEnd.setDisplayedChild(MONTH_AND_DAY_VIEW);
						mCurrentView = viewIndex;
					}
					pulseAnimator.start();
					pulseAnimatorTwo.start();

					int flags = DateUtils.FORMAT_SHOW_DATE;
					String dayString = DateUtils.formatDateTime(getActivity(), millis, flags);
					String dayStringEnd = DateUtils.formatDateTime(getActivity(), millisEnd, flags);
					mAnimator.setContentDescription(mDayPickerDescription+": "+dayString);
					mAnimatorEnd.setContentDescription(mDayPickerDescription+": "+dayStringEnd);
					Utils.tryAccessibilityAnnounce(mAnimator, mSelectDay);
					Utils.tryAccessibilityAnnounce(mAnimatorEnd, mSelectDay);
					break;
				case YEAR_VIEW:
					pulseAnimator = Utils.getPulseAnimator(mYearView, 0.85f, 1.1f);
					pulseAnimatorTwo = Utils.getPulseAnimator(mYearViewEnd, 0.85f, 1.1f);
					if(mDelayAnimation) {
						pulseAnimator.setStartDelay(ANIMATION_DELAY);
						pulseAnimatorTwo.setStartDelay(ANIMATION_DELAY);
						mDelayAnimation = false;
					}
					mYearPickerView.onDateChanged();
					mYearPickerViewEnd.onDateChanged();
					if(mCurrentView != viewIndex) {
						mMonthAndDayView.setSelected(false);
						mYearView.setSelected(true);
						mAnimator.setDisplayedChild(YEAR_VIEW);
						mCurrentView = viewIndex;

						mMonthAndDayViewEnd.setSelected(false);
						mYearViewEnd.setSelected(true);
						mAnimatorEnd.setDisplayedChild(YEAR_VIEW);
						mCurrentViewEnd = viewIndex;
					}
					pulseAnimator.start();
					pulseAnimatorTwo.start();

					CharSequence yearString = YEAR_FORMAT.format(millis);
					CharSequence yearStringEnd = YEAR_FORMAT.format(millisEnd);
					mAnimator.setContentDescription(mYearPickerDescription+": "+yearString);
					mAnimatorEnd.setContentDescription(mYearPickerDescription+": "+yearStringEnd);
					Utils.tryAccessibilityAnnounce(mAnimator, mSelectYear);
					Utils.tryAccessibilityAnnounce(mAnimatorEnd, mSelectYear);
					break;
			}
		}
		private void updateDisplay(boolean announce)
		{
			if(mDayOfWeekView != null) {
				mDayOfWeekView.setText(mCalendar.getDisplayName(Calendar.DAY_OF_WEEK, Calendar.LONG, Locale.getDefault()).toUpperCase(Locale.getDefault()));
			}
			mSelectedMonthTextView.setText(mCalendar.getDisplayName(Calendar.MONTH, Calendar.SHORT, Locale.getDefault()).toUpperCase(Locale.getDefault()));
			mSelectedMonthTextViewEnd.setText(mCalendarEnd.getDisplayName(Calendar.MONTH, Calendar.SHORT, Locale.getDefault()).toUpperCase(Locale.getDefault()));
			mSelectedDayTextView.setText(DAY_FORMAT.format(mCalendar.getTime()));
			mSelectedDayTextViewEnd.setText(DAY_FORMAT.format(mCalendarEnd.getTime()));
			mYearView.setText(YEAR_FORMAT.format(mCalendar.getTime()));
			mYearViewEnd.setText(YEAR_FORMAT.format(mCalendarEnd.getTime()));
			// Accessibility.
			long millis = mCalendar.getTimeInMillis();
			long millisEnd = mCalendarEnd.getTimeInMillis();
			mAnimator.setDateMillis(millis);
			mAnimatorEnd.setDateMillis(millisEnd);
			int flags = DateUtils.FORMAT_SHOW_DATE | DateUtils.FORMAT_NO_YEAR;
			String monthAndDayText = DateUtils.formatDateTime(getActivity(), millis, flags);
			String monthAndDayTextEnd = DateUtils.formatDateTime(getActivity(), millisEnd, flags);
			mMonthAndDayView.setContentDescription(monthAndDayText);
			mMonthAndDayViewEnd.setContentDescription(monthAndDayTextEnd);
			if(announce) {
				flags = DateUtils.FORMAT_SHOW_DATE | DateUtils.FORMAT_SHOW_YEAR;
				String fullDateText = DateUtils.formatDateTime(getActivity(), millis, flags);
				String fullDateTextEnd = DateUtils.formatDateTime(getActivity(), millisEnd, flags);
				Utils.tryAccessibilityAnnounce(mAnimator, fullDateText);
				Utils.tryAccessibilityAnnounce(mAnimatorEnd, fullDateTextEnd);
			}
		}
		//
		// Set whether the device should vibrate when touching fields
		// @param vibrate true if the device should vibrate when touching a field
		//
		public void vibrate(boolean vibrate)
		{
			mVibrate = vibrate;
		}
		//
		// Set whether the picker should dismiss itself when being paused or whether it should try to survive an orientation change
		// @param dismissOnPause true if the dialog should dismiss itself when it's pausing
		//
		public void dismissOnPause(boolean dismissOnPause)
		{
			mDismissOnPause = dismissOnPause;
		}
		//
		// Set whether the dark theme should be used
		// @param themeDark true if the dark theme should be used, false if the default theme should be used
		//
		public void setThemeDark(boolean themeDark)
		{
			mThemeDark = themeDark;
		}
		//
		// Returns true when the dark theme should be used
		// @return true if the dark theme should be used, false if the default theme should be used
		//
		@Override public boolean isThemeDark() { return mThemeDark; }
		//
		// Set the accent color of this dialog
		// @param accentColor the accent color you want
		//
		public void setAccentColor(int accentColor) { mAccentColor = accentColor; }
		//
		// Get the accent color of this dialog
		// @return accent color
		//
		public int getAccentColor() { return mAccentColor; }
		@SuppressWarnings("unused") public void setFirstDayOfWeek(int startOfWeek,int startWeekEnd)
		{
			if(startOfWeek < Calendar.SUNDAY || startOfWeek > Calendar.SATURDAY) {
				throw new IllegalArgumentException("Value must be between Calendar.SUNDAY and " + "Calendar.SATURDAY");
			}
			mWeekStart = startOfWeek;
			mWeekStartEnd = startWeekEnd;
			if(mDayPickerView != null)
				mDayPickerView.onChange();
			if(mDayPickerViewEnd != null)
				mDayPickerViewEnd.onChange();
		}
		@SuppressWarnings("unused") public void setYearRange(int startYear, int endYear)
		{
			if(endYear < startYear) {
				throw new IllegalArgumentException("Year end must be larger than or equal to year start");
			}
			mMinYear = startYear;
			mMaxYear = endYear;
			if(mDayPickerView != null&&mDayPickerViewEnd!=null) {
				mDayPickerView.onChange();
				mDayPickerViewEnd.onChange();
			}
		}
		//
		// Sets the minimal date supported by this DatePicker. Dates before (but not including) the
		// specified date will be disallowed from being selected.
		// @param calendar a Calendar object set to the year, month, day desired as the mindate.
		//
		@SuppressWarnings("unused") public void setMinDate(Calendar calendar)
		{
			mMinDate = calendar;
			if(mDayPickerView != null&&mDayPickerViewEnd!=null) {
				mDayPickerView.onChange();
				mDayPickerViewEnd.onChange();
			}
		}
		/**
		 * @return The minimal date supported by this DatePicker. Null if it has not been set.
		 */
		@Override public Calendar getMinDate() { return mMinDate; }
		/**
		 * Sets the minimal date supported by this DatePicker. Dates after (but not including) the
		 * specified date will be disallowed from being selected.
		 * @param calendar a Calendar object set to the year, month, day desired as the maxdate.
		 */
		@SuppressWarnings("unused") public void setMaxDate(Calendar calendar)
		{
			mMaxDate = calendar;
			if(mDayPickerView != null&&mDayPickerViewEnd!=null) {
				mDayPickerView.onChange();
				mDayPickerViewEnd.onChange();
			}
		}
		//
		// @return The maximal date supported by this DatePicker. Null if it has not been set.
		//
		@Override public Calendar getMaxDate() { return mMaxDate; }
		//
		// Sets an array of dates which should be highlighted when the picker is drawn
		// This will turn off auto highlighting.
		// @param highlightedDays an Array of Calendar objects containing the dates to be highlighted
		//
		@SuppressWarnings("unused") public void setHighlightedDays(Calendar[] highlightedDays,Calendar[] highlightedDaysEnd)
		{
			mAutoHighlight = false;
			// Sort the array to optimize searching over it later on
			Arrays.sort(highlightedDays);
			Arrays.sort(highlightedDaysEnd);
			this.highlightedDays = highlightedDays;
			this.highlightedDaysEnd = highlightedDaysEnd;
		}
		//
		// @return The list of dates, as Calendar Objects, which should be highlighted. null is no dates should be highlighted
		//
		@Override public Calendar[] getHighlightedDays() { return highlightedDays; }
		//
		// Set's a list of days which are the only valid selections.
		// Setting this value will take precedence over using setMinDate() and setMaxDate()
		// @param selectableDays an Array of Calendar Objects containing the selectable dates
		//
		@SuppressWarnings("unused") public void setSelectableDays(Calendar[] selectableDays)
		{
			// Sort the array to optimize searching over it later on
			Arrays.sort(selectableDays);
			this.selectableDays = selectableDays;
		}
		@SuppressWarnings("unused") public void setSelectableDaysEnd(Calendar[] selectableDaysEnd)
		{
			// Sort the array to optimize searching over it later on
			Arrays.sort(selectableDaysEnd);
			this.selectableDaysEnd = selectableDaysEnd;
		}
		//
		// @return an Array of Calendar objects containing the list with selectable items. null if no restriction is set
		//
		@Override public Calendar[] getSelectableDays() { return selectableDays; }
		// @sobolev @SuppressWarnings("unused") public void setOnDateSetListener(OnDateSetListener listener) { mCallBack = listener; }
		@SuppressWarnings("unused") public void setOnCancelListener(DialogInterface.OnCancelListener onCancelListener)
		{
			mOnCancelListener = onCancelListener;
		}
		@SuppressWarnings("unused") public void setOnDismissListener(DialogInterface.OnDismissListener onDismissListener)
		{
			mOnDismissListener = onDismissListener;
		}
		// If the newly selected month / year does not contain the currently selected day number,
		// change the selected day number to the last day of the selected month or year.
		//      e.g. Switching from Mar to Apr when Mar 31 is selected -> Apr 30
		//      e.g. Switching from 2012 to 2013 when Feb 29, 2012 is selected -> Feb 28, 2013
		private void adjustDayInMonthIfNeeded( Calendar calendar )
		{
			int day = calendar.get(Calendar.DAY_OF_MONTH);
			int daysInMonth = calendar.getActualMaximum(Calendar.DAY_OF_MONTH);
			if(day > daysInMonth) {
				calendar.set(Calendar.DAY_OF_MONTH, daysInMonth);
			}
		}
		@Override public void onClick(View v)
		{
			tryVibrate();
			if(v.getId() == R.id.range_date_picker_year ||v.getId() == R.id.range_date_picker_year_end) {
				setCurrentView(YEAR_VIEW);
			}
			else if(v.getId() == R.id.range_date_picker_month_and_day ||v.getId() == R.id.range_date_picker_month_and_day_end) {
				setCurrentView(MONTH_AND_DAY_VIEW);
			}
		}
		@Override public void onYearSelected(int year)
		{
			adjustDayInMonthIfNeeded(mCalendar);
			adjustDayInMonthIfNeeded(mCalendarEnd);
			if(tabHost.getCurrentTab()==0){
				mCalendar.set(Calendar.YEAR, year);
			}
			else {
				mCalendarEnd.set(Calendar.YEAR, year);
			}
			updatePickers();
			setCurrentView(MONTH_AND_DAY_VIEW);
			updateDisplay(true);
		}
		@Override public void onDayOfMonthSelected(int year, int month, int day)
		{
			if(tabHost.getCurrentTab()==0){
				mCalendar.set(Calendar.YEAR, year);
				mCalendar.set(Calendar.MONTH, month);
				mCalendar.set(Calendar.DAY_OF_MONTH, day);
			}
			else{
				mCalendarEnd.set(Calendar.YEAR, year);
				mCalendarEnd.set(Calendar.MONTH, month);
				mCalendarEnd.set(Calendar.DAY_OF_MONTH, day);
			}
			if(mAutoHighlight) {
				calculateHighlightedDays();
			}
			updatePickers();
			updateDisplay(true);
		}
		private void calculateHighlightedDays()
		{
			int numDays = (int)Math.round((mCalendarEnd.getTimeInMillis() - mCalendar.getTimeInMillis()) / 86400000d);
			// In case user chooses an end day before the start day.
			int dir = 1;
			if(numDays < 0) {
				dir = -1;
			}
			numDays = Math.abs(numDays);
			// +1 to account for the end day which should be highlighted as well
			highlightedDays = new Calendar[numDays+1];
			for(int i = 0; i < numDays; i++) {
				highlightedDays[i] = new GregorianCalendar(mCalendar.get(Calendar.YEAR), mCalendar.get(Calendar.MONTH),
						mCalendar.get(Calendar.DAY_OF_MONTH));
				highlightedDays[i].add(Calendar.DAY_OF_MONTH, i*dir);
			}
			highlightedDays[numDays] = mCalendarEnd;
			highlightedDaysEnd = highlightedDays;
		}
		private void updatePickers()
		{
			for(OnDateChangedListener listener : mListeners)
				listener.onDateChanged();
		}
		@Override public MonthAdapter.CalendarDay getSelectedDay()
		{
			return (tabHost.getCurrentTab() == 0) ? new MonthAdapter.CalendarDay(mCalendar) : new MonthAdapter.CalendarDay(mCalendarEnd);
		}
		@Override public int getMinYear()
		{
			if(selectableDays != null)
				return selectableDays[0].get(Calendar.YEAR);
			// Ensure no years can be selected outside of the given minimum date
			return mMinDate != null && mMinDate.get(Calendar.YEAR) > mMinYear ? mMinDate.get(Calendar.YEAR) : mMinYear;
		}
		@Override public int getMaxYear()
		{
			if(selectableDays != null)
				return selectableDays[selectableDays.length-1].get(Calendar.YEAR);
			// Ensure no years can be selected outside of the given maximum date
			return mMaxDate != null && mMaxDate.get(Calendar.YEAR) < mMaxYear ? mMaxDate.get(Calendar.YEAR) : mMaxYear;
		}
		@Override public int getFirstDayOfWeek() { return mWeekStart; }
		@Override public void registerOnDateChangedListener(OnDateChangedListener listener)
		{
			mListeners.add(listener);
		}
		@Override public void unregisterOnDateChangedListener(OnDateChangedListener listener)
		{
			mListeners.remove(listener);
		}
		@Override public void tryVibrate()
		{
			if(mVibrate)
				mHapticFeedbackController.tryVibrate();
		}
		//
		// setStartTitle
		// @param String the title to display for start panel
		//
		public void setStartTitle(String startTitle) { this.startTitle = startTitle; }
		//
		// setEndTitle
		// @param String the title to display for end panel
		//
		public void setEndTitle(String endTitle) { this.endTitle = endTitle; }
	}
	//
	// A text view which, when pressed or activated, displays a colored circle around the text.
	//
	public static class TextViewWithCircularIndicator extends /*TextView*/androidx.appcompat.widget.AppCompatTextView {
		private static final int SELECTED_CIRCLE_ALPHA = 255;
		Paint mCirclePaint = new Paint();
		private final int mRadius;
		private int mCircleColor;
		//private final String mItemIsSelectedText;
		private boolean mDrawCircle;

		public TextViewWithCircularIndicator(Context ctx, AttributeSet attrs)
		{
			super(ctx, attrs);
			Resources res = ctx.getResources();
			mCircleColor = res.getColor(R.color.range_accent_color);
			mRadius = res.getDimensionPixelOffset(R.dimen.range_month_select_circle_radius);
			//mItemIsSelectedText = ctx.getResources().getString(R.string.range_item_is_selected);
			init();
		}
		private void init()
		{
			mCirclePaint.setFakeBoldText(true);
			mCirclePaint.setAntiAlias(true);
			mCirclePaint.setColor(mCircleColor);
			mCirclePaint.setTextAlign(Paint.Align.CENTER);
			mCirclePaint.setStyle(Paint.Style.FILL);
			mCirclePaint.setAlpha(SELECTED_CIRCLE_ALPHA);
		}
		public void setAccentColor(int color)
		{
			mCircleColor = color;
			mCirclePaint.setColor(mCircleColor);
			setTextColor(createTextColor(color));
		}
		//
		// Programmatically set the color state list (see range_date_picker_year_selector)
		// @param accentColor pressed state text color
		// @return ColorStateList with pressed state
		//
		private ColorStateList createTextColor(int accentColor)
		{
			int[][] states = new int[][]{ new int[]{android.R.attr.state_pressed}/*pressed*/,
					new int[]{android.R.attr.state_selected}/*selected*/, new int[]{} };
			int[] colors = new int[] { accentColor, Color.WHITE, Color.BLACK };
			return new ColorStateList(states, colors);
		}
		public void drawIndicator(boolean drawCircle)
		{
			mDrawCircle = drawCircle;
		}
		@Override public void onDraw(@NonNull Canvas canvas)
		{
			if(mDrawCircle) {
				final int width = getWidth();
				final int height = getHeight();
				int radius = Math.min(width, height) / 2;
				canvas.drawCircle(width / 2, height / 2, radius, mCirclePaint);
			}
			setSelected(mDrawCircle);
			super.onDraw(canvas);
		}
		@Override public CharSequence getContentDescription()
		{
			CharSequence itemText = getText();
			//mItemIsSelectedText = ctx.getResources().getString(R.string.range_item_is_selected);
			String fmt_buf = "%1$s selected"; // @todo (to resource)
			return mDrawCircle ? String.format(/*mItemIsSelectedText*/fmt_buf, itemText) : itemText;
		}
	}
	//
	// Displays a selectable list of years.
	//
	public static class YearPickerView extends ListView implements AdapterView.OnItemClickListener, DatePickerDialog.OnDateChangedListener {
		private static final String TAG = "YearPickerView";

		private final DatePickerController mController;
		private YearAdapter mAdapter;
		private int mViewSize;
		private int mChildSize;
		private TextViewWithCircularIndicator mSelectedView;
		private int mAccentColor;
		//
		// @param context
		//
		public YearPickerView(Context context, DatePickerController controller)
		{
			super(context);
			mController = controller;
			mController.registerOnDateChangedListener(this);
			ViewGroup.LayoutParams frame = new ViewGroup.LayoutParams(LayoutParams.MATCH_PARENT,
					LayoutParams.WRAP_CONTENT);
			setLayoutParams(frame);
			Resources res = context.getResources();
			mViewSize = res.getDimensionPixelOffset(R.dimen.range_date_picker_view_animator_height);
			mChildSize = res.getDimensionPixelOffset(R.dimen.range_year_label_height);
			setVerticalFadingEdgeEnabled(true);
			setFadingEdgeLength(mChildSize / 3);
			init(context);
			setOnItemClickListener(this);
			setSelector(new StateListDrawable());
			setDividerHeight(0);
			onDateChanged();
		}
		private void init(Context context)
		{
			ArrayList<String> years = new ArrayList<String>();
			for(int year = mController.getMinYear(); year <= mController.getMaxYear(); year++) {
				years.add(String.format("%d", year));
			}
			mAdapter = new YearAdapter(context, R.layout.range_year_label_text_view, years);
			setAdapter(mAdapter);
		}
		public void setAccentColor(int accentColor)
		{
			mAccentColor = accentColor;
		}
		@Override public void onItemClick(AdapterView<?> parent, View view, int position, long id)
		{
			mController.tryVibrate();
			TextViewWithCircularIndicator clickedView = (TextViewWithCircularIndicator) view;
			if(clickedView != null) {
				if(clickedView != mSelectedView) {
					if(mSelectedView != null) {
						mSelectedView.drawIndicator(false);
						mSelectedView.requestLayout();
					}
					clickedView.drawIndicator(true);
					clickedView.requestLayout();
					mSelectedView = clickedView;
				}
				mController.onYearSelected(getYearFromTextView(clickedView));
				mAdapter.notifyDataSetChanged();
			}
		}
		private static int getYearFromTextView(TextView view) { return Integer.valueOf(view.getText().toString()); }
		private class YearAdapter extends ArrayAdapter<String> {
			public YearAdapter(Context context, int resource, List<String> objects)
			{
				super(context, resource, objects);
			}
			@Override public View getView(int position, View convertView, ViewGroup parent)
			{
				TextViewWithCircularIndicator v = (TextViewWithCircularIndicator)super.getView(position, convertView, parent);
				v.setAccentColor(mAccentColor);
				v.requestLayout();
				int year = getYearFromTextView(v);
				boolean selected = mController.getSelectedDay().year == year;
				v.drawIndicator(selected);
				if(selected) {
					mSelectedView = v;
				}
				return v;
			}
		}
		public void postSetSelectionCentered(final int position)
		{
			postSetSelectionFromTop(position, mViewSize / 2 - mChildSize / 2);
		}
		public void postSetSelectionFromTop(final int position, final int offset) {
			post(new Runnable() {
				@Override public void run()
				{
					setSelectionFromTop(position, offset);
					requestLayout();
				}
			});
		}
		public int getFirstPositionOffset()
		{
			final View firstChild = getChildAt(0);
			return (firstChild == null) ? 0 : firstChild.getTop();
		}
		@Override public void onDateChanged()
		{
			mAdapter.notifyDataSetChanged();
			postSetSelectionCentered(mController.getSelectedDay().year - mController.getMinYear());
		}
		@Override public void onInitializeAccessibilityEvent(AccessibilityEvent event)
		{
			super.onInitializeAccessibilityEvent(event);
			if(event.getEventType() == AccessibilityEvent.TYPE_VIEW_SCROLLED) {
				event.setFromIndex(0);
				event.setToIndex(0);
			}
		}
	}
	//
	// Draw the two smaller AM and PM circles next to where the larger circle will be.
	//
	public static class AmPmCirclesView extends View {
		private static final String TAG = "AmPmCirclesView";
		// Alpha level for selected circle.
		private static final int SELECTED_ALPHA = Utils.SELECTED_ALPHA;
		private static final int SELECTED_ALPHA_THEME_DARK = Utils.SELECTED_ALPHA_THEME_DARK;
		private final Paint mPaint = new Paint();
		private int mSelectedAlpha;
		private int mTouchedColor;
		private int mUnselectedColor;
		private int mAmPmTextColor;
		private int mAmPmSelectedTextColor;
		private int mSelectedColor;
		private float mCircleRadiusMultiplier;
		private float mAmPmCircleRadiusMultiplier;
		private String mAmText;
		private String mPmText;
		private boolean mIsInitialized;
		private static final int AM = TimePickerDialog.AM;
		private static final int PM = TimePickerDialog.PM;
		private boolean mDrawValuesReady;
		private int mAmPmCircleRadius;
		private int mAmXCenter;
		private int mPmXCenter;
		private int mAmPmYCenter;
		private int mAmOrPm;
		private int mAmOrPmPressed;
		public AmPmCirclesView(Context context)
		{
			super(context);
			mIsInitialized = false;
		}
		public void initialize(Context context, int amOrPm)
		{
			if(mIsInitialized) {
				Log.e(TAG, "AmPmCirclesView may only be initialized once.");
				return;
			}
			Resources res = context.getResources();
			mUnselectedColor = res.getColor(R.color.White);
			mSelectedColor = res.getColor(R.color.range_accent_color);
			mTouchedColor = res.getColor(R.color.range_accent_color_dark);
			mAmPmTextColor = res.getColor(R.color.range_ampm_text_color);
			mAmPmSelectedTextColor = res.getColor(R.color.White);
			mSelectedAlpha = SELECTED_ALPHA;
			String typefaceFamily = "sans-serif";//res.getString(R.string.range_sans_serif);
			Typeface tf = Typeface.create(typefaceFamily, Typeface.NORMAL);
			mPaint.setTypeface(tf);
			mPaint.setAntiAlias(true);
			mPaint.setTextAlign(Paint.Align.CENTER);
			mCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_circle_radius_multiplier));
			mAmPmCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_ampm_circle_radius_multiplier));
			String[] amPmTexts = new DateFormatSymbols().getAmPmStrings();
			mAmText = amPmTexts[0];
			mPmText = amPmTexts[1];
			setAmOrPm(amOrPm);
			mAmOrPmPressed = -1;
			mIsInitialized = true;
		}
		/* package */ void setTheme(Context context, boolean themeDark)
		{
			Resources res = context.getResources();
			if(themeDark) {
				mUnselectedColor = res.getColor(R.color.range_circle_background_dark_theme);
				mSelectedColor = res.getColor(R.color.range_red);
				mAmPmTextColor = res.getColor(R.color.White);
				mSelectedAlpha = SELECTED_ALPHA_THEME_DARK;
			}
			else {
				mUnselectedColor = res.getColor(R.color.White);
				mSelectedColor = res.getColor(R.color.range_accent_color);
				mAmPmTextColor = res.getColor(R.color.range_ampm_text_color);
				mSelectedAlpha = SELECTED_ALPHA;
			}
		}
		public void setAccentColor(int accentColor) { mSelectedColor = accentColor; }
		public void setAmOrPm(int amOrPm) { mAmOrPm = amOrPm; }
		public void setAmOrPmPressed(int amOrPmPressed) { mAmOrPmPressed = amOrPmPressed; }
		//
		// Calculate whether the coordinates are touching the AM or PM circle.
		//
		public int getIsTouchingAmOrPm(float xCoord, float yCoord)
		{
			if(!mDrawValuesReady) {
				return -1;
			}
			int squaredYDistance = (int) ((yCoord - mAmPmYCenter)*(yCoord - mAmPmYCenter));
			int distanceToAmCenter = (int) Math.sqrt((xCoord - mAmXCenter)*(xCoord - mAmXCenter) + squaredYDistance);
			if(distanceToAmCenter <= mAmPmCircleRadius) {
				return AM;
			}
			int distanceToPmCenter = (int) Math.sqrt((xCoord - mPmXCenter)*(xCoord - mPmXCenter) + squaredYDistance);
			if(distanceToPmCenter <= mAmPmCircleRadius) {
				return PM;
			}
			// Neither was close enough.
			return -1;
		}
		@Override public void onDraw(Canvas canvas)
		{
			int viewWidth = getWidth();
			if(viewWidth == 0 || !mIsInitialized) {
				return;
			}
			if(!mDrawValuesReady) {
				int layoutXCenter = getWidth() / 2;
				int layoutYCenter = getHeight() / 2;
				int circleRadius = (int) (Math.min(layoutXCenter, layoutYCenter) * mCircleRadiusMultiplier);
				mAmPmCircleRadius = (int) (circleRadius * mAmPmCircleRadiusMultiplier);
				layoutYCenter += mAmPmCircleRadius*0.75;
				int textSize = mAmPmCircleRadius * 3/4;
				mPaint.setTextSize(textSize);
				// Line up the vertical center of the AM/PM circles with the bottom of the main circle.
				mAmPmYCenter = layoutYCenter - mAmPmCircleRadius / 2 + circleRadius;
				// Line up the horizontal edges of the AM/PM circles with the horizontal edges
				// of the main circle.
				mAmXCenter = layoutXCenter - circleRadius + mAmPmCircleRadius;
				mPmXCenter = layoutXCenter + circleRadius - mAmPmCircleRadius;
				mDrawValuesReady = true;
			}
			// We'll need to draw either a lighter blue (for selection), a darker blue (for touching)
			// or white (for not selected).
			int amColor = mUnselectedColor;
			int amAlpha = 255;
			int amTextColor = mAmPmTextColor;
			int pmColor = mUnselectedColor;
			int pmAlpha = 255;
			int pmTextColor = mAmPmTextColor;
			if(mAmOrPm == AM) {
				amColor = mSelectedColor;
				amAlpha = mSelectedAlpha;
				amTextColor = mAmPmSelectedTextColor;
			}
			else if(mAmOrPm == PM) {
				pmColor = mSelectedColor;
				pmAlpha = mSelectedAlpha;
				pmTextColor = mAmPmSelectedTextColor;
			}
			if(mAmOrPmPressed == AM) {
				amColor = mTouchedColor;
				amAlpha = mSelectedAlpha;
			}
			else if(mAmOrPmPressed == PM) {
				pmColor = mTouchedColor;
				pmAlpha = mSelectedAlpha;
			}
			// Draw the two circles.
			mPaint.setColor(amColor);
			mPaint.setAlpha(amAlpha);
			canvas.drawCircle(mAmXCenter, mAmPmYCenter, mAmPmCircleRadius, mPaint);
			mPaint.setColor(pmColor);
			mPaint.setAlpha(pmAlpha);
			canvas.drawCircle(mPmXCenter, mAmPmYCenter, mAmPmCircleRadius, mPaint);
			// Draw the AM/PM texts on top.
			mPaint.setColor(amTextColor);
			int textYCenter = mAmPmYCenter - (int) (mPaint.descent() + mPaint.ascent()) / 2;
			canvas.drawText(mAmText, mAmXCenter, textYCenter, mPaint);
			mPaint.setColor(pmTextColor);
			canvas.drawText(mPmText, mPmXCenter, textYCenter, mPaint);
		}
	}
	//
	// Draws a simple white circle on which the numbers will be drawn.
	//
	public static class CircleView extends View {
		private static final String TAG = "CircleView";
		private final Paint mPaint = new Paint();
		private boolean mIs24HourMode;
		private int mCircleColor;
		private int mDotColor;
		private float mCircleRadiusMultiplier;
		private float mAmPmCircleRadiusMultiplier;
		private boolean mIsInitialized;
		private boolean mDrawValuesReady;
		private int mXCenter;
		private int mYCenter;
		private int mCircleRadius;

		public CircleView(Context context)
		{
			super(context);
			Resources res = context.getResources();
			mCircleColor = res.getColor(R.color.range_circle_color);
			mDotColor = res.getColor(R.color.range_accent_color);
			mPaint.setAntiAlias(true);
			mIsInitialized = false;
		}
		public void initialize(Context context, boolean is24HourMode)
		{
			if(mIsInitialized) {
				Log.e(TAG, "CircleView may only be initialized once.");
				return;
			}
			Resources res = context.getResources();
			mIs24HourMode = is24HourMode;
			if(is24HourMode) {
				mCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_circle_radius_multiplier_24HourMode));
			}
			else {
				mCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_circle_radius_multiplier));
				mAmPmCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_ampm_circle_radius_multiplier));
			}
			mIsInitialized = true;
		}
		/* package */ void setTheme(Context context, boolean dark)
		{
			Resources res = context.getResources();
			mCircleColor = res.getColor(dark ? R.color.range_circle_background_dark_theme : R.color.range_circle_color);
		}
		void setAccentColor(int accentColor) {
			mDotColor = accentColor;
		}
		@Override public void onDraw(Canvas canvas)
		{
			int viewWidth = getWidth();
			if(viewWidth == 0 || !mIsInitialized) {
				return;
			}
			if(!mDrawValuesReady) {
				mXCenter = getWidth() / 2;
				mYCenter = getHeight() / 2;
				mCircleRadius = (int) (Math.min(mXCenter, mYCenter) * mCircleRadiusMultiplier);
				if(!mIs24HourMode) {
					// We'll need to draw the AM/PM circles, so the main circle will need to have
					// a slightly higher center. To keep the entire view centered vertically, we'll
					// have to push it up by half the radius of the AM/PM circles.
					int amPmCircleRadius = (int) (mCircleRadius * mAmPmCircleRadiusMultiplier);
					mYCenter -= amPmCircleRadius*0.75;
				}
				mDrawValuesReady = true;
			}
			// Draw the white circle.
			mPaint.setColor(mCircleColor);
			canvas.drawCircle(mXCenter, mYCenter, mCircleRadius, mPaint);
			// Draw a small black circle in the center.
			mPaint.setColor(mDotColor);
			canvas.drawCircle(mXCenter, mYCenter, 8, mPaint);
		}
	}
	//
	// The primary layout to hold the circular picker, and the am/pm buttons. This view will measure
	// itself to end up as a square. It also handles touches to be passed in to views that need to know
	// when they'd been touched.
	//
	public static class RadialPickerLayout extends FrameLayout implements View.OnTouchListener {
		private static final String TAG = "RadialPickerLayout";
		private final int TOUCH_SLOP;
		private final int TAP_TIMEOUT;
		private static final int VISIBLE_DEGREES_STEP_SIZE = 30;
		private static final int HOUR_VALUE_TO_DEGREES_STEP_SIZE = VISIBLE_DEGREES_STEP_SIZE;
		private static final int MINUTE_VALUE_TO_DEGREES_STEP_SIZE = 6;
		private static final int HOUR_INDEX = TimePickerDialog.HOUR_INDEX;
		private static final int MINUTE_INDEX = TimePickerDialog.MINUTE_INDEX;
		private static final int AMPM_INDEX = TimePickerDialog.AMPM_INDEX;
		private static final int ENABLE_PICKER_INDEX = TimePickerDialog.ENABLE_PICKER_INDEX;
		private static final int AM = TimePickerDialog.AM;
		private static final int PM = TimePickerDialog.PM;
		private int mLastValueSelected;
		private TimePickerDialog mTimePickerDialog;
		private OnValueSelectedListener mListener;
		private boolean mTimeInitialized;
		private int mCurrentHoursOfDay;
		private int mCurrentMinutes;
		private boolean mIs24HourMode;
		private boolean mHideAmPm;
		private int mCurrentItemShowing;
		private CircleView mCircleView;
		private AmPmCirclesView mAmPmCirclesView;
		private RadialTextsView mHourRadialTextsView;
		private RadialTextsView mMinuteRadialTextsView;
		private RadialSelectorView mHourRadialSelectorView;
		private RadialSelectorView mMinuteRadialSelectorView;
		private View mGrayBox;
		private int[] mSnapPrefer30sMap;
		private boolean mInputEnabled;
		private int mIsTouchingAmOrPm = -1;
		private boolean mDoingMove;
		private boolean mDoingTouch;
		private int mDownDegrees;
		private float mDownX;
		private float mDownY;
		private AccessibilityManager mAccessibilityManager;
		private AnimatorSet mTransition;
		private Handler mHandler = new Handler();
		public interface OnValueSelectedListener {
			void onValueSelected(int pickerIndex, int newValue, boolean autoAdvance);
		}
		public RadialPickerLayout(Context context, AttributeSet attrs)
		{
			super(context, attrs);
			setOnTouchListener(this);
			ViewConfiguration vc = ViewConfiguration.get(context);
			TOUCH_SLOP = vc.getScaledTouchSlop();
			TAP_TIMEOUT = ViewConfiguration.getTapTimeout();
			mDoingMove = false;
			mCircleView = new CircleView(context);
			addView(mCircleView);
			mAmPmCirclesView = new AmPmCirclesView(context);
			addView(mAmPmCirclesView);
			mHourRadialSelectorView = new RadialSelectorView(context);
			addView(mHourRadialSelectorView);
			mMinuteRadialSelectorView = new RadialSelectorView(context);
			addView(mMinuteRadialSelectorView);
			mHourRadialTextsView = new RadialTextsView(context);
			addView(mHourRadialTextsView);
			mMinuteRadialTextsView = new RadialTextsView(context);
			addView(mMinuteRadialTextsView);
			// Prepare mapping to snap touchable degrees to selectable degrees.
			preparePrefer30sMap();
			mLastValueSelected = -1;
			mInputEnabled = true;
			mGrayBox = new View(context);
			mGrayBox.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
			mGrayBox.setBackgroundColor(getResources().getColor(R.color.range_transparent_black));
			mGrayBox.setVisibility(View.INVISIBLE);
			addView(mGrayBox);
			mAccessibilityManager = (AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE);
			mTimeInitialized = false;
		}
		//
		// Measure the view to end up as a square, based on the minimum of the height and width.
		//
		/**
		 @Override
		 public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		 int measuredWidth = MeasureSpec.getSize(widthMeasureSpec);
		 int widthMode = MeasureSpec.getMode(widthMeasureSpec);
		 int measuredHeight = MeasureSpec.getSize(heightMeasureSpec);
		 int heightMode = MeasureSpec.getMode(heightMeasureSpec);
		 int minDimension = Math.min(measuredWidth, measuredHeight);

		 super.onMeasure(MeasureSpec.makeMeasureSpec(minDimension, widthMode),
		 MeasureSpec.makeMeasureSpec(minDimension, heightMode));
		 }
		 **/
		public void setOnValueSelectedListener(OnValueSelectedListener listener)
		{
			mListener = listener;
		}
		//
		// Initialize the Layout with starting values.
		// @param context
		// @param initialHoursOfDay
		// @param initialMinutes
		// @param is24HourMode
		//
		public void initialize(Context context, TimePickerDialog timePickerDialog, int initialHoursOfDay, int initialMinutes, boolean is24HourMode)
		{
			if(mTimeInitialized) {
				Log.e(TAG, "Time has already been initialized.");
				return;
			}
			mTimePickerDialog = timePickerDialog;
			mIs24HourMode = is24HourMode;
			mHideAmPm = mAccessibilityManager.isTouchExplorationEnabled() || mIs24HourMode;
			// Initialize the circle and AM/PM circles if applicable.
			mCircleView.initialize(context, mHideAmPm);
			mCircleView.invalidate();
			if(!mHideAmPm) {
				mAmPmCirclesView.initialize(context, initialHoursOfDay < 12? AM : PM);
				mAmPmCirclesView.invalidate();
			}
			// Initialize the hours and minutes numbers.
			Resources res = context.getResources();
			int[] hours = {12, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
			int[] hours_24 = {0, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
			int[] minutes = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55};
			String[] hoursTexts = new String[12];
			String[] innerHoursTexts = new String[12];
			String[] minutesTexts = new String[12];
			for(int i = 0; i < 12; i++) {
				hoursTexts[i] = is24HourMode ? String.format("%02d", hours_24[i]) : String.format("%d", hours[i]);
				innerHoursTexts[i] = String.format("%d", hours[i]);
				minutesTexts[i] = String.format("%02d", minutes[i]);
			}
			mHourRadialTextsView.initialize(res, hoursTexts, (is24HourMode? innerHoursTexts : null), mHideAmPm, true);
			mHourRadialTextsView.setSelection(is24HourMode ? initialHoursOfDay : hours[initialHoursOfDay % 12]);
			mHourRadialTextsView.invalidate();
			mMinuteRadialTextsView.initialize(res, minutesTexts, null, mHideAmPm, false);
			mMinuteRadialTextsView.setSelection(initialMinutes);
			mMinuteRadialTextsView.invalidate();
			// Initialize the currently-selected hour and minute.
			setValueForItem(HOUR_INDEX, initialHoursOfDay);
			setValueForItem(MINUTE_INDEX, initialMinutes);
			int hourDegrees = (initialHoursOfDay % 12) * HOUR_VALUE_TO_DEGREES_STEP_SIZE;
			mHourRadialSelectorView.initialize(context, mHideAmPm, is24HourMode, true, hourDegrees, isHourInnerCircle(initialHoursOfDay));
			int minuteDegrees = initialMinutes * MINUTE_VALUE_TO_DEGREES_STEP_SIZE;
			mMinuteRadialSelectorView.initialize(context, mHideAmPm, false, false, minuteDegrees, false);
			mTimeInitialized = true;
		}
		/* package */ void setTheme(Context context, boolean themeDark)
		{
			mCircleView.setTheme(context, themeDark);
			mAmPmCirclesView.setTheme(context, themeDark);
			mHourRadialTextsView.setTheme(context, themeDark);
			mMinuteRadialTextsView.setTheme(context, themeDark);
			mHourRadialSelectorView.setTheme(context, themeDark);
			mMinuteRadialSelectorView.setTheme(context, themeDark);
		}
		public void setAccentColor(int accentColor)
		{
			mHourRadialSelectorView.setAccentColor(accentColor);
			mMinuteRadialSelectorView.setAccentColor(accentColor);
			mAmPmCirclesView.setAccentColor(accentColor);
			mCircleView.setAccentColor(accentColor);
		}
		public void setTime(int hours, int minutes)
		{
			setItem(HOUR_INDEX, hours);
			setItem(MINUTE_INDEX, minutes);
		}
		//
		// Set either the hour or the minute. Will set the internal value, and set the selection.
		//
		private void setItem(int index, int value)
		{
			if(index == HOUR_INDEX) {
				setValueForItem(HOUR_INDEX, value);
				int hourDegrees = (value % 12) * HOUR_VALUE_TO_DEGREES_STEP_SIZE;
				mHourRadialSelectorView.setSelection(hourDegrees, isHourInnerCircle(value), false);
				mHourRadialSelectorView.invalidate();
				mHourRadialTextsView.setSelection(value);
				mHourRadialTextsView.invalidate();
			}
			else if(index == MINUTE_INDEX) {
				setValueForItem(MINUTE_INDEX, value);
				int minuteDegrees = value * MINUTE_VALUE_TO_DEGREES_STEP_SIZE;
				mMinuteRadialSelectorView.setSelection(minuteDegrees, false, false);
				mMinuteRadialSelectorView.invalidate();
				mMinuteRadialTextsView.setSelection(value);
				mHourRadialTextsView.invalidate();
			}
		}
		//
		// Check if a given hour appears in the outer circle or the inner circle
		// @return true if the hour is in the inner circle, false if it's in the outer circle.
		//
		private boolean isHourInnerCircle(int hourOfDay)
		{
			// We'll have the 00 hours on the outside circle.
			return mIs24HourMode && (hourOfDay <= 12 && hourOfDay != 0);
		}
		public int getHours() { return mCurrentHoursOfDay; }
		public int getMinutes() { return mCurrentMinutes; }
		//
		// If the hours are showing, return the current hour. If the minutes are showing, return the
		// current minute.
		//
		private int getCurrentlyShowingValue()
		{
			int currentIndex = getCurrentItemShowing();
			if(currentIndex == HOUR_INDEX)
				return mCurrentHoursOfDay;
			else if(currentIndex == MINUTE_INDEX)
				return mCurrentMinutes;
			else
				return -1;
		}
		public int getIsCurrentlyAmOrPm()
		{
			if(mCurrentHoursOfDay < 12)
				return AM;
			else if(mCurrentHoursOfDay < 24)
				return PM;
			else
				return -1;
		}
		//
		// Set the internal value for the hour, minute, or AM/PM.
		//
		private void setValueForItem(int index, int value)
		{
			if(index == HOUR_INDEX) {
				mCurrentHoursOfDay = value;
			}
			else if(index == MINUTE_INDEX){
				mCurrentMinutes = value;
			}
			else if(index == AMPM_INDEX) {
				if(value == AM) {
					mCurrentHoursOfDay = mCurrentHoursOfDay % 12;
				}
				else if(value == PM) {
					mCurrentHoursOfDay = (mCurrentHoursOfDay % 12) + 12;
				}
			}
		}
		//
		// Set the internal value as either AM or PM, and update the AM/PM circle displays.
		// @param amOrPm
		//
		public void setAmOrPm(int amOrPm)
		{
			mAmPmCirclesView.setAmOrPm(amOrPm);
			mAmPmCirclesView.invalidate();
			setValueForItem(AMPM_INDEX, amOrPm);
		}
		/**
		 * Split up the 360 degrees of the circle among the 60 selectable values. Assigns a larger
		 * selectable area to each of the 12 visible values, such that the ratio of space apportioned
		 * to a visible value : space apportioned to a non-visible value will be 14 : 4.
		 * E.g. the output of 30 degrees should have a higher range of input associated with it than
		 * the output of 24 degrees, because 30 degrees corresponds to a visible number on the clock
		 * circle (5 on the minutes, 1 or 13 on the hours).
		 */
		private void preparePrefer30sMap()
		{
			// We'll split up the visible output and the non-visible output such that each visible
			// output will correspond to a range of 14 associated input degrees, and each non-visible
			// output will correspond to a range of 4 associate input degrees, so visible numbers
			// are more than 3 times easier to get than non-visible numbers:
			// {354-359,0-7}:0, {8-11}:6, {12-15}:12, {16-19}:18, {20-23}:24, {24-37}:30, etc.
			//
			// If an output of 30 degrees should correspond to a range of 14 associated degrees, then
			// we'll need any input between 24 - 37 to snap to 30. Working out from there, 20-23 should
			// snap to 24, while 38-41 should snap to 36. This is somewhat counter-intuitive, that you
			// can be touching 36 degrees but have the selection snapped to 30 degrees; however, this
			// inconsistency isn't noticeable at such fine-grained degrees, and it affords us the
			// ability to aggressively prefer the visible values by a factor of more than 3:1, which
			// greatly contributes to the selectability of these values.

			// Our input will be 0 through 360.
			mSnapPrefer30sMap = new int[361];
			// The first output is 0, and each following output will increment by 6 {0, 6, 12, ...}.
			int snappedOutputDegrees = 0;
			// Count of how many inputs we've designated to the specified output.
			int count = 1;
			// How many input we expect for a specified output. This will be 14 for output divisible
			// by 30, and 4 for the remaining output. We'll special case the outputs of 0 and 360, so
			// the caller can decide which they need.
			int expectedCount = 8;
			// Iterate through the input.
			for(int degrees = 0; degrees < 361; degrees++) {
				// Save the input-output mapping.
				mSnapPrefer30sMap[degrees] = snappedOutputDegrees;
				// If this is the last input for the specified output, calculate the next output and
				// the next expected count.
				if(count == expectedCount) {
					snappedOutputDegrees += 6;
					if(snappedOutputDegrees == 360) {
						expectedCount = 7;
					}
					else if(snappedOutputDegrees % 30 == 0) {
						expectedCount = 14;
					}
					else {
						expectedCount = 4;
					}
					count = 1;
				}
				else {
					count++;
				}
			}
		}
		//
		// Returns mapping of any input degrees (0 to 360) to one of 60 selectable output degrees,
		// where the degrees corresponding to visible numbers (i.e. those divisible by 30) will be
		// weighted heavier than the degrees corresponding to non-visible numbers.
		// See {@link #preparePrefer30sMap()} documentation for the rationale and generation of the
		// mapping.
		//
		private int snapPrefer30s(int degrees)
		{
			return (mSnapPrefer30sMap == null) ? -1 : mSnapPrefer30sMap[degrees];
		}
		/**
		 * Returns mapping of any input degrees (0 to 360) to one of 12 visible output degrees (all
		 * multiples of 30), where the input will be "snapped" to the closest visible degrees.
		 * @param degrees The input degrees
		 * @param forceHigherOrLower The output may be forced to either the higher or lower step, or may
		 * be allowed to snap to whichever is closer. Use 1 to force strictly higher, -1 to force
		 * strictly lower, and 0 to snap to the closer one.
		 * @return output degrees, will be a multiple of 30
		 */
		private static int snapOnly30s(int degrees, int forceHigherOrLower)
		{
			int stepSize = HOUR_VALUE_TO_DEGREES_STEP_SIZE;
			int floor = (degrees / stepSize) * stepSize;
			int ceiling = floor + stepSize;
			if(forceHigherOrLower == 1) {
				degrees = ceiling;
			}
			else if(forceHigherOrLower == -1) {
				if(degrees == floor) {
					floor -= stepSize;
				}
				degrees = floor;
			}
			else {
				if((degrees - floor) < (ceiling - degrees)) {
					degrees = floor;
				}
				else {
					degrees = ceiling;
				}
			}
			return degrees;
		}
		/**
		 * For the currently showing view (either hours or minutes), re-calculate the position for the
		 * selector, and redraw it at that position. The input degrees will be snapped to a selectable
		 * value. The text representing the currently selected value will be redrawn if required.
		 * @param degrees Degrees which should be selected.
		 * @param isInnerCircle Whether the selection should be in the inner circle; will be ignored
		 * if there is no inner circle.
		 * @param forceToVisibleValue Even if the currently-showing circle allows for fine-grained
		 * selection (i.e. minutes), force the selection to one of the visibly-showing values.
		 * @param forceDrawDot The dot in the circle will generally only be shown when the selection
		 * is on non-visible values, but use this to force the dot to be shown.
		 * @return The value that was selected, i.e. 0-23 for hours, 0-59 for minutes.
		 */
		private int reselectSelector(int degrees, boolean isInnerCircle, boolean forceToVisibleValue, boolean forceDrawDot)
		{
			if(degrees == -1) {
				return -1;
			}
			int currentShowing = getCurrentItemShowing();
			int stepSize;
			boolean allowFineGrained = !forceToVisibleValue && (currentShowing == MINUTE_INDEX);
			if(allowFineGrained) {
				degrees = snapPrefer30s(degrees);
			}
			else {
				degrees = snapOnly30s(degrees, 0);
			}
			RadialSelectorView radialSelectorView;
			if(currentShowing == HOUR_INDEX) {
				radialSelectorView = mHourRadialSelectorView;
				stepSize = HOUR_VALUE_TO_DEGREES_STEP_SIZE;
			}
			else {
				radialSelectorView = mMinuteRadialSelectorView;
				stepSize = MINUTE_VALUE_TO_DEGREES_STEP_SIZE;
			}
			radialSelectorView.setSelection(degrees, isInnerCircle, forceDrawDot);
			radialSelectorView.invalidate();
			if(currentShowing == HOUR_INDEX) {
				if(mIs24HourMode) {
					if(degrees == 0 && isInnerCircle) {
						degrees = 360;
					}
					else if(degrees == 360 && !isInnerCircle) {
						degrees = 0;
					}
				}
				else if(degrees == 0) {
					degrees = 360;
				}
			}
			else if(degrees == 360 && currentShowing == MINUTE_INDEX) {
				degrees = 0;
			}
			int value = degrees / stepSize;
			if(currentShowing == HOUR_INDEX && mIs24HourMode && !isInnerCircle && degrees != 0) {
				value += 12;
			}
			// Redraw the text if necessary
			if(getCurrentItemShowing() == HOUR_INDEX) {
				mHourRadialTextsView.setSelection(value);
				mHourRadialTextsView.invalidate();
			}
			else if(getCurrentItemShowing() == MINUTE_INDEX) {
				mMinuteRadialTextsView.setSelection(value);
				mMinuteRadialTextsView.invalidate();
			}
			return value;
		}
		/**
		 * Calculate the degrees within the circle that corresponds to the specified coordinates, if
		 * the coordinates are within the range that will trigger a selection.
		 * @param pointX The x coordinate.
		 * @param pointY The y coordinate.
		 * @param forceLegal Force the selection to be legal, regardless of how far the coordinates are
		 * from the actual numbers.
		 * @param isInnerCircle If the selection may be in the inner circle, pass in a size-1 boolean
		 * array here, inside which the value will be true if the selection is in the inner circle,
		 * and false if in the outer circle.
		 * @return Degrees from 0 to 360, if the selection was within the legal range. -1 if not.
		 */
		private int getDegreesFromCoords(float pointX, float pointY, boolean forceLegal, final Boolean[] isInnerCircle)
		{
			int currentItem = getCurrentItemShowing();
			if(currentItem == HOUR_INDEX) {
				return mHourRadialSelectorView.getDegreesFromCoords(pointX, pointY, forceLegal, isInnerCircle);
			}
			else if(currentItem == MINUTE_INDEX) {
				return mMinuteRadialSelectorView.getDegreesFromCoords(pointX, pointY, forceLegal, isInnerCircle);
			}
			else {
				return -1;
			}
		}
		/**
		 * Get the item (hours or minutes) that is currently showing.
		 */
		public int getCurrentItemShowing()
		{
			if(mCurrentItemShowing != HOUR_INDEX && mCurrentItemShowing != MINUTE_INDEX) {
				Log.e(TAG, "Current item showing was unfortunately set to "+mCurrentItemShowing);
				return -1;
			}
			return mCurrentItemShowing;
		}
		/**
		 * Set either minutes or hours as showing.
		 * @param animate True to animate the transition, false to show with no animation.
		 */
		public void setCurrentItemShowing(int index, boolean animate)
		{
			if(index != HOUR_INDEX && index != MINUTE_INDEX) {
				Log.e(TAG, "TimePicker does not support view at index "+index);
				return;
			}
			int lastIndex = getCurrentItemShowing();
			mCurrentItemShowing = index;
			if(animate && (index != lastIndex)) {
				ObjectAnimator[] anims = new ObjectAnimator[4];
				if(index == MINUTE_INDEX) {
					anims[0] = mHourRadialTextsView.getDisappearAnimator();
					anims[1] = mHourRadialSelectorView.getDisappearAnimator();
					anims[2] = mMinuteRadialTextsView.getReappearAnimator();
					anims[3] = mMinuteRadialSelectorView.getReappearAnimator();
				}
				else if(index == HOUR_INDEX){
					anims[0] = mHourRadialTextsView.getReappearAnimator();
					anims[1] = mHourRadialSelectorView.getReappearAnimator();
					anims[2] = mMinuteRadialTextsView.getDisappearAnimator();
					anims[3] = mMinuteRadialSelectorView.getDisappearAnimator();
				}
				if(mTransition != null && mTransition.isRunning()) {
					mTransition.end();
				}
				mTransition = new AnimatorSet();
				mTransition.playTogether(anims);
				mTransition.start();
			}
			else {
				int hourAlpha = (index == HOUR_INDEX) ? 255 : 0;
				int minuteAlpha = (index == MINUTE_INDEX) ? 255 : 0;
				mHourRadialTextsView.setAlpha(hourAlpha);
				mHourRadialSelectorView.setAlpha(hourAlpha);
				mMinuteRadialTextsView.setAlpha(minuteAlpha);
				mMinuteRadialSelectorView.setAlpha(minuteAlpha);
			}
		}
		@Override public boolean onTouch(View v, MotionEvent event)
		{
			final float eventX = event.getX();
			final float eventY = event.getY();
			int degrees;
			int value;
			final Boolean[] isInnerCircle = new Boolean[1];
			isInnerCircle[0] = false;
			switch(event.getAction()) {
				case MotionEvent.ACTION_DOWN:
					if(!mInputEnabled) {
						return true;
					}
					mDownX = eventX;
					mDownY = eventY;
					mLastValueSelected = -1;
					mDoingMove = false;
					mDoingTouch = true;
					// If we're showing the AM/PM, check to see if the user is touching it.
					if(!mHideAmPm) {
						mIsTouchingAmOrPm = mAmPmCirclesView.getIsTouchingAmOrPm(eventX, eventY);
					}
					else {
						mIsTouchingAmOrPm = -1;
					}
					if(mIsTouchingAmOrPm == AM || mIsTouchingAmOrPm == PM) {
						// If the touch is on AM or PM, set it as "touched" after the TAP_TIMEOUT
						// in case the user moves their finger quickly.
						mTimePickerDialog.tryVibrate();
						mDownDegrees = -1;
						mHandler.postDelayed(new Runnable() {
							@Override public void run()
							{
								mAmPmCirclesView.setAmOrPmPressed(mIsTouchingAmOrPm);
								mAmPmCirclesView.invalidate();
							}
						}, TAP_TIMEOUT);
					}
					else {
						// If we're in accessibility mode, force the touch to be legal. Otherwise,
						// it will only register within the given touch target zone.
						boolean forceLegal = mAccessibilityManager.isTouchExplorationEnabled();
						// Calculate the degrees that is currently being touched.
						mDownDegrees = getDegreesFromCoords(eventX, eventY, forceLegal, isInnerCircle);
						if(mDownDegrees != -1) {
							// If it's a legal touch, set that number as "selected" after the
							// TAP_TIMEOUT in case the user moves their finger quickly.
							mTimePickerDialog.tryVibrate();
							mHandler.postDelayed(new Runnable() {
								@Override public void run()
								{
									mDoingMove = true;
									int value = reselectSelector(mDownDegrees, isInnerCircle[0],false, true);
									mLastValueSelected = value;
									mListener.onValueSelected(getCurrentItemShowing(), value, false);
								}
							}, TAP_TIMEOUT);
						}
					}
					return true;
				case MotionEvent.ACTION_MOVE:
					if(!mInputEnabled) {
						// We shouldn't be in this state, because input is disabled.
						Log.e(TAG, "Input was disabled, but received ACTION_MOVE.");
						return true;
					}
					float dY = Math.abs(eventY - mDownY);
					float dX = Math.abs(eventX - mDownX);
					if(!mDoingMove && dX <= TOUCH_SLOP && dY <= TOUCH_SLOP) {
						// Hasn't registered down yet, just slight, accidental movement of finger.
						break;
					}
					// If we're in the middle of touching down on AM or PM, check if we still are.
					// If so, no-op. If not, remove its pressed state. Either way, no need to check
					// for touches on the other circle.
					if(mIsTouchingAmOrPm == AM || mIsTouchingAmOrPm == PM) {
						mHandler.removeCallbacksAndMessages(null);
						int isTouchingAmOrPm = mAmPmCirclesView.getIsTouchingAmOrPm(eventX, eventY);
						if(isTouchingAmOrPm != mIsTouchingAmOrPm) {
							mAmPmCirclesView.setAmOrPmPressed(-1);
							mAmPmCirclesView.invalidate();
							mIsTouchingAmOrPm = -1;
						}
						break;
					}
					if(mDownDegrees == -1) {
						// Original down was illegal, so no movement will register.
						break;
					}
					// We're doing a move along the circle, so move the selection as appropriate.
					mDoingMove = true;
					mHandler.removeCallbacksAndMessages(null);
					degrees = getDegreesFromCoords(eventX, eventY, true, isInnerCircle);
					if(degrees != -1) {
						value = reselectSelector(degrees, isInnerCircle[0], false, true);
						if(value != mLastValueSelected) {
							mTimePickerDialog.tryVibrate();
							mLastValueSelected = value;
							mListener.onValueSelected(getCurrentItemShowing(), value, false);
						}
					}
					return true;
				case MotionEvent.ACTION_UP:
					if(!mInputEnabled) {
						// If our touch input was disabled, tell the listener to re-enable us.
						Log.d(TAG, "Input was disabled, but received ACTION_UP.");
						mListener.onValueSelected(ENABLE_PICKER_INDEX, 1, false);
						return true;
					}
					mHandler.removeCallbacksAndMessages(null);
					mDoingTouch = false;
					// If we're touching AM or PM, set it as selected, and tell the listener.
					if(mIsTouchingAmOrPm == AM || mIsTouchingAmOrPm == PM) {
						int isTouchingAmOrPm = mAmPmCirclesView.getIsTouchingAmOrPm(eventX, eventY);
						mAmPmCirclesView.setAmOrPmPressed(-1);
						mAmPmCirclesView.invalidate();
						if(isTouchingAmOrPm == mIsTouchingAmOrPm) {
							mAmPmCirclesView.setAmOrPm(isTouchingAmOrPm);
							if(getIsCurrentlyAmOrPm() != isTouchingAmOrPm) {
								mListener.onValueSelected(AMPM_INDEX, mIsTouchingAmOrPm, false);
								setValueForItem(AMPM_INDEX, isTouchingAmOrPm);
							}
						}
						mIsTouchingAmOrPm = -1;
						break;
					}
					// If we have a legal degrees selected, set the value and tell the listener.
					if(mDownDegrees != -1) {
						degrees = getDegreesFromCoords(eventX, eventY, mDoingMove, isInnerCircle);
						if(degrees != -1) {
							value = reselectSelector(degrees, isInnerCircle[0], !mDoingMove, false);
							if(getCurrentItemShowing() == HOUR_INDEX && !mIs24HourMode) {
								int amOrPm = getIsCurrentlyAmOrPm();
								if(amOrPm == AM && value == 12) {
									value = 0;
								}
								else if(amOrPm == PM && value != 12) {
									value += 12;
								}
							}
							setValueForItem(getCurrentItemShowing(), value);
							mListener.onValueSelected(getCurrentItemShowing(), value, true);
						}
					}
					mDoingMove = false;
					return true;
				default:
					break;
			}
			return false;
		}
		//
		// Set touch input as enabled or disabled, for use with keyboard mode.
		//
		public boolean trySettingInputEnabled(boolean inputEnabled)
		{
			if(mDoingTouch && !inputEnabled) {
				// If we're trying to disable input, but we're in the middle of a touch event,
				// we'll allow the touch event to continue before disabling input.
				return false;
			}
			mInputEnabled = inputEnabled;
			mGrayBox.setVisibility(inputEnabled? View.INVISIBLE : View.VISIBLE);
			return true;
		}
		//
		// Necessary for accessibility, to ensure we support "scrolling" forward and backward
		// in the circle.
		//
		@Override @SuppressWarnings("deprecation") public void onInitializeAccessibilityNodeInfo(@NonNull AccessibilityNodeInfo info)
		{
			super.onInitializeAccessibilityNodeInfo(info);
			if(Build.VERSION.SDK_INT >= 21) {
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_BACKWARD);
				info.addAction(AccessibilityNodeInfo.AccessibilityAction.ACTION_SCROLL_FORWARD);
			}
			else {
				info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_FORWARD);
				info.addAction(AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD);
			}
		}
		//
		// Announce the currently-selected time when launched.
		//
		@Override public boolean dispatchPopulateAccessibilityEvent(AccessibilityEvent event)
		{
			if(event.getEventType() == AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED) {
				// Clear the event's current text so that only the current time will be spoken.
				event.getText().clear();
				Calendar time = Calendar.getInstance();
				time.set(Calendar.HOUR, getHours());
				time.set(Calendar.MINUTE, getMinutes());
				long millis = time.getTimeInMillis();
				int flags = DateUtils.FORMAT_SHOW_TIME;
				if(mIs24HourMode) {
					flags |= DateUtils.FORMAT_24HOUR;
				}
				String timeString = DateUtils.formatDateTime(getContext(), millis, flags);
				event.getText().add(timeString);
				return true;
			}
			return super.dispatchPopulateAccessibilityEvent(event);
		}
		//
		// When scroll forward/backward events are received, jump the time to the higher/lower
		// discrete, visible value on the circle.
		//
		@SuppressLint("NewApi") @Override public boolean performAccessibilityAction(int action, Bundle arguments)
		{
			if(super.performAccessibilityAction(action, arguments)) {
				return true;
			}
			int changeMultiplier = 0;
			if(action == AccessibilityNodeInfo.ACTION_SCROLL_FORWARD) {
				changeMultiplier = 1;
			}
			else if(action == AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD) {
				changeMultiplier = -1;
			}
			if(changeMultiplier != 0) {
				int value = getCurrentlyShowingValue();
				int stepSize = 0;
				int currentItemShowing = getCurrentItemShowing();
				if(currentItemShowing == HOUR_INDEX) {
					stepSize = HOUR_VALUE_TO_DEGREES_STEP_SIZE;
					value %= 12;
				}
				else if(currentItemShowing == MINUTE_INDEX) {
					stepSize = MINUTE_VALUE_TO_DEGREES_STEP_SIZE;
				}
				int degrees = value * stepSize;
				degrees = snapOnly30s(degrees, changeMultiplier);
				value = degrees / stepSize;
				int maxValue = 0;
				int minValue = 0;
				if(currentItemShowing == HOUR_INDEX) {
					if(mIs24HourMode) {
						maxValue = 23;
					}
					else {
						maxValue = 12;
						minValue = 1;
					}
				}
				else {
					maxValue = 55;
				}
				if(value > maxValue) {
					// If we scrolled forward past the highest number, wrap around to the lowest.
					value = minValue;
				}
				else if(value < minValue) {
					// If we scrolled backward past the lowest number, wrap around to the highest.
					value = maxValue;
				}
				setItem(currentItemShowing, value);
				mListener.onValueSelected(currentItemShowing, value, false);
				return true;
			}
			return false;
		}
	}
	//
	// View to show what number is selected. This will draw a blue circle over the number, with a blue
	// line coming from the center of the main circle to the edge of the blue selection.
	//
	public static class RadialSelectorView extends View {
		private static final String TAG = "RadialSelectorView";
		// Alpha level for selected circle.
		private static final int SELECTED_ALPHA = Utils.SELECTED_ALPHA;
		private static final int SELECTED_ALPHA_THEME_DARK = Utils.SELECTED_ALPHA_THEME_DARK;
		// Alpha level for the line.
		private static final int FULL_ALPHA = Utils.FULL_ALPHA;
		private final Paint mPaint = new Paint();
		private boolean mIsInitialized;
		private boolean mDrawValuesReady;
		private float mCircleRadiusMultiplier;
		private float mAmPmCircleRadiusMultiplier;
		private float mInnerNumbersRadiusMultiplier;
		private float mOuterNumbersRadiusMultiplier;
		private float mNumbersRadiusMultiplier;
		private float mSelectionRadiusMultiplier;
		private float mAnimationRadiusMultiplier;
		private boolean mIs24HourMode;
		private boolean mHasInnerCircle;
		private int mSelectionAlpha;
		private int mXCenter;
		private int mYCenter;
		private int mCircleRadius;
		private float mTransitionMidRadiusMultiplier;
		private float mTransitionEndRadiusMultiplier;
		private int mLineLength;
		private int mSelectionRadius;
		private InvalidateUpdateListener mInvalidateUpdateListener;
		private int mSelectionDegrees;
		private double mSelectionRadians;
		private boolean mForceDrawDot;

		public RadialSelectorView(Context context)
		{
			super(context);
			mIsInitialized = false;
		}
		/**
		 * Initialize this selector with the state of the picker.
		 * @param context Current context.
		 * @param is24HourMode Whether the selector is in 24-hour mode, which will tell us
		 * whether the circle's center is moved up slightly to make room for the AM/PM circles.
		 * @param hasInnerCircle Whether we have both an inner and an outer circle of numbers
		 * that may be selected. Should be true for 24-hour mode in the hours circle.
		 * @param disappearsOut Whether the numbers' animation will have them disappearing out
		 * or disappearing in.
		 * @param selectionDegrees The initial degrees to be selected.
		 * @param isInnerCircle Whether the initial selection is in the inner or outer circle.
		 * Will be ignored when hasInnerCircle is false.
		 */
		public void initialize(Context context, boolean is24HourMode, boolean hasInnerCircle, boolean disappearsOut, int selectionDegrees, boolean isInnerCircle)
		{
			if(mIsInitialized) {
				Log.e(TAG, "This RadialSelectorView may only be initialized once.");
				return;
			}
			Resources res = context.getResources();
			int accentColor = res.getColor(R.color.range_accent_color);
			mPaint.setColor(accentColor);
			mPaint.setAntiAlias(true);
			mSelectionAlpha = SELECTED_ALPHA;
			// Calculate values for the circle radius size.
			mIs24HourMode = is24HourMode;
			if(is24HourMode) {
				mCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_circle_radius_multiplier_24HourMode));
			}
			else {
				mCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_circle_radius_multiplier));
				mAmPmCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_ampm_circle_radius_multiplier));
			}
			// Calculate values for the radius size(s) of the numbers circle(s).
			mHasInnerCircle = hasInnerCircle;
			if(hasInnerCircle) {
				mInnerNumbersRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_numbers_radius_multiplier_inner));
				mOuterNumbersRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_numbers_radius_multiplier_outer));
			}
			else {
				mNumbersRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_numbers_radius_multiplier_normal));
			}
			mSelectionRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_selection_radius_multiplier));
			// Calculate values for the transition mid-way states.
			mAnimationRadiusMultiplier = 1;
			mTransitionMidRadiusMultiplier = 1f + (0.05f * (disappearsOut? -1 : 1));
			mTransitionEndRadiusMultiplier = 1f + (0.3f * (disappearsOut? 1 : -1));
			mInvalidateUpdateListener = new InvalidateUpdateListener();
			setSelection(selectionDegrees, isInnerCircle, false);
			mIsInitialized = true;
		}
		/* package */ void setTheme(Context context, boolean themeDark)
		{
			Resources res = context.getResources();
			int color;
			if(themeDark) {
				color = res.getColor(R.color.range_accent_color);
				mSelectionAlpha = SELECTED_ALPHA_THEME_DARK;
			}
			else {
				color = res.getColor(R.color.range_accent_color);
				mSelectionAlpha = SELECTED_ALPHA;
			}
			mPaint.setColor(color);
		}
		public void setAccentColor(int accentColor) {
			mPaint.setColor(accentColor);
		}
		/**
		 * Set the selection.
		 * @param selectionDegrees The degrees to be selected.
		 * @param isInnerCircle Whether the selection should be in the inner circle or outer. Will be
		 * ignored if hasInnerCircle was initialized to false.
		 * @param forceDrawDot Whether to force the dot in the center of the selection circle to be
		 * drawn. If false, the dot will be drawn only when the degrees is not a multiple of 30, i.e.
		 * the selection is not on a visible number.
		 */
		public void setSelection(int selectionDegrees, boolean isInnerCircle, boolean forceDrawDot)
		{
			mSelectionDegrees = selectionDegrees;
			mSelectionRadians = selectionDegrees * Math.PI / 180;
			mForceDrawDot = forceDrawDot;
			if(mHasInnerCircle)
				mNumbersRadiusMultiplier = isInnerCircle ? mInnerNumbersRadiusMultiplier : mOuterNumbersRadiusMultiplier;
		}
		//
		// Allows for smoother animations.
		//
		@Override public boolean hasOverlappingRendering() {
			return false;
		}
		//
		// Set the multiplier for the radius. Will be used during animations to move in/out.
		//
		public void setAnimationRadiusMultiplier(float animationRadiusMultiplier)
		{
			mAnimationRadiusMultiplier = animationRadiusMultiplier;
		}
		public int getDegreesFromCoords(float pointX, float pointY, boolean forceLegal, final Boolean[] isInnerCircle)
		{
			if(!mDrawValuesReady) {
				return -1;
			}
			double hypotenuse = Math.sqrt((pointY - mYCenter)*(pointY - mYCenter) + (pointX - mXCenter)*(pointX - mXCenter));
			// Check if we're outside the range
			if(mHasInnerCircle) {
				if(forceLegal) {
					// If we're told to force the coordinates to be legal, we'll set the isInnerCircle
					// boolean based based off whichever number the coordinates are closer to.
					int innerNumberRadius = (int) (mCircleRadius * mInnerNumbersRadiusMultiplier);
					int distanceToInnerNumber = (int) Math.abs(hypotenuse - innerNumberRadius);
					int outerNumberRadius = (int) (mCircleRadius * mOuterNumbersRadiusMultiplier);
					int distanceToOuterNumber = (int) Math.abs(hypotenuse - outerNumberRadius);
					isInnerCircle[0] = (distanceToInnerNumber <= distanceToOuterNumber);
				}
				else {
					// Otherwise, if we're close enough to either number (with the space between the
					// two allotted equally), set the isInnerCircle boolean as the closer one.
					// appropriately, but otherwise return -1.
					int minAllowedHypotenuseForInnerNumber = (int) (mCircleRadius * mInnerNumbersRadiusMultiplier) - mSelectionRadius;
					int maxAllowedHypotenuseForOuterNumber = (int) (mCircleRadius * mOuterNumbersRadiusMultiplier) + mSelectionRadius;
					int halfwayHypotenusePoint = (int) (mCircleRadius * ((mOuterNumbersRadiusMultiplier + mInnerNumbersRadiusMultiplier) / 2));
					if(hypotenuse >= minAllowedHypotenuseForInnerNumber && hypotenuse <= halfwayHypotenusePoint) {
						isInnerCircle[0] = true;
					}
					else if(hypotenuse <= maxAllowedHypotenuseForOuterNumber && hypotenuse >= halfwayHypotenusePoint) {
						isInnerCircle[0] = false;
					}
					else {
						return -1;
					}
				}
			}
			else {
				// If there's just one circle, we'll need to return -1 if:
				// we're not told to force the coordinates to be legal, and
				// the coordinates' distance to the number is within the allowed distance.
				if(!forceLegal) {
					int distanceToNumber = (int) Math.abs(hypotenuse - mLineLength);
					// The max allowed distance will be defined as the distance from the center of the
					// number to the edge of the circle.
					int maxAllowedDistance = (int) (mCircleRadius * (1 - mNumbersRadiusMultiplier));
					if(distanceToNumber > maxAllowedDistance) {
						return -1;
					}
				}
			}
			float opposite = Math.abs(pointY - mYCenter);
			double radians = Math.asin(opposite / hypotenuse);
			int degrees = (int) (radians * 180 / Math.PI);
			// Now we have to translate to the correct quadrant.
			boolean rightSide = (pointX > mXCenter);
			boolean topSide = (pointY < mYCenter);
			if(rightSide && topSide) {
				degrees = 90 - degrees;
			}
			else if(rightSide && !topSide) {
				degrees = 90 + degrees;
			}
			else if(!rightSide && !topSide) {
				degrees = 270 - degrees;
			}
			else if(!rightSide && topSide) {
				degrees = 270 + degrees;
			}
			return degrees;
		}
		@Override public void onDraw(Canvas canvas)
		{
			int viewWidth = getWidth();
			if(viewWidth == 0 || !mIsInitialized) {
				return;
			}
			if(!mDrawValuesReady) {
				mXCenter = getWidth() / 2;
				mYCenter = getHeight() / 2;
				mCircleRadius = (int) (Math.min(mXCenter, mYCenter) * mCircleRadiusMultiplier);
				if(!mIs24HourMode) {
					// We'll need to draw the AM/PM circles, so the main circle will need to have
					// a slightly higher center. To keep the entire view centered vertically, we'll
					// have to push it up by half the radius of the AM/PM circles.
					int amPmCircleRadius = (int) (mCircleRadius * mAmPmCircleRadiusMultiplier);
					mYCenter -= amPmCircleRadius *0.75;
				}
				mSelectionRadius = (int) (mCircleRadius * mSelectionRadiusMultiplier);
				mDrawValuesReady = true;
			}
			// Calculate the current radius at which to place the selection circle.
			mLineLength = (int) (mCircleRadius * mNumbersRadiusMultiplier * mAnimationRadiusMultiplier);
			int pointX = mXCenter + (int) (mLineLength * Math.sin(mSelectionRadians));
			int pointY = mYCenter - (int) (mLineLength * Math.cos(mSelectionRadians));
			// Draw the selection circle.
			mPaint.setAlpha(mSelectionAlpha);
			canvas.drawCircle(pointX, pointY, mSelectionRadius, mPaint);
			if(mForceDrawDot | mSelectionDegrees % 30 != 0) {
				// We're not on a direct tick (or we've been told to draw the dot anyway).
				mPaint.setAlpha(FULL_ALPHA);
				canvas.drawCircle(pointX, pointY, (mSelectionRadius * 2 / 7), mPaint);
			}
			else {
				// We're not drawing the dot, so shorten the line to only go as far as the edge of the
				// selection circle.
				int lineLength = mLineLength;
				lineLength -= mSelectionRadius;
				pointX = mXCenter + (int) (lineLength * Math.sin(mSelectionRadians));
				pointY = mYCenter - (int) (lineLength * Math.cos(mSelectionRadians));
			}
			// Draw the line from the center of the circle.
			mPaint.setAlpha(255);
			mPaint.setStrokeWidth(4);
			canvas.drawLine(mXCenter, mYCenter, pointX, pointY, mPaint);
		}
		public ObjectAnimator getDisappearAnimator()
		{
			if(!mIsInitialized || !mDrawValuesReady) {
				Log.e(TAG, "RadialSelectorView was not ready for animation.");
				return null;
			}
			Keyframe kf0, kf1, kf2;
			float midwayPoint = 0.2f;
			int duration = 500;
			kf0 = Keyframe.ofFloat(0f, 1);
			kf1 = Keyframe.ofFloat(midwayPoint, mTransitionMidRadiusMultiplier);
			kf2 = Keyframe.ofFloat(1f, mTransitionEndRadiusMultiplier);
			PropertyValuesHolder radiusDisappear = PropertyValuesHolder.ofKeyframe("animationRadiusMultiplier", kf0, kf1, kf2);
			kf0 = Keyframe.ofFloat(0f, 1f);
			kf1 = Keyframe.ofFloat(1f, 0f);
			PropertyValuesHolder fadeOut = PropertyValuesHolder.ofKeyframe("alpha", kf0, kf1);
			ObjectAnimator disappearAnimator = ObjectAnimator.ofPropertyValuesHolder(this, radiusDisappear, fadeOut).setDuration(duration);
			disappearAnimator.addUpdateListener(mInvalidateUpdateListener);
			return disappearAnimator;
		}
		public ObjectAnimator getReappearAnimator()
		{
			if(!mIsInitialized || !mDrawValuesReady) {
				Log.e(TAG, "RadialSelectorView was not ready for animation.");
				return null;
			}
			Keyframe kf0, kf1, kf2, kf3;
			float midwayPoint = 0.2f;
			int duration = 500;
			// The time points are half of what they would normally be, because this animation is
			// staggered against the disappear so they happen seamlessly. The reappear starts
			// halfway into the disappear.
			float delayMultiplier = 0.25f;
			float transitionDurationMultiplier = 1f;
			float totalDurationMultiplier = transitionDurationMultiplier + delayMultiplier;
			int totalDuration = (int) (duration * totalDurationMultiplier);
			float delayPoint = (delayMultiplier * duration) / totalDuration;
			midwayPoint = 1 - (midwayPoint * (1 - delayPoint));
			kf0 = Keyframe.ofFloat(0f, mTransitionEndRadiusMultiplier);
			kf1 = Keyframe.ofFloat(delayPoint, mTransitionEndRadiusMultiplier);
			kf2 = Keyframe.ofFloat(midwayPoint, mTransitionMidRadiusMultiplier);
			kf3 = Keyframe.ofFloat(1f, 1);
			PropertyValuesHolder radiusReappear = PropertyValuesHolder.ofKeyframe("animationRadiusMultiplier", kf0, kf1, kf2, kf3);
			kf0 = Keyframe.ofFloat(0f, 0f);
			kf1 = Keyframe.ofFloat(delayPoint, 0f);
			kf2 = Keyframe.ofFloat(1f, 1f);
			PropertyValuesHolder fadeIn = PropertyValuesHolder.ofKeyframe("alpha", kf0, kf1, kf2);
			ObjectAnimator reappearAnimator = ObjectAnimator.ofPropertyValuesHolder(this, radiusReappear, fadeIn).setDuration(totalDuration);
			reappearAnimator.addUpdateListener(mInvalidateUpdateListener);
			return reappearAnimator;
		}
		//
		// We'll need to invalidate during the animation.
		//
		private class InvalidateUpdateListener implements ValueAnimator.AnimatorUpdateListener {
			@Override public void onAnimationUpdate(ValueAnimator animation)
			{
				RadialSelectorView.this.invalidate();
			}
		}
	}
	//
	// A view to show a series of numbers in a circular pattern.
	//
	public static class RadialTextsView extends View {
		private final static String TAG = "RadialTextsView";
		private final Paint mPaint = new Paint();
		private final Paint mSelectedPaint = new Paint();
		private boolean mDrawValuesReady;
		private boolean mIsInitialized;
		private int selection = -1;
		private Typeface mTypefaceLight;
		private Typeface mTypefaceRegular;
		private String[] mTexts;
		private String[] mInnerTexts;
		private boolean mIs24HourMode;
		private boolean mHasInnerCircle;
		private float mCircleRadiusMultiplier;
		private float mAmPmCircleRadiusMultiplier;
		private float mNumbersRadiusMultiplier;
		private float mInnerNumbersRadiusMultiplier;
		private float mTextSizeMultiplier;
		private float mInnerTextSizeMultiplier;
		private int mXCenter;
		private int mYCenter;
		private float mCircleRadius;
		private boolean mTextGridValuesDirty;
		private float mTextSize;
		private float mInnerTextSize;
		private float[] mTextGridHeights;
		private float[] mTextGridWidths;
		private float[] mInnerTextGridHeights;
		private float[] mInnerTextGridWidths;
		private float mAnimationRadiusMultiplier;
		private float mTransitionMidRadiusMultiplier;
		private float mTransitionEndRadiusMultiplier;
		ObjectAnimator mDisappearAnimator;
		ObjectAnimator mReappearAnimator;
		private InvalidateUpdateListener mInvalidateUpdateListener;

		public RadialTextsView(Context context)
		{
			super(context);
			mIsInitialized = false;
		}
		public void initialize(Resources res, String[] texts, String[] innerTexts, boolean is24HourMode, boolean disappearsOut)
		{
			if(mIsInitialized) {
				Log.e(TAG, "This RadialTextsView may only be initialized once.");
				return;
			}
			// Set up the paint.
			int numbersTextColor = res.getColor(R.color.range_numbers_text_color);
			mPaint.setColor(numbersTextColor);
			String typefaceFamily = "sans-serif";//res.getString(R.string.range_radial_numbers_typeface);
			mTypefaceLight = Typeface.create(typefaceFamily, Typeface.NORMAL);
			String typefaceFamilyRegular = "sans-serif";//res.getString(R.string.range_sans_serif);
			mTypefaceRegular = Typeface.create(typefaceFamilyRegular, Typeface.NORMAL);
			mPaint.setAntiAlias(true);
			mPaint.setTextAlign(Paint.Align.CENTER);
			// Set up the selected paint
			int selectedTextColor = res.getColor(R.color.White);
			mSelectedPaint.setColor(selectedTextColor);
			mSelectedPaint.setAntiAlias(true);
			mSelectedPaint.setTextAlign(Paint.Align.CENTER);
			mTexts = texts;
			mInnerTexts = innerTexts;
			mIs24HourMode = is24HourMode;
			mHasInnerCircle = (innerTexts != null);
			// Calculate the radius for the main circle.
			if(is24HourMode) {
				mCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_circle_radius_multiplier_24HourMode));
			}
			else {
				mCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_circle_radius_multiplier));
				mAmPmCircleRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_ampm_circle_radius_multiplier));
			}
			// Initialize the widths and heights of the grid, and calculate the values for the numbers.
			mTextGridHeights = new float[7];
			mTextGridWidths = new float[7];
			if(mHasInnerCircle) {
				mNumbersRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_numbers_radius_multiplier_outer));
				mTextSizeMultiplier = Float.parseFloat(res.getString(R.string.range_text_size_multiplier_outer));
				mInnerNumbersRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_numbers_radius_multiplier_inner));
				mInnerTextSizeMultiplier = Float.parseFloat(res.getString(R.string.range_text_size_multiplier_inner));
				mInnerTextGridHeights = new float[7];
				mInnerTextGridWidths = new float[7];
			}
			else {
				mNumbersRadiusMultiplier = Float.parseFloat(res.getString(R.string.range_numbers_radius_multiplier_normal));
				mTextSizeMultiplier = Float.parseFloat(res.getString(R.string.range_text_size_multiplier_normal));
			}
			mAnimationRadiusMultiplier = 1;
			mTransitionMidRadiusMultiplier = 1f + (0.05f * (disappearsOut? -1 : 1));
			mTransitionEndRadiusMultiplier = 1f + (0.3f * (disappearsOut? 1 : -1));
			mInvalidateUpdateListener = new InvalidateUpdateListener();
			mTextGridValuesDirty = true;
			mIsInitialized = true;
		}
		/* package */ void setTheme(Context context, boolean themeDark)
		{
			Resources res = context.getResources();
			int textColor = res.getColor(themeDark ? R.color.White : R.color.range_numbers_text_color);
			mPaint.setColor(textColor);
		}
		//
		// Set the value of the selected text. Depending on the theme this will be rendered differently
		// @param selection The text which is currently selected
		//
		protected void setSelection(int selection) { this.selection = selection; }
		//
		// Allows for smoother animation.
		//
		@Override public boolean hasOverlappingRendering() { return false; }
		//
		// Used by the animation to move the numbers in and out.
		//
		@SuppressWarnings("unused") public void setAnimationRadiusMultiplier(float animationRadiusMultiplier)
		{
			mAnimationRadiusMultiplier = animationRadiusMultiplier;
			mTextGridValuesDirty = true;
		}
		@Override public void onDraw(Canvas canvas)
		{
			int viewWidth = getWidth();
			if(viewWidth == 0 || !mIsInitialized) {
				return;
			}
			if(!mDrawValuesReady) {
				mXCenter = getWidth() / 2;
				mYCenter = getHeight() / 2;
				mCircleRadius = Math.min(mXCenter, mYCenter) * mCircleRadiusMultiplier;
				if(!mIs24HourMode) {
					// We'll need to draw the AM/PM circles, so the main circle will need to have
					// a slightly higher center. To keep the entire view centered vertically, we'll
					// have to push it up by half the radius of the AM/PM circles.
					float amPmCircleRadius = mCircleRadius * mAmPmCircleRadiusMultiplier;
					mYCenter -= amPmCircleRadius *0.75;
				}
				mTextSize = mCircleRadius * mTextSizeMultiplier;
				if(mHasInnerCircle) {
					mInnerTextSize = mCircleRadius * mInnerTextSizeMultiplier;
				}
				// Because the text positions will be static, pre-render the animations.
				renderAnimations();
				mTextGridValuesDirty = true;
				mDrawValuesReady = true;
			}
			// Calculate the text positions, but only if they've changed since the last onDraw.
			if(mTextGridValuesDirty) {
				float numbersRadius = mCircleRadius * mNumbersRadiusMultiplier * mAnimationRadiusMultiplier;
				// Calculate the positions for the 12 numbers in the main circle.
				calculateGridSizes(numbersRadius, mXCenter, mYCenter, mTextSize, mTextGridHeights, mTextGridWidths);
				if(mHasInnerCircle) {
					// If we have an inner circle, calculate those positions too.
					float innerNumbersRadius = mCircleRadius * mInnerNumbersRadiusMultiplier * mAnimationRadiusMultiplier;
					calculateGridSizes(innerNumbersRadius, mXCenter, mYCenter, mInnerTextSize, mInnerTextGridHeights, mInnerTextGridWidths);
				}
				mTextGridValuesDirty = false;
			}
			// Draw the texts in the pre-calculated positions.
			drawTexts(canvas, mTextSize, mTypefaceLight, mTexts, mTextGridWidths, mTextGridHeights);
			if(mHasInnerCircle) {
				drawTexts(canvas, mInnerTextSize, mTypefaceRegular, mInnerTexts, mInnerTextGridWidths, mInnerTextGridHeights);
			}
		}
		//
		// Using the trigonometric Unit Circle, calculate the positions that the text will need to be
		// drawn at based on the specified circle radius. Place the values in the textGridHeights and
		// textGridWidths parameters.
		//
		private void calculateGridSizes(float numbersRadius, float xCenter, float yCenter, float textSize, float[] textGridHeights, float[] textGridWidths)
		{
			/*
			 * The numbers need to be drawn in a 7x7 grid, representing the points on the Unit Circle.
			 */
			float offset1 = numbersRadius;
			// cos(30) = a / r => r * cos(30) = a => r * √3/2 = a
			float offset2 = numbersRadius * ((float) Math.sqrt(3)) / 2f;
			// sin(30) = o / r => r * sin(30) = o => r / 2 = a
			float offset3 = numbersRadius / 2f;
			mPaint.setTextSize(textSize);
			mSelectedPaint.setTextSize(textSize);
			// We'll need yTextBase to be slightly lower to account for the text's baseline.
			yCenter -= (mPaint.descent() + mPaint.ascent()) / 2;
			textGridHeights[0] = yCenter - offset1;
			textGridWidths[0] = xCenter - offset1;
			textGridHeights[1] = yCenter - offset2;
			textGridWidths[1] = xCenter - offset2;
			textGridHeights[2] = yCenter - offset3;
			textGridWidths[2] = xCenter - offset3;
			textGridHeights[3] = yCenter;
			textGridWidths[3] = xCenter;
			textGridHeights[4] = yCenter + offset3;
			textGridWidths[4] = xCenter + offset3;
			textGridHeights[5] = yCenter + offset2;
			textGridWidths[5] = xCenter + offset2;
			textGridHeights[6] = yCenter + offset1;
			textGridWidths[6] = xCenter + offset1;
		}
		//
		// Draw the 12 text values at the positions specified by the textGrid parameters.
		//
		private void drawTexts(Canvas canvas, float textSize, Typeface typeface, String[] texts, float[] textGridWidths, float[] textGridHeights)
		{
			mPaint.setTextSize(textSize);
			mPaint.setTypeface(typeface);
			canvas.drawText(texts[0], textGridWidths[3], textGridHeights[0], Integer.parseInt(texts[0]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[1], textGridWidths[4], textGridHeights[1], Integer.parseInt(texts[1]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[2], textGridWidths[5], textGridHeights[2], Integer.parseInt(texts[2]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[3], textGridWidths[6], textGridHeights[3], Integer.parseInt(texts[3]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[4], textGridWidths[5], textGridHeights[4], Integer.parseInt(texts[4]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[5], textGridWidths[4], textGridHeights[5], Integer.parseInt(texts[5]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[6], textGridWidths[3], textGridHeights[6], Integer.parseInt(texts[6]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[7], textGridWidths[2], textGridHeights[5], Integer.parseInt(texts[7]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[8], textGridWidths[1], textGridHeights[4], Integer.parseInt(texts[8]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[9], textGridWidths[0], textGridHeights[3], Integer.parseInt(texts[9]) == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[10], textGridWidths[1], textGridHeights[2], Integer.parseInt(texts[10])  == selection ? mSelectedPaint : mPaint);
			canvas.drawText(texts[11], textGridWidths[2], textGridHeights[1], Integer.parseInt(texts[11])  == selection ? mSelectedPaint : mPaint);
		}
		//
		// Render the animations for appearing and disappearing.
		//
		private void renderAnimations()
		{
			Keyframe kf0, kf1, kf2, kf3;
			float midwayPoint = 0.2f;
			int duration = 500;
			// Set up animator for disappearing.
			kf0 = Keyframe.ofFloat(0f, 1);
			kf1 = Keyframe.ofFloat(midwayPoint, mTransitionMidRadiusMultiplier);
			kf2 = Keyframe.ofFloat(1f, mTransitionEndRadiusMultiplier);
			PropertyValuesHolder radiusDisappear = PropertyValuesHolder.ofKeyframe("animationRadiusMultiplier", kf0, kf1, kf2);
			kf0 = Keyframe.ofFloat(0f, 1f);
			kf1 = Keyframe.ofFloat(1f, 0f);
			PropertyValuesHolder fadeOut = PropertyValuesHolder.ofKeyframe("alpha", kf0, kf1);
			mDisappearAnimator = ObjectAnimator.ofPropertyValuesHolder(this, radiusDisappear, fadeOut).setDuration(duration);
			mDisappearAnimator.addUpdateListener(mInvalidateUpdateListener);
			// Set up animator for reappearing.
			float delayMultiplier = 0.25f;
			float transitionDurationMultiplier = 1f;
			float totalDurationMultiplier = transitionDurationMultiplier + delayMultiplier;
			int totalDuration = (int) (duration * totalDurationMultiplier);
			float delayPoint = (delayMultiplier * duration) / totalDuration;
			midwayPoint = 1 - (midwayPoint * (1 - delayPoint));
			kf0 = Keyframe.ofFloat(0f, mTransitionEndRadiusMultiplier);
			kf1 = Keyframe.ofFloat(delayPoint, mTransitionEndRadiusMultiplier);
			kf2 = Keyframe.ofFloat(midwayPoint, mTransitionMidRadiusMultiplier);
			kf3 = Keyframe.ofFloat(1f, 1);
			PropertyValuesHolder radiusReappear = PropertyValuesHolder.ofKeyframe("animationRadiusMultiplier", kf0, kf1, kf2, kf3);
			kf0 = Keyframe.ofFloat(0f, 0f);
			kf1 = Keyframe.ofFloat(delayPoint, 0f);
			kf2 = Keyframe.ofFloat(1f, 1f);
			PropertyValuesHolder fadeIn = PropertyValuesHolder.ofKeyframe("alpha", kf0, kf1, kf2);
			mReappearAnimator = ObjectAnimator.ofPropertyValuesHolder(this, radiusReappear, fadeIn).setDuration(totalDuration);
			mReappearAnimator.addUpdateListener(mInvalidateUpdateListener);
		}
		public ObjectAnimator getDisappearAnimator()
		{
			if(!mIsInitialized || !mDrawValuesReady || mDisappearAnimator == null) {
				Log.e(TAG, "RadialTextView was not ready for animation.");
				return null;
			}
			return mDisappearAnimator;
		}
		public ObjectAnimator getReappearAnimator()
		{
			if(!mIsInitialized || !mDrawValuesReady || mReappearAnimator == null) {
				Log.e(TAG, "RadialTextView was not ready for animation.");
				return null;
			}
			return mReappearAnimator;
		}
		private class InvalidateUpdateListener implements ValueAnimator.AnimatorUpdateListener {
			@Override public void onAnimationUpdate(ValueAnimator animation)
			{
				RadialTextsView.this.invalidate();
			}
		}
	}
	//
	// Dialog to set a time.
	//
	public static class TimePickerDialog extends DialogFragment implements RadialPickerLayout.OnValueSelectedListener {
		private static final String TAG = "TimePickerDialog";
		private static final String KEY_HOUR_OF_DAY = "hour_of_day";
		private static final String KEY_MINUTE = "minute";
		private static final String KEY_HOUR_OF_DAY_END = "hour_of_day_end";
		private static final String KEY_MINUTE_END = "minute_end";
		private static final String KEY_IS_24_HOUR_VIEW = "is_24_hour_view";
		private static final String KEY_TITLE = "dialog_title";
		private static final String KEY_CURRENT_ITEM_SHOWING = "current_item_showing";
		private static final String KEY_CURRENT_ITEM_SHOWING_END="current_item_showing_end";
		private static final String KEY_IN_KB_MODE = "in_kb_mode";
		private static final String KEY_TYPED_TIMES = "typed_times";
		private static final String KEY_DARK_THEME = "dark_theme";
		private static final String KEY_ACCENT = "accent";
		private static final String KEY_VIBRATE = "vibrate";
		private static final String KEY_DISMISS = "dismiss";
		public static final int HOUR_INDEX = 0;
		public static final int MINUTE_INDEX = 1;
		// NOT a real index for the purpose of what's showing.
		public static final int AMPM_INDEX = 2;
		// Also NOT a real index, just used for keyboard mode.
		public static final int ENABLE_PICKER_INDEX = 3;
		public static final int AM = 0;
		public static final int PM = 1;
		// Delay before starting the pulse animation, in ms.
		private static final int PULSE_ANIMATOR_DELAY = 300;
		//private OnTimeSetListener mCallback;
		private SLib.EventHandler Handler;
		private DialogInterface.OnCancelListener mOnCancelListener;
		private DialogInterface.OnDismissListener mOnDismissListener;
		private HapticFeedbackController mHapticFeedbackController;
		private Button mCancelButton;
		private Button mOkButton;
		private TextView mHourView;
		private TextView mHourSpaceView;
		private TextView mMinuteView;
		private TextView mMinuteSpaceView;
		private TextView mAmPmTextView;
		private View mAmPmHitspace;
		private RadialPickerLayout mTimePicker;
		private int mSelectedColor;
		private int mUnselectedColor;
		private String mAmText;
		private String mPmText;
		private String mIndicatorFrom;
		private String mIndicatorTo;
		private boolean mAllowAutoAdvance;
		private int mInitialHourOfDay;
		private int mInitialMinute;
		private int mInitialHourOfDayEnd;
		private int mInitialMinuteEnd;
		private boolean mIs24HourMode;
		private String mTitle;
		private boolean mThemeDark;
		private boolean mVibrate;
		private int mAccentColor = -1;
		private boolean mDismissOnPause;
		// For hardware IME input.
		private char mPlaceholderText;
		private String mDoublePlaceholderText;
		private String mDeletedKeyFormat;
		private boolean mInKbMode;
		private ArrayList<Integer> mTypedTimes;
		private Node mLegalTimesTree;
		private int mAmKeyCode;
		private int mPmKeyCode;
		// Accessibility strings.
		private String mHourPickerDescription;
		private String mSelectHours;
		private String mMinutePickerDescription;
		private String mSelectMinutes;
		private TabHost tabHost;
		private TextView mHourViewEnd;
		private TextView mHourSpaceViewEnd;
		private TextView mMinuteSpaceViewEnd;
		private TextView mMinuteViewEnd;
		private TextView mAmPmTextViewEnd;
		private RadialPickerLayout mTimePickerEnd;
		private View mAmPmHitspaceEnd;
		//
		// The callback interface used to indicate the user is done filling in
		// the time (they clicked on the 'Set' button).
		//
		public interface OnTimeSetListener {
			//
			// @param view The view associated with this listener.
			// @param hourOfDay The hour that was set.
			// @param minute The minute that was set.
			//
			void onTimeSet(RadialPickerLayout view, int hourOfDay, int minute, int hourOfDayEnd, int minuteEnd);
		}
		public TimePickerDialog()
		{
			// Empty constructor required for dialog fragment.
		}
		/**
		 public TimePickerDialog(Context context, int theme, OnTimeSetListener callback,
		 int hourOfDay, int minute, boolean is24HourMode) {
		 // Empty constructor required for dialog fragment.
		 }
		 **/
		public static TimePickerDialog newInstance(SLib.EventHandler handler, int hourOfDay, int minute, boolean is24HourMode)
		{
			TimePickerDialog ret = new TimePickerDialog();
			ret.initialize(handler, hourOfDay, minute, is24HourMode);
			return ret;
		}
		public static TimePickerDialog newInstance(SLib.EventHandler handler, int hourOfDay, int minute, boolean is24HourMode, int hourOfDayEnd, int minuteEnd)
		{
			TimePickerDialog ret = new TimePickerDialog();
			ret.initialize(handler, hourOfDay, minute,hourOfDayEnd,minuteEnd,is24HourMode);
			return ret;
		}
		public void initialize(SLib.EventHandler handler, int hourOfDay, int minute, boolean is24HourMode)
		{
			initialize(handler, hourOfDay, minute, hourOfDay, minute, is24HourMode);
		}
		public void initialize(SLib.EventHandler handler, int hourOfDay, int minute, int hourOfDayEnd, int minuteEnd, boolean is24HourMode)
		{
			//mCallback = callback;
			Handler = handler;
			mInitialHourOfDay = hourOfDay;
			mInitialMinute = minute;
			mInitialHourOfDayEnd = hourOfDayEnd;
			mInitialMinuteEnd = minuteEnd;
			mIs24HourMode = is24HourMode;
			mInKbMode = false;
			mTitle = "";
			mThemeDark = false;
			mAccentColor = -1;
			mVibrate = true;
			mDismissOnPause = false;
		}
		//
		// Set a title. NOTE: this will only take effect with the next onCreateView
		//
		public void setTitle(String title) {
			mTitle = title;
		}
		public String getTitle() {
			return mTitle;
		}
		//
		// Set tab indicators. NOTE: this will only take effect with the next onCreateView
		//
		public void setTabIndicators(String from, String to)
		{
			mIndicatorFrom = from;
			mIndicatorTo = to;
		}
		//
		// Set a dark or light theme. NOTE: this will only take effect for the next onCreateView.
		//
		public void setThemeDark(boolean dark) {
			mThemeDark = dark;
		}
		public void setAccentColor(int color) {
			mAccentColor = color;
		}
		public boolean isThemeDark() {
			return mThemeDark;
		}
		//
		// Set whether the device should vibrate when touching fields
		// @param vibrate true if the device should vibrate when touching a field
		//
		public void vibrate(boolean vibrate) {
			mVibrate = vibrate;
		}
		//
		// Set whether the picker should dismiss itself when it's pausing or whether it should try to survive an orientation change
		// @param dismissOnPause true if the picker should dismiss itself
		//
		public void dismissOnPause(boolean dismissOnPause) {
			mDismissOnPause = dismissOnPause;
		}
		//public void setOnTimeSetListener(OnTimeSetListener callback) { mCallback = callback; }
		public void SetEventHandler(SLib.EventHandler handler) { Handler = handler; }
		public void setOnCancelListener(DialogInterface.OnCancelListener onCancelListener)
		{
			mOnCancelListener = onCancelListener;
		}
		public void setOnDismissListener(DialogInterface.OnDismissListener onDismissListener)
		{
			mOnDismissListener = onDismissListener;
		}
		public void setStartTime(int hourOfDay, int minute)
		{
			mInitialHourOfDay = hourOfDay;
			mInitialMinute = minute;
			mInKbMode = false;
		}
		public void setEndTime(int hourOfDayEnd, int minuteEnd)
		{
			mInitialHourOfDayEnd = hourOfDayEnd;
			mInitialMinuteEnd = minuteEnd;
			mInKbMode = false;
		}
		@Override public void onCreate(Bundle savedInstanceState)
		{
			super.onCreate(savedInstanceState);
			if(savedInstanceState != null && savedInstanceState.containsKey(KEY_HOUR_OF_DAY) &&
					savedInstanceState.containsKey(KEY_MINUTE) && savedInstanceState.containsKey(KEY_IS_24_HOUR_VIEW)) {
				mInitialHourOfDay = savedInstanceState.getInt(KEY_HOUR_OF_DAY);
				mInitialMinute = savedInstanceState.getInt(KEY_MINUTE);
				mInitialHourOfDayEnd = savedInstanceState.getInt(KEY_HOUR_OF_DAY_END);
				mInitialMinuteEnd = savedInstanceState.getInt(KEY_MINUTE_END);
				mIs24HourMode = savedInstanceState.getBoolean(KEY_IS_24_HOUR_VIEW);
				mInKbMode = savedInstanceState.getBoolean(KEY_IN_KB_MODE);
				mTitle = savedInstanceState.getString(KEY_TITLE);
				mThemeDark = savedInstanceState.getBoolean(KEY_DARK_THEME);
				mAccentColor = savedInstanceState.getInt(KEY_ACCENT);
				mVibrate = savedInstanceState.getBoolean(KEY_VIBRATE);
			}
		}
		@Override public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
		{
			String text_from = "";
			String text_to = "";
			getDialog().getWindow().requestFeature(Window.FEATURE_NO_TITLE);
			View view = inflater.inflate(R.layout.range_time_picker_dialog, null);
			KeyboardListener keyboardListener = new KeyboardListener();
			view.findViewById(R.id.range_time_picker_dialog).setOnKeyListener(keyboardListener);
			Resources res = getResources();
			{
				Context _ctx = getContext();
				if(_ctx != null && _ctx instanceof StyloQApp) {
					StyloQApp app_ctx = (StyloQApp)_ctx;
					text_from = app_ctx.GetString("from");
					text_to = app_ctx.GetString("to");
					mHourPickerDescription = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_ROUNDHOURPICKER); // res.getString(R.string.range_hour_picker_description);
					mMinutePickerDescription = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_ROUNDMINUTEPICKER); // res.getString(R.string.range_minute_picker_description);
					mSelectHours = app_ctx.GetString("selecthour");//res.getString(R.string.range_select_hours);
					mSelectMinutes = app_ctx.GetString("selectminut"); //res.getString(R.string.range_select_minutes);
				}
				//mHourPickerDescription = res.getString(R.string.range_hour_picker_description);
				//mSelectHours = res.getString(R.string.range_select_hours);
				//mMinutePickerDescription = res.getString(R.string.range_minute_picker_description);
				//mSelectMinutes = res.getString(R.string.range_select_minutes);
				mSelectedColor = res.getColor(R.color.White);
				mUnselectedColor = res.getColor(R.color.range_accent_color_focused);
			}
			tabHost = (TabHost) view.findViewById(R.id.range_tabHost);
			tabHost.findViewById(R.id.range_tabHost);
			tabHost.setup();
			TabHost.TabSpec startDatePage = tabHost.newTabSpec("start");
			startDatePage.setContent(R.id.start_date_group);
			startDatePage.setIndicator(TextUtils.isEmpty(mIndicatorFrom) ? text_from/*getActivity().getResources().getString(R.string.range_from)*/ : mIndicatorFrom);
			TabHost.TabSpec endDatePage = tabHost.newTabSpec("end");
			endDatePage.setContent(R.id.range_end_date_group);
			endDatePage.setIndicator(TextUtils.isEmpty(mIndicatorTo) ? text_to/*getActivity().getResources().getString(R.string.range_to)*/ : mIndicatorTo);
			tabHost.addTab(startDatePage);
			tabHost.addTab(endDatePage);
			mHourView = (TextView) view.findViewById(R.id.range_hours);
			mHourView.setOnKeyListener(keyboardListener);
			mHourViewEnd = (TextView) view.findViewById(R.id.range_hours_end);
			mHourViewEnd.setOnKeyListener(keyboardListener);
			mHourSpaceView = (TextView) view.findViewById(R.id.range_hour_space);
			mHourSpaceViewEnd = (TextView) view.findViewById(R.id.range_hour_space_end);
			mMinuteSpaceView = (TextView) view.findViewById(R.id.range_minutes_space);
			mMinuteSpaceViewEnd = (TextView) view.findViewById(R.id.range_minutes_space_end);
			mMinuteView = (TextView) view.findViewById(R.id.range_minutes);
			mMinuteView.setOnKeyListener(keyboardListener);
			mMinuteViewEnd = (TextView) view.findViewById(R.id.range_minutes_end);
			mMinuteViewEnd.setOnKeyListener(keyboardListener);
			mAmPmTextView = (TextView) view.findViewById(R.id.range_ampm_label);
			mAmPmTextView.setOnKeyListener(keyboardListener);
			mAmPmTextViewEnd = (TextView) view.findViewById(R.id.range_ampm_label_end);
			mAmPmTextViewEnd.setOnKeyListener(keyboardListener);
			String[] amPmTexts = new DateFormatSymbols().getAmPmStrings();
			mAmText = amPmTexts[0];
			mPmText = amPmTexts[1];
			mHapticFeedbackController = new HapticFeedbackController(getActivity());
			mTimePicker = (RadialPickerLayout) view.findViewById(R.id.range_time_picker);
			mTimePicker.setOnValueSelectedListener(this);
			mTimePicker.setOnKeyListener(keyboardListener);
			mTimePicker.initialize(getActivity(), this, mInitialHourOfDay, mInitialMinute, mIs24HourMode);
			mTimePickerEnd = (RadialPickerLayout) view.findViewById(R.id.range_time_picker_end);
			mTimePickerEnd.setOnValueSelectedListener(this);
			mTimePickerEnd.setOnKeyListener(keyboardListener);
			mTimePickerEnd.initialize(getActivity(), this, mInitialHourOfDayEnd, mInitialMinuteEnd, mIs24HourMode);
			int currentItemShowing = HOUR_INDEX;
			int currentItemShowingEnd = HOUR_INDEX;
			if(savedInstanceState != null && savedInstanceState.containsKey(KEY_CURRENT_ITEM_SHOWING)) {
				currentItemShowing = savedInstanceState.getInt(KEY_CURRENT_ITEM_SHOWING);
			}
			if(savedInstanceState != null && savedInstanceState.containsKey(KEY_CURRENT_ITEM_SHOWING_END)) {
				currentItemShowingEnd = savedInstanceState.getInt(KEY_CURRENT_ITEM_SHOWING_END);
			}
			setCurrentItemShowing(currentItemShowing, false, true, true);
			setCurrentItemShowing(currentItemShowingEnd, false, true, true);
			mTimePicker.invalidate();
			mTimePickerEnd.invalidate();
			mHourView.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v)
				{
					setCurrentItemShowing(HOUR_INDEX, true, false, true);
					tryVibrate();
				}
			});
			mHourViewEnd.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v)
				{
					setCurrentItemShowing(HOUR_INDEX, true, false, true);
					tryVibrate();
				}
			});
			mMinuteView.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v)
				{
					setCurrentItemShowing(MINUTE_INDEX, true, false, true);
					tryVibrate();
				}
			});
			mMinuteViewEnd.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v)
				{
					setCurrentItemShowing(MINUTE_INDEX, true, false, true);
					tryVibrate();
				}
			});
			mOkButton = (Button) view.findViewById(R.id.range_ok);
			mOkButton.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v)
				{
					if(mInKbMode && isTypedTimeFullyLegal()) {
						finishKbMode(false);
					}
					else {
						tryVibrate();
					}
					/*if(mCallback != null) {
						mCallback.onTimeSet(mTimePicker, mTimePicker.getHours(), mTimePicker.getMinutes(),mTimePickerEnd.getHours(), mTimePickerEnd.getMinutes());
					}*/
					if(Handler != null) {
						SLib.STimeChunk tc = new SLib.STimeChunk();
						Handler.HandleEvent(SLib.EV_DATETIMEPICKERREPLY, this, null);
					}
					dismiss();
				}
			});
			mOkButton.setOnKeyListener(keyboardListener);
			mOkButton.setTypeface(TypefaceHelper.get(getDialog().getContext(), "Roboto-Medium"));
			mCancelButton = (Button) view.findViewById(R.id.range_cancel);
			mCancelButton.setOnClickListener(new View.OnClickListener() {
				@Override public void onClick(View v)
				{
					tryVibrate();
					if(getDialog() != null)
						getDialog().cancel();
				}
			});
			mCancelButton.setTypeface(TypefaceHelper.get(getDialog().getContext(),"Roboto-Medium"));
			mCancelButton.setVisibility(isCancelable() ? View.VISIBLE : View.GONE);
			// Enable or disable the AM/PM view.
			mAmPmHitspace = view.findViewById(R.id.range_ampm_hitspace);
			mAmPmHitspaceEnd = view.findViewById(R.id.range_ampm_hitspace_end);
			if(mIs24HourMode) {
				mAmPmTextView.setVisibility(View.GONE);
				mAmPmTextViewEnd.setVisibility(View.GONE);
				RelativeLayout.LayoutParams paramsSeparator = new RelativeLayout.LayoutParams(ActionBar.LayoutParams.WRAP_CONTENT, ActionBar.LayoutParams.WRAP_CONTENT);
				paramsSeparator.addRule(RelativeLayout.CENTER_IN_PARENT);
				TextView separatorView = (TextView) view.findViewById(R.id.range_separator);
				TextView separatorViewEnd = (TextView) view.findViewById(R.id.range_separator_end);
				separatorView.setLayoutParams(paramsSeparator);
				separatorViewEnd.setLayoutParams(paramsSeparator);
			}
			else {
				mAmPmTextView.setVisibility(View.VISIBLE);
				mAmPmTextViewEnd.setVisibility(View.VISIBLE);
				updateAmPmDisplay(mInitialHourOfDay < 12 ? AM : PM);
				mAmPmHitspace.setOnClickListener(new View.OnClickListener() {
					@Override public void onClick(View v)
					{
						tryVibrate();
						int amOrPm = mTimePicker.getIsCurrentlyAmOrPm();
						if(amOrPm == AM) {
							amOrPm = PM;
						}
						else if(amOrPm == PM) {
							amOrPm = AM;
						}
						updateAmPmDisplay(amOrPm);
						mTimePicker.setAmOrPm(amOrPm);
					}
				});
				mAmPmHitspaceEnd.setOnClickListener(new View.OnClickListener() {
					@Override public void onClick(View v)
					{
						tryVibrate();
						int amOrPm = mTimePickerEnd.getIsCurrentlyAmOrPm();
						if(amOrPm == AM) {
							amOrPm = PM;
						}
						else if(amOrPm == PM){
							amOrPm = AM;
						}
						updateAmPmDisplay(amOrPm);
						mTimePickerEnd.setAmOrPm(amOrPm);
					}
				});
			}
			mAllowAutoAdvance = true;
			setHour(mInitialHourOfDay, true);
			setMinute(mInitialMinute);
			// Set up for keyboard mode.
			mDoublePlaceholderText = "--"; //res.getString(R.string.range_time_placeholder);
			mDeletedKeyFormat = "%1$s deleted"; //res.getString(R.string.range_deleted_key);
			mPlaceholderText = mDoublePlaceholderText.charAt(0);
			mAmKeyCode = mPmKeyCode = -1;
			generateLegalTimesTree();
			if(mInKbMode) {
				mTypedTimes = savedInstanceState.getIntegerArrayList(KEY_TYPED_TIMES);
				tryStartingKbMode(-1);
				mHourView.invalidate();
				mHourViewEnd.invalidate();
			}
			else if(mTypedTimes == null) {
				mTypedTimes = new ArrayList<>();
			}
			// Set the title (if any)
			TextView timePickerHeader = (TextView) view.findViewById(R.id.range_time_picker_header);
			TextView timePickerHeaderEnd = (TextView) view.findViewById(R.id.range_time_picker_header_end);
			if(!mTitle.isEmpty()) {
				timePickerHeader.setVisibility(TextView.VISIBLE);
				timePickerHeader.setText(mTitle);
				timePickerHeaderEnd.setVisibility(TextView.VISIBLE);
				timePickerHeaderEnd.setText(mTitle);
			}
			// Set the theme at the end so that the initialize()s above don't counteract the theme.
			mTimePicker.setTheme(getActivity().getApplicationContext(), mThemeDark);
			mTimePickerEnd.setTheme(getActivity().getApplicationContext(), mThemeDark);
			//If an accent color has not been set manually, try and get it from the context
			if(mAccentColor == -1) {
				int accentColor = Utils.getAccentColorFromThemeIfAvailable(getActivity());
				if(accentColor != -1) {
					mAccentColor = accentColor;
				}
			}
			if(mAccentColor != -1) {
				mTimePicker.setAccentColor(mAccentColor);
				mTimePickerEnd.setAccentColor(mAccentColor);
				mOkButton.setTextColor(mAccentColor);
			}
			else {
				int circleBackground = res.getColor(R.color.range_circle_background);
				int backgroundColor = res.getColor(R.color.range_background_color);
				int darkBackgroundColor = res.getColor(R.color.range_light_gray);
				int lightGray = res.getColor(R.color.range_light_gray);
				mTimePicker.setBackgroundColor(mThemeDark? lightGray : circleBackground);
				mTimePickerEnd.setBackgroundColor(mThemeDark ? lightGray : circleBackground);
				view.findViewById(R.id.range_time_picker_dialog).setBackgroundColor(mThemeDark ? darkBackgroundColor : backgroundColor);
			}
			tabHost.setOnTabChangedListener(new TabHost.OnTabChangeListener() {
				@Override public void onTabChanged(String tabId)
				{
					if(tabId=="start"){
						setCurrentItemShowing(mTimePicker.getCurrentItemShowing(), true, false, true);
						setHour(mTimePicker.getHours(),false);
						setMinute(mTimePicker.getMinutes());
						updateAmPmDisplay(mTimePicker.getIsCurrentlyAmOrPm());
					}
					else {
						setCurrentItemShowing(mTimePickerEnd.getCurrentItemShowing(), true, false, true);
						setHour(mTimePickerEnd.getHours(),false);
						setMinute(mTimePickerEnd.getMinutes());
						updateAmPmDisplay(mTimePickerEnd.getIsCurrentlyAmOrPm());
					}
				}
			});
			return view;
		}
		@Override public void onResume()
		{
			super.onResume();
			mHapticFeedbackController.start();
		}
		@Override public void onPause()
		{
			super.onPause();
			mHapticFeedbackController.stop();
			if(mDismissOnPause)
				dismiss();
		}
		@Override public void onCancel(DialogInterface dialog)
		{
			super.onCancel(dialog);
			if(mOnCancelListener != null)
				mOnCancelListener.onCancel(dialog);
		}
		@Override public void onDismiss(DialogInterface dialog)
		{
			super.onDismiss(dialog);
			if(mOnDismissListener != null)
				mOnDismissListener.onDismiss(dialog);
		}
		public void tryVibrate()
		{
			if(mVibrate)
				mHapticFeedbackController.tryVibrate();
		}
		private void updateAmPmDisplay(int amOrPm)
		{
			if(amOrPm == AM) {
				if(tabHost.getCurrentTab()==0){
					mAmPmTextView.setText(mAmText);
					mAmPmHitspace.setContentDescription(mAmText);
					Utils.tryAccessibilityAnnounce(mTimePicker, mAmText);
				}
				else{
					mAmPmTextViewEnd.setText(mAmText);
					mAmPmHitspaceEnd.setContentDescription(mAmText);
					Utils.tryAccessibilityAnnounce(mTimePickerEnd, mAmText);
				}

			}
			else if(amOrPm == PM){
				if(tabHost.getCurrentTab()==0){
					mAmPmTextView.setText(mPmText);
					mAmPmHitspace.setContentDescription(mPmText);
					Utils.tryAccessibilityAnnounce(mTimePicker, mPmText);
				}
				else{
					mAmPmTextViewEnd.setText(mPmText);
					mAmPmHitspaceEnd.setContentDescription(mPmText);
					Utils.tryAccessibilityAnnounce(mTimePickerEnd, mPmText);
				}

			}
			else {
				if(tabHost.getCurrentTab()==0){
					mAmPmTextView.setText(mDoublePlaceholderText);
				}
				else{
					mAmPmTextViewEnd.setText(mDoublePlaceholderText);
				}
			}
		}
		@Override public void onSaveInstanceState(@NonNull Bundle outState)
		{
			if(mTimePicker != null) {
				outState.putInt(KEY_HOUR_OF_DAY, mTimePicker.getHours());
				outState.putInt(KEY_MINUTE, mTimePicker.getMinutes());
				outState.putInt(KEY_HOUR_OF_DAY_END, mTimePickerEnd.getHours());
				outState.putInt(KEY_MINUTE_END, mTimePickerEnd.getMinutes());
				outState.putBoolean(KEY_IS_24_HOUR_VIEW, mIs24HourMode);
				outState.putInt(KEY_CURRENT_ITEM_SHOWING, mTimePicker.getCurrentItemShowing());
				outState.putInt(KEY_CURRENT_ITEM_SHOWING_END, mTimePickerEnd.getCurrentItemShowing());
				outState.putBoolean(KEY_IN_KB_MODE, mInKbMode);
				if(mInKbMode) {
					outState.putIntegerArrayList(KEY_TYPED_TIMES, mTypedTimes);
				}
				outState.putString(KEY_TITLE, mTitle);
				outState.putBoolean(KEY_DARK_THEME, mThemeDark);
				outState.putInt(KEY_ACCENT, mAccentColor);
				outState.putBoolean(KEY_VIBRATE, mVibrate);
			}
		}
		//
		// Called by the picker for updating the header display.
		//
		@Override public void onValueSelected(int pickerIndex, int newValue, boolean autoAdvance)
		{
			if(pickerIndex == HOUR_INDEX) {
				setHour(newValue, false);
				String announcement = String.format("%d", newValue);
				if(mAllowAutoAdvance && autoAdvance) {
					setCurrentItemShowing(MINUTE_INDEX, true, true, false);
					announcement += ". " + mSelectMinutes;
				}
				else {
					if(tabHost.getCurrentTab()==0){
						mTimePicker.setContentDescription(mHourPickerDescription + ": " + newValue);
						Utils.tryAccessibilityAnnounce(mTimePicker, announcement);
					}
					else{
						mTimePickerEnd.setContentDescription(mHourPickerDescription + ": " + newValue);
						Utils.tryAccessibilityAnnounce(mTimePickerEnd, announcement);
					}
				}

			}
			else if(pickerIndex == MINUTE_INDEX){
				setMinute(newValue);
				if(tabHost.getCurrentTab()==0){
					mTimePicker.setContentDescription(mMinutePickerDescription + ": " + newValue);
				}
				else{
					mTimePickerEnd.setContentDescription(mMinutePickerDescription + ": " + newValue);
				}

			}
			else if(pickerIndex == AMPM_INDEX) {
				updateAmPmDisplay(newValue);
			}
			else if(pickerIndex == ENABLE_PICKER_INDEX) {
				if(!isTypedTimeFullyLegal()) {
					mTypedTimes.clear();
				}
				finishKbMode(true);
			}
		}
		private void setHour(int value, boolean announce)
		{
			String format;
			if(mIs24HourMode) {
				format = "%02d";
			}
			else {
				format = "%d";
				value = value % 12;
				if(value == 0)
					value = 12;
			}
			CharSequence text = String.format(format, value);
			if(tabHost.getCurrentTab()==0){
				mHourView.setText(text);
				mHourSpaceView.setText(text);
				if(announce) {
					Utils.tryAccessibilityAnnounce(mTimePicker, text);
				}
			}
			else{
				mHourViewEnd.setText(text);
				mHourSpaceViewEnd.setText(text);
				if(announce) {
					Utils.tryAccessibilityAnnounce(mTimePickerEnd, text);
				}
			}
		}
		private void setMinute(int value)
		{
			if(value == 60) {
				value = 0;
			}
			CharSequence text = String.format(Locale.getDefault(), "%02d", value);
			if(tabHost.getCurrentTab()==0){
				Utils.tryAccessibilityAnnounce(mTimePicker, text);
				mMinuteView.setText(text);
				mMinuteSpaceView.setText(text);
			}
			else {
				Utils.tryAccessibilityAnnounce(mTimePickerEnd, text);
				mMinuteViewEnd.setText(text);
				mMinuteSpaceViewEnd.setText(text);
			}
		}
		// Show either Hours or Minutes.
		private void setCurrentItemShowing(int index, boolean animateCircle, boolean delayLabelAnimate, boolean announce)
		{
			if(tabHost.getCurrentTab()==0){
				mTimePicker.setCurrentItemShowing(index, animateCircle);
				TextView labelToAnimate;
				if(index == HOUR_INDEX) {
					int hours = mTimePicker.getHours();
					if(!mIs24HourMode) {
						hours = hours % 12;
					}
					mTimePicker.setContentDescription(mHourPickerDescription + ": " + hours);
					if(announce) {
						Utils.tryAccessibilityAnnounce(mTimePicker, mSelectHours);
					}
					labelToAnimate = mHourView;
				}
				else {
					int minutes = mTimePicker.getMinutes();
					mTimePicker.setContentDescription(mMinutePickerDescription + ": " + minutes);
					if(announce) {
						Utils.tryAccessibilityAnnounce(mTimePicker, mSelectMinutes);
					}
					labelToAnimate = mMinuteView;
				}
				int hourColor = (index == HOUR_INDEX)? mSelectedColor : mUnselectedColor;
				int minuteColor = (index == MINUTE_INDEX)? mSelectedColor : mUnselectedColor;
				mHourView.setTextColor(hourColor);
				mMinuteView.setTextColor(minuteColor);
				ObjectAnimator pulseAnimator = Utils.getPulseAnimator(labelToAnimate, 0.85f, 1.1f);
				if(delayLabelAnimate) {
					pulseAnimator.setStartDelay(PULSE_ANIMATOR_DELAY);
				}
				pulseAnimator.start();
			}
			else{
				mTimePickerEnd.setCurrentItemShowing(index, animateCircle);
				TextView labelToAnimate;
				if(index == HOUR_INDEX) {
					int hours = mTimePickerEnd.getHours();
					if(!mIs24HourMode) {
						hours = hours % 12;
					}
					mTimePickerEnd.setContentDescription(mHourPickerDescription + ": " + hours);
					if(announce) {
						Utils.tryAccessibilityAnnounce(mTimePickerEnd, mSelectHours);
					}
					labelToAnimate = mHourViewEnd;
				}
				else {
					int minutes = mTimePickerEnd.getMinutes();
					mTimePickerEnd.setContentDescription(mMinutePickerDescription + ": " + minutes);
					if(announce) {
						Utils.tryAccessibilityAnnounce(mTimePickerEnd, mSelectMinutes);
					}
					labelToAnimate = mMinuteViewEnd;
				}
				int hourColor = (index == HOUR_INDEX)? mSelectedColor : mUnselectedColor;
				int minuteColor = (index == MINUTE_INDEX)? mSelectedColor : mUnselectedColor;
				mHourViewEnd.setTextColor(hourColor);
				mMinuteViewEnd.setTextColor(minuteColor);
				ObjectAnimator pulseAnimator = Utils.getPulseAnimator(labelToAnimate, 0.85f, 1.1f);
				if(delayLabelAnimate) {
					pulseAnimator.setStartDelay(PULSE_ANIMATOR_DELAY);
				}
				pulseAnimator.start();
			}
		}
		//
		// For keyboard mode, processes key events.
		// @param keyCode the pressed key.
		// @return true if the key was successfully processed, false otherwise.
		//
		private boolean processKeyUp(int keyCode)
		{
			if(keyCode == KeyEvent.KEYCODE_ESCAPE || keyCode == KeyEvent.KEYCODE_BACK) {
				if(isCancelable())
					dismiss();
				return true;
			}
			else if(keyCode == KeyEvent.KEYCODE_TAB) {
				if(mInKbMode) {
					if(isTypedTimeFullyLegal()) {
						finishKbMode(true);
					}
					return true;
				}
			}
			else if(keyCode == KeyEvent.KEYCODE_ENTER) {
				if(mInKbMode) {
					if(!isTypedTimeFullyLegal())
						return true;
					finishKbMode(false);
				}
				/*if(mCallback != null) {
					mCallback.onTimeSet(mTimePicker, mTimePicker.getHours(), mTimePicker.getMinutes(),mTimePickerEnd.getHours(), mTimePickerEnd.getMinutes());
				}*/
				if(Handler != null) {
					SLib.STimeChunk tc = new SLib.STimeChunk();
					Handler.HandleEvent(SLib.EV_DATETIMEPICKERREPLY, this, null);
				}
				dismiss();
				return true;
			}
			else if(keyCode == KeyEvent.KEYCODE_DEL) {
				if(mInKbMode) {
					if(!mTypedTimes.isEmpty()) {
						int deleted = deleteLastTypedKey();
						String deletedKeyStr;
						if(deleted == getAmOrPmKeyCode(AM)) {
							deletedKeyStr = mAmText;
						}
						else if(deleted == getAmOrPmKeyCode(PM)) {
							deletedKeyStr = mPmText;
						}
						else {
							deletedKeyStr = String.format("%d", getValFromKeyCode(deleted));
						}
						if(tabHost.getCurrentTab()==0){
							Utils.tryAccessibilityAnnounce(mTimePicker, String.format(mDeletedKeyFormat, deletedKeyStr));
						}
						else {
							Utils.tryAccessibilityAnnounce(mTimePickerEnd, String.format(mDeletedKeyFormat, deletedKeyStr));
						}
						updateDisplay(true);
					}
				}
			}
			else if(keyCode == KeyEvent.KEYCODE_0 || keyCode == KeyEvent.KEYCODE_1
					|| keyCode == KeyEvent.KEYCODE_2 || keyCode == KeyEvent.KEYCODE_3
					|| keyCode == KeyEvent.KEYCODE_4 || keyCode == KeyEvent.KEYCODE_5
					|| keyCode == KeyEvent.KEYCODE_6 || keyCode == KeyEvent.KEYCODE_7
					|| keyCode == KeyEvent.KEYCODE_8 || keyCode == KeyEvent.KEYCODE_9
					|| (!mIs24HourMode && (keyCode == getAmOrPmKeyCode(AM) || keyCode == getAmOrPmKeyCode(PM)))) {
				if(!mInKbMode) {
					if(mTimePicker == null) {
						// Something's wrong, because time picker should definitely not be null.
						Log.e(TAG, "Unable to initiate keyboard mode, TimePicker was null.");
						return true;
					}
					mTypedTimes.clear();
					tryStartingKbMode(keyCode);
					return true;
				}
				// We're already in keyboard mode.
				if(addKeyIfLegal(keyCode)) {
					updateDisplay(false);
				}
				return true;
			}
			return false;
		}
		//
		// Try to start keyboard mode with the specified key, as long as the timepicker is not in the
		// middle of a touch-event.
		// @param keyCode The key to use as the first press. Keyboard mode will not be started if the
		// key is not legal to start with. Or, pass in -1 to get into keyboard mode without a starting
		// key.
		//
		private void tryStartingKbMode(int keyCode)
		{
			if(mTimePicker.trySettingInputEnabled(false) && (keyCode == -1 || addKeyIfLegal(keyCode))) {
				mInKbMode = true;
				mOkButton.setEnabled(false);
				updateDisplay(false);
			}
		}
		private boolean addKeyIfLegal(int keyCode)
		{
			// If we're in 24hour mode, we'll need to check if the input is full. If in AM/PM mode,
			// we'll need to see if AM/PM have been typed.
			if ((mIs24HourMode && mTypedTimes.size() == 4) || (!mIs24HourMode && isTypedTimeFullyLegal())) {
				return false;
			}
			mTypedTimes.add(keyCode);
			if(!isTypedTimeLegalSoFar()) {
				deleteLastTypedKey();
				return false;
			}
			int val = getValFromKeyCode(keyCode);
			if(tabHost.getCurrentTab()==0){
				Utils.tryAccessibilityAnnounce(mTimePicker, String.format("%d", val));
			}
			else{
				Utils.tryAccessibilityAnnounce(mTimePickerEnd, String.format("%d", val));
			}
			// Automatically fill in 0's if AM or PM was legally entered.
			if(isTypedTimeFullyLegal()) {
				if(!mIs24HourMode && mTypedTimes.size() <= 3) {
					mTypedTimes.add(mTypedTimes.size() - 1, KeyEvent.KEYCODE_0);
					mTypedTimes.add(mTypedTimes.size() - 1, KeyEvent.KEYCODE_0);
				}
				mOkButton.setEnabled(true);
			}
			return true;
		}
		//
		// Traverse the tree to see if the keys that have been typed so far are legal as is,
		// or may become legal as more keys are typed (excluding backspace).
		//
		private boolean isTypedTimeLegalSoFar()
		{
			Node node = mLegalTimesTree;
			for(int keyCode : mTypedTimes) {
				node = node.canReach(keyCode);
				if(node == null)
					return false;
			}
			return true;
		}
		//
		// Check if the time that has been typed so far is completely legal, as is.
		//
		private boolean isTypedTimeFullyLegal()
		{
			if(mIs24HourMode) {
				// For 24-hour mode, the time is legal if the hours and minutes are each legal. Note:
				// getEnteredTime() will ONLY call isTypedTimeFullyLegal() when NOT in 24hour mode.
				int[] values = getEnteredTime(null);
				return (values[0] >= 0 && values[1] >= 0 && values[1] < 60);
			}
			else {
				// For AM/PM mode, the time is legal if it contains an AM or PM, as those can only be
				// legally added at specific times based on the tree's algorithm.
				return (mTypedTimes.contains(getAmOrPmKeyCode(AM)) || mTypedTimes.contains(getAmOrPmKeyCode(PM)));
			}
		}
		private int deleteLastTypedKey()
		{
			int deleted = mTypedTimes.remove(mTypedTimes.size() - 1);
			if(!isTypedTimeFullyLegal())
				mOkButton.setEnabled(false);
			return deleted;
		}
		//
		// Get out of keyboard mode. If there is nothing in typedTimes, revert to TimePicker's time.
		// @param updateDisplays If true, update the displays with the relevant time.
		//
		private void finishKbMode(boolean updateDisplays)
		{
			mInKbMode = false;
			if(!mTypedTimes.isEmpty()) {
				int values[] = getEnteredTime(null);
				if(tabHost.getCurrentTab()==0){
					mTimePicker.setTime(values[0], values[1]);
					if(!mIs24HourMode)
						mTimePicker.setAmOrPm(values[2]);
				}
				else{
					mTimePickerEnd.setTime(values[0], values[1]);
					if(!mIs24HourMode)
						mTimePickerEnd.setAmOrPm(values[2]);
				}
				mTypedTimes.clear();
			}
			if(updateDisplays) {
				updateDisplay(false);
				if(tabHost.getCurrentTab()==0)
					mTimePicker.trySettingInputEnabled(true);
				else
					mTimePickerEnd.trySettingInputEnabled(true);
			}
		}
		//
		// Update the hours, minutes, and AM/PM displays with the typed times. If the typedTimes is
		// empty, either show an empty display (filled with the placeholder text), or update from the
		// timepicker's values.
		// @param allowEmptyDisplay if true, then if the typedTimes is empty, use the placeholder text.
		// Otherwise, revert to the timepicker's values.
		//
		private void updateDisplay(boolean allowEmptyDisplay)
		{
			if(!allowEmptyDisplay && mTypedTimes.isEmpty()) {
				if(tabHost.getCurrentTab()==0){
					int hour = mTimePicker.getHours();
					int minute = mTimePicker.getMinutes();
					setHour(hour, true);
					setMinute(minute);
					if(!mIs24HourMode) {
						updateAmPmDisplay(hour < 12? AM : PM);
					}
					setCurrentItemShowing(mTimePicker.getCurrentItemShowing(), true, true, true);
				}
				else{
					int hour = mTimePickerEnd.getHours();
					int minute = mTimePickerEnd.getMinutes();
					setHour(hour, true);
					setMinute(minute);
					if(!mIs24HourMode) {
						updateAmPmDisplay(hour < 12? AM : PM);
					}
					setCurrentItemShowing(mTimePickerEnd.getCurrentItemShowing(), true, true, true);
				}
				mOkButton.setEnabled(true);
			}
			else {
				Boolean[] enteredZeros = {false, false};
				int[] values = getEnteredTime(enteredZeros);
				String hourFormat = enteredZeros[0]? "%02d" : "%2d";
				String minuteFormat = (enteredZeros[1])? "%02d" : "%2d";
				String hourStr = (values[0] == -1)? mDoublePlaceholderText : String.format(hourFormat, values[0]).replace(' ', mPlaceholderText);
				String minuteStr = (values[1] == -1)? mDoublePlaceholderText : String.format(minuteFormat, values[1]).replace(' ', mPlaceholderText);
				if(tabHost.getCurrentTab()==0){
					mHourView.setText(hourStr);
					mHourSpaceView.setText(hourStr);
					mHourView.setTextColor(mUnselectedColor);
					mMinuteView.setText(minuteStr);
					mMinuteSpaceView.setText(minuteStr);
					mMinuteView.setTextColor(mUnselectedColor);
				}
				else{
					mHourViewEnd.setText(hourStr);
					mHourSpaceViewEnd.setText(hourStr);
					mHourViewEnd.setTextColor(mUnselectedColor);
					mMinuteViewEnd.setText(minuteStr);
					mMinuteSpaceViewEnd.setText(minuteStr);
					mMinuteViewEnd.setTextColor(mUnselectedColor);
				}
				if(!mIs24HourMode) {
					updateAmPmDisplay(values[2]);
				}
			}
		}
		private static int getValFromKeyCode(int keyCode)
		{
			switch (keyCode) {
				case KeyEvent.KEYCODE_0: return 0;
				case KeyEvent.KEYCODE_1: return 1;
				case KeyEvent.KEYCODE_2: return 2;
				case KeyEvent.KEYCODE_3: return 3;
				case KeyEvent.KEYCODE_4: return 4;
				case KeyEvent.KEYCODE_5: return 5;
				case KeyEvent.KEYCODE_6: return 6;
				case KeyEvent.KEYCODE_7: return 7;
				case KeyEvent.KEYCODE_8: return 8;
				case KeyEvent.KEYCODE_9: return 9;
				default: return -1;
			}
		}
		//
		// Get the currently-entered time, as integer values of the hours and minutes typed.
		// @param enteredZeros A size-2 boolean array, which the caller should initialize, and which
		// may then be used for the caller to know whether zeros had been explicitly entered as either
		// hours of minutes. This is helpful for deciding whether to show the dashes, or actual 0's.
		// @return A size-3 int array. The first value will be the hours, the second value will be the
		// minutes, and the third will be either TimePickerDialog.AM or TimePickerDialog.PM.
		//
		private int[] getEnteredTime(Boolean[] enteredZeros)
		{
			int amOrPm = -1;
			int startIndex = 1;
			if(!mIs24HourMode && isTypedTimeFullyLegal()) {
				int keyCode = mTypedTimes.get(mTypedTimes.size() - 1);
				if(keyCode == getAmOrPmKeyCode(AM))
					amOrPm = AM;
				else if(keyCode == getAmOrPmKeyCode(PM))
					amOrPm = PM;
				startIndex = 2;
			}
			int minute = -1;
			int hour = -1;
			for(int i = startIndex; i <= mTypedTimes.size(); i++) {
				int val = getValFromKeyCode(mTypedTimes.get(mTypedTimes.size() - i));
				if(i == startIndex)
					minute = val;
				else if(i == startIndex+1) {
					minute += 10*val;
					if(enteredZeros != null && val == 0)
						enteredZeros[1] = true;
				}
				else if(i == startIndex+2)
					hour = val;
				else if(i == startIndex+3) {
					hour += 10*val;
					if(enteredZeros != null && val == 0)
						enteredZeros[0] = true;
				}
			}
			return new int[] {hour, minute, amOrPm};
		}
		//
		// Get the keycode value for AM and PM in the current language.
		//
		private int getAmOrPmKeyCode(int amOrPm)
		{
			// Cache the codes.
			if(mAmKeyCode == -1 || mPmKeyCode == -1) {
				// Find the first character in the AM/PM text that is unique.
				KeyCharacterMap kcm = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);
				char amChar;
				char pmChar;
				for(int i = 0; i < Math.max(mAmText.length(), mPmText.length()); i++) {
					amChar = mAmText.toLowerCase(Locale.getDefault()).charAt(i);
					pmChar = mPmText.toLowerCase(Locale.getDefault()).charAt(i);
					if(amChar != pmChar) {
						KeyEvent[] events = kcm.getEvents(new char[]{amChar, pmChar});
						// There should be 4 events: a down and up for both AM and PM.
						if(events != null && events.length == 4) {
							mAmKeyCode = events[0].getKeyCode();
							mPmKeyCode = events[2].getKeyCode();
						}
						else
							Log.e(TAG, "Unable to find keycodes for AM and PM.");
						break;
					}
				}
			}
			return (amOrPm == AM) ? mAmKeyCode : ((amOrPm == PM) ? mPmKeyCode : -1);
		}
		//
		// Create a tree for deciding what keys can legally be typed.
		//
		private void generateLegalTimesTree()
		{
			// Create a quick cache of numbers to their keycodes.
			int k0 = KeyEvent.KEYCODE_0;
			int k1 = KeyEvent.KEYCODE_1;
			int k2 = KeyEvent.KEYCODE_2;
			int k3 = KeyEvent.KEYCODE_3;
			int k4 = KeyEvent.KEYCODE_4;
			int k5 = KeyEvent.KEYCODE_5;
			int k6 = KeyEvent.KEYCODE_6;
			int k7 = KeyEvent.KEYCODE_7;
			int k8 = KeyEvent.KEYCODE_8;
			int k9 = KeyEvent.KEYCODE_9;
			// The root of the tree doesn't contain any numbers.
			mLegalTimesTree = new Node();
			if(mIs24HourMode) {
				// We'll be re-using these nodes, so we'll save them.
				Node minuteFirstDigit = new Node(k0, k1, k2, k3, k4, k5);
				Node minuteSecondDigit = new Node(k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
				// The first digit must be followed by the second digit.
				minuteFirstDigit.addChild(minuteSecondDigit);
				// The first digit may be 0-1.
				Node firstDigit = new Node(k0, k1);
				mLegalTimesTree.addChild(firstDigit);
				// When the first digit is 0-1, the second digit may be 0-5.
				Node secondDigit = new Node(k0, k1, k2, k3, k4, k5);
				firstDigit.addChild(secondDigit);
				// We may now be followed by the first minute digit. E.g. 00:09, 15:58.
				secondDigit.addChild(minuteFirstDigit);
				// When the first digit is 0-1, and the second digit is 0-5, the third digit may be 6-9.
				Node thirdDigit = new Node(k6, k7, k8, k9);
				// The time must now be finished. E.g. 0:55, 1:08.
				secondDigit.addChild(thirdDigit);
				// When the first digit is 0-1, the second digit may be 6-9.
				secondDigit = new Node(k6, k7, k8, k9);
				firstDigit.addChild(secondDigit);
				// We must now be followed by the first minute digit. E.g. 06:50, 18:20.
				secondDigit.addChild(minuteFirstDigit);
				// The first digit may be 2.
				firstDigit = new Node(k2);
				mLegalTimesTree.addChild(firstDigit);
				// When the first digit is 2, the second digit may be 0-3.
				secondDigit = new Node(k0, k1, k2, k3);
				firstDigit.addChild(secondDigit);
				// We must now be followed by the first minute digit. E.g. 20:50, 23:09.
				secondDigit.addChild(minuteFirstDigit);
				// When the first digit is 2, the second digit may be 4-5.
				secondDigit = new Node(k4, k5);
				firstDigit.addChild(secondDigit);
				// We must now be followd by the last minute digit. E.g. 2:40, 2:53.
				secondDigit.addChild(minuteSecondDigit);
				// The first digit may be 3-9.
				firstDigit = new Node(k3, k4, k5, k6, k7, k8, k9);
				mLegalTimesTree.addChild(firstDigit);
				// We must now be followed by the first minute digit. E.g. 3:57, 8:12.
				firstDigit.addChild(minuteFirstDigit);
			}
			else {
				// We'll need to use the AM/PM node a lot.
				// Set up AM and PM to respond to "a" and "p".
				Node ampm = new Node(getAmOrPmKeyCode(AM), getAmOrPmKeyCode(PM));
				// The first hour digit may be 1.
				Node firstDigit = new Node(k1);
				mLegalTimesTree.addChild(firstDigit);
				// We'll allow quick input of on-the-hour times. E.g. 1pm.
				firstDigit.addChild(ampm);
				// When the first digit is 1, the second digit may be 0-2.
				Node secondDigit = new Node(k0, k1, k2);
				firstDigit.addChild(secondDigit);
				// Also for quick input of on-the-hour times. E.g. 10pm, 12am.
				secondDigit.addChild(ampm);
				// When the first digit is 1, and the second digit is 0-2, the third digit may be 0-5.
				Node thirdDigit = new Node(k0, k1, k2, k3, k4, k5);
				secondDigit.addChild(thirdDigit);
				// The time may be finished now. E.g. 1:02pm, 1:25am.
				thirdDigit.addChild(ampm);
				// When the first digit is 1, the second digit is 0-2, and the third digit is 0-5,
				// the fourth digit may be 0-9.
				Node fourthDigit = new Node(k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
				thirdDigit.addChild(fourthDigit);
				// The time must be finished now. E.g. 10:49am, 12:40pm.
				fourthDigit.addChild(ampm);
				// When the first digit is 1, and the second digit is 0-2, the third digit may be 6-9.
				thirdDigit = new Node(k6, k7, k8, k9);
				secondDigit.addChild(thirdDigit);
				// The time must be finished now. E.g. 1:08am, 1:26pm.
				thirdDigit.addChild(ampm);
				// When the first digit is 1, the second digit may be 3-5.
				secondDigit = new Node(k3, k4, k5);
				firstDigit.addChild(secondDigit);
				// When the first digit is 1, and the second digit is 3-5, the third digit may be 0-9.
				thirdDigit = new Node(k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
				secondDigit.addChild(thirdDigit);
				// The time must be finished now. E.g. 1:39am, 1:50pm.
				thirdDigit.addChild(ampm);
				// The hour digit may be 2-9.
				firstDigit = new Node(k2, k3, k4, k5, k6, k7, k8, k9);
				mLegalTimesTree.addChild(firstDigit);
				// We'll allow quick input of on-the-hour-times. E.g. 2am, 5pm.
				firstDigit.addChild(ampm);
				// When the first digit is 2-9, the second digit may be 0-5.
				secondDigit = new Node(k0, k1, k2, k3, k4, k5);
				firstDigit.addChild(secondDigit);
				// When the first digit is 2-9, and the second digit is 0-5, the third digit may be 0-9.
				thirdDigit = new Node(k0, k1, k2, k3, k4, k5, k6, k7, k8, k9);
				secondDigit.addChild(thirdDigit);
				// The time must be finished now. E.g. 2:57am, 9:30pm.
				thirdDigit.addChild(ampm);
			}
		}
		//
		// Simple node class to be used for traversal to check for legal times.
		// mLegalKeys represents the keys that can be typed to get to the node.
		// mChildren are the children that can be reached from this node.
		//
		private static class Node {
			private int[] mLegalKeys;
			private ArrayList<Node> mChildren;
			public Node(int... legalKeys)
			{
				mLegalKeys = legalKeys;
				mChildren = new ArrayList<>();
			}
			public void addChild(Node child)
			{
				mChildren.add(child);
			}
			public boolean containsKey(int key)
			{
				for(int i = 0; i < mLegalKeys.length; i++) {
					if(mLegalKeys[i] == key) {
						return true;
					}
				}
				return false;
			}
			public Node canReach(int key)
			{
				if(mChildren == null) {
					return null;
				}
				for(Node child : mChildren) {
					if(child.containsKey(key)) {
						return child;
					}
				}
				return null;
			}
		}
		private class KeyboardListener implements View.OnKeyListener {
			@Override public boolean onKey(View v, int keyCode, KeyEvent event) {
				if(event.getAction() == KeyEvent.ACTION_UP) {
					return processKeyUp(keyCode);
				}
				return false;
			}
		}
	}
}
